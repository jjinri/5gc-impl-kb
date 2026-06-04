/*
 * subscription_unsubscribe_handler.h — NSSAIAvailabilityUnsubscribe inbound HTTP
 * handler (WI-subscription-handlers).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten inbound handler.
 *
 * Implements DELETE /nssai-availability/subscriptions/{subscriptionId} per
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailabilityUnsubscribe row):
 *
 *   handler → SubscriptionStore::delete(sub_id)    [remove subscription]
 *   write SubscriptionRepository::delete()          [via store, libpq DELETE]
 *   security: TLS (ADR-0004) + OAuth2 scope nnssf-nssaiavailability (M3 inbound)
 *
 * No request body — only the {subscriptionId} path parameter. The store delete is
 * IDEMPOTENT (deleting an absent / already-deleted subscription is success), so a
 * successful or no-op delete both answer 204; only a store failure answers 500.
 *
 * Transport-agnostic: a server-wiring step fills the request struct and calls
 * nssf_subscription_unsubscribe_handle(); the same entry drives tests with no
 * socket. The security gate runs BEFORE any business logic.
 */
#ifndef NSSF_HANDLERS_SUBSCRIPTION_UNSUBSCRIBE_HANDLER_H
#define NSSF_HANDLERS_SUBSCRIPTION_UNSUBSCRIBE_HANDLER_H

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
 *   request_target  — :path pseudo-header (diagnostics). May be NULL.
 *   max_uri_len     — request-target length cap. Longer → 414. 0 → no cap.
 */
typedef struct {
    const char *authorization;
    const char *subscription_id;
    const char *request_target;
    size_t max_uri_len;
} nssf_subscription_unsubscribe_request_t;

/* Handler dependencies (all borrowed). NULL jwks_cache/store → 500. */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_subscription_store_t *store;
} nssf_subscription_unsubscribe_deps_t;

/*
 * Outbound response. status ∈ {204, 401, 403, 414, 500}. content_type static.
 * body heap-allocated (errors) or NULL (204).
 */
typedef struct {
    int status;
    const char *content_type;
    char *body;
} nssf_subscription_unsubscribe_response_t;

#ifndef NSSF_CT_JSON
#define NSSF_CT_JSON          "application/json"
#endif
#ifndef NSSF_CT_PROBLEM_JSON
#define NSSF_CT_PROBLEM_JSON  "application/problem+json"
#endif

/*
 * Handle one DELETE /nssai-availability/subscriptions/{subscriptionId} request.
 *
 * Order:
 *   1. security gate — bearer + scope nnssf-nssaiavailability. Deny → 401/403.
 *   2. request-target length cap → 414.
 *   3. SubscriptionStore::delete() — idempotent; success/no-op → 204.
 *   4. map result → 204 (OK) / 500 (store error).
 *
 * `req`, `deps`, `out` non-NULL. Returns the chosen status (also in out->status).
 */
int nssf_subscription_unsubscribe_handle(
    const nssf_subscription_unsubscribe_request_t *req,
    const nssf_subscription_unsubscribe_deps_t *deps,
    nssf_subscription_unsubscribe_response_t *out);

void nssf_subscription_unsubscribe_response_free(
    nssf_subscription_unsubscribe_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_HANDLERS_SUBSCRIPTION_UNSUBSCRIBE_HANDLER_H */
