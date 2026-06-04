/*
 * router.h — NSSF SBI request router + nghttp2/libuv server (WI-nsselection-handler).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten dispatch + transport layer.
 *
 * Two cooperating halves of the inbound transport live here.
 *
 *   1. The router — a thin (method, path) → handler dispatch table. One decoded
 *      HTTP/2 request becomes an nssf_router_request_t; nssf_router_dispatch()
 *      matches the route and invokes the handler, producing an
 *      nssf_router_response_t the server streams back. Registered routes:
 *        GET    /network-slice-information   → nssf_nsselection_get_handle
 *        PUT    /nssai-availability/{nfId}   → nssf_nssaiavailability_put_handle
 *        PATCH  /nssai-availability/{nfId}   → nssf_nssaiavailability_patch_handle
 *        DELETE /nssai-availability/{nfId}   → nssf_nssaiavailability_delete_handle
 *        OPTIONS /nssai-availability         → nssf_nssaiavailability_options_handle
 *        POST   /nssai-availability/subscriptions      → nssf_subscription_post_handle
 *        DELETE /nssai-availability/subscriptions/{id} → nssf_subscription_unsubscribe_handle
 *        PATCH  /nssai-availability/subscriptions/{id} → nssf_subscription_patch_handle
 *      A new handler slots into the table without touching the server I/O code
 *      (request-flow.md §module dispatch). An unmatched method/path answers
 *      404 / 405 here so the server never needs route knowledge.
 *
 *   2. The server — the nghttp2/libuv/OpenSSL event-loop transport that decodes
 *      requests into the router and streams responses back. It is kept next to
 *      the dispatch glue because the per-connection state (SSL session, nghttp2
 *      session, per-stream request accumulation) and the route dispatch share
 *      the same data path; main.c only sees the server lifecycle API below.
 */
#ifndef NSSF_SERVER_ROUTER_H
#define NSSF_SERVER_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "availability_engine.h"
#include "nssaiavailability_options_handler.h"  /* NSSF_NSSAIAVAIL_ALLOW_LEN */
#include "nsselection_get_handler.h"
#include "oauth2_jwks.h"
#include "subscription_post_handler.h"  /* NSSF_SUBSCRIPTION_LOCATION_LEN */
#include "subscription_store.h"
#include "tls_context.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Decoded inbound request the server hands the router. All borrowed.
 *
 * NSSelectionGet (GET, no body) needs only method/path/authorization/max_uri_len.
 * The mutating NSSAIAvailability routes (PUT/PATCH) additionally need the request
 * body + its media type + declared length so the router can build the per-handler
 * request envelope (the body-bearing handlers run their own 411/413/415 checks).
 * The body fields default to absent (NULL body, 0 length, has_content_length
 * false) for the bodyless routes.
 */
typedef struct {
    const char *method;         /* :method pseudo-header, e.g. "GET". */
    const char *path;           /* :path pseudo-header incl. query, e.g.
                                   "/network-slice-information?tai=...". */
    const char *authorization;  /* "Authorization" header value, or NULL. */
    size_t max_uri_len;         /* operator-configured :path length cap (0=off). */

    const char *content_type;   /* request body media type, or NULL. */
    const char *body;           /* request body (borrowed, NUL-terminated), NULL. */
    size_t content_length;      /* declared Content-Length, when present. */
    bool has_content_length;    /* whether a Content-Length header was present. */
    size_t max_body_len;        /* operator-configured body cap (0=off). */
} nssf_router_request_t;

/*
 * Router response — owns body (free with nssf_router_response_free).
 *
 * `allow` carries the OPTIONS Allow-header value (empty string when the route
 * produces no Allow header); the server emits it as the `Allow` response header.
 * `location` carries the subscription Post 201 Location-header value (empty
 * otherwise); the server emits it as the `Location` response header.
 */
typedef struct {
    int status;
    const char *content_type;   /* static string — not freed. */
    char *body;                 /* heap JSON body, or NULL. */
    char allow[NSSF_NSSAIAVAIL_ALLOW_LEN];  /* Allow header value, or "" (none). */
    char location[NSSF_SUBSCRIPTION_LOCATION_LEN];  /* Location value, or "". */
} nssf_router_response_t;

/*
 * Combined handler dependencies wired at bootstrap and borrowed by every
 * dispatch. The router copies the struct by value (members stay borrowed) and
 * builds each per-handler deps_t internally from these — it does NOT mutate the
 * caller's per-handler deps types.
 *
 *   jwks_cache         — inbound OAuth2 bearer validator (shared by all routes).
 *   selection_engine   — SelectionEngine for NSSelectionGet. NULL → that route
 *                        answers 500.
 *   availability_engine — AvailabilityEngine for the NSSAIAvailability routes.
 *                        NULL → those routes answer 500.
 *   subscription_store  — SubscriptionStore for the Subscription routes
 *                        (Post/Unsubscribe/SubModifyPatch). NULL → those routes
 *                        answer 500.
 */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_selection_engine_t *selection_engine;
    nssf_availability_engine_t *availability_engine;
    nssf_subscription_store_t *subscription_store;
} nssf_router_deps_t;

typedef struct nssf_router nssf_router_t;

/* Build a router over the combined handler dependencies. NULL on OOM/NULL deps. */
nssf_router_t *nssf_router_create(const nssf_router_deps_t *deps);

/* Tear down a router (idempotent on NULL). Does not free the dependencies. */
void nssf_router_free(nssf_router_t *router);

/*
 * Match (method, path) and invoke the handler. Always sets *out (200/4xx/5xx).
 * Returns the chosen HTTP status. `req` and `out` must be non-NULL; a NULL
 * router answers 500.
 */
int nssf_router_dispatch(const nssf_router_t *router,
                         const nssf_router_request_t *req,
                         nssf_router_response_t *out);

/* Free a response body produced by nssf_router_dispatch (idempotent). */
void nssf_router_response_free(nssf_router_response_t *out);

/* ─── nghttp2/libuv server (single-process event loop) ───────────────────
 *
 * Single-process event-loop HTTP/2 server (engineering-design.md
 * sbi_server_stack = nghttp2 + TLS app_library). libuv drives the accept/IO
 * loop; each accepted connection is wrapped in an OpenSSL SSL from the operator
 * TLS context (nssf_tls_context), and nghttp2 runs the HTTP/2 session over it.
 * Decoded requests are handed to the router above; responses stream back.
 *
 * TLS posture: nghttp2_session_server_new is always created with non-NULL
 * user_data (per-connection state), and the listening socket only serves TLS-
 * wrapped connections — there is no plaintext listener (ADR-0004 M1).
 */

/* Operator-supplied bind config — sourced from main.c config, never hardcoded. */
typedef struct {
    const char *bind_address;  /* listen address, e.g. "0.0.0.0" / "::". */
    uint16_t port;             /* listen port. */
} nssf_server_config_t;

typedef struct nssf_server nssf_server_t;

/*
 * Create the server bound to cfg, serving over tls_ctx, dispatching through
 * router. All three are borrowed (the caller owns their lifetimes and frees
 * them after nssf_server_run returns). Returns NULL on bind/listen/OOM failure,
 * writing a NUL-terminated diagnostic into errbuf when errbuf && errlen.
 */
nssf_server_t *nssf_server_create(const nssf_server_config_t *cfg,
                                  nssf_tls_context_t *tls_ctx,
                                  nssf_router_t *router,
                                  char *errbuf, size_t errlen);

/*
 * Run the event loop until nssf_server_stop() is called (from a signal handler
 * or another thread). Returns 0 on clean shutdown, nonzero on loop error.
 */
int nssf_server_run(nssf_server_t *server);

/*
 * Request graceful shutdown — stop accepting, drain in-flight streams, then let
 * nssf_server_run return. Safe to call from a signal handler (async-signal-safe
 * libuv async wake). Idempotent.
 */
void nssf_server_stop(nssf_server_t *server);

/* Tear down the server (after nssf_server_run returns). Idempotent on NULL. */
void nssf_server_free(nssf_server_t *server);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_SERVER_ROUTER_H */
