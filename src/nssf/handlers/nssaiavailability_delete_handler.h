/*
 * nssaiavailability_delete_handler.h — NSSAIAvailabilityDelete inbound HTTP
 * handler (WI-availability-handlers).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten inbound handler.
 *
 * Implements DELETE /nssai-availability/{nfId} per
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailabilityDelete row):
 *
 *   handler → AvailabilityEngine::delete()    [remove nfId availability]
 *   write AvailabilityRepository::delete()      [via engine, libpq DELETE]
 *   security: TLS (ADR-0004) + OAuth2 scope nnssf-nssaiavailability (M3 inbound)
 *
 * No request body — only the {nfId} path parameter. A successful delete answers
 * 204; an unknown nfId answers 404.
 *
 * Transport-agnostic: a later server-wiring step fills the request struct and
 * calls nssf_nssaiavailability_delete_handle(); the same entry drives tests with
 * no socket. The security gate runs BEFORE any business logic.
 *
 * Deferred cascade seam — the WI's "cascade subscription tombstone via
 * SubscriptionStore::on_availability_deleted()" is satisfied by the
 * AvailabilityEngine DELETED change-event publish (injected at engine
 * construction, #124). SubscriptionStore (phase3) is the consumer and does not
 * exist yet, so the phase2 publish is a no-op. The handler neither owns nor fires
 * the seam and never calls SubscriptionStore.
 *
 * M4-independence: inbound only — scope check + engine delete + deferred
 * change-event seam. No libcurl/NRF/notification egress.
 */
#ifndef NSSF_HANDLERS_NSSAIAVAILABILITY_DELETE_HANDLER_H
#define NSSF_HANDLERS_NSSAIAVAILABILITY_DELETE_HANDLER_H

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
 *   nf_id          — the {nfId} path parameter (record key). Unknown → 404.
 *   request_target — :path pseudo-header (diagnostics). May be NULL.
 *   max_uri_len    — request-target length cap. Longer → 414. 0 → no cap.
 */
typedef struct {
    const char *authorization;
    const char *nf_id;
    const char *request_target;
    size_t max_uri_len;
} nssf_nssaiavailability_delete_request_t;

/* Handler dependencies (all borrowed). NULL jwks_cache/engine → 500. */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_availability_engine_t *engine;
} nssf_nssaiavailability_delete_deps_t;

/*
 * Outbound response. status ∈ {204, 401, 403, 404, 414, 429, 500, 503}.
 * content_type static. body heap-allocated (errors) or NULL (204).
 */
typedef struct {
    int status;
    const char *content_type;
    char *body;
} nssf_nssaiavailability_delete_response_t;

#ifndef NSSF_CT_JSON
#define NSSF_CT_JSON          "application/json"
#endif
#ifndef NSSF_CT_PROBLEM_JSON
#define NSSF_CT_PROBLEM_JSON  "application/problem+json"
#endif

/*
 * Handle one DELETE /nssai-availability/{nfId} request.
 *
 * Order:
 *   1. security gate — bearer + scope nnssf-nssaiavailability. Deny → 401/403.
 *   2. request-target length cap → 414.
 *   3. AvailabilityEngine::delete() — unknown nfId → 404, success → 204.
 *   4. map result → 204 (NO_CONTENT) / 404 / 400 / 503 (ERR_INTERNAL) / 500.
 *
 * `req`, `deps`, `out` non-NULL. Returns the chosen status (also in out->status).
 */
int nssf_nssaiavailability_delete_handle(
    const nssf_nssaiavailability_delete_request_t *req,
    const nssf_nssaiavailability_delete_deps_t *deps,
    nssf_nssaiavailability_delete_response_t *out);

void nssf_nssaiavailability_delete_response_free(
    nssf_nssaiavailability_delete_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_HANDLERS_NSSAIAVAILABILITY_DELETE_HANDLER_H */
