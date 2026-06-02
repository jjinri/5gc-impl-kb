/*
 * nssaiavailability_patch_handler.h — NSSAIAvailabilityPatch inbound HTTP handler
 * (WI-availability-handlers).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten inbound handler.
 *
 * Implements PATCH /nssai-availability/{nfId} per
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailabilityPatch row):
 *
 *   handler → AvailabilityEngine::patch()    [RFC 6902 atomic apply]
 *   write AvailabilityRepository::patch()      [via engine, UPDATE w/ conflict check]
 *   security: TLS (ADR-0004) + OAuth2 scope nnssf-nssaiavailability (M3 inbound)
 *
 * The PATCH body is a PatchDocument — an `application/json-patch+json` RFC 6902
 * array (29.531 §6.1.6; open-gaps G-04 / G-OQ-01). A non-json-patch+json
 * content-type answers 415. The routing TAI is carried alongside the body (an
 * RFC 6902 array does not embed it) so the engine can read the addressed
 * availability row before applying the ops.
 *
 * Transport-agnostic: a later server-wiring step fills
 * nssf_nssaiavailability_patch_request_t and calls
 * nssf_nssaiavailability_patch_handle(); the same entry drives integration tests
 * with no live socket. The security gate runs BEFORE any business logic.
 *
 * Deferred enqueue seam — the WI's "retry_queue enqueue" is the AvailabilityEngine
 * PATCHED change-event publish (injected at engine construction, #124); in phase2
 * the consumer is absent so the publish is a no-op. The handler neither owns nor
 * fires the seam.
 *
 * M4-independence: inbound only — scope check + engine patch + deferred
 * change-event seam. No libcurl/NRF/notification egress.
 */
#ifndef NSSF_HANDLERS_NSSAIAVAILABILITY_PATCH_HANDLER_H
#define NSSF_HANDLERS_NSSAIAVAILABILITY_PATCH_HANDLER_H

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
 *   nf_id          — the {nfId} path parameter (record key).
 *   content_type   — request body media type. Must be
 *                    "application/json-patch+json" else 415 (engine classifies).
 *   body           — RFC 6902 PatchDocument JSON array string (borrowed). Parsed
 *                    as a cJSON array; non-JSON / non-array → 400 (engine).
 *   tai            — routing Tai JSON document ({"plmnId":{...},"tac":"..."}) the
 *                    transport carries; parsed and passed to the engine as the
 *                    addressed availability row. NULL/non-object → 400 (engine).
 *   content_length — declared body length. has_content_length=false → 411.
 *                    content_length > max_body_len → 413.
 *   has_content_length — whether a Content-Length header was present.
 *   max_body_len   — operator body cap. 0 → no cap.
 *   request_target — :path pseudo-header (diagnostics). May be NULL.
 *   max_uri_len    — request-target length cap. 0 → no cap.
 */
typedef struct {
    const char *authorization;
    const char *nf_id;
    const char *content_type;
    const char *body;
    const char *tai;
    size_t content_length;
    bool has_content_length;
    size_t max_body_len;
    const char *request_target;
    size_t max_uri_len;
} nssf_nssaiavailability_patch_request_t;

/* Handler dependencies (all borrowed). NULL jwks_cache/engine → 500. */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_availability_engine_t *engine;
} nssf_nssaiavailability_patch_deps_t;

/*
 * Outbound response. status ∈ {200, 204, 400, 401, 403, 404, 411, 413, 415,
 * 429, 500, 503}. content_type static. body heap-allocated or NULL.
 */
typedef struct {
    int status;
    const char *content_type;
    char *body;
} nssf_nssaiavailability_patch_response_t;

#ifndef NSSF_CT_JSON
#define NSSF_CT_JSON          "application/json"
#endif
#ifndef NSSF_CT_PROBLEM_JSON
#define NSSF_CT_PROBLEM_JSON  "application/problem+json"
#endif

/*
 * Handle one PATCH /nssai-availability/{nfId} request.
 *
 * Order:
 *   1. security gate — bearer + scope nnssf-nssaiavailability. Deny → 401/403.
 *   2. request-envelope checks — 414, 411, 413.
 *   3. parse body (RFC 6902 array) + tai cJSON.
 *   4. AvailabilityEngine::patch() — content-type 415, op-validation 400,
 *      unknown nfId@tai 404, atomic apply.
 *   5. map result → 200 (OK + body) / 204 (NO_CONTENT) / 400 / 404 / 415 /
 *      503 (ERR_INTERNAL) / 500.
 *
 * `req`, `deps`, `out` non-NULL. Returns the chosen status (also in out->status).
 */
int nssf_nssaiavailability_patch_handle(
    const nssf_nssaiavailability_patch_request_t *req,
    const nssf_nssaiavailability_patch_deps_t *deps,
    nssf_nssaiavailability_patch_response_t *out);

void nssf_nssaiavailability_patch_response_free(
    nssf_nssaiavailability_patch_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_HANDLERS_NSSAIAVAILABILITY_PATCH_HANDLER_H */
