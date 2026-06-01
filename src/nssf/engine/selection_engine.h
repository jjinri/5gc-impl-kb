/*
 * selection_engine.h — SelectionEngine for inbound NSSelectionGet (WI-selection-engine).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten business-logic module.
 *
 * Implements the NSSelectionGet core decision of
 * design/nssf/module-decomposition/SelectionEngine.md:
 *
 *     allowed = requested ∩ subscribed ∩ plmn_policy ∩ availability[tai]
 *
 * The engine is a near-pure input→output mapping function. The collaborators
 * the design names as interface seams (subscribed-NSSAI source / UDM, PLMN
 * policy source, target-AMF-set resolver / NRF) are *injected* here as caller-
 * supplied function pointers and config data — NOT real network calls. This
 * keeps the slice M4-independent (no libcurl / HTTP / NRF token / OAuth
 * outbound). The redirect target (open-gaps-and-assumptions G-07) is likewise
 * an operator-provided table; with no table the engine decides no redirect.
 *
 * Availability is read through the AvailabilityRepository interface
 * (src/nssf/repository/availability_repository.h), which works against either
 * the libpq backend or the in-memory test seam — so the engine is unit-testable
 * with nssf_availability_repo_new_inmemory() (no DB, no network).
 *
 * Design intent — design/nssf/module-decomposition/SelectionEngine.md and
 * design/nssf/architecture/request-flow.md (NSSelectionGet dispatch).
 */
#ifndef NSSF_SELECTION_ENGINE_H
#define NSSF_SELECTION_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#include "availability_repository.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nssf_selection_engine nssf_selection_engine_t;

/*
 * Result classification of a select() call. The caller (handler /
 * ProblemDetailsMapper) maps these onto HTTP status + 3GPP cause. Per
 * SelectionEngine.md §Decisions a partial reject stays 200 with rejected lists
 * populated; a full reject is signalled so the caller can answer 4xx.
 */
typedef enum {
    NSSF_SELECT_FULL_ALLOW = 0,   /* every requested S-NSSAI allowed → 200 */
    NSSF_SELECT_PARTIAL = 1,      /* some allowed, some rejected → 200 + rejected lists */
    NSSF_SELECT_FULL_REJECT = 2,  /* nothing allowed → caller answers 4xx */
} nssf_select_result_e;

/*
 * UDM seam — subscribed-NSSAI source.
 *
 * Resolve the subscribed S-NSSAI set for the requesting context. Implemented
 * by the caller against UDM (or a config/cache). The engine never performs the
 * lookup itself.
 *
 *   ctx                 — opaque caller state (see nssf_selection_seams_t.ctx).
 *   request_query       — the borrowed NSSelectionGet query/body cJSON.
 *   out_subscribed_array — on success the callee sets *out to a cJSON ARRAY of
 *                          Snssai objects ({"sst":..,"sd":..}). Ownership of the
 *                          returned array transfers to the engine, which frees it.
 *
 * Returns true when a (possibly empty) array was produced, false on lookup
 * failure. When the seam pointer is NULL the request body's own subscribedNssai
 * (from SliceInfoForRegistration / SliceInfoForUEConfigurationUpdate) is used.
 */
typedef bool (*nssf_subscribed_nssai_fn)(void *ctx,
                                         const cJSON *request_query,
                                         cJSON **out_subscribed_array);

/*
 * NRF seam — target-AMF-set resolver.
 *
 * Resolve the target AMF set id for the computed allowed NSSAI. Implemented by
 * the caller against NRF discovery (or config). NOT a real NRF call here.
 *
 *   ctx          — opaque caller state.
 *   allowed_array — borrowed cJSON ARRAY of the allowed Snssai objects.
 *   request_query — borrowed NSSelectionGet query/body cJSON.
 *
 * Returns a heap string the engine takes ownership of and frees (becomes
 * AuthorizedNetworkSliceInfo.targetAmfSet), or NULL to leave no candidate AMF
 * decided. A NULL seam pointer also means no AMF set is decided.
 */
typedef char *(*nssf_amf_set_resolver_fn)(void *ctx,
                                          const cJSON *allowed_array,
                                          const cJSON *request_query);

/*
 * Injected collaborator seams + operator policy data. All fields optional;
 * a zero-initialized struct yields a body-only engine (subscribed set taken
 * from the request body, no AMF set, no PLMN allow-list filtering, no redirect).
 *
 * The cJSON pointers are BORROWED — the engine never frees or retains them; the
 * caller owns them for the engine's lifetime.
 */
typedef struct {
    /* UDM seam (subscribed-NSSAI). NULL → use request-body subscribedNssai. */
    nssf_subscribed_nssai_fn subscribed_nssai;

    /* NRF seam (target-AMF-set). NULL → no targetAmfSet decided. */
    nssf_amf_set_resolver_fn amf_set_resolver;

    /* Opaque caller state passed verbatim to both seam callbacks. */
    void *ctx;

    /*
     * PLMN policy allow-list — a borrowed cJSON ARRAY of Snssai objects the
     * serving PLMN permits. NULL means "no PLMN policy constraint" (policy step
     * is identity). Non-NULL restricts allowed ⊆ this set.
     */
    const cJSON *plmn_policy_allow_list;

    /*
     * Operator redirect table (G-07) — borrowed cJSON ARRAY of
     * {"plmnId":{...},"target":"<uri>"} entries. NULL/empty → engine decides no
     * redirect. Reserved for multi-PLMN/TAI handling; the in-process engine only
     * reads, never invents, a redirect target.
     */
    const cJSON *redirect_table;
} nssf_selection_seams_t;

/*
 * Construct an engine over the given availability repository and seams.
 * `avail_repo` is borrowed (not owned, not freed by the engine). `seams` is
 * copied by value (its cJSON/fn-ptr members stay borrowed). Returns NULL only
 * on OOM or when avail_repo is NULL.
 */
nssf_selection_engine_t *nssf_selection_engine_new(nssf_availability_repo_t *avail_repo,
                                                   const nssf_selection_seams_t *seams);

/* Tear down an engine (idempotent on NULL). Does not free the repository. */
void nssf_selection_engine_free(nssf_selection_engine_t *engine);

/*
 * Run the NSSelectionGet selection.
 *
 *   engine             — non-NULL engine handle.
 *   nsselection_query  — borrowed cJSON of the NSSelectionGet query+body. The
 *                        engine detects which slice-info variant is present
 *                        (sliceInfoForRegistration / sliceInfoForPDUSession /
 *                        sliceInfoForUEConfigurationUpdate) and processes it.
 *   out_authorized_info — on success *out receives a newly allocated
 *                        AuthorizedNetworkSliceInfo cJSON the CALLER must
 *                        cJSON_Delete. Set even for FULL_REJECT (carries the
 *                        rejected lists). NULL on error.
 *   out_result         — on success receives the nssf_select_result_e
 *                        classification (may be NULL if the caller does not
 *                        want it).
 *   errbuf/errlen      — on error a NUL-terminated diagnostic is written when
 *                        errbuf != NULL && errlen > 0.
 *
 * Returns 0 on success (out_authorized_info + out_result set), -1 on error
 * (malformed query, missing TAI, repository failure, OOM) with *out_authorized_info
 * left NULL.
 */
int nssf_selection_engine_select(nssf_selection_engine_t *engine,
                                 const cJSON *nsselection_query,
                                 cJSON **out_authorized_info,
                                 nssf_select_result_e *out_result,
                                 char *errbuf,
                                 size_t errlen);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_SELECTION_ENGINE_H */
