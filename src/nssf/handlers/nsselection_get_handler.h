/*
 * nsselection_get_handler.h — NSSelectionGet inbound HTTP handler
 * (WI-nsselection-handler).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten inbound handler.
 *
 * Implements GET /network-slice-information per
 * dev/nssf/api-implementation-matrix.md (NSSelectionGet row):
 *
 *   handler → SelectionEngine::select()    [business decision]
 *   read-only AvailabilityRepository::find_for_plmn_tai()  [via engine seam]
 *   security: TLS (ADR-0004) + OAuth2 scope nnssf-nsselection (M3 inbound)
 *
 * The handler is transport-agnostic on purpose. The nghttp2 server (server/
 * router) decodes one inbound HTTP/2 request into nssf_nsselection_request_t and
 * calls nssf_nsselection_get_handle(); the same entry drives integration/
 * contract tests with no live socket. The security gate (bearer validation +
 * scope match) runs BEFORE any business logic — a deny short-circuits to 401/403
 * before the SelectionEngine is touched.
 *
 * M4-independence: this slice is inbound only. The AMF-set resolver and
 * subscribed-NSSAI source are injected SelectionEngine seams (operator config /
 * NULL), never outbound network calls. There is no libcurl/NRF egress here.
 */
#ifndef NSSF_HANDLERS_NSSELECTION_GET_HANDLER_H
#define NSSF_HANDLERS_NSSELECTION_GET_HANDLER_H

#include <stddef.h>

#include "oauth2_jwks.h"
#include "selection_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Per-request inbound view, populated by the transport (server/router) from the
 * decoded HTTP/2 request. All pointers are BORROWED for the duration of the
 * call — the handler copies nothing it keeps.
 *
 *   authorization  — full "Authorization" header value, e.g. "Bearer <jwt>".
 *                    NULL/empty → token-missing (401).
 *   raw_query      — the raw URL query string (the part after '?'), percent-
 *                    encoded, NUL-terminated. NULL → empty. The NSSelectionGet
 *                    query params are JSON documents per 29.531 (deep objects
 *                    serialized into single params); the handler parses each
 *                    value as JSON and assembles the SelectionEngine query.
 *   request_target — the :path pseudo-header (used only for diagnostics /
 *                    ProblemDetails instance). May be NULL.
 *   max_uri_len    — operator-configured request-target length cap. A
 *                    request_target longer than this answers 414. 0 → no cap.
 */
typedef struct {
    const char *authorization;
    const char *raw_query;
    const char *request_target;
    size_t max_uri_len;
} nssf_nsselection_request_t;

/*
 * Handler dependencies, wired once at process bootstrap (main.c) and shared
 * across requests. All borrowed — the handler owns none of them.
 *
 *   jwks_cache — inbound OAuth2 bearer validator (M3). Required; NULL forces
 *                500 (a server with no validator cannot serve securely).
 *   engine     — SelectionEngine over the AvailabilityRepository + injected
 *                seams. Required; NULL forces 500.
 */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_selection_engine_t *engine;
} nssf_nsselection_deps_t;

/*
 * Outbound response, produced by the handler. The caller (server/router or a
 * test) sends `status` + `content_type` + `body`, then frees the body with
 * nssf_nsselection_response_free().
 *
 *   status       — HTTP status (200 / 401 / 403 / 404 / 414 / 500).
 *   content_type — "application/json" for 200, "application/problem+json" for
 *                  every error. Static string — do not free.
 *   body         — heap-allocated, NUL-terminated serialized JSON body. NULL
 *                  only on catastrophic OOM (caller then sends an empty body).
 */
typedef struct {
    int status;
    const char *content_type;
    char *body;
} nssf_nsselection_response_t;

/* Content-type constants for the two response families. */
#define NSSF_CT_JSON          "application/json"
#define NSSF_CT_PROBLEM_JSON  "application/problem+json"

/*
 * Handle one GET /network-slice-information request.
 *
 * Order (request-flow.md §inbound + acceptance):
 *   1. security gate — bearer present? token valid? scope nnssf-nsselection?
 *      Failures answer 401/403 (application/problem+json) and never reach the
 *      engine.
 *   2. request-target length cap → 414 when exceeded.
 *   3. parse the query into the SelectionEngine cJSON shape.
 *   4. SelectionEngine::select() → AuthorizedNetworkSliceInfo cJSON.
 *   5. map result → 200 (FULL_ALLOW/PARTIAL), 404 (no availability / engine
 *      error keyed to the TAI read), 500 (internal failure).
 *
 * `req`, `deps`, `out` must be non-NULL. Returns the chosen HTTP status (also
 * stored in out->status); out->body is always set on a non-NULL out (NULL only
 * on OOM). The function never blocks on the network — the only I/O is the
 * read-only availability SELECT performed inside the engine.
 */
int nssf_nsselection_get_handle(const nssf_nsselection_request_t *req,
                                const nssf_nsselection_deps_t *deps,
                                nssf_nsselection_response_t *out);

/* Free a response body produced by nssf_nsselection_get_handle (idempotent). */
void nssf_nsselection_response_free(nssf_nsselection_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_HANDLERS_NSSELECTION_GET_HANDLER_H */
