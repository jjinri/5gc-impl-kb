/*
 * subscription_post_handler.h — NSSAIAvailabilityPost inbound HTTP handler
 * (WI-subscription-handlers).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten inbound handler.
 *
 * Implements POST /nssai-availability/subscriptions per
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailabilityPost row):
 *
 *   handler → SubscriptionStore::create(req)    [persist + initial snapshot]
 *   write SubscriptionRepository::insert()       [via store, JSONB filter column]
 *   security: TLS (ADR-0004) + OAuth2 scope nnssf-nssaiavailability (M3 inbound)
 *
 * Success answers 201 Created with a Location header carrying the new
 * subscription resource path. On create() the store performs the ONE call-driven
 * initial-snapshot dispatch through its snapshot seam (no worker thread / polling
 * loop) — the handler does not loop or drive any redispatch cadence.
 *
 * The request callbackUri is validated BEFORE the store is touched, reusing the
 * NotificationDispatcher's shared URL-policy gate
 * (nssf_notification_dispatcher_callback_url_allowed) so the inbound and outbound
 * paths share ONE policy (https-only / no userinfo / no fragment / well-formed).
 *
 * Transport-agnostic: a server-wiring step fills the request struct and calls
 * nssf_subscription_post_handle(); the same entry drives integration tests with
 * no live socket. The security gate runs BEFORE any business logic.
 */
#ifndef NSSF_HANDLERS_SUBSCRIPTION_POST_HANDLER_H
#define NSSF_HANDLERS_SUBSCRIPTION_POST_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

#include "oauth2_jwks.h"
#include "subscription_store.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Location-header value buffer — fits the resource path + 36-char UUID + slack. */
#define NSSF_SUBSCRIPTION_LOCATION_LEN 96

/*
 * Per-request inbound view (all pointers BORROWED).
 *
 *   authorization  — "Authorization" header. NULL/empty → 401.
 *   content_type   — request body media type. Non-json → 415.
 *   body           — request body JSON (borrowed, NUL-terminated). Parsed as the
 *                    subscription create document; non-JSON / empty → 400.
 *   content_length — declared body length. has_content_length=false → 411.
 *                    content_length > max_body_len → 413.
 *   has_content_length — whether a Content-Length header was present.
 *   max_body_len   — operator body cap. 0 → no cap.
 *   request_target — :path pseudo-header (diagnostics). May be NULL.
 *   max_uri_len    — request-target length cap. Longer → 414. 0 → no cap.
 */
typedef struct {
    const char *authorization;
    const char *content_type;
    const char *body;
    size_t content_length;
    bool has_content_length;
    size_t max_body_len;
    const char *request_target;
    size_t max_uri_len;
} nssf_subscription_post_request_t;

/* Handler dependencies (all borrowed). NULL jwks_cache/store → 500. */
typedef struct {
    nssf_jwks_cache_t *jwks_cache;
    nssf_subscription_store_t *store;
} nssf_subscription_post_deps_t;

/*
 * Outbound response. The caller sends status + content_type + body (+ location on
 * 201), then frees the body with nssf_subscription_post_response_free().
 *
 *   status       — HTTP status (201 / 400 / 401 / 403 / 411 / 413 / 414 / 415 /
 *                  500).
 *   location     — NUL-terminated Location-header value, set on 201, "" otherwise.
 *   content_type — static string; "application/problem+json" for errors,
 *                  NULL for 201 (no body).
 *   body         — heap-allocated JSON (errors) or NULL (201 / OOM).
 */
typedef struct {
    int status;
    char location[NSSF_SUBSCRIPTION_LOCATION_LEN];
    const char *content_type;
    char *body;
} nssf_subscription_post_response_t;

#ifndef NSSF_CT_PROBLEM_JSON
#define NSSF_CT_PROBLEM_JSON  "application/problem+json"
#endif

/*
 * Handle one POST /nssai-availability/subscriptions request.
 *
 * Order:
 *   1. security gate — bearer + scope nnssf-nssaiavailability. Deny → 401/403.
 *   2. request-envelope checks — 414, 411, 413, 415 (non-json content_type).
 *   3. parse body; non-JSON → 400. Extract + validate callbackUri via the shared
 *      dispatcher URL-policy gate; reject → 400.
 *   4. SubscriptionStore::create() — persists + fires the ONE call-driven
 *      initial-snapshot dispatch through the store's snapshot seam.
 *   5. 201 Created + Location header (new subscription id), or 500 on store error.
 *
 * `req`, `deps`, `out` non-NULL. Returns the chosen status (also in out->status).
 */
int nssf_subscription_post_handle(
    const nssf_subscription_post_request_t *req,
    const nssf_subscription_post_deps_t *deps,
    nssf_subscription_post_response_t *out);

void nssf_subscription_post_response_free(
    nssf_subscription_post_response_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_HANDLERS_SUBSCRIPTION_POST_HANDLER_H */
