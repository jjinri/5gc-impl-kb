/*
 * nsselection_get_handler.c — NSSelectionGet inbound handler.
 *
 * Security-gate-first, then SelectionEngine dispatch, then result→HTTP mapping.
 * Transport-agnostic: the nghttp2 server fills nssf_nsselection_request_t; the
 * same path drives tests without a socket.
 */

#include "nsselection_get_handler.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strncasecmp — case-insensitive "Bearer " scheme match. */

#include "authorized_network_slice_info.h"
#include "cJSON.h"
#include "nf_problem_details_wrapper.h"
#include "oauth2_scope_check.h"
#include "problem_details.h"

/*
 * 29.531 NSSelectionGet query params are kebab-case and each value is a JSON
 * document (a serialized SliceInfoForRegistration / Tai / PlmnId / NFType …).
 * The SelectionEngine consumes one assembled cJSON object keyed in camelCase
 * (selection_engine.c KEY_* constants). This table is the kebab→camel bridge so
 * the engine's keys stay the single source of truth.
 */
typedef struct {
    const char *query_name;   /* on-the-wire kebab-case param name. */
    const char *engine_key;   /* camelCase key the engine reads. */
} nsselection_param_map_t;

static const nsselection_param_map_t PARAM_MAP[] = {
    { "slice-info-for-registration", "sliceInfoForRegistration" },
    { "slice-info-for-pdu-session", "sliceInfoForPDUSession" },
    { "slice-info-for-ue-cu", "sliceInfoForUEConfigurationUpdate" },
    { "tai", "tai" },
    { "home-plmn-id", "homePlmnId" },
    { "nf-id", "nfId" },
    { "nf-type", "nfType" },
    { "supported-features", "supportedFeatures" },
    { "l2-info", "l2Info" },
    { "home-snssais", "homeSnssais" },
    { "source-tai", "sourceTai" },
};
static const size_t PARAM_MAP_LEN = sizeof(PARAM_MAP) / sizeof(PARAM_MAP[0]);

static const char *engine_key_for(const char *query_name, size_t name_len)
{
    for (size_t i = 0; i < PARAM_MAP_LEN; i++) {
        if (strlen(PARAM_MAP[i].query_name) == name_len &&
            memcmp(PARAM_MAP[i].query_name, query_name, name_len) == 0) {
            return PARAM_MAP[i].engine_key;
        }
    }
    return NULL;
}

/* Percent-decode src (len bytes) in place into a fresh NUL-terminated buffer. */
static char *percent_decode(const char *src, size_t len)
{
    char *out = malloc(len + 1);
    if (out == NULL) {
        return NULL;
    }
    size_t w = 0;
    for (size_t r = 0; r < len; r++) {
        char c = src[r];
        if (c == '+') {
            out[w++] = ' ';
        } else if (c == '%' && r + 2 < len) {
            /* hex pair; leave malformed escapes verbatim rather than guessing. */
            char hi = src[r + 1];
            char lo = src[r + 2];
            int hv = (hi >= '0' && hi <= '9') ? hi - '0'
                   : (hi >= 'A' && hi <= 'F') ? hi - 'A' + 10
                   : (hi >= 'a' && hi <= 'f') ? hi - 'a' + 10 : -1;
            int lv = (lo >= '0' && lo <= '9') ? lo - '0'
                   : (lo >= 'A' && lo <= 'F') ? lo - 'A' + 10
                   : (lo >= 'a' && lo <= 'f') ? lo - 'a' + 10 : -1;
            if (hv >= 0 && lv >= 0) {
                out[w++] = (char)((hv << 4) | lv);
                r += 2;
            } else {
                out[w++] = c;
            }
        } else {
            out[w++] = c;
        }
    }
    out[w] = '\0';
    return out;
}

/*
 * Parse the raw query string into the SelectionEngine cJSON object. Each known
 * param value is percent-decoded then parsed as JSON; on a value that is not
 * valid JSON the param is treated as a plain string. Unknown params are ignored
 * (forward-compatible per request-flow.md). Returns a new cJSON object the
 * caller cJSON_Deletes, or NULL on OOM.
 */
static cJSON *build_engine_query(const char *raw_query)
{
    cJSON *query = cJSON_CreateObject();
    if (query == NULL) {
        return NULL;
    }
    if (raw_query == NULL || raw_query[0] == '\0') {
        return query;
    }

    const char *p = raw_query;
    while (*p != '\0') {
        const char *amp = strchr(p, '&');
        size_t pair_len = (amp != NULL) ? (size_t)(amp - p) : strlen(p);

        const char *eq = memchr(p, '=', pair_len);
        if (eq != NULL) {
            size_t name_len = (size_t)(eq - p);
            const char *val = eq + 1;
            size_t val_len = pair_len - name_len - 1;

            const char *engine_key = engine_key_for(p, name_len);
            if (engine_key != NULL) {
                char *decoded = percent_decode(val, val_len);
                if (decoded == NULL) {
                    cJSON_Delete(query);
                    return NULL;
                }
                cJSON *parsed = cJSON_Parse(decoded);
                if (parsed != NULL) {
                    cJSON_AddItemToObject(query, engine_key, parsed);
                } else {
                    cJSON_AddStringToObject(query, engine_key, decoded);
                }
                free(decoded);
            }
        }

        if (amp == NULL) {
            break;
        }
        p = amp + 1;
    }
    return query;
}

/* Serialize a ProblemDetails into the response (application/problem+json). */
static int emit_problem(nssf_nsselection_response_t *out,
                        problem_details_t *pd,
                        int fallback_status)
{
    if (pd == NULL) {
        /* OOM building the ProblemDetails — answer the status with empty body. */
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

/*
 * Security gate — runs before any business logic. Returns true when the request
 * is authenticated and scoped for NSSelectionGet; on a deny writes the 401/403
 * ProblemDetails into `out` and returns false.
 */
static bool security_gate(const nssf_nsselection_request_t *req,
                          const nssf_nsselection_deps_t *deps,
                          nssf_nsselection_response_t *out)
{
    const char *auth = req->authorization;
    if (auth == NULL || auth[0] == '\0') {
        emit_problem(out,
                     nf_problem_details_make_401_token_missing(NULL, req->request_target),
                     401);
        return false;
    }

    /* Strip the "Bearer " scheme prefix (case-insensitive) before validation. */
    const char *bearer = auth;
    const char prefix[] = "Bearer ";
    if (strncasecmp(auth, prefix, sizeof(prefix) - 1) == 0) {
        bearer = auth + (sizeof(prefix) - 1);
    } else {
        emit_problem(out,
                     nf_problem_details_make_401_token_missing(NULL, req->request_target),
                     401);
        return false;
    }

    char *scope_claim = NULL;
    char errbuf[256] = {0};
    if (nssf_oauth2_validate_bearer(deps->jwks_cache, bearer, &scope_claim,
                                    errbuf, sizeof(errbuf)) != 0) {
        free(scope_claim);
        emit_problem(out,
                     nf_problem_details_make_401_token_invalid(errbuf[0] ? errbuf : NULL,
                                                               req->request_target),
                     401);
        return false;
    }

    bool scoped = nssf_oauth2_scope_match(scope_claim, NSSF_SCOPE_NSSELECTION);
    free(scope_claim);
    if (!scoped) {
        emit_problem(out,
                     nf_problem_details_make_403("required scope " NSSF_SCOPE_NSSELECTION
                                                 " not present in access token",
                                                 req->request_target),
                     403);
        return false;
    }
    return true;
}

/* Map a successful engine call onto a 200 AuthorizedNetworkSliceInfo response. */
static int emit_authorized(nssf_nsselection_response_t *out, cJSON *authorized)
{
    out->status = 200;
    out->content_type = NSSF_CT_JSON;
    out->body = cJSON_PrintUnformatted(authorized);
    return out->status;
}

int nssf_nsselection_get_handle(const nssf_nsselection_request_t *req,
                                const nssf_nsselection_deps_t *deps,
                                nssf_nsselection_response_t *out)
{
    if (req == NULL || deps == NULL || out == NULL) {
        if (out != NULL) {
            return emit_problem(out, nf_problem_details_make_500(NULL, NULL), 500);
        }
        return 500;
    }
    out->status = 0;
    out->content_type = NULL;
    out->body = NULL;

    /* A server with no validator or engine cannot serve securely → 500. */
    if (deps->jwks_cache == NULL || deps->engine == NULL) {
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }

    /* 1. Security gate BEFORE business logic. */
    if (!security_gate(req, deps, out)) {
        return out->status;
    }

    /* 2. Request-target length cap → 414. */
    if (req->max_uri_len > 0 && req->request_target != NULL &&
        strlen(req->request_target) > req->max_uri_len) {
        return emit_problem(out, nf_problem_details_make_414(NULL, req->request_target), 414);
    }

    /* 3. Assemble the SelectionEngine query from the URL query params. */
    cJSON *engine_query = build_engine_query(req->raw_query);
    if (engine_query == NULL) {
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }

    /* 4. SelectionEngine::select() — the only I/O is the read-only TAI SELECT. */
    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_REJECT;
    char errbuf[256] = {0};
    int rc = nssf_selection_engine_select(deps->engine, engine_query, &authorized,
                                          &result, errbuf, sizeof(errbuf));
    cJSON_Delete(engine_query);

    if (rc != 0) {
        /*
         * The engine collapses several failures into rc=-1 and distinguishes
         * them only via errbuf. Two response families per the acceptance set
         * (404 / 500):
         *   - "OOM" / "repository" → an internal/persistence failure the client
         *     cannot fix → 500.
         *   - everything else (no slice-info variant, missing/unknown TAI, no
         *     AuthorizedNetworkSliceInfo producible) → the NSSelectionGet
         *     no-availability path → 404.
         * The substring probe keeps 500 reachable for true server faults while
         * the TAI-resolution failures answer 404 (matches the integration
         * matrix's 200/403/404/414 paths — no 400/503 surface here).
         */
        bool internal = (strstr(errbuf, "OOM") != NULL) ||
                        (strstr(errbuf, "repository") != NULL);
        if (internal) {
            return emit_problem(out,
                                nf_problem_details_make_500(errbuf[0] ? errbuf : NULL,
                                                            req->request_target),
                                500);
        }
        return emit_problem(out,
                            nf_problem_details_make_404_no_availability(errbuf[0] ? errbuf : NULL,
                                                                        req->request_target),
                            404);
    }

    /* 5. Result → HTTP. FULL_ALLOW / PARTIAL stay 200 with rejected lists. */
    int status = emit_authorized(out, authorized);
    cJSON_Delete(authorized);
    if (out->body == NULL) {
        /* serialization OOM — fall back to 500. */
        return emit_problem(out, nf_problem_details_make_500(NULL, req->request_target), 500);
    }
    return status;
}

void nssf_nsselection_response_free(nssf_nsselection_response_t *out)
{
    if (out == NULL) {
        return;
    }
    free(out->body);
    out->body = NULL;
}
