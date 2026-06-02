/*
 * availability_engine.h — AvailabilityEngine for inbound Nnssf_NSSAIAvailability
 * (WI-availability-engine).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten business-logic module.
 *
 * Implements the synchronous CRUD core of
 * design/nssf/module-decomposition/AvailabilityEngine.md — the store / patch /
 * delete / options business logic over an AMF's per-nfId S-NSSAI availability.
 * The four entry points mirror the api-implementation-matrix NSSAIAvailability
 * rows (Put → replace, Patch → patch, Delete → delete, Options → options); the
 * HTTP handlers that adapt the wire form onto these calls are a LATER slice.
 *
 * Single responsibility (AvailabilityEngine.md §Responsibility) — synchronous
 * CRUD only. The module owns no subscription lifecycle and no outbound
 * notification. On a committed mutation it PUBLISHES an abstract change event
 * (tai, change_type, before, after) through an injected seam; SubscriptionStore
 * (phase3) + NotificationDispatcher are the *consumers* and do not exist yet. A
 * NULL seam (phase2 reality) makes the publish a no-op.
 *
 * M4-independence — the engine performs no outbound HTTP / libcurl / NRF /
 * notification egress. The only collaborators are the AvailabilityRepository
 * interface (src/nssf/repository/availability_repository.h — libpq or in-memory
 * test seam) and the in-process change-event seam. So the engine is unit-testable
 * with nssf_availability_repo_new_inmemory() + a stub seam (no DB, no network).
 *
 * Result classification — every entry point returns a typed
 * nssf_availability_result_e; the caller (handler / ProblemDetailsMapper) maps
 * it onto HTTP status + 3GPP cause. In particular a non-`application/json-patch+json`
 * patch body is classified distinctly (NSSF_AVAIL_ERR_UNSUPPORTED_MEDIA_TYPE) so
 * the handler can answer 415 (open-gaps G-04).
 *
 * Design intent — design/nssf/module-decomposition/AvailabilityEngine.md,
 * dev/nssf/api-implementation-matrix.md (NSSAIAvailability Put/Patch/Delete/
 * Options rows), open-gaps-and-assumptions G-04 (RFC 6902 JSON Patch) + G-11
 * (amfSetReallocationInd default false).
 */
#ifndef NSSF_AVAILABILITY_ENGINE_H
#define NSSF_AVAILABILITY_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#include "availability_repository.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nssf_availability_engine nssf_availability_engine_t;

/*
 * Result classification of an engine call. The caller maps these onto HTTP
 * status + 3GPP cause; the engine itself never builds a ProblemDetails (the
 * handler owns HTTP mapping — same convention as SelectionEngine).
 */
typedef enum {
    NSSF_AVAIL_OK = 0,                    /* committed; success body (if any) set */
    NSSF_AVAIL_NO_CONTENT = 1,            /* committed; caller answers 204 */
    NSSF_AVAIL_ERR_BAD_REQUEST = 2,       /* malformed input → caller answers 400 */
    NSSF_AVAIL_ERR_NOT_FOUND = 3,         /* nfId has no availability → caller 404 */
    NSSF_AVAIL_ERR_UNSUPPORTED_MEDIA_TYPE = 4, /* non json-patch+json body → 415 */
    NSSF_AVAIL_ERR_INTERNAL = 5,          /* repository failure / OOM → caller 500 */
} nssf_availability_result_e;

/*
 * Change-event type — the kind of mutation that was committed. Carried in the
 * abstract change event so consumers (SubscriptionStore / NotificationDispatcher)
 * can decide what to notify (AvailabilityEngine.md §Outputs).
 */
typedef enum {
    NSSF_AVAIL_CHANGE_REPLACED = 0,  /* full-replace (Put) committed */
    NSSF_AVAIL_CHANGE_PATCHED = 1,   /* partial update (Patch) committed */
    NSSF_AVAIL_CHANGE_DELETED = 2,   /* availability removed (Delete) committed */
} nssf_availability_change_e;

/*
 * Abstract change event published after a committed mutation. All cJSON members
 * are BORROWED — they are valid only for the duration of the seam call; a
 * consumer that needs to retain them must deep-copy. `before`/`after` carry the
 * availability snapshot as the engine sees it (NULL when not applicable — e.g.
 * `after` is NULL for a delete, `before` is NULL for a first-time replace).
 */
typedef struct {
    const char *nf_id;                 /* the nfId whose availability changed */
    const cJSON *tai;                  /* serving TAI of the change (may be NULL) */
    nssf_availability_change_e change_type;
    const cJSON *before;               /* prior snssais snapshot, or NULL */
    const cJSON *after;                /* new snssais snapshot, or NULL */
} nssf_availability_change_event_t;

/*
 * Change-event publish seam — the cascade collaborator point.
 *
 * Invoked AFTER a successful repository commit (AvailabilityEngine.md §Decisions
 * — commit success then synchronous event publish; on commit failure no event).
 * Implemented by the caller (phase3 SubscriptionStore + NotificationDispatcher
 * fan-out). The engine never performs the fan-out itself — this keeps the slice
 * M4-independent (no notification egress here).
 *
 *   ctx — opaque caller state (see nssf_availability_engine_seams_t.ctx).
 *   ev  — borrowed event; valid only during the call.
 */
typedef void (*nssf_availability_change_publish_fn)(
    void *ctx, const nssf_availability_change_event_t *ev);

/*
 * Injected collaborator seams. A zero-initialized struct (NULL publish, NULL
 * ctx) yields a CRUD-only engine whose change-event publish is a no-op — the
 * phase2 reality where no consumer is registered yet. DO NOT hard-call
 * SubscriptionStore here; it does not exist.
 */
typedef struct {
    /* Change-event publish. NULL → publish is a no-op. */
    nssf_availability_change_publish_fn publish;

    /* Opaque caller state passed verbatim to the publish callback. */
    void *ctx;
} nssf_availability_engine_seams_t;

/*
 * Construct an engine over the given availability repository and seams.
 * `repo` is borrowed (not owned, not freed by the engine). `seams` is copied by
 * value (its fn-ptr + ctx members stay borrowed). Returns NULL only on OOM or
 * when repo is NULL.
 */
nssf_availability_engine_t *nssf_availability_engine_new(
    nssf_availability_repo_t *repo,
    const nssf_availability_engine_seams_t *seams);

/* Tear down an engine (idempotent on NULL). Does not free the repository. */
void nssf_availability_engine_free(nssf_availability_engine_t *engine);

/*
 * replace() — full-replace of nfId's availability (NSSAIAvailabilityPut).
 *
 * Parses `info` (an NssaiAvailabilityInfo cJSON — supportedNssaiAvailabilityData
 * per-TA list + optional supportedFeatures + amfSetReallocationInd), applies
 * S-NSSAI dedup and the amfSetReallocationInd default (false, open-gaps G-11),
 * and commits via nssf_availability_repo_upsert(). On commit it publishes a
 * REPLACED change event.
 *
 *   nf_id — borrowed NUL-terminated nfId (UUID, the record key). Must be a
 *           non-empty string that fits the record id field, else BAD_REQUEST.
 *   info  — borrowed NssaiAvailabilityInfo cJSON. Must be an object carrying at
 *           least one supportedNssaiAvailabilityData entry with a tai, else
 *           BAD_REQUEST.
 *   out_authorized — on NSSF_AVAIL_OK *out receives a newly allocated
 *           AuthorizedNssaiAvailabilityInfo-shaped cJSON the CALLER must
 *           cJSON_Delete; NULL on any non-OK result. May be NULL if the caller
 *           does not want the body. When the replace clears all slices the
 *           result is NSSF_AVAIL_NO_CONTENT (204) and no body is produced.
 *   errbuf/errlen — optional NUL-terminated diagnostic.
 *
 * Returns the result classification.
 */
nssf_availability_result_e nssf_availability_engine_replace(
    nssf_availability_engine_t *engine,
    const char *nf_id,
    const cJSON *info,
    cJSON **out_authorized,
    char *errbuf,
    size_t errlen);

/*
 * patch() — RFC 6902 JSON Patch over nfId's availability (NSSAIAvailabilityPatch).
 *
 * `content_type` is the request Content-Type the handler observed; a body that
 * is not `application/json-patch+json` is rejected with
 * NSSF_AVAIL_ERR_UNSUPPORTED_MEDIA_TYPE so the handler can answer 415 (G-04).
 * `patch` must be a cJSON ARRAY of RFC 6902 operations; each op is validated
 * (op ∈ {add,remove,replace,move,copy,test}; path present; value present for
 * add/replace/test; from present for move/copy — per 29.531 §6.1.6 +
 * TS29571 PatchItem). An invalid op rejects the WHOLE document before any
 * mutation (all-or-nothing) → NSSF_AVAIL_ERR_BAD_REQUEST.
 *
 * `tai` routes the patch to the stored availability row: the engine reads the
 * pre-image snssais snapshot for (tai.plmnId, tai) + nfId, applies the validated
 * ops to a COPY of that snapshot, and only on a fully successful apply commits
 * the new snssais via nssf_availability_repo_patch(). The original snapshot is
 * never mutated until the apply fully succeeds. Unknown nfId at that (plmn,tai)
 * → NSSF_AVAIL_ERR_NOT_FOUND. On commit it publishes a PATCHED change event
 * carrying before/after snapshots.
 *
 *   nf_id        — borrowed nfId (record key).
 *   content_type — borrowed Content-Type string (NULL treated as unsupported).
 *   tai          — borrowed Tai cJSON ({"plmnId":{...},"tac":"..."}) routing key.
 *                  Must be an object with a plmnId, else BAD_REQUEST.
 *   patch        — borrowed cJSON ARRAY of RFC 6902 operations.
 *   out_authorized — as in replace(); NULL on any non-OK result.
 *   errbuf/errlen  — optional diagnostic.
 */
nssf_availability_result_e nssf_availability_engine_patch(
    nssf_availability_engine_t *engine,
    const char *nf_id,
    const char *content_type,
    const cJSON *tai,
    const cJSON *patch,
    cJSON **out_authorized,
    char *errbuf,
    size_t errlen);

/*
 * delete() — remove nfId's availability (NSSAIAvailabilityDelete).
 *
 * Reads the prior snapshot (for the change event), commits the delete via
 * nssf_availability_repo_delete(), and on success publishes a DELETED change
 * event — the cascade seam by which SubscriptionStore/NotificationDispatcher
 * (later) tombstone affected subscriptions. With a NULL seam the publish is a
 * no-op. Unknown nfId → NSSF_AVAIL_ERR_NOT_FOUND; a successful delete →
 * NSSF_AVAIL_NO_CONTENT (caller answers 204).
 *
 *   nf_id         — borrowed nfId (record key).
 *   errbuf/errlen — optional diagnostic.
 */
nssf_availability_result_e nssf_availability_engine_delete(
    nssf_availability_engine_t *engine,
    const char *nf_id,
    char *errbuf,
    size_t errlen);

/*
 * options() — static method metadata (NSSAIAvailabilityOptions). No persistence,
 * no event. Writes the supported HTTP methods into a caller-supplied NUL-
 * terminated Allow-header value (e.g. "PUT, PATCH, DELETE, OPTIONS"). Returns
 * NSSF_AVAIL_OK when the buffer was large enough, NSSF_AVAIL_ERR_BAD_REQUEST on
 * a NULL/zero buffer.
 *
 *   allow_out / allow_len — destination buffer for the Allow header value.
 */
nssf_availability_result_e nssf_availability_engine_options(
    nssf_availability_engine_t *engine,
    char *allow_out,
    size_t allow_len);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_AVAILABILITY_ENGINE_H */
