/*
 * subscription_patch_handler.h — NSSAIAvailabilitySubModifyPatch inbound HTTP
 * handler (WI-subscription-handlers).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten inbound handler.
 *
 * Implements PATCH /nssai-availability/subscriptions/{subscriptionId} per
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailabilitySubModifyPatch row):
 *
 *   handler → SubscriptionStore::patch(sub_id, patch_doc)    [RFC 6902 apply]
 *   write SubscriptionRepository::patch()                     [via store, UPDATE]
 *   security: TLS (ADR-0004) + OAuth2 scope nnssf-nssaiavailability (M3 inbound)
 *
 * The body is a PatchDocument — an `application/json-patch+json` RFC 6902 array
 * (29.531 §6.1.6). A non-json-patch+json content-type answers 415 (the store
 * classifies it before the body is touched). A successful atomic apply answers
 * 200; an absent id answers 404; an invalid op/path answers 400.
 *
 * Transport-agnostic: a server-wiring step fills the request struct and calls
 * nssf_subscription_patch_handle(); the same entry drives tests with no socket.
 * The security gate runs BEFORE any business logic.
 */
#ifndef NSSF_HANDLERS_SUBSCRIPTION_PATCH_HANDLER_H
#define NSSF_HANDLERS_SUBSCRIPTION_PATCH_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

#include "oauth2_jwks.h"
#include "subscription_store.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Per-request inbound view (all pointers BORROWED).
 *
 *   authorization   — "Authorization" header. NULL/empty → 401.
 *   subscription_id — the {subscriptionId} path parameter (record key).
 *   content_type    — request body media type. Must be
 *                     "application/json-patch+json" else 415 (store classifies).
 *   body            — RFC 6902 PatchDocument JSON array string (borrowed). Parsed
 *                     as a cJSON array; non-JSON / non-array → 400.
 *   content_length  — declared body length. has_content_length=false → 411.
 *                     content_length > max_body_len → 413.
 *   has_content_length — whether a Content-Length header was present.
 *   max_body_len    — operator body cap. 0 → no cap.
 *   request_target  — :path pseudo-header (diagnostics). May be NULL.
 *   max_uri_len     — request-target length cap. 0 → no cap.
 */
typedef struct {
    const char *authorization;
    const char *subscription_id;
    const char *content_type;
    const char *body;
    size_t content_length;
    bool has_content_length;
    size_t max_body_len;
    const char *request_target;
    size_t max_uri_len;
} nssf_subscription_patch_request_t;

/* Handler dependencies (all borrowed). NULL jwks_cache/store → 500. */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_subscription_store_t *store;
} nssf_subscription_patch_deps_t;

/*
 * Outbound response. status ∈ {200, 400, 401, 403, 404, 411, 413, 414, 415,
 * 500}. content_type static. body heap-allocated (errors) or NULL.
 */
typedef struct {
    int status;
    const char *content_type;
    char *body;
} nssf_subscription_patch_response_t;

#ifndef NSSF_CT_JSON
#define NSSF_CT_JSON          "application/json"
#endif
#ifndef NSSF_CT_PROBLEM_JSON
#define NSSF_CT_PROBLEM_JSON  "application/problem+json"
#endif

/*
 * Handle one PATCH /nssai-availability/subscriptions/{subscriptionId} request.
 *
 * Order:
 *   1. security gate — bearer + scope nnssf-nssaiavailability. Deny → 401/403.
 *   2. request-envelope checks — 414, 411, 413.
 *   3. parse body (RFC 6902 array); non-JSON → 400.
 *   4. SubscriptionStore::patch() — content-type 415, invalid op 400, absent 404,
 *      atomic apply.
 *   5. map result → 200 (OK) / 400 / 404 / 415 / 500.
 *
 * `req`, `deps`, `out` non-NULL. Returns the chosen status (also in out->status).
 */
int nssf_subscription_patch_handle(
    const nssf_subscription_patch_request_t *req,
    const nssf_subscription_patch_deps_t *deps,
    nssf_subscription_patch_response_t *out);

void nssf_subscription_patch_response_free(
    nssf_subscription_patch_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_HANDLERS_SUBSCRIPTION_PATCH_HANDLER_H */
