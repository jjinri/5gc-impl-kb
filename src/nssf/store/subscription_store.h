/*
 * subscription_store.h — SubscriptionStore interface (WI-subscription-store).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten persistence module.
 *
 * Abstract store over the `subscription` table (infra/nssf/schema.sql,
 * schema_version=1) for the inbound Nnssf_NSSAIAvailability subscription
 * lifecycle (SubscriptionStore.md §Responsibility) — Subscribe (POST),
 * SubModifyPatch (PATCH), Unsubscribe (DELETE). Two backends share one vtable
 * behind an opaque handle: a libpq/PostgreSQL backend (production, PQexecParams
 * positional params only) and an in-memory backend (test seam, unit/module-
 * integration only). Column truth = schema.sql.
 *
 * This module owns the subscription resource and its lifecycle ONLY. It does
 * not own the availability map (AvailabilityEngine/Repository) and does not make
 * outbound calls itself — outbound POSTs are NotificationDispatcher's job. On
 * create() it reads the current availability snapshot and enqueues exactly ONE
 * retry_queue row through the dispatcher's store seam, then triggers one
 * call-driven dispatch (NO worker thread / polling loop — see create()).
 *
 * Design intent — design/nssf/module-decomposition/SubscriptionStore.md,
 * design/nssf/architecture/state-persistence.md (`subscription` table +
 * repository abstraction with PG impl + in-memory test seam),
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailability Post/Unsubscribe/
 * SubModifyPatch rows), dev/nssf/open-gaps-and-assumptions.md G-10 (default
 * 24h expiry) + G-08 addendum (no dispatcher hot-loop).
 */
#ifndef NSSF_SUBSCRIPTION_STORE_H
#define NSSF_SUBSCRIPTION_STORE_H

#include <stdbool.h>
#include <stddef.h>

#include "cJSON.h"

#include "availability_engine.h"
#include "availability_repository.h"
#include "notification_dispatcher.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Default subscription lifetime when the request carries no validityTime
 * (G-10). Operator-overridable via subscription.default_expiry config; the
 * value is passed to create() rather than hardcoded inside the store. */
#define NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS (24 * 60 * 60)

/*
 * Result codes shared across the store API. The handler layer maps these to
 * HTTP status (the store itself never speaks HTTP).
 */
typedef enum {
    NSSF_SUB_OK = 0,            /* success. */
    NSSF_SUB_NOT_FOUND = 1,     /* id did not match a live row. */
    NSSF_SUB_BAD_PATCH = 2,     /* RFC 6902 patch invalid → 400. */
    NSSF_SUB_UNSUPPORTED = 3,   /* wrong patch content-type → 415. */
    NSSF_SUB_ERROR = 4,         /* store/internal error. */
} nssf_sub_result_e;

/*
 * One row of the `subscription` table. The record OWNS its cJSON `filter` tree
 * and its heap strings (callback_uri); nssf_subscription_record_clear() frees
 * them. `id` is a fixed 36-char UUID text + NUL. `expiry_at` is a heap copy of
 * the RFC 3339 timestamp text (NULL when the column is SQL NULL / lazily
 * defaulted). `has_expiry` distinguishes "no expiry column" from an empty
 * string.
 */
typedef struct {
    char id[37];
    char *callback_uri;
    cJSON *filter;
    char *expiry_at;   /* RFC 3339 text, or NULL. */
    bool tombstone;
} nssf_subscription_record_t;

/* Opaque handle — backend (libpq or in-memory) hidden behind the vtable. */
typedef struct nssf_subscription_store nssf_subscription_store_t;

/*
 * Construct a libpq-backed store. `conninfo` is a libpq connection string
 * sourced from operator config (never hardcoded). Returns NULL on connect or
 * setup failure and writes a NUL-terminated diagnostic into errbuf (when
 * errbuf != NULL && errlen > 0).
 */
nssf_subscription_store_t *nssf_subscription_store_new_pg(const char *conninfo,
                                                          char *errbuf,
                                                          size_t errlen);

/* Construct the in-memory test-seam store. Returns NULL only on OOM. */
nssf_subscription_store_t *nssf_subscription_store_new_inmemory(void);

/* Tear down a store of either backend (idempotent on NULL). */
void nssf_subscription_store_free(nssf_subscription_store_t *store);

/*
 * Install the initial-snapshot dispatch collaborators. Both are BORROWED (not
 * owned, not freed by the store): the availability repository the store reads
 * the current snapshot from, the dispatcher's retry store the store enqueues
 * the one initial-snapshot row into, and the dispatcher the store triggers a
 * single call-driven dispatch on. When any is NULL, create() persists the
 * subscription but skips the initial-snapshot dispatch (the no-collaborator
 * unit path). plmn_id/tai are the borrowed snapshot key used to read the
 * current availability for newly created subscriptions; pass NULL to skip the
 * snapshot read (no row enqueued).
 */
void nssf_subscription_store_set_snapshot_seam(
    nssf_subscription_store_t *store,
    nssf_availability_repo_t *avail_repo,
    nssf_retry_store_t *retry_store,
    nssf_notification_dispatcher_t *dispatcher);

/*
 * Acceptance #1 — persist a new subscription with its JSONB filter column.
 * `callback_uri` and `filter` are borrowed (the store copies what it keeps).
 * `expiry_seconds` is the effective lifetime: pass
 * NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS when the request omitted
 * validityTime (G-10), or the operator/request value otherwise. A fresh UUID is
 * generated and written into out_id (37-byte buffer) for the Location header.
 *
 * After the row commits, when a snapshot seam is installed (and the snapshot
 * key plmn_id/tai is set) the store reads the current availability snapshot for
 * the filter, enqueues EXACTLY ONE retry_queue row carrying the real
 * subscription UUID + its callbackUri (an nssf_event_notification payload), and
 * triggers ONE call-driven dispatch. This is the
 * phase3_initial_snapshot_dispatch contract — see the .c for the WHY this is
 * exactly one dispatch and carries no worker thread / polling loop.
 *
 * Returns NSSF_SUB_OK and fills out_id on success; NSSF_SUB_ERROR on
 * store/internal failure (out_id is zeroed). A failed initial-snapshot dispatch
 * does NOT fail create() — the subscription is durable and the dispatcher's own
 * retry path owns redelivery.
 */
nssf_sub_result_e nssf_subscription_store_create(
    nssf_subscription_store_t *store,
    const char *callback_uri,
    const cJSON *filter,
    long expiry_seconds,
    char out_id[37]);

/*
 * Acceptance #2 — Unsubscribe. IDEMPOTENT: deleting an absent (or already
 * deleted) subscription is success, not an error. Returns NSSF_SUB_OK whether
 * or not a row matched, NSSF_SUB_ERROR only on a store failure.
 */
nssf_sub_result_e nssf_subscription_store_delete(
    nssf_subscription_store_t *store, const char *id);

/*
 * Acceptance #3 — SubModifyPatch (RFC 6902 JSON Patch, 29.531 §6.1.6 +
 * TS29571 PatchItem). `content_type` is the request Content-Type; anything
 * other than application/json-patch+json returns NSSF_SUB_UNSUPPORTED (415)
 * before the body is touched. `patch_doc` is the borrowed RFC 6902 op array.
 * The patch is applied ATOMICALLY to the subscription resource view (filter +
 * callbackUri + expiry): all ops succeed or none are persisted. Returns
 * NSSF_SUB_OK on commit, NSSF_SUB_NOT_FOUND when id is absent, NSSF_SUB_BAD_PATCH
 * (400) on an invalid op/path, NSSF_SUB_ERROR on a store failure.
 */
nssf_sub_result_e nssf_subscription_store_patch(
    nssf_subscription_store_t *store,
    const char *id,
    const char *content_type,
    const cJSON *patch_doc);

/*
 * Acceptance #4 — cascade tombstone of the subscriptions affected by a deleted
 * nf/availability. Tombstones (soft-deletes) the subscription identified by
 * `subscription_id` so a pending retry_queue row cascades per the schema FK on a
 * later hard delete. Idempotent on an absent/already-tombstoned row. Returns
 * NSSF_SUB_OK on success, NSSF_SUB_ERROR on a store failure. The fan-out
 * integration (nssf_subscription_store_fanout_change) calls this for each matched
 * subscription on a DELETED change event.
 */
nssf_sub_result_e nssf_subscription_store_on_availability_deleted(
    nssf_subscription_store_t *store, const char *subscription_id);

/*
 * Fan-out resolve + enqueue — the engine→store→dispatcher cascade activation
 * (PR-phase3-fanout-integration, closes the B3 deferred fan-out). Resolves a
 * committed AvailabilityEngine change event to the set of LIVE (non-tombstoned)
 * subscriptions whose persisted `filter` matches, and for EACH match enqueues
 * EXACTLY ONE retry_queue row carrying the real subscription UUID + that
 * subscription's callbackUri + an nssf_event_notification-shaped payload, via the
 * borrowed `retry_store` (nssf_retry_store_enqueue). A subscription whose
 * callbackUri fails the shared B1 URL gate
 * (nssf_notification_dispatcher_callback_url_allowed) is skipped (never enqueue an
 * un-postable target). On a DELETED change event each matched subscription is also
 * tombstoned (on_availability_deleted contract / schema FK cascade).
 *
 * This is STORE-LAYER MATCHING ONLY — it performs no outbound POST and starts no
 * loop. The caller (main.c publish trampoline) triggers ONE call-driven
 * dispatch_pending after this returns. `ev` is borrowed (the engine's snapshots
 * are valid only for the call); nothing is retained. `retry_store` is borrowed.
 *
 * Returns the count of rows enqueued (>= 0), or -1 on a store error. A NULL
 * store/retry_store/ev returns -1; a no-match event returns 0.
 */
int nssf_subscription_store_fanout_change(
    nssf_subscription_store_t *store,
    nssf_retry_store_t *retry_store,
    const nssf_availability_change_event_t *ev);

/*
 * Read one live (non-tombstoned) subscription by id. On success returns
 * NSSF_SUB_OK and fills *out (caller frees with
 * nssf_subscription_record_clear); NSSF_SUB_NOT_FOUND when id is absent or
 * tombstoned (out zeroed); NSSF_SUB_ERROR on a store failure. Supports the
 * handler read-back and unit assertions.
 */
nssf_sub_result_e nssf_subscription_store_get(
    nssf_subscription_store_t *store,
    const char *id,
    nssf_subscription_record_t *out);

/* Free the cJSON filter + heap strings of a record and zero it. */
void nssf_subscription_record_clear(nssf_subscription_record_t *rec);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_SUBSCRIPTION_STORE_H */
