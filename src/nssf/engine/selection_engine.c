/*
 * selection_engine.c — NSSelectionGet allowed-NSSAI selection (WI-selection-engine).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten business-logic module.
 *
 * See selection_engine.h for the contract. The body is a near-pure mapping
 * function — the only external read is the AvailabilityRepository interface,
 * and the UDM/NRF collaborators arrive as injected seams. No outbound network
 * calls live here (the slice is M4-independent by construction).
 */
#include "selection_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Envelope keys. NSSelectionGet is a GET with query params; the transport layer
 * assembles them into a single cJSON the engine consumes. The variant keys
 * mirror the 3GPP query-param names (slice-info-for-*) in camelCase, and tai /
 * homePlmnId carry the serving TAI and the home PLMN. These are the engine's
 * stable input contract — the handler maps the wire form onto them.
 */
#define KEY_SLICE_INFO_REGISTRATION "sliceInfoForRegistration"
#define KEY_SLICE_INFO_PDU_SESSION  "sliceInfoForPDUSession"
#define KEY_SLICE_INFO_UE_CONFIG    "sliceInfoForUEConfigurationUpdate"
#define KEY_TAI                     "tai"
#define KEY_HOME_PLMN_ID            "homePlmnId"

/* Model JSON keys (confirmed against the generated model serializers). */
#define KEY_REQUESTED_NSSAI         "requestedNssai"
#define KEY_SUBSCRIBED_NSSAI        "subscribedNssai"
#define KEY_SUBSCRIBED_SNSSAI       "subscribedSnssai"
#define KEY_PLMN_ID                 "plmnId"
#define KEY_SST                     "sst"
#define KEY_SD                      "sd"
#define KEY_ALLOWED_SNSSAI          "allowedSnssai"

struct nssf_selection_engine {
    nssf_availability_repo_t *avail_repo;  /* borrowed */
    nssf_selection_seams_t seams;          /* fn ptrs + borrowed cJSON/ctx */
};

static void set_err(char *errbuf, size_t errlen, const char *msg)
{
    if (errbuf != NULL && errlen > 0) {
        snprintf(errbuf, errlen, "%s", msg);
    }
}

/*
 * S-NSSAI value equality. Compare by (sst, sd); sd is optional and only equal
 * when both sides agree (both absent, or both present and string-equal). Whole-
 * value compare — never substring. A missing/non-numeric sst makes the object
 * unmatchable (returns false), which is the safe choice for an allow decision.
 */
static bool snssai_equal(const cJSON *a, const cJSON *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }

    const cJSON *sst_a = cJSON_GetObjectItemCaseSensitive(a, KEY_SST);
    const cJSON *sst_b = cJSON_GetObjectItemCaseSensitive(b, KEY_SST);
    if (!cJSON_IsNumber(sst_a) || !cJSON_IsNumber(sst_b)) {
        return false;
    }
    if (sst_a->valueint != sst_b->valueint) {
        return false;
    }

    const cJSON *sd_a = cJSON_GetObjectItemCaseSensitive(a, KEY_SD);
    const cJSON *sd_b = cJSON_GetObjectItemCaseSensitive(b, KEY_SD);
    bool has_sd_a = cJSON_IsString(sd_a) && sd_a->valuestring != NULL;
    bool has_sd_b = cJSON_IsString(sd_b) && sd_b->valuestring != NULL;
    if (has_sd_a != has_sd_b) {
        return false;
    }
    if (has_sd_a) {
        return strcmp(sd_a->valuestring, sd_b->valuestring) == 0;
    }
    return true;  /* both sd-less, same sst */
}

/* Is `target` value-equal to any Snssai object in array `set`? */
static bool snssai_in_array(const cJSON *set, const cJSON *target)
{
    if (!cJSON_IsArray(set)) {
        return false;
    }
    const cJSON *it = NULL;
    cJSON_ArrayForEach(it, set) {
        if (snssai_equal(it, target)) {
            return true;
        }
    }
    return false;
}

/*
 * Extract a flat cJSON array of requested Snssai objects from a variant body.
 * Registration / UEConfigUpdate carry requestedNssai (an array of Snssai);
 * PDUSession carries a single sNssai. Returns a newly allocated array (caller
 * cJSON_Deletes), or NULL on OOM. Entries are deep copies, so the array owns
 * its contents independently of the borrowed query.
 */
static cJSON *collect_requested(const cJSON *variant_body, bool is_pdu_session)
{
    cJSON *out = cJSON_CreateArray();
    if (out == NULL) {
        return NULL;
    }

    if (is_pdu_session) {
        const cJSON *snssai = cJSON_GetObjectItemCaseSensitive(variant_body, "sNssai");
        if (cJSON_IsObject(snssai)) {
            cJSON *dup = cJSON_Duplicate(snssai, true);
            if (dup == NULL) {
                cJSON_Delete(out);
                return NULL;
            }
            cJSON_AddItemToArray(out, dup);
        }
        return out;
    }

    const cJSON *requested = cJSON_GetObjectItemCaseSensitive(variant_body, KEY_REQUESTED_NSSAI);
    if (cJSON_IsArray(requested)) {
        const cJSON *it = NULL;
        cJSON_ArrayForEach(it, requested) {
            cJSON *dup = cJSON_Duplicate(it, true);
            if (dup == NULL) {
                cJSON_Delete(out);
                return NULL;
            }
            cJSON_AddItemToArray(out, dup);
        }
    }
    return out;
}

/*
 * Resolve the subscribed-NSSAI set as a flat array of Snssai objects (caller
 * cJSON_Deletes). When the UDM seam is present it provides the set; otherwise
 * the variant body's own subscribedNssai (list of SubscribedSnssai, each
 * wrapping subscribedSnssai → Snssai) is used. Returns NULL on OOM / seam
 * failure; *ok distinguishes the two (false = seam failure).
 */
static cJSON *resolve_subscribed(const nssf_selection_engine_t *engine,
                                 const cJSON *query,
                                 const cJSON *variant_body,
                                 bool *ok)
{
    *ok = true;

    if (engine->seams.subscribed_nssai != NULL) {
        cJSON *seam_out = NULL;
        if (!engine->seams.subscribed_nssai(engine->seams.ctx, query, &seam_out)) {
            cJSON_Delete(seam_out);
            *ok = false;
            return NULL;
        }
        if (seam_out == NULL) {
            seam_out = cJSON_CreateArray();  /* normalize "none" to empty array */
            if (seam_out == NULL) {
                *ok = false;
            }
        }
        return seam_out;
    }

    /* Body-sourced fallback — flatten SubscribedSnssai[].subscribedSnssai. */
    cJSON *out = cJSON_CreateArray();
    if (out == NULL) {
        *ok = false;
        return NULL;
    }
    const cJSON *subscribed = cJSON_GetObjectItemCaseSensitive(variant_body, KEY_SUBSCRIBED_NSSAI);
    if (cJSON_IsArray(subscribed)) {
        const cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, subscribed) {
            const cJSON *snssai = cJSON_GetObjectItemCaseSensitive(entry, KEY_SUBSCRIBED_SNSSAI);
            if (!cJSON_IsObject(snssai)) {
                continue;
            }
            cJSON *dup = cJSON_Duplicate(snssai, true);
            if (dup == NULL) {
                cJSON_Delete(out);
                *ok = false;
                return NULL;
            }
            cJSON_AddItemToArray(out, dup);
        }
    }
    return out;
}

/*
 * Union the snssais arrays of every availability record for (plmn, tai) into a
 * single flat array of Snssai objects (caller cJSON_Deletes). Returns NULL on
 * repository error or OOM; *ok false signals the failure. An empty result with
 * *ok true means "TAI known, but no slice available there".
 */
static cJSON *collect_availability(const nssf_selection_engine_t *engine,
                                   const cJSON *serving_plmn,
                                   const cJSON *tai,
                                   bool *ok)
{
    *ok = true;
    cJSON *out = cJSON_CreateArray();
    if (out == NULL) {
        *ok = false;
        return NULL;
    }

    nssf_availability_record_list_t list = {0};
    if (nssf_availability_repo_find_for_plmn_tai(engine->avail_repo, serving_plmn, tai, &list) != 0) {
        cJSON_Delete(out);
        *ok = false;
        return NULL;
    }

    for (size_t i = 0; i < list.count; i++) {
        const cJSON *snssais = list.records[i].snssais;
        if (!cJSON_IsArray(snssais)) {
            continue;
        }
        const cJSON *it = NULL;
        cJSON_ArrayForEach(it, snssais) {
            if (!cJSON_IsObject(it)) {
                continue;
            }
            /* Skip duplicates so the availability union is a set. */
            if (snssai_in_array(out, it)) {
                continue;
            }
            cJSON *dup = cJSON_Duplicate(it, true);
            if (dup == NULL) {
                cJSON_Delete(out);
                nssf_availability_record_list_free(&list);
                *ok = false;
                return NULL;
            }
            cJSON_AddItemToArray(out, dup);
        }
    }

    nssf_availability_record_list_free(&list);
    return out;
}

/* Wrap an Snssai object as an AllowedSnssai entry ({"allowedSnssai": {...}}). */
static cJSON *make_allowed_snssai_entry(const cJSON *snssai)
{
    cJSON *entry = cJSON_CreateObject();
    if (entry == NULL) {
        return NULL;
    }
    cJSON *dup = cJSON_Duplicate(snssai, true);
    if (dup == NULL) {
        cJSON_Delete(entry);
        return NULL;
    }
    cJSON_AddItemToObject(entry, KEY_ALLOWED_SNSSAI, dup);
    return entry;
}

nssf_selection_engine_t *nssf_selection_engine_new(nssf_availability_repo_t *avail_repo,
                                                   const nssf_selection_seams_t *seams)
{
    if (avail_repo == NULL) {
        return NULL;
    }
    nssf_selection_engine_t *engine = calloc(1, sizeof(*engine));
    if (engine == NULL) {
        return NULL;
    }
    engine->avail_repo = avail_repo;
    if (seams != NULL) {
        engine->seams = *seams;
    }
    return engine;
}

void nssf_selection_engine_free(nssf_selection_engine_t *engine)
{
    free(engine);
}

/*
 * Locate the slice-info variant in the query. Sets *variant to the borrowed
 * body object and *is_pdu to whether it is the PDU-session shape. Returns true
 * when exactly one recognised variant is present.
 */
static bool detect_variant(const cJSON *query, const cJSON **variant, bool *is_pdu)
{
    const cJSON *reg = cJSON_GetObjectItemCaseSensitive(query, KEY_SLICE_INFO_REGISTRATION);
    const cJSON *pdu = cJSON_GetObjectItemCaseSensitive(query, KEY_SLICE_INFO_PDU_SESSION);
    const cJSON *ue = cJSON_GetObjectItemCaseSensitive(query, KEY_SLICE_INFO_UE_CONFIG);

    int present = (cJSON_IsObject(reg) ? 1 : 0) + (cJSON_IsObject(pdu) ? 1 : 0) +
                  (cJSON_IsObject(ue) ? 1 : 0);
    if (present != 1) {
        return false;
    }

    if (cJSON_IsObject(reg)) {
        *variant = reg;
        *is_pdu = false;
    } else if (cJSON_IsObject(ue)) {
        *variant = ue;
        *is_pdu = false;
    } else {
        *variant = pdu;
        *is_pdu = true;
    }
    return true;
}

int nssf_selection_engine_select(nssf_selection_engine_t *engine,
                                 const cJSON *nsselection_query,
                                 cJSON **out_authorized_info,
                                 nssf_select_result_e *out_result,
                                 char *errbuf,
                                 size_t errlen)
{
    if (out_authorized_info != NULL) {
        *out_authorized_info = NULL;
    }
    if (engine == NULL || nsselection_query == NULL || out_authorized_info == NULL) {
        set_err(errbuf, errlen, "selection_engine: null argument");
        return -1;
    }

    const cJSON *variant_body = NULL;
    bool is_pdu = false;
    if (!detect_variant(nsselection_query, &variant_body, &is_pdu)) {
        set_err(errbuf, errlen,
                "selection_engine: exactly one slice-info variant required");
        return -1;
    }

    /* TAI is mandatory — it keys the availability read; its PLMN is serving PLMN. */
    const cJSON *tai = cJSON_GetObjectItemCaseSensitive(nsselection_query, KEY_TAI);
    if (!cJSON_IsObject(tai)) {
        set_err(errbuf, errlen, "selection_engine: tai missing or not an object");
        return -1;
    }
    const cJSON *serving_plmn = cJSON_GetObjectItemCaseSensitive(tai, KEY_PLMN_ID);
    if (!cJSON_IsObject(serving_plmn)) {
        set_err(errbuf, errlen, "selection_engine: tai.plmnId missing");
        return -1;
    }

    /* Gather the four input sets. */
    cJSON *requested = collect_requested(variant_body, is_pdu);
    if (requested == NULL) {
        set_err(errbuf, errlen, "selection_engine: OOM collecting requested NSSAI");
        return -1;
    }

    bool ok = false;
    cJSON *subscribed = resolve_subscribed(engine, nsselection_query, variant_body, &ok);
    if (!ok) {
        cJSON_Delete(requested);
        set_err(errbuf, errlen, "selection_engine: subscribed-NSSAI seam failure");
        return -1;
    }

    cJSON *availability = collect_availability(engine, serving_plmn, tai, &ok);
    if (!ok) {
        cJSON_Delete(requested);
        cJSON_Delete(subscribed);
        set_err(errbuf, errlen, "selection_engine: availability read failed");
        return -1;
    }

    /*
     * allowed = requested ∩ subscribed ∩ plmn_policy ∩ availability[tai].
     * Each requested S-NSSAI passing all gates lands in allowed; the rest are
     * rejected (SelectionEngine.md §Decisions — partial reject keeps 200).
     */
    const cJSON *plmn_policy = engine->seams.plmn_policy_allow_list;  /* NULL = no constraint */

    cJSON *allowed_list = cJSON_CreateArray();
    cJSON *rejected_list = cJSON_CreateArray();
    if (allowed_list == NULL || rejected_list == NULL) {
        cJSON_Delete(allowed_list);
        cJSON_Delete(rejected_list);
        cJSON_Delete(requested);
        cJSON_Delete(subscribed);
        cJSON_Delete(availability);
        set_err(errbuf, errlen, "selection_engine: OOM building result");
        return -1;
    }

    int requested_count = 0;
    int allowed_count = 0;
    bool oom = false;
    const cJSON *req = NULL;
    cJSON_ArrayForEach(req, requested) {
        requested_count++;

        bool pass = snssai_in_array(subscribed, req) &&
                    snssai_in_array(availability, req) &&
                    (plmn_policy == NULL || snssai_in_array(plmn_policy, req));

        if (pass) {
            cJSON *entry = make_allowed_snssai_entry(req);
            if (entry == NULL) {
                oom = true;
                break;
            }
            cJSON_AddItemToArray(allowed_list, entry);
            allowed_count++;
        } else {
            cJSON *dup = cJSON_Duplicate(req, true);
            if (dup == NULL) {
                oom = true;
                break;
            }
            cJSON_AddItemToArray(rejected_list, dup);
        }
    }

    cJSON_Delete(requested);
    cJSON_Delete(subscribed);
    cJSON_Delete(availability);

    if (oom) {
        cJSON_Delete(allowed_list);
        cJSON_Delete(rejected_list);
        set_err(errbuf, errlen, "selection_engine: OOM classifying NSSAI");
        return -1;
    }

    /* Classify the outcome. */
    nssf_select_result_e result;
    if (requested_count == 0 || allowed_count == requested_count) {
        result = NSSF_SELECT_FULL_ALLOW;
    } else if (allowed_count == 0) {
        result = NSSF_SELECT_FULL_REJECT;
    } else {
        result = NSSF_SELECT_PARTIAL;
    }

    /*
     * Build AuthorizedNetworkSliceInfo. allowedNssaiList carries one AllowedNssai
     * group (access-typed allowed S-NSSAI). rejectedNssaiInTa holds the rejected
     * set — its semantics ("not available in this TAI") match the availability
     * gate. The target AMF set comes from the NRF seam when present.
     */
    cJSON *authorized = cJSON_CreateObject();
    if (authorized == NULL) {
        cJSON_Delete(allowed_list);
        cJSON_Delete(rejected_list);
        set_err(errbuf, errlen, "selection_engine: OOM building AuthorizedNetworkSliceInfo");
        return -1;
    }

    if (allowed_count > 0) {
        cJSON *allowed_nssai = cJSON_CreateObject();
        if (allowed_nssai == NULL) {
            cJSON_Delete(authorized);
            cJSON_Delete(allowed_list);
            cJSON_Delete(rejected_list);
            set_err(errbuf, errlen, "selection_engine: OOM building allowedNssai");
            return -1;
        }
        /* allowedSnssaiList holds the AllowedSnssai entries built above. */
        cJSON_AddItemToObject(allowed_nssai, "allowedSnssaiList", allowed_list);
        allowed_list = NULL;  /* ownership moved */
        cJSON_AddStringToObject(allowed_nssai, "accessType", "3GPP_ACCESS");

        cJSON *allowed_nssai_list = cJSON_CreateArray();
        if (allowed_nssai_list == NULL) {
            cJSON_Delete(allowed_nssai);
            cJSON_Delete(authorized);
            cJSON_Delete(rejected_list);
            set_err(errbuf, errlen, "selection_engine: OOM building allowedNssaiList");
            return -1;
        }
        cJSON_AddItemToArray(allowed_nssai_list, allowed_nssai);
        cJSON_AddItemToObject(authorized, "allowedNssaiList", allowed_nssai_list);
    } else {
        cJSON_Delete(allowed_list);
        allowed_list = NULL;
    }

    /* Rejected S-NSSAI — keyed to the TAI availability gate. */
    if (cJSON_GetArraySize(rejected_list) > 0) {
        cJSON_AddItemToObject(authorized, "rejectedNssaiInTa", rejected_list);
        rejected_list = NULL;  /* ownership moved */
    } else {
        cJSON_Delete(rejected_list);
        rejected_list = NULL;
    }

    /* NRF seam — optional target AMF set, only meaningful when something passed. */
    if (result != NSSF_SELECT_FULL_REJECT && engine->seams.amf_set_resolver != NULL) {
        const cJSON *allowed_for_resolver = cJSON_GetObjectItemCaseSensitive(authorized, "allowedNssaiList");
        char *amf_set = engine->seams.amf_set_resolver(engine->seams.ctx,
                                                        allowed_for_resolver,
                                                        nsselection_query);
        if (amf_set != NULL) {
            cJSON_AddStringToObject(authorized, "targetAmfSet", amf_set);
            free(amf_set);  /* seam transferred ownership; cJSON copied the string */
        }
    }

    *out_authorized_info = authorized;
    if (out_result != NULL) {
        *out_result = result;
    }
    return 0;
}
