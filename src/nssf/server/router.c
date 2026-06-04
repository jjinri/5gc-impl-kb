/*
 * router.c — (method, path) → handler dispatch + nghttp2/libuv server.
 *
 * Two halves (see router.h):
 *
 *   1. Router — the path may carry a query string ("/p?q=..."); the router
 *      splits at '?' and forwards the query to the handler. A path mismatch is
 *      404; a known path with the wrong method is 405. The dispatch table is
 *      static so registering a Phase 2 handler is one row, no I/O changes.
 *
 *   2. Server — nghttp2 over OpenSSL over libuv, single-process event loop.
 *      Data path per connection:
 *        socket --libuv--> read BIO --SSL_read--> nghttp2_session_mem_recv
 *        nghttp2 send callback --SSL_write--> write BIO --libuv--> socket
 *      Each connection owns its SSL + nghttp2 session; request headers
 *      accumulate on the stream until END_STREAM, where the router runs and the
 *      response is submitted. No business logic lives here — the handler owns it.
 */

#include "router.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include <nghttp2/nghttp2.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <uv.h>

#include "nf_problem_details_wrapper.h"
#include "nssaiavailability_delete_handler.h"
#include "nssaiavailability_patch_handler.h"
#include "nssaiavailability_put_handler.h"
#include "problem_details.h"
#include "subscription_patch_handler.h"
#include "subscription_post_handler.h"
#include "subscription_unsubscribe_handler.h"

/* cJSON for serializing the router-level ProblemDetails (404/405/500). */
#include "cJSON.h"

#define NSSF_CT_PROBLEM_JSON  "application/problem+json"

#define ROUTE_NSSELECTION       "/network-slice-information"
#define ROUTE_NSSAIAVAIL        "/nssai-availability"
#define ROUTE_NSSAIAVAIL_ID     "/nssai-availability/"  /* prefix + {nfId}. */
#define ROUTE_SUBSCRIPTIONS     "/nssai-availability/subscriptions"
#define ROUTE_SUBSCRIPTION_ID   "/nssai-availability/subscriptions/"  /* prefix + {id}. */

struct nssf_router {
    nssf_router_deps_t deps;  /* copied by value; members borrowed. */
};

/* Defined in the server half below; used by the {nfId} path-param copy. */
static void *xstrndup(const uint8_t *src, size_t len);

nssf_router_t *nssf_router_create(const nssf_router_deps_t *deps)
{
    if (deps == NULL) {
        return NULL;
    }
    nssf_router_t *router = calloc(1, sizeof(*router));
    if (router == NULL) {
        return NULL;
    }
    router->deps = *deps;
    return router;
}

void nssf_router_free(nssf_router_t *router)
{
    free(router);
}

/* Serialize a ProblemDetails into the router response. Consumes pd. */
static int emit_problem(nssf_router_response_t *out, problem_details_t *pd,
                        int fallback_status)
{
    out->allow[0] = '\0';     /* error responses carry no Allow header. */
    out->location[0] = '\0';  /* nor a Location header. */
    if (pd == NULL) {
        out->status = fallback_status;
        out->content_type = NSSF_CT_PROBLEM_JSON;
        out->body = NULL;
        return out->status;
    }
    out->status = pd->status;
    out->content_type = NSSF_CT_PROBLEM_JSON;
    cJSON *json = problem_details_convertToJSON(pd);
    out->body = (json != NULL) ? cJSON_PrintUnformatted(json) : NULL;
    cJSON_Delete(json);
    problem_details_free(pd);
    return out->status;
}

/* True when `path` (its first path_len bytes, query already split off) equals
 * the literal `route` exactly. */
static bool path_matches(const char *path, size_t path_len, const char *route)
{
    return strlen(route) == path_len && memcmp(route, path, path_len) == 0;
}

/*
 * When `route_path` (the query-stripped :path, length route_len) is under the
 * "/nssai-availability/" prefix, return the borrowed {nfId} segment start and its
 * length. Returns false (no path param) when the path is not under the prefix or
 * carries an empty / further-nested nfId.
 */
static bool match_nssaiavail_id(const char *route_path, size_t route_len,
                                const char **nf_id_out, size_t *nf_id_len_out)
{
    const size_t prefix_len = sizeof(ROUTE_NSSAIAVAIL_ID) - 1;
    if (route_len <= prefix_len ||
        memcmp(route_path, ROUTE_NSSAIAVAIL_ID, prefix_len) != 0) {
        return false;
    }
    const char *seg = route_path + prefix_len;
    size_t seg_len = route_len - prefix_len;
    /* A nested path ("/nssai-availability/{nfId}/...") is not this route. */
    if (memchr(seg, '/', seg_len) != NULL) {
        return false;
    }
    *nf_id_out = seg;
    *nf_id_len_out = seg_len;
    return true;
}

/*
 * When `route_path` (query-stripped, length route_len) is under the
 * "/nssai-availability/subscriptions/" prefix, return the borrowed
 * {subscriptionId} segment + its length. Returns false when not under the prefix
 * or when the id is empty / further-nested.
 */
static bool match_subscription_id(const char *route_path, size_t route_len,
                                  const char **sub_id_out, size_t *sub_id_len_out)
{
    const size_t prefix_len = sizeof(ROUTE_SUBSCRIPTION_ID) - 1;
    if (route_len <= prefix_len ||
        memcmp(route_path, ROUTE_SUBSCRIPTION_ID, prefix_len) != 0) {
        return false;
    }
    const char *seg = route_path + prefix_len;
    size_t seg_len = route_len - prefix_len;
    if (memchr(seg, '/', seg_len) != NULL) {
        return false;
    }
    *sub_id_out = seg;
    *sub_id_len_out = seg_len;
    return true;
}

int nssf_router_dispatch(const nssf_router_t *router,
                         const nssf_router_request_t *req,
                         nssf_router_response_t *out)
{
    if (req == NULL || out == NULL) {
        if (out != NULL) {
            return emit_problem(out, nf_problem_details_make_500(NULL, NULL), 500);
        }
        return 500;
    }
    out->status = 0;
    out->content_type = NULL;
    out->body = NULL;
    out->allow[0] = '\0';
    out->location[0] = '\0';

    if (router == NULL || req->path == NULL || req->method == NULL) {
        return emit_problem(out, nf_problem_details_make_500(NULL, req ? req->path : NULL), 500);
    }

    /* Split :path into route + query at the first '?'. */
    const char *qmark = strchr(req->path, '?');
    size_t route_len = (qmark != NULL) ? (size_t)(qmark - req->path) : strlen(req->path);
    const char *raw_query = (qmark != NULL) ? qmark + 1 : NULL;

    /*
     * Route table. The match is path-first so a known path with the wrong method
     * answers 405 (Allow), unknown paths answer 404. Registering a handler is one
     * row, no server-I/O changes — each row builds its per-handler deps from the
     * router's combined deps and transfers body ownership into out.
     */

    /* GET /network-slice-information → NSSelectionGet. */
    if (path_matches(req->path, route_len, ROUTE_NSSELECTION)) {
        if (strcmp(req->method, "GET") != 0) {
            return emit_problem(out,
                                nf_problem_details_make_405(NULL, req->path),
                                405);
        }

        nssf_nsselection_deps_t deps = {
            .jwks_cache = router->deps.jwks_cache,
            .engine = router->deps.selection_engine,
        };
        nssf_nsselection_request_t hreq = {
            .authorization = req->authorization,
            .raw_query = raw_query,
            .request_target = req->path,
            .max_uri_len = req->max_uri_len,
        };
        nssf_nsselection_response_t hres = {0};
        nssf_nsselection_get_handle(&hreq, &deps, &hres);

        out->status = hres.status;
        out->content_type = hres.content_type;
        out->body = hres.body;   /* ownership transferred. */
        return out->status;
    }

    /* OPTIONS /nssai-availability → NSSAIAvailabilityOptions (no nfId). */
    if (path_matches(req->path, route_len, ROUTE_NSSAIAVAIL)) {
        if (strcmp(req->method, "OPTIONS") != 0) {
            return emit_problem(out,
                                nf_problem_details_make_405(NULL, req->path),
                                405);
        }

        nssf_nssaiavailability_options_deps_t deps = {
            .jwks_cache = router->deps.jwks_cache,
            .engine = router->deps.availability_engine,
        };
        nssf_nssaiavailability_options_request_t hreq = {
            .authorization = req->authorization,
            .request_target = req->path,
            .max_uri_len = req->max_uri_len,
        };
        nssf_nssaiavailability_options_response_t hres = {0};
        nssf_nssaiavailability_options_handle(&hreq, &deps, &hres);

        out->status = hres.status;
        out->content_type = hres.content_type;
        out->body = hres.body;   /* ownership transferred. */
        /* Carry the Allow header value the server emits on success. */
        memcpy(out->allow, hres.allow, sizeof(out->allow));
        return out->status;
    }

    /*
     * Subscription routes — matched BEFORE the {nfId} route so the collection
     * path "/nssai-availability/subscriptions" is not mis-claimed as an
     * nfId="subscriptions" record (the {nfId} matcher accepts any single segment).
     */

    /* POST /nssai-availability/subscriptions → NSSAIAvailabilityPost. */
    if (path_matches(req->path, route_len, ROUTE_SUBSCRIPTIONS)) {
        if (strcmp(req->method, "POST") != 0) {
            return emit_problem(out,
                                nf_problem_details_make_405(NULL, req->path),
                                405);
        }

        nssf_subscription_post_deps_t deps = {
            .jwks_cache = router->deps.jwks_cache,
            .store = router->deps.subscription_store,
        };
        nssf_subscription_post_request_t hreq = {
            .authorization = req->authorization,
            .content_type = req->content_type,
            .body = req->body,
            .content_length = req->content_length,
            .has_content_length = req->has_content_length,
            .max_body_len = req->max_body_len,
            .request_target = req->path,
            .max_uri_len = req->max_uri_len,
        };
        nssf_subscription_post_response_t hres = {0};
        nssf_subscription_post_handle(&hreq, &deps, &hres);
        out->status = hres.status;
        out->content_type = hres.content_type;
        out->body = hres.body;   /* ownership transferred. */
        /* Carry the 201 Location header value the server emits on success. */
        memcpy(out->location, hres.location, sizeof(out->location));
        return out->status;
    }

    /* {DELETE,PATCH} /nssai-availability/subscriptions/{subscriptionId}. */
    const char *sub_id = NULL;
    size_t sub_id_len = 0;
    if (match_subscription_id(req->path, route_len, &sub_id, &sub_id_len)) {
        char *sub_id_z = xstrndup((const uint8_t *)sub_id, sub_id_len);
        if (sub_id_z == NULL) {
            return emit_problem(out, nf_problem_details_make_500(NULL, req->path), 500);
        }

        if (strcmp(req->method, "DELETE") == 0) {
            nssf_subscription_unsubscribe_deps_t deps = {
                .jwks_cache = router->deps.jwks_cache,
                .store = router->deps.subscription_store,
            };
            nssf_subscription_unsubscribe_request_t hreq = {
                .authorization = req->authorization,
                .subscription_id = sub_id_z,
                .request_target = req->path,
                .max_uri_len = req->max_uri_len,
            };
            nssf_subscription_unsubscribe_response_t hres = {0};
            nssf_subscription_unsubscribe_handle(&hreq, &deps, &hres);
            out->status = hres.status;
            out->content_type = hres.content_type;
            out->body = hres.body;   /* ownership transferred. */
            free(sub_id_z);
            return out->status;
        }

        if (strcmp(req->method, "PATCH") == 0) {
            nssf_subscription_patch_deps_t deps = {
                .jwks_cache = router->deps.jwks_cache,
                .store = router->deps.subscription_store,
            };
            nssf_subscription_patch_request_t hreq = {
                .authorization = req->authorization,
                .subscription_id = sub_id_z,
                .content_type = req->content_type,
                .body = req->body,
                .content_length = req->content_length,
                .has_content_length = req->has_content_length,
                .max_body_len = req->max_body_len,
                .request_target = req->path,
                .max_uri_len = req->max_uri_len,
            };
            nssf_subscription_patch_response_t hres = {0};
            nssf_subscription_patch_handle(&hreq, &deps, &hres);
            out->status = hres.status;
            out->content_type = hres.content_type;
            out->body = hres.body;   /* ownership transferred. */
            free(sub_id_z);
            return out->status;
        }

        free(sub_id_z);
        return emit_problem(out,
                            nf_problem_details_make_405(NULL, req->path),
                            405);
    }

    /* {PUT,PATCH,DELETE} /nssai-availability/{nfId}. */
    const char *nf_id = NULL;
    size_t nf_id_len = 0;
    if (match_nssaiavail_id(req->path, route_len, &nf_id, &nf_id_len)) {
        /* The {nfId} segment is borrowed from req->path (not NUL-terminated at
         * the segment boundary); the per-handler request needs a NUL-terminated
         * copy. */
        char *nf_id_z = xstrndup((const uint8_t *)nf_id, nf_id_len);
        if (nf_id_z == NULL) {
            return emit_problem(out, nf_problem_details_make_500(NULL, req->path), 500);
        }

        if (strcmp(req->method, "PUT") == 0) {
            nssf_nssaiavailability_put_deps_t deps = {
                .jwks_cache = router->deps.jwks_cache,
                .engine = router->deps.availability_engine,
            };
            nssf_nssaiavailability_put_request_t hreq = {
                .authorization = req->authorization,
                .nf_id = nf_id_z,
                .content_type = req->content_type,
                .body = req->body,
                .content_length = req->content_length,
                .has_content_length = req->has_content_length,
                .max_body_len = req->max_body_len,
                .request_target = req->path,
                .max_uri_len = req->max_uri_len,
            };
            nssf_nssaiavailability_put_response_t hres = {0};
            nssf_nssaiavailability_put_handle(&hreq, &deps, &hres);
            out->status = hres.status;
            out->content_type = hres.content_type;
            out->body = hres.body;   /* ownership transferred. */
            free(nf_id_z);
            return out->status;
        }

        if (strcmp(req->method, "PATCH") == 0) {
            nssf_nssaiavailability_patch_deps_t deps = {
                .jwks_cache = router->deps.jwks_cache,
                .engine = router->deps.availability_engine,
            };
            /*
             * The PATCH routing Tai is not a transport header — an RFC 6902 array
             * does not embed it. With no transport source the router passes NULL;
             * the engine classifies a NULL/absent Tai as 400 (the handler's own
             * contract). A later slice that introduces a Tai source wires it here.
             */
            nssf_nssaiavailability_patch_request_t hreq = {
                .authorization = req->authorization,
                .nf_id = nf_id_z,
                .content_type = req->content_type,
                .body = req->body,
                .tai = NULL,
                .content_length = req->content_length,
                .has_content_length = req->has_content_length,
                .max_body_len = req->max_body_len,
                .request_target = req->path,
                .max_uri_len = req->max_uri_len,
            };
            nssf_nssaiavailability_patch_response_t hres = {0};
            nssf_nssaiavailability_patch_handle(&hreq, &deps, &hres);
            out->status = hres.status;
            out->content_type = hres.content_type;
            out->body = hres.body;   /* ownership transferred. */
            free(nf_id_z);
            return out->status;
        }

        if (strcmp(req->method, "DELETE") == 0) {
            nssf_nssaiavailability_delete_deps_t deps = {
                .jwks_cache = router->deps.jwks_cache,
                .engine = router->deps.availability_engine,
            };
            nssf_nssaiavailability_delete_request_t hreq = {
                .authorization = req->authorization,
                .nf_id = nf_id_z,
                .request_target = req->path,
                .max_uri_len = req->max_uri_len,
            };
            nssf_nssaiavailability_delete_response_t hres = {0};
            nssf_nssaiavailability_delete_handle(&hreq, &deps, &hres);
            out->status = hres.status;
            out->content_type = hres.content_type;
            out->body = hres.body;   /* ownership transferred. */
            free(nf_id_z);
            return out->status;
        }

        free(nf_id_z);
        return emit_problem(out,
                            nf_problem_details_make_405(NULL, req->path),
                            405);
    }

    /* No route matched. */
    return emit_problem(out,
                        nf_problem_details_make_404_no_availability(
                            "no resource at the requested path", req->path),
                        404);
}

void nssf_router_response_free(nssf_router_response_t *out)
{
    if (out == NULL) {
        return;
    }
    free(out->body);
    out->body = NULL;
}

/* ════ nghttp2/libuv server ══════════════════════════════════════════════ */

/* Per-stream request accumulation. */
typedef struct {
    int32_t stream_id;
    char *method;
    char *path;
    char *authorization;
    /* response body being streamed out (owned). */
    char *resp_body;
    size_t resp_len;
    size_t resp_off;
} nssf_stream_ctx_t;

/* Per-connection state — this is the nghttp2 user_data (never NULL → ADR M1). */
typedef struct {
    nssf_server_t *server;
    uv_tcp_t handle;
    SSL *ssl;
    BIO *read_bio;   /* libuv → SSL. */
    BIO *write_bio;  /* SSL → libuv. */
    nghttp2_session *session;
    bool handshake_done;
    bool closing;
} nssf_conn_t;

struct nssf_server {
    nssf_server_config_t cfg;
    nssf_tls_context_t *tls_ctx;
    nssf_router_t *router;
    uv_loop_t loop;
    uv_tcp_t listener;
    uv_async_t stop_async;
    bool stopping;
    bool listener_active;
};

/* ─── small helpers ─────────────────────────────────────────────────── */

static void conn_close(nssf_conn_t *conn);
static void try_send(nssf_conn_t *conn);

static void *xstrndup(const uint8_t *src, size_t len)
{
    char *out = malloc(len + 1);
    if (out == NULL) {
        return NULL;
    }
    memcpy(out, src, len);
    out[len] = '\0';
    return out;
}

static void stream_ctx_free(nssf_stream_ctx_t *sc)
{
    if (sc == NULL) {
        return;
    }
    free(sc->method);
    free(sc->path);
    free(sc->authorization);
    free(sc->resp_body);
    free(sc);
}

/* ─── nghttp2 callbacks ─────────────────────────────────────────────── */

/* nghttp2 wants to send `length` bytes — push them through SSL_write. */
static ssize_t send_cb(nghttp2_session *session, const uint8_t *data,
                       size_t length, int flags, void *user_data)
{
    (void)session;
    (void)flags;
    nssf_conn_t *conn = user_data;
    if (!conn->handshake_done) {
        return NGHTTP2_ERR_WOULDBLOCK;
    }
    int n = SSL_write(conn->ssl, data, (int)length);
    if (n <= 0) {
        int err = SSL_get_error(conn->ssl, n);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return NGHTTP2_ERR_WOULDBLOCK;
        }
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    return n;
}

static int on_begin_headers_cb(nghttp2_session *session,
                               const nghttp2_frame *frame, void *user_data)
{
    (void)user_data;
    if (frame->hd.type != NGHTTP2_HEADERS ||
        frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        return 0;
    }
    nssf_stream_ctx_t *sc = calloc(1, sizeof(*sc));
    if (sc == NULL) {
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    sc->stream_id = frame->hd.stream_id;
    nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, sc);
    return 0;
}

static int on_header_cb(nghttp2_session *session, const nghttp2_frame *frame,
                        const uint8_t *name, size_t namelen,
                        const uint8_t *value, size_t valuelen,
                        uint8_t flags, void *user_data)
{
    (void)flags;
    (void)user_data;
    if (frame->hd.type != NGHTTP2_HEADERS ||
        frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        return 0;
    }
    nssf_stream_ctx_t *sc =
        nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (sc == NULL) {
        return 0;
    }

#define HDR_IS(lit) (namelen == sizeof(lit) - 1 && memcmp(name, lit, namelen) == 0)
    char **slot = NULL;
    if (HDR_IS(":method")) {
        slot = &sc->method;
    } else if (HDR_IS(":path")) {
        slot = &sc->path;
    } else if (HDR_IS("authorization")) {
        slot = &sc->authorization;
    }
#undef HDR_IS
    if (slot != NULL && *slot == NULL) {
        *slot = xstrndup(value, valuelen);
        if (*slot == NULL) {
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
    }
    return 0;
}

/* Body read callback for the response DATA frames. */
static ssize_t resp_read_cb(nghttp2_session *session, int32_t stream_id,
                            uint8_t *buf, size_t length, uint32_t *data_flags,
                            nghttp2_data_source *source, void *user_data)
{
    (void)session;
    (void)stream_id;
    (void)user_data;
    nssf_stream_ctx_t *sc = source->ptr;
    size_t remaining = sc->resp_len - sc->resp_off;
    size_t n = (remaining < length) ? remaining : length;
    if (n > 0) {
        memcpy(buf, sc->resp_body + sc->resp_off, n);
        sc->resp_off += n;
    }
    if (sc->resp_off >= sc->resp_len) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }
    return (ssize_t)n;
}

/* Run the router for a completed request and submit the response. */
static int submit_response(nghttp2_session *session, nssf_conn_t *conn,
                           nssf_stream_ctx_t *sc)
{
    nssf_router_request_t rreq = {
        .method = sc->method ? sc->method : "",
        .path = sc->path ? sc->path : "",
        .authorization = sc->authorization,
        .max_uri_len = 0,
    };
    nssf_router_response_t rres = {0};
    nssf_router_dispatch(conn->server->router, &rreq, &rres);

    /* Move the body onto the stream ctx so resp_read_cb can stream it. */
    sc->resp_body = rres.body;     /* ownership transferred. */
    sc->resp_len = rres.body ? strlen(rres.body) : 0;
    sc->resp_off = 0;

    char status_str[8];
    snprintf(status_str, sizeof(status_str), "%d", rres.status);
    const char *ct = rres.content_type ? rres.content_type : "application/json";

    /*
     * OPTIONS success answers with an Allow header (rres.allow non-empty) and no
     * content-type (empty body); the subscription Post 201 answers with a Location
     * header and no content-type (empty body); every other response carries
     * content-type. allow / location are mutually exclusive across the routes.
     */
    bool has_allow = (rres.allow[0] != '\0');
    bool has_location = (rres.location[0] != '\0');
    nghttp2_nv hdrs[3];
    size_t nhdrs = 0;
    hdrs[nhdrs++] = (nghttp2_nv){ (uint8_t *)":status", (uint8_t *)status_str, 7,
                                  strlen(status_str), NGHTTP2_NV_FLAG_NONE };
    if (has_allow) {
        hdrs[nhdrs++] = (nghttp2_nv){ (uint8_t *)"allow", (uint8_t *)rres.allow, 5,
                                      strlen(rres.allow), NGHTTP2_NV_FLAG_NONE };
    } else if (has_location) {
        hdrs[nhdrs++] = (nghttp2_nv){ (uint8_t *)"location", (uint8_t *)rres.location, 8,
                                      strlen(rres.location), NGHTTP2_NV_FLAG_NONE };
    } else {
        hdrs[nhdrs++] = (nghttp2_nv){ (uint8_t *)"content-type", (uint8_t *)ct, 12,
                                      strlen(ct), NGHTTP2_NV_FLAG_NONE };
    }

    nghttp2_data_provider prd = {
        .source = { .ptr = sc },
        .read_callback = resp_read_cb,
    };
    int rv = nghttp2_submit_response(session, sc->stream_id, hdrs, nhdrs, &prd);
    if (rv != 0) {
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    return 0;
}

static int on_frame_recv_cb(nghttp2_session *session,
                            const nghttp2_frame *frame, void *user_data)
{
    nssf_conn_t *conn = user_data;
    /* Request is complete when the request HEADERS carry END_STREAM (GET has no
     * body), or a DATA frame closes the stream. */
    if ((frame->hd.flags & NGHTTP2_FLAG_END_STREAM) &&
        (frame->hd.type == NGHTTP2_HEADERS || frame->hd.type == NGHTTP2_DATA)) {
        nssf_stream_ctx_t *sc =
            nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
        if (sc != NULL) {
            if (submit_response(session, conn, sc) != 0) {
                return NGHTTP2_ERR_CALLBACK_FAILURE;
            }
        }
    }
    return 0;
}

static int on_stream_close_cb(nghttp2_session *session, int32_t stream_id,
                              uint32_t error_code, void *user_data)
{
    (void)error_code;
    (void)user_data;
    nssf_stream_ctx_t *sc =
        nghttp2_session_get_stream_user_data(session, stream_id);
    stream_ctx_free(sc);
    nghttp2_session_set_stream_user_data(session, stream_id, NULL);
    return 0;
}

static nghttp2_session_callbacks *make_callbacks(void)
{
    nghttp2_session_callbacks *cbs = NULL;
    if (nghttp2_session_callbacks_new(&cbs) != 0) {
        return NULL;
    }
    nghttp2_session_callbacks_set_send_callback(cbs, send_cb);
    nghttp2_session_callbacks_set_on_begin_headers_callback(cbs, on_begin_headers_cb);
    nghttp2_session_callbacks_set_on_header_callback(cbs, on_header_cb);
    nghttp2_session_callbacks_set_on_frame_recv_callback(cbs, on_frame_recv_cb);
    nghttp2_session_callbacks_set_on_stream_close_callback(cbs, on_stream_close_cb);
    return cbs;
}

/* Send the HTTP/2 server connection preface (SETTINGS). */
static int send_server_connection_header(nssf_conn_t *conn)
{
    nghttp2_settings_entry iv[] = {
        { NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100 },
    };
    return nghttp2_submit_settings(conn->session, NGHTTP2_FLAG_NONE, iv,
                                   sizeof(iv) / sizeof(iv[0]));
}

/* ─── libuv write plumbing ──────────────────────────────────────────── */

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

static void on_write_done(uv_write_t *req, int status)
{
    (void)status;
    write_req_t *wr = (write_req_t *)req;
    free(wr->buf.base);
    free(wr);
}

/* Drain pending bytes from the SSL write BIO out to the socket via libuv. */
static void flush_write_bio(nssf_conn_t *conn)
{
    int pending = BIO_pending(conn->write_bio);
    while (pending > 0) {
        char *buf = malloc((size_t)pending);
        if (buf == NULL) {
            conn_close(conn);
            return;
        }
        int n = BIO_read(conn->write_bio, buf, pending);
        if (n <= 0) {
            free(buf);
            break;
        }
        write_req_t *wr = calloc(1, sizeof(*wr));
        if (wr == NULL) {
            free(buf);
            conn_close(conn);
            return;
        }
        wr->buf = uv_buf_init(buf, (unsigned int)n);
        if (uv_write(&wr->req, (uv_stream_t *)&conn->handle, &wr->buf, 1,
                     on_write_done) != 0) {
            free(buf);
            free(wr);
            conn_close(conn);
            return;
        }
        pending = BIO_pending(conn->write_bio);
    }
}

/* Pump nghttp2 → SSL → write BIO → socket. */
static void try_send(nssf_conn_t *conn)
{
    if (conn->session != NULL && nghttp2_session_send(conn->session) != 0) {
        conn_close(conn);
        return;
    }
    flush_write_bio(conn);

    if (conn->session != NULL &&
        nghttp2_session_want_read(conn->session) == 0 &&
        nghttp2_session_want_write(conn->session) == 0) {
        conn_close(conn);
    }
}

/* Decrypt available SSL data and feed it to nghttp2. */
static void pump_ssl_read(nssf_conn_t *conn)
{
    uint8_t buf[16384];
    for (;;) {
        int n = SSL_read(conn->ssl, buf, sizeof(buf));
        if (n <= 0) {
            int err = SSL_get_error(conn->ssl, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                break;
            }
            if (err == SSL_ERROR_ZERO_RETURN) {
                conn_close(conn);
            }
            break;
        }
        ssize_t consumed =
            nghttp2_session_mem_recv(conn->session, buf, (size_t)n);
        if (consumed < 0) {
            conn_close(conn);
            return;
        }
    }
}

/* Advance the TLS handshake; once done create the nghttp2 session. */
static void drive_handshake(nssf_conn_t *conn)
{
    int rv = SSL_accept(conn->ssl);
    if (rv == 1) {
        conn->handshake_done = true;

        nghttp2_session_callbacks *cbs = make_callbacks();
        if (cbs == NULL) {
            conn_close(conn);
            return;
        }
        int srv = nghttp2_session_server_new(&conn->session, cbs, conn);
        nghttp2_session_callbacks_del(cbs);
        if (srv != 0 || conn->session == NULL) {
            conn_close(conn);
            return;
        }
        if (send_server_connection_header(conn) != 0) {
            conn_close(conn);
            return;
        }
        try_send(conn);
        return;
    }

    int err = SSL_get_error(conn->ssl, rv);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        flush_write_bio(conn);
        return;
    }
    conn_close(conn);
}

/* ─── libuv read plumbing ───────────────────────────────────────────── */

static void alloc_cb(uv_handle_t *handle, size_t suggested, uv_buf_t *buf)
{
    (void)handle;
    buf->base = malloc(suggested);
    buf->len = (buf->base != NULL) ? suggested : 0;
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    nssf_conn_t *conn = (nssf_conn_t *)stream->data;
    if (nread < 0) {
        free(buf->base);
        conn_close(conn);
        return;
    }
    if (nread > 0) {
        BIO_write(conn->read_bio, buf->base, (int)nread);
        if (!conn->handshake_done) {
            drive_handshake(conn);
        } else {
            pump_ssl_read(conn);
            try_send(conn);
        }
    }
    free(buf->base);
}

/* ─── connection lifecycle ──────────────────────────────────────────── */

static void on_conn_closed(uv_handle_t *handle)
{
    nssf_conn_t *conn = (nssf_conn_t *)handle->data;
    if (conn->session != NULL) {
        nghttp2_session_del(conn->session);
    }
    if (conn->ssl != NULL) {
        /* SSL_free also frees the BIOs attached via SSL_set_bio. */
        SSL_free(conn->ssl);
    }
    free(conn);
}

static void conn_close(nssf_conn_t *conn)
{
    if (conn == NULL || conn->closing) {
        return;
    }
    conn->closing = true;
    if (!uv_is_closing((uv_handle_t *)&conn->handle)) {
        uv_close((uv_handle_t *)&conn->handle, on_conn_closed);
    }
}

static void on_new_connection(uv_stream_t *server_stream, int status)
{
    if (status != 0) {
        return;
    }
    nssf_server_t *server = (nssf_server_t *)server_stream->data;
    if (server->stopping) {
        return;
    }

    nssf_conn_t *conn = calloc(1, sizeof(*conn));
    if (conn == NULL) {
        return;
    }
    conn->server = server;
    if (uv_tcp_init(&server->loop, &conn->handle) != 0) {
        free(conn);
        return;
    }
    conn->handle.data = conn;

    if (uv_accept(server_stream, (uv_stream_t *)&conn->handle) != 0) {
        uv_close((uv_handle_t *)&conn->handle, on_conn_closed);
        return;
    }

    SSL_CTX *ssl_ctx = nssf_tls_context_ssl_ctx(server->tls_ctx);
    conn->ssl = SSL_new(ssl_ctx);
    if (conn->ssl == NULL) {
        conn_close(conn);
        return;
    }
    conn->read_bio = BIO_new(BIO_s_mem());
    conn->write_bio = BIO_new(BIO_s_mem());
    if (conn->read_bio == NULL || conn->write_bio == NULL) {
        conn_close(conn);
        return;
    }
    SSL_set_bio(conn->ssl, conn->read_bio, conn->write_bio);
    SSL_set_accept_state(conn->ssl);

    if (uv_read_start((uv_stream_t *)&conn->handle, alloc_cb, on_read) != 0) {
        conn_close(conn);
    }
}

/* ─── server lifecycle ──────────────────────────────────────────────── */

static void on_stop_async(uv_async_t *async)
{
    nssf_server_t *server = (nssf_server_t *)async->data;
    server->stopping = true;
    /* Stop accepting; in-flight connections drain themselves then close. */
    if (server->listener_active && !uv_is_closing((uv_handle_t *)&server->listener)) {
        uv_close((uv_handle_t *)&server->listener, NULL);
        server->listener_active = false;
    }
    if (!uv_is_closing((uv_handle_t *)&server->stop_async)) {
        uv_close((uv_handle_t *)&server->stop_async, NULL);
    }
}

nssf_server_t *nssf_server_create(const nssf_server_config_t *cfg,
                                  nssf_tls_context_t *tls_ctx,
                                  nssf_router_t *router,
                                  char *errbuf, size_t errlen)
{
    if (cfg == NULL || tls_ctx == NULL || router == NULL) {
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "nssf_server: null argument");
        }
        return NULL;
    }
    nssf_server_t *server = calloc(1, sizeof(*server));
    if (server == NULL) {
        return NULL;
    }
    server->cfg = *cfg;
    server->tls_ctx = tls_ctx;
    server->router = router;

    if (uv_loop_init(&server->loop) != 0) {
        free(server);
        return NULL;
    }
    if (uv_tcp_init(&server->loop, &server->listener) != 0) {
        uv_loop_close(&server->loop);
        free(server);
        return NULL;
    }
    server->listener.data = server;

    if (uv_async_init(&server->loop, &server->stop_async, on_stop_async) != 0) {
        uv_loop_close(&server->loop);
        free(server);
        return NULL;
    }
    server->stop_async.data = server;

    struct sockaddr_storage addr;
    const char *host = cfg->bind_address ? cfg->bind_address : "0.0.0.0";
    int rc;
    if (strchr(host, ':') != NULL) {
        rc = uv_ip6_addr(host, cfg->port, (struct sockaddr_in6 *)&addr);
    } else {
        rc = uv_ip4_addr(host, cfg->port, (struct sockaddr_in *)&addr);
    }
    if (rc != 0) {
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "nssf_server: bad bind address %s", host);
        }
        nssf_server_free(server);
        return NULL;
    }

    if (uv_tcp_bind(&server->listener, (const struct sockaddr *)&addr, 0) != 0 ||
        uv_listen((uv_stream_t *)&server->listener, 128, on_new_connection) != 0) {
        if (errbuf != NULL && errlen > 0) {
            snprintf(errbuf, errlen, "nssf_server: bind/listen failed on %s:%u",
                     host, (unsigned)cfg->port);
        }
        nssf_server_free(server);
        return NULL;
    }
    server->listener_active = true;
    return server;
}

int nssf_server_run(nssf_server_t *server)
{
    if (server == NULL) {
        return -1;
    }
    return uv_run(&server->loop, UV_RUN_DEFAULT);
}

void nssf_server_stop(nssf_server_t *server)
{
    if (server == NULL) {
        return;
    }
    /* async send is async-signal-safe → callable from a signal handler. */
    uv_async_send(&server->stop_async);
}

void nssf_server_free(nssf_server_t *server)
{
    if (server == NULL) {
        return;
    }
    /* Best-effort: close any still-open handles, then run the loop once so the
     * close callbacks fire before the loop is torn down. */
    if (server->listener_active && !uv_is_closing((uv_handle_t *)&server->listener)) {
        uv_close((uv_handle_t *)&server->listener, NULL);
        server->listener_active = false;
    }
    if (!uv_is_closing((uv_handle_t *)&server->stop_async)) {
        uv_close((uv_handle_t *)&server->stop_async, NULL);
    }
    uv_run(&server->loop, UV_RUN_NOWAIT);
    uv_loop_close(&server->loop);
    free(server);
}
