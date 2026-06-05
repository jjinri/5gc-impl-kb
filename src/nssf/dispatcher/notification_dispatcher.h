/*
 * notification_dispatcher.h — NSSF NotificationDispatcher (WI-notification-dispatcher).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten outbound module.
 *
 * The NSSF's single outbound responsibility (NotificationDispatcher.md
 * §Responsibility) — turn a committed AvailabilityEngine change event into an
 * outbound notification POST to a subscription's callback URI (AMF). This slice
 * is the Phase 1~3 MINIMUM (open-gaps-and-assumptions G-08): synchronous
 * dispatch + ONE retry. There is no exponential backoff, no max_attempts ceiling,
 * no jitter, and no dead-letter table here — those are Phase 4 hardening.
 *
 * Two responsibilities behind one handle:
 *   1. PRODUCER — change_publish() matches the engine's
 *      nssf_availability_change_publish_fn seam
 *      (src/nssf/engine/availability_engine.h). The AvailabilityEngine invokes it
 *      after a committed mutation; it enqueues one retry_queue row. This is the
 *      "Put/Patch 후 retry_queue 에 row enqueue" acceptance. WHY it does NOT
 *      fabricate a subscription_id: real fan-out (changed nfId/tai → affected
 *      subscriptions + their callbackUri) is the Phase-3 SubscriptionStore's job.
 *      A retry_queue row is only enqueued when a real subscription UUID AND a
 *      callbackUri are both present (the in-memory test backend supplies them; the
 *      production producer-to-PG wiring stays behind a not-ready path until
 *      Phase 3 — see change_publish()). The slice deliverable is the
 *      enqueue-on-commit SEAM, not the fan-out.
 *   2. CONSUMER — dispatch_pending() dequeues a row (FOR UPDATE SKIP LOCKED on
 *      the libpq backend), POSTs once to the callback URI, and on a retriable
 *      failure (5xx / timeout / connection / TLS handshake) retries exactly once
 *      before stopping (4xx → no retry).
 *
 * Security baseline (ADR-0004 Layer A) on the outbound POST:
 *   - The callback URI is validated by a SHARED url policy gate BEFORE any token
 *     is attached and BEFORE the POST is emitted: https-only (production),
 *     userinfo('@') / fragment('#') / malformed rejected. WHY before-attach: this
 *     closes the window where an OAuth2 bearer could be attached to a plaintext or
 *     attacker-shifted URL — libcurl's own protocol guard alone runs too late.
 *   - TLS peer + hostname verification MANDATORY, min TLS 1.2, redirects never
 *     followed — delegated to libcurl/OpenSSL, never disabled. The libcurl
 *     transport additionally caps the allowed protocol to HTTPS as belt-and-
 *     suspenders.
 *   - https-only for the production callback; a test-only loopback dispatcher
 *     mirrors oauth2_outbound's create/create_insecure split for http://127.0.0.1
 *     mocks (nssf_notification_dispatcher_new vs _new_insecure).
 *   - OAuth2 bearer attach via nssf_oauth2_outbound_attach_bearer (first M4
 *     consumer); fail-closed — NSSF_OAUTH2_ABORT FORBIDS the POST.
 *
 * Two injected seams keep the module unit-testable with no DB and no network:
 *   - the retry-queue STORE seam (in-memory test backend / libpq production),
 *   - the outbound TRANSPORT seam (mock AMF callback / libcurl production),
 *     installed exactly like nssf_oauth2_outbound_set_transport.
 *
 * Design intent — design/nssf/module-decomposition/NotificationDispatcher.md,
 * design/nssf/architecture/state-persistence.md (retry_queue strategy),
 * dev/nssf/open-gaps-and-assumptions.md G-08 (minimal retry policy).
 */
#ifndef NSSF_NOTIFICATION_DISPATCHER_H
#define NSSF_NOTIFICATION_DISPATCHER_H

#include <stdbool.h>
#include <stddef.h>

#include <curl/curl.h>

#include "availability_engine.h"
#include "oauth2_outbound.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nssf_notification_dispatcher nssf_notification_dispatcher_t;

/*
 * Shared callback-URL policy gate (B1), exported for the inbound Subscription
 * Post handler to validate a request's callbackUri with the SAME rules the
 * outbound dispatch path enforces — there is exactly one policy, never a
 * duplicated inline copy. PRODUCTION posture: https-only, plus userinfo('@') /
 * fragment('#') / empty-authority / malformed / NULL rejected. Returns true when
 * the URL is acceptable for an outbound notification POST.
 */
bool nssf_notification_dispatcher_callback_url_allowed(const char *url);

/*
 * One dequeued retry-queue work item. The dispatcher OWNS the heap strings and
 * frees them; a backend that fills this on dequeue transfers ownership to the
 * dispatcher. `id` keys the row for the done/requeue follow-up (an opaque text
 * form of the BIGSERIAL primary key). `payload_json` is the serialized
 * notification body that gets POSTed; callback_uri + correlation_id are carried
 * alongside the body because the Phase 1~3 retry_queue schema persists only
 * subscription_id/payload/attempt_count/next_attempt_at (infra/nssf/schema.sql)
 * — the per-row routing/trace metadata rides in the row's payload envelope.
 */
typedef struct {
    char *id;              /* opaque row key (BIGSERIAL text). */
    char *subscription_id; /* UUID of the originating subscription. */
    char *callback_uri;    /* AMF callback URI the POST targets. */
    char *payload_json;    /* serialized notification body. */
    char *correlation_id;  /* inbound 3gpp-Sbi-Correlation-Info (may be NULL). */
    int attempt_count;     /* attempts already made for this row. */
} nssf_retry_item_t;

/*
 * Retry-queue STORE seam. Two backends share this vtable behind an opaque handle
 * (mirrors nssf_availability_repo): a libpq backend (production) and an in-memory
 * backend (unit-test seam). Production dequeue uses FOR UPDATE SKIP LOCKED.
 */
typedef struct nssf_retry_store nssf_retry_store_t;

/*
 * Construct the libpq-backed retry store. `conninfo` is an operator-config libpq
 * connection string (never hardcoded). Returns NULL on connect failure; writes a
 * NUL-terminated diagnostic into errbuf when errbuf != NULL && errlen > 0.
 *
 * CONCURRENCY CONTRACT (F4) — the PG store is SINGLE-WORKER / NOT thread-safe.
 * Ownership unit = ONE store (and its one PGconn) per worker. dequeue() opens a
 * transaction on that single connection and holds the row under FOR UPDATE SKIP
 * LOCKED until complete()/requeue() commits; a second concurrent caller on the
 * SAME store would corrupt that in-flight transaction. Cross-WORKER concurrency
 * is safe (SKIP LOCKED prevents double-send across separate conns); intra-store
 * concurrency is the caller's contract to forbid — give each worker its own
 * store/conn. The PG backend therefore takes NO internal mutex (unlike the
 * in-memory test seam) and guards the contract with a debug owner-check.
 */
nssf_retry_store_t *nssf_retry_store_new_pg(const char *conninfo,
                                            char *errbuf,
                                            size_t errlen);

/* Construct the in-memory test-seam store. Returns NULL only on OOM. */
nssf_retry_store_t *nssf_retry_store_new_inmemory(void);

/* Tear down a store of either backend (idempotent on NULL). */
void nssf_retry_store_free(nssf_retry_store_t *store);

/*
 * Enqueue one pending row. `subscription_id`, `callback_uri`, `payload_json` and
 * (optional) `correlation_id` are borrowed — the store copies what it keeps.
 * Returns 0 on commit, -1 on error.
 *
 * Defense-in-depth: a STRUCTURALLY un-dispatchable callback_uri is hard-rejected
 * at THIS enqueue boundary (returns -1, no row written) — NULL/empty, userinfo
 * ('@'), fragment('#'), empty-authority, or an un-dispatchable scheme. This is the
 * permissive structural check (it admits BOTH production https and the test
 * loopback-http target); the https-only decision stays a dispatch-time / ctor
 * concern, re-checked by the dispatch-path policy gate as the second layer.
 */
int nssf_retry_store_enqueue(nssf_retry_store_t *store,
                             const char *subscription_id,
                             const char *callback_uri,
                             const char *payload_json,
                             const char *correlation_id);

/*
 * Dequeue the next due pending row (lowest next_attempt_at). On the libpq backend
 * this opens a transaction and locks the row with FOR UPDATE SKIP LOCKED so
 * concurrent dispatchers do not double-send. Returns 1 and fills *out (owned, the
 * dispatcher frees with nssf_retry_item_clear) when a row was claimed, 0 when the
 * queue has no due row, -1 on error. *out is zeroed on 0/-1.
 */
int nssf_retry_store_dequeue(nssf_retry_store_t *store, nssf_retry_item_t *out);

/* Delete the claimed row — terminal success or terminal (non-retriable) failure. */
int nssf_retry_store_complete(nssf_retry_store_t *store, const char *id);

/*
 * Re-arm the claimed row for ONE more immediate attempt (Phase 1~3: no backoff —
 * next_attempt_at = NOW()). attempt_count is incremented. Returns 0 on success.
 */
int nssf_retry_store_requeue(nssf_retry_store_t *store, const char *id);

/* Free the heap strings of a dequeued item and zero it. */
void nssf_retry_item_clear(nssf_retry_item_t *item);

/*
 * Outbound TRANSPORT seam. The default transport POSTs `body` to `callback_uri`
 * over libcurl with TLS peer+host verification, min TLS 1.2, and no
 * redirect-follow. Tests inject a mock AMF callback by replacing this with
 * nssf_notification_dispatcher_set_transport.
 *
 * `headers` is the fully built curl_slist (Content-Type, correlation headers, and
 * — when M4 is enabled — Authorization: Bearer). On success the callback sets
 * *out_http_status to the response status and returns 0; any transport / connect
 * / TLS failure returns nonzero (so the dispatcher classifies it as retriable).
 */
typedef int (*nssf_notification_transport_fn)(
    const nssf_notification_dispatcher_t *disp,
    const char *callback_uri,
    const char *body,
    struct curl_slist *headers,
    long *out_http_status,
    void *transport_ctx);

/*
 * Construct a PRODUCTION dispatcher.
 *   store — borrowed retry store (not owned, not freed by the dispatcher).
 *   oauth — borrowed outbound-OAuth2 handle for bearer attach (may be NULL → no
 *           bearer is ever attached, the dev no-attach path).
 * Returns NULL on OOM or when store is NULL.
 *
 * The callback URI is enforced https-only by the url policy gate: a row whose
 * callbackUri is not https:// (or is malformed / carries userinfo / fragment) is
 * dropped terminally and NO token is attached. Use _new_insecure for loopback
 * mock targets in tests.
 */
nssf_notification_dispatcher_t *nssf_notification_dispatcher_new(
    nssf_retry_store_t *store,
    nssf_oauth2_outbound_t *oauth);

/*
 * TEST/DEV ONLY. Mirrors nssf_oauth2_outbound_create_insecure: relaxes the
 * https-only callback gate so integration tests can POST to an http://127.0.0.1
 * (or [::1] / localhost) loopback mock AMF callback. A non-loopback http target
 * is still rejected, and userinfo('@') / fragment('#') / malformed URLs are
 * rejected on BOTH ctors. MUST NOT be used in production. Same ownership /
 * NULL-on-failure contract as _new.
 */
nssf_notification_dispatcher_t *nssf_notification_dispatcher_new_insecure(
    nssf_retry_store_t *store,
    nssf_oauth2_outbound_t *oauth);

void nssf_notification_dispatcher_free(nssf_notification_dispatcher_t *disp);

/*
 * Install a custom outbound transport (mock AMF callback) and its context.
 * transport_fn==NULL restores the default libcurl transport. ctx lifetime is the
 * caller's. Mirrors nssf_oauth2_outbound_set_transport.
 */
void nssf_notification_dispatcher_set_transport(
    nssf_notification_dispatcher_t *disp,
    nssf_notification_transport_fn transport_fn,
    void *transport_ctx);

/*
 * PRODUCER seam. Signature-compatible with nssf_availability_change_publish_fn
 * so it can be wired directly into nssf_availability_engine_seams_t.publish. `ctx`
 * MUST be the nssf_notification_dispatcher_t*. Enqueues one retry_queue row built
 * from the change event. Borrowed event; nothing is retained past the call.
 */
void nssf_notification_dispatcher_change_publish(
    void *ctx, const nssf_availability_change_event_t *ev);

/*
 * Outcome of dispatching one row.
 */
typedef enum {
    NSSF_DISPATCH_SENT = 0,      /* a row was delivered (2xx). */
    NSSF_DISPATCH_NONE = 1,      /* queue had no due row. */
    NSSF_DISPATCH_DROPPED = 2,   /* terminal failure (4xx, or retry exhausted). */
    NSSF_DISPATCH_FORBIDDEN = 3, /* fail-closed: OAuth2 attach ABORTed the POST. */
    NSSF_DISPATCH_ERROR = 4,     /* store/internal error. */
} nssf_dispatch_result_e;

/*
 * Dequeue the next due row and attempt delivery. On a retriable failure
 * (5xx / timeout / connection / TLS handshake) it retries EXACTLY ONCE in-line
 * (Phase 1~3 minimum, G-08) and then stops — no dead-letter table. A 4xx is
 * terminal (no retry). Returns the outcome classification.
 *
 * This entry is CALL-DRIVEN — it dispatches AT MOST one row per call and does
 * NOT spin an internal polling/worker loop. The caller drives the cadence.
 *
 * CALLER CONTRACT (F3, deferred backoff) — when this returns
 * NSSF_DISPATCH_FORBIDDEN (fail-closed OAuth2 ABORT, row re-armed) or any
 * retriable outcome, the caller MUST NOT tight-loop calling back immediately.
 * There is no backoff / dead-letter / hot-loop guard in THIS phase; a follow-up
 * WI (Phase 4) adds fail-closed backoff + dead-letter. Until then a tight retry
 * loop on FORBIDDEN/retriable status would hammer the NRF token endpoint / AMF.
 *
 * `inbound_correlation_id` overrides the row's stored correlation id for this
 * dispatch when non-NULL; otherwise the row's id (or a freshly generated one) is
 * propagated as 3gpp-Sbi-Correlation-Info + W3C traceparent. Any correlation id
 * is sanitized (CR/LF/control/over-length rejected → safe id regenerated) before
 * it reaches a header — header injection is closed off (B2).
 */
nssf_dispatch_result_e nssf_notification_dispatcher_dispatch_pending(
    nssf_notification_dispatcher_t *disp,
    const char *inbound_correlation_id);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_NOTIFICATION_DISPATCHER_H */
