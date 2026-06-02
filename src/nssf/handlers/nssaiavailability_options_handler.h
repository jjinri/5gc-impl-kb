/*
 * nssaiavailability_options_handler.h — NSSAIAvailabilityOptions inbound HTTP
 * handler (WI-availability-handlers).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten inbound handler.
 *
 * Implements OPTIONS /nssai-availability per
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailabilityOptions row):
 *
 *   handler → AvailabilityEngine::options()    [static method metadata]
 *   security: TLS (ADR-0004) + OAuth2 scope nnssf-nssaiavailability (M3 inbound)
 *
 * No request body and no persistence — the response is metadata only. The
 * handler answers 200 (empty body, contract success status) with an `Allow`
 * header value listing the supported methods (e.g. "PUT, PATCH, DELETE,
 * OPTIONS"). The integration probe asserts the `Allow:` header exact match.
 *
 * Transport-agnostic: the server-wiring step fills the request struct, calls
 * nssf_nssaiavailability_options_handle(), and emits `out.allow` as the `Allow`
 * response header. The security gate runs BEFORE the engine call.
 *
 * M4-independence: inbound only — scope check + engine options (pure, no I/O).
 * No libcurl/NRF/notification egress, no change-event seam (read-only metadata).
 */
#ifndef NSSF_HANDLERS_NSSAIAVAILABILITY_OPTIONS_HANDLER_H
#define NSSF_HANDLERS_NSSAIAVAILABILITY_OPTIONS_HANDLER_H

#include <stddef.h>

#include "availability_engine.h"
#include "oauth2_jwks.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Per-request inbound view (all pointers BORROWED).
 *
 *   authorization  — "Authorization" header. NULL/empty → 401.
 *   request_target — :path pseudo-header (diagnostics). May be NULL.
 *   max_uri_len    — request-target length cap. Longer → 414. 0 → no cap.
 */
typedef struct {
    const char *authorization;
    const char *request_target;
    size_t max_uri_len;
} nssf_nssaiavailability_options_request_t;

/* Handler dependencies (all borrowed). NULL jwks_cache/engine → 500. */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_availability_engine_t *engine;
} nssf_nssaiavailability_options_deps_t;

/* Allow-header value buffer length — fits "PUT, PATCH, DELETE, OPTIONS" + slack. */
#define NSSF_NSSAIAVAIL_ALLOW_LEN 64

/*
 * Outbound response. status ∈ {200, 401, 403, 414, 429, 500}.
 *
 *   allow — NUL-terminated Allow-header value the engine produced. Emitted as
 *           the `Allow` response header on 200. Empty on any error.
 *   content_type — static; "application/problem+json" for errors, NULL for 200.
 *   body  — heap-allocated (errors) or NULL (200).
 */
typedef struct {
    int status;
    char allow[NSSF_NSSAIAVAIL_ALLOW_LEN];
    const char *content_type;
    char *body;
} nssf_nssaiavailability_options_response_t;

#ifndef NSSF_CT_PROBLEM_JSON
#define NSSF_CT_PROBLEM_JSON  "application/problem+json"
#endif

/*
 * Handle one OPTIONS /nssai-availability request.
 *
 * Order:
 *   1. security gate — bearer + scope nnssf-nssaiavailability. Deny → 401/403.
 *   2. request-target length cap → 414.
 *   3. AvailabilityEngine::options() → Allow value into out->allow.
 *   4. 200 (empty body) with the Allow header (or 500 on a buffer/engine error).
 *
 * `req`, `deps`, `out` non-NULL. Returns the chosen status (also in out->status).
 */
int nssf_nssaiavailability_options_handle(
    const nssf_nssaiavailability_options_request_t *req,
    const nssf_nssaiavailability_options_deps_t *deps,
    nssf_nssaiavailability_options_response_t *out);

void nssf_nssaiavailability_options_response_free(
    nssf_nssaiavailability_options_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_HANDLERS_NSSAIAVAILABILITY_OPTIONS_HANDLER_H */
