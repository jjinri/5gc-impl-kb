/*
 * nssaiavailability_put_handler.h — NSSAIAvailabilityPut inbound HTTP handler
 * (WI-availability-handlers).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten inbound handler.
 *
 * Implements PUT /nssai-availability/{nfId} per
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailabilityPut row):
 *
 *   handler → AvailabilityEngine::replace()    [full-replace business decision]
 *   write AvailabilityRepository::upsert()      [via engine, single transaction]
 *   security: TLS (ADR-0004) + OAuth2 scope nnssf-nssaiavailability (M3 inbound)
 *
 * The handler is transport-agnostic on purpose. A later server-wiring step (the
 * nghttp2 router slice) decodes one inbound HTTP/2 request into
 * nssf_nssaiavailability_put_request_t and calls
 * nssf_nssaiavailability_put_handle(); the same entry drives integration tests
 * with no live socket. The security gate (bearer validation + scope match) runs
 * BEFORE any business logic — a deny short-circuits to 401/403 before the
 * AvailabilityEngine is touched.
 *
 * Deferred enqueue seam — the WI's "retry_queue enqueue for NotificationDispatcher"
 * is the AvailabilityEngine change-event publish, injected at engine construction
 * (#124). The consumer (SubscriptionStore phase3 + NotificationDispatcher M4) does
 * not exist yet, so the phase2 publish is a no-op; the handler neither owns nor
 * fires the seam — committing the replace IS the enqueue intent.
 *
 * M4-independence: this slice is inbound only — scope check + engine replace +
 * deferred change-event seam. There is no libcurl/NRF/notification egress here.
 */
#ifndef NSSF_HANDLERS_NSSAIAVAILABILITY_PUT_HANDLER_H
#define NSSF_HANDLERS_NSSAIAVAILABILITY_PUT_HANDLER_H

#include <stddef.h>

#include "availability_engine.h"
#include "oauth2_jwks.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Per-request inbound view, populated by the transport from the decoded HTTP/2
 * request. All pointers are BORROWED for the duration of the call.
 *
 *   authorization  — full "Authorization" header value, e.g. "Bearer <jwt>".
 *                    NULL/empty → token-missing (401).
 *   nf_id          — the {nfId} path parameter (NfInstanceId, the record key).
 *                    NULL/empty → 400 invalid-body (no target record).
 *   content_type   — request body media type. PUT body is NssaiAvailabilityInfo
 *                    (application/json); a non-json content_type → 415.
 *   body           — request body JSON string (borrowed, NUL-terminated). Parsed
 *                    as NssaiAvailabilityInfo; non-JSON / empty → 400.
 *   content_length — declared body length. has_content_length=false where a body
 *                    is required → 411. content_length > max_body_len → 413.
 *   has_content_length — whether a Content-Length header was present.
 *   max_body_len   — operator-configured body cap. 0 → no cap.
 *   request_target — the :path pseudo-header (diagnostics / ProblemDetails
 *                    instance). May be NULL.
 *   max_uri_len    — request-target length cap. Longer → 414. 0 → no cap.
 */
typedef struct {
    const char *authorization;
    const char *nf_id;
    const char *content_type;
    const char *body;
    size_t content_length;
    bool has_content_length;
    size_t max_body_len;
    const char *request_target;
    size_t max_uri_len;
} nssf_nssaiavailability_put_request_t;

/*
 * Handler dependencies, wired once at process bootstrap and shared across
 * requests. All borrowed — the handler owns none of them.
 *
 *   jwks_cache — inbound OAuth2 bearer validator (M3). NULL → 500.
 *   engine     — AvailabilityEngine over the AvailabilityRepository + the
 *                change-event seam (wired at engine construction). NULL → 500.
 */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_availability_engine_t *engine;
} nssf_nssaiavailability_put_deps_t;

/*
 * Outbound response. The caller sends `status` + `content_type` + `body`, then
 * frees the body with nssf_nssaiavailability_put_response_free().
 *
 *   status       — HTTP status (200 / 204 / 400 / 401 / 403 / 411 / 413 / 415 /
 *                  429 / 500 / 503).
 *   content_type — "application/json" for 200, "application/problem+json" for
 *                  every error. Static string — do not free.
 *   body         — heap-allocated NUL-terminated JSON body, or NULL (204 / OOM).
 */
typedef struct {
    int status;
    const char *content_type;
    char *body;
} nssf_nssaiavailability_put_response_t;

#ifndef NSSF_CT_JSON
#define NSSF_CT_JSON          "application/json"
#endif
#ifndef NSSF_CT_PROBLEM_JSON
#define NSSF_CT_PROBLEM_JSON  "application/problem+json"
#endif

/*
 * Handle one PUT /nssai-availability/{nfId} request.
 *
 * Order:
 *   1. security gate — bearer present? token valid? scope nnssf-nssaiavailability?
 *      Failures answer 401/403 and never reach the engine.
 *   2. request-envelope checks — 414 (uri cap), 411 (no Content-Length),
 *      413 (body over cap), 415 (non-json content_type).
 *   3. parse body as NssaiAvailabilityInfo cJSON; non-JSON → 400.
 *   4. AvailabilityEngine::replace() → AuthorizedNssaiAvailabilityInfo cJSON.
 *   5. map result → 200 (OK + body), 204 (NO_CONTENT), 400 (BAD_REQUEST),
 *      415 (UNSUPPORTED_MEDIA_TYPE), 503 (ERR_INTERNAL — repo/DB failure),
 *      500 (catastrophic).
 *
 * `req`, `deps`, `out` must be non-NULL. Returns the chosen HTTP status (also in
 * out->status). The function never blocks on the network — the only I/O is the
 * availability upsert performed inside the engine.
 */
int nssf_nssaiavailability_put_handle(
    const nssf_nssaiavailability_put_request_t *req,
    const nssf_nssaiavailability_put_deps_t *deps,
    nssf_nssaiavailability_put_response_t *out);

/* Free a response body (idempotent). */
void nssf_nssaiavailability_put_response_free(
    nssf_nssaiavailability_put_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_HANDLERS_NSSAIAVAILABILITY_PUT_HANDLER_H */
