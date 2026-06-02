/*
 * availability_engine.c — NSSAIAvailability synchronous CRUD engine
 * (WI-availability-engine).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten business-logic module.
 *
 * See availability_engine.h for the contract. The body is pure synchronous CRUD
 * over the AvailabilityRepository interface plus an in-process change-event
 * publish seam. No outbound network calls live here — notification egress is
 * NotificationDispatcher's job (M4-gated, later). The slice is M4-independent by
 * construction.
 */
#include "availability_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * NssaiAvailabilityInfo / Snssai keys (confirmed against the generated model
 * serializers — supported_nssai_availability_data.h, snssai.h). These are the
 * engine's stable input contract; the handler maps the wire form onto them.
 */
#define KEY_SUPPORTED_DATA      "supportedNssaiAvailabilityData"
#define KEY_SUPPORTED_SNSSAI    "supportedSnssaiList"
#define KEY_SUPPORTED_FEATURES  "supportedFeatures"
#define KEY_AMF_REALLOC_IND     "amfSetReallocationInd"
#define KEY_TAI                 "tai"
#define KEY_PLMN_ID             "plmnId"
#define KEY_SST                 "sst"
#define KEY_SD                  "sd"
#define KEY_AUTHORIZED_DATA     "authorizedNssaiAvailabilityData"

/* RFC 6902 / json-patch+json constants. */
#define JSON_PATCH_CONTENT_TYPE "application/json-patch+json"
#define KEY_OP                  "op"
#define KEY_PATH                "path"
#define KEY_VALUE               "value"
#define KEY_FROM                "from"

struct nssf_availability_engine {
    nssf_availability_repo_t *repo;          /* borrowed */
    nssf_availability_engine_seams_t seams;  /* fn ptr + borrowed ctx */
};

static void set_err(char *errbuf, size_t errlen, const char *msg)
{
    if (errbuf != NULL && errlen > 0) {
        snprintf(errbuf, errlen, "%s", msg);
    }
}

/* nfId must be a non-empty string short enough to be a record id (UUID + NUL). */
static bool nf_id_valid(const char *nf_id)
{
    return nf_id != NULL && nf_id[0] != '\0' &&
           strlen(nf_id) < sizeof(((nssf_availability_record_t *)0)->id);
}

/*
 * S-NSSAI value equality by (sst, sd) — whole-value, never substring. sd is
 * optional and equal only when both sides agree. A missing/non-numeric sst makes
 * the object unmatchable. Mirrors selection_engine.c so the dedup notion of
 * "same slice" is consistent across the two engines.
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
    return true;
}

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
 * Flatten + dedup the S-NSSAI set of an NssaiAvailabilityInfo into a single
 * array of Snssai objects (caller cJSON_Deletes). Every
 * supportedNssaiAvailabilityData[*].supportedSnssaiList entry is unioned; the
 * dedup makes the result a set (Put §"snssai dedup"). *out_first_tai (when
 * non-NULL) receives a BORROWED pointer to the first entry's tai — the serving
 * TAI that keys the change event + the availability row. Returns NULL on OOM.
 */
static cJSON *collect_snssais(const cJSON *info, const cJSON **out_first_tai)
{
    if (out_first_tai != NULL) {
        *out_first_tai = NULL;
    }
    cJSON *out = cJSON_CreateArray();
    if (out == NULL) {
        return NULL;
    }

    const cJSON *data = cJSON_GetObjectItemCaseSensitive(info, KEY_SUPPORTED_DATA);
    if (!cJSON_IsArray(data)) {
        return out;  /* empty set — replace() treats as "no slices" (204). */
    }

    const cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, data) {
        if (!cJSON_IsObject(entry)) {
            continue;
        }
        if (out_first_tai != NULL && *out_first_tai == NULL) {
            const cJSON *tai = cJSON_GetObjectItemCaseSensitive(entry, KEY_TAI);
            if (cJSON_IsObject(tai)) {
                *out_first_tai = tai;  /* borrowed from info */
            }
        }
        const cJSON *snssais = cJSON_GetObjectItemCaseSensitive(entry, KEY_SUPPORTED_SNSSAI);
        if (!cJSON_IsArray(snssais)) {
            continue;
        }
        const cJSON *it = NULL;
        cJSON_ArrayForEach(it, snssais) {
            if (!cJSON_IsObject(it) || snssai_in_array(out, it)) {
                continue;  /* dedup */
            }
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
 * Build the AuthorizedNssaiAvailabilityInfo-shaped success body that echoes the
 * authorized snssais back to the AMF. The engine authorizes the full deduped
 * supported set (no restriction policy in this slice — that is a SelectionEngine
 * concern), so authorized == supported here. `snssais` + `tai` are borrowed.
 * Returns NULL on OOM.
 */
static cJSON *make_authorized_info(const cJSON *snssais,
                                   const cJSON *tai,
                                   const char *supported_features)
{
    cJSON *info = cJSON_CreateObject();
    if (info == NULL) {
        return NULL;
    }

    cJSON *data_list = cJSON_CreateArray();
    if (data_list == NULL) {
        cJSON_Delete(info);
        return NULL;
    }
    cJSON_AddItemToObject(info, KEY_AUTHORIZED_DATA, data_list);

    cJSON *data = cJSON_CreateObject();
    if (data == NULL) {
        cJSON_Delete(info);
        return NULL;
    }
    cJSON_AddItemToArray(data_list, data);

    if (tai != NULL) {
        cJSON *tai_dup = cJSON_Duplicate(tai, true);
        if (tai_dup == NULL) {
            cJSON_Delete(info);
            return NULL;
        }
        cJSON_AddItemToObject(data, KEY_TAI, tai_dup);
    }

    cJSON *snssais_dup = cJSON_Duplicate(snssais, true);
    if (snssais_dup == NULL) {
        cJSON_Delete(info);
        return NULL;
    }
    cJSON_AddItemToObject(data, KEY_SUPPORTED_SNSSAI, snssais_dup);

    if (supported_features != NULL) {
        if (cJSON_AddStringToObject(info, KEY_SUPPORTED_FEATURES,
                                    supported_features) == NULL) {
            cJSON_Delete(info);
            return NULL;
        }
    }
    return info;
}

/*
 * Read the current snssais snapshot of nfId (for change-event `before` and as
 * the base of a JSON-patch apply). The repo is keyed by (plmn, tai) for reads,
 * so we cannot fetch by id alone — instead the caller passes the (plmn, tai)
 * it is operating on. Returns a newly allocated array (caller cJSON_Deletes) of
 * the matching record's snssais, or NULL with *found=false when no row matches
 * (or on error — *err distinguishes). An empty array with *found=true means the
 * row exists but lists no slice.
 */
static cJSON *read_snapshot(nssf_availability_engine_t *engine,
                            const char *nf_id,
                            const cJSON *plmn_id,
                            const cJSON *tai,
                            bool *found,
                            bool *err)
{
    *found = false;
    *err = false;

    nssf_availability_record_list_t list = {0};
    if (nssf_availability_repo_find_for_plmn_tai(engine->repo, plmn_id, tai,
                                                 &list) != 0) {
        *err = true;
        return NULL;
    }

    cJSON *snapshot = NULL;
    for (size_t i = 0; i < list.count; i++) {
        if (strcmp(list.records[i].id, nf_id) != 0) {
            continue;
        }
        *found = true;
        if (cJSON_IsArray(list.records[i].snssais)) {
            snapshot = cJSON_Duplicate(list.records[i].snssais, true);
            if (snapshot == NULL) {
                *err = true;
            }
        } else {
            snapshot = cJSON_CreateArray();
            if (snapshot == NULL) {
                *err = true;
            }
        }
        break;
    }
    nssf_availability_record_list_free(&list);
    return snapshot;
}

/* Publish a change event through the seam, if one is registered. No-op when NULL. */
static void publish_change(nssf_availability_engine_t *engine,
                           const char *nf_id,
                           const cJSON *tai,
                           nssf_availability_change_e change_type,
                           const cJSON *before,
                           const cJSON *after)
{
    if (engine->seams.publish == NULL) {
        return;  /* phase2 reality — no consumer registered. */
    }
    nssf_availability_change_event_t ev = {
        .nf_id = nf_id,
        .tai = tai,
        .change_type = change_type,
        .before = before,
        .after = after,
    };
    engine->seams.publish(engine->seams.ctx, &ev);
}

nssf_availability_engine_t *nssf_availability_engine_new(
    nssf_availability_repo_t *repo,
    const nssf_availability_engine_seams_t *seams)
{
    if (repo == NULL) {
        return NULL;
    }
    nssf_availability_engine_t *engine = calloc(1, sizeof(*engine));
    if (engine == NULL) {
        return NULL;
    }
    engine->repo = repo;
    if (seams != NULL) {
        engine->seams = *seams;
    }
    return engine;
}

void nssf_availability_engine_free(nssf_availability_engine_t *engine)
{
    free(engine);
}

/* ── replace (NSSAIAvailabilityPut) ───────────────────────────────────────── */

nssf_availability_result_e nssf_availability_engine_replace(
    nssf_availability_engine_t *engine,
    const char *nf_id,
    const cJSON *info,
    cJSON **out_authorized,
    char *errbuf,
    size_t errlen)
{
    if (out_authorized != NULL) {
        *out_authorized = NULL;
    }
    if (engine == NULL || info == NULL) {
        set_err(errbuf, errlen, "availability_engine: null argument");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }
    if (!nf_id_valid(nf_id)) {
        set_err(errbuf, errlen, "availability_engine: invalid nfId");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }
    if (!cJSON_IsObject(info)) {
        set_err(errbuf, errlen, "availability_engine: info not an object");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }

    const cJSON *first_tai = NULL;
    cJSON *snssais = collect_snssais(info, &first_tai);
    if (snssais == NULL) {
        set_err(errbuf, errlen, "availability_engine: OOM collecting snssais");
        return NSSF_AVAIL_ERR_INTERNAL;
    }

    /* A Put with at least one supportedNssaiAvailabilityData must carry a TAI —
     * the TAI keys the availability row and the change event. */
    if (first_tai == NULL) {
        cJSON_Delete(snssais);
        set_err(errbuf, errlen,
                "availability_engine: supportedNssaiAvailabilityData missing tai");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }
    const cJSON *plmn_id = cJSON_GetObjectItemCaseSensitive(first_tai, KEY_PLMN_ID);
    if (!cJSON_IsObject(plmn_id)) {
        cJSON_Delete(snssais);
        set_err(errbuf, errlen, "availability_engine: tai.plmnId missing");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }

    /* amfSetReallocationInd default is false (open-gaps G-11) — absent/non-bool
     * collapses to false, so a missing flag never enables reallocation. */
    const cJSON *realloc_ind = cJSON_GetObjectItemCaseSensitive(info, KEY_AMF_REALLOC_IND);
    bool reallocation = cJSON_IsTrue(realloc_ind);

    const cJSON *features = cJSON_GetObjectItemCaseSensitive(info, KEY_SUPPORTED_FEATURES);
    const char *features_str =
        (cJSON_IsString(features) && features->valuestring != NULL)
            ? features->valuestring
            : NULL;

    /* Prior snapshot for the change event (best-effort — a read error here does
     * not block the replace; `before` simply stays NULL). */
    bool had_prior = false, read_err = false;
    cJSON *before = read_snapshot(engine, nf_id, plmn_id, first_tai,
                                  &had_prior, &read_err);

    /* Full-replace via upsert. The record OWNS its cJSON, but upsert() deep-copies
     * what it keeps, so we hand it borrowed-style pointers and never transfer. */
    nssf_availability_record_t rec = {0};
    strncpy(rec.id, nf_id, sizeof(rec.id) - 1);
    rec.id[sizeof(rec.id) - 1] = '\0';
    rec.plmn_id = (cJSON *)plmn_id;     /* borrowed into rec; upsert deep-copies */
    rec.tai = (cJSON *)first_tai;       /* borrowed */
    rec.snssais = snssais;              /* borrowed */
    rec.amf_set_id = NULL;
    rec.reallocation_indication = reallocation;

    int rc = nssf_availability_repo_upsert(engine->repo, &rec);
    if (rc != 0) {
        cJSON_Delete(before);
        cJSON_Delete(snssais);
        set_err(errbuf, errlen, "availability_engine: upsert failed");
        return NSSF_AVAIL_ERR_INTERNAL;
    }

    /* Commit succeeded → publish the REPLACED change event (after = new snssais). */
    publish_change(engine, nf_id, first_tai, NSSF_AVAIL_CHANGE_REPLACED,
                   before, snssais);
    cJSON_Delete(before);

    /* "No supported slices after successful update" → 204 (Put response 204). */
    if (cJSON_GetArraySize(snssais) == 0) {
        cJSON_Delete(snssais);
        return NSSF_AVAIL_NO_CONTENT;
    }

    if (out_authorized != NULL) {
        cJSON *authorized = make_authorized_info(snssais, first_tai, features_str);
        if (authorized == NULL) {
            cJSON_Delete(snssais);
            set_err(errbuf, errlen, "availability_engine: OOM building response");
            return NSSF_AVAIL_ERR_INTERNAL;
        }
        *out_authorized = authorized;
    }
    cJSON_Delete(snssais);
    return NSSF_AVAIL_OK;
}

/* ── patch (NSSAIAvailabilityPatch, RFC 6902) ─────────────────────────────── */

/*
 * Validate one RFC 6902 operation (29.531 §6.1.6 + TS29571 PatchItem). op must be
 * a known token; path must be present; add/replace/test require value; move/copy
 * require from. Returns true when the op is structurally valid.
 */
static bool patch_op_valid(const cJSON *op)
{
    if (!cJSON_IsObject(op)) {
        return false;
    }
    const cJSON *op_name = cJSON_GetObjectItemCaseSensitive(op, KEY_OP);
    const cJSON *path = cJSON_GetObjectItemCaseSensitive(op, KEY_PATH);
    if (!cJSON_IsString(op_name) || op_name->valuestring == NULL) {
        return false;
    }
    if (!cJSON_IsString(path) || path->valuestring == NULL) {
        return false;
    }

    const char *name = op_name->valuestring;
    const bool has_value =
        cJSON_GetObjectItemCaseSensitive(op, KEY_VALUE) != NULL;
    const cJSON *from = cJSON_GetObjectItemCaseSensitive(op, KEY_FROM);
    const bool has_from = cJSON_IsString(from) && from->valuestring != NULL;

    if (strcmp(name, "add") == 0 || strcmp(name, "replace") == 0 ||
        strcmp(name, "test") == 0) {
        return has_value;
    }
    if (strcmp(name, "move") == 0 || strcmp(name, "copy") == 0) {
        return has_from;
    }
    if (strcmp(name, "remove") == 0) {
        return true;  /* path only */
    }
    return false;  /* unknown op token */
}

/*
 * Parse the array index a "/snssais/<n>" or "/<n>" style path tail refers to.
 * The patchable target of this resource is the snssais array; supported paths
 * are the array root ("/-" append, or "/<n>" element) — a minimal RFC 6902
 * subset sufficient for per-TA S-NSSAI add/remove/replace. Returns the parsed
 * index into *idx (or the array size for the "-" append token) and true on a
 * recognised path; false rejects the op (caller → all-or-nothing fail).
 *
 * `array_len` is the current length; `allow_append` permits the "-" token and an
 * index == array_len (valid append target for `add`).
 */
static bool parse_array_index(const char *path, int array_len,
                              bool allow_append, int *idx)
{
    if (path == NULL || path[0] != '/') {
        return false;
    }
    const char *tail = path + 1;
    if (allow_append && strcmp(tail, "-") == 0) {
        *idx = array_len;  /* append position */
        return true;
    }
    /* Digits only. */
    if (tail[0] == '\0') {
        return false;
    }
    long v = 0;
    for (const char *p = tail; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return false;
        }
        v = v * 10 + (*p - '0');
        if (v > array_len) {  /* bound early; index must address existing/append */
            break;
        }
    }
    if (v < 0 || v > array_len || (!allow_append && v >= array_len)) {
        return false;
    }
    *idx = (int)v;
    return true;
}

/*
 * Apply one validated RFC 6902 op to the snssais array `target` in place.
 * Returns true on success; false when the op cannot be applied (out-of-range
 * index, failed `test`, unsupported path) — the caller discards the working copy
 * so nothing is committed (all-or-nothing). Only the snssais array is patchable.
 */
static bool apply_one_op(cJSON *target, const cJSON *op)
{
    const char *name = cJSON_GetObjectItemCaseSensitive(op, KEY_OP)->valuestring;
    const char *path = cJSON_GetObjectItemCaseSensitive(op, KEY_PATH)->valuestring;
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(op, KEY_VALUE);
    int len = cJSON_GetArraySize(target);
    int idx = 0;

    if (strcmp(name, "add") == 0) {
        if (!parse_array_index(path, len, true, &idx)) {
            return false;
        }
        cJSON *dup = cJSON_Duplicate(value, true);
        if (dup == NULL) {
            return false;
        }
        if (idx >= len) {
            cJSON_AddItemToArray(target, dup);  /* append */
        } else {
            cJSON_InsertItemInArray(target, idx, dup);  /* shifts right */
        }
        return true;
    }
    if (strcmp(name, "remove") == 0) {
        if (!parse_array_index(path, len, false, &idx)) {
            return false;
        }
        cJSON_DeleteItemFromArray(target, idx);
        return true;
    }
    if (strcmp(name, "replace") == 0) {
        if (!parse_array_index(path, len, false, &idx)) {
            return false;
        }
        cJSON *dup = cJSON_Duplicate(value, true);
        if (dup == NULL) {
            return false;
        }
        cJSON_ReplaceItemInArray(target, idx, dup);
        return true;
    }
    if (strcmp(name, "test") == 0) {
        if (!parse_array_index(path, len, false, &idx)) {
            return false;
        }
        const cJSON *at = cJSON_GetArrayItem(target, idx);
        return cJSON_Compare(at, value, true);
    }
    /* move/copy address two pointers; out of scope for the snssais-array subset
     * this slice patches — reject so the document fails atomically rather than
     * silently mis-applying. */
    return false;
}

nssf_availability_result_e nssf_availability_engine_patch(
    nssf_availability_engine_t *engine,
    const char *nf_id,
    const char *content_type,
    const cJSON *tai,
    const cJSON *patch,
    cJSON **out_authorized,
    char *errbuf,
    size_t errlen)
{
    if (out_authorized != NULL) {
        *out_authorized = NULL;
    }
    if (engine == NULL) {
        set_err(errbuf, errlen, "availability_engine: null argument");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }

    /* Content negotiation first — a non-json-patch+json body is 415 (G-04),
     * classified distinctly so the handler answers 415 not 400/500. */
    if (content_type == NULL ||
        strcmp(content_type, JSON_PATCH_CONTENT_TYPE) != 0) {
        set_err(errbuf, errlen,
                "availability_engine: content-type not application/json-patch+json");
        return NSSF_AVAIL_ERR_UNSUPPORTED_MEDIA_TYPE;
    }
    if (!nf_id_valid(nf_id)) {
        set_err(errbuf, errlen, "availability_engine: invalid nfId");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }
    if (!cJSON_IsObject(tai)) {
        set_err(errbuf, errlen, "availability_engine: tai missing or not an object");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }
    const cJSON *plmn_id = cJSON_GetObjectItemCaseSensitive(tai, KEY_PLMN_ID);
    if (!cJSON_IsObject(plmn_id)) {
        set_err(errbuf, errlen, "availability_engine: tai.plmnId missing");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }
    /* RFC 6902 body is a JSON array of operations. */
    if (!cJSON_IsArray(patch)) {
        set_err(errbuf, errlen, "availability_engine: patch body not an array");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }

    /* Validate every op up front — atomic apply means an invalid op rejects the
     * whole document before any mutation (all-or-nothing). */
    const cJSON *op = NULL;
    cJSON_ArrayForEach(op, patch) {
        if (!patch_op_valid(op)) {
            set_err(errbuf, errlen, "availability_engine: invalid RFC 6902 op");
            return NSSF_AVAIL_ERR_BAD_REQUEST;
        }
    }

    /* Read the pre-image snssais snapshot for (plmn, tai) + nfId. */
    bool found = false, read_err = false;
    cJSON *before = read_snapshot(engine, nf_id, plmn_id, tai, &found, &read_err);
    if (read_err) {
        cJSON_Delete(before);
        set_err(errbuf, errlen, "availability_engine: snapshot read failed");
        return NSSF_AVAIL_ERR_INTERNAL;
    }
    if (!found) {
        cJSON_Delete(before);
        set_err(errbuf, errlen, "availability_engine: nfId has no availability");
        return NSSF_AVAIL_ERR_NOT_FOUND;
    }

    /* Apply to a WORKING COPY so the pre-image stays intact until full success. */
    cJSON *working = cJSON_Duplicate(before, true);
    if (working == NULL) {
        cJSON_Delete(before);
        set_err(errbuf, errlen, "availability_engine: OOM duplicating snapshot");
        return NSSF_AVAIL_ERR_INTERNAL;
    }
    cJSON_ArrayForEach(op, patch) {
        if (!apply_one_op(working, op)) {
            cJSON_Delete(working);
            cJSON_Delete(before);
            set_err(errbuf, errlen, "availability_engine: RFC 6902 apply failed");
            return NSSF_AVAIL_ERR_BAD_REQUEST;
        }
    }

    /* Commit the patched snssais by id (COALESCE leaves other columns intact). */
    int rc = nssf_availability_repo_patch(engine->repo, nf_id,
                                          NULL, NULL, working, NULL,
                                          false, false);
    if (rc == 1) {
        cJSON_Delete(working);
        cJSON_Delete(before);
        set_err(errbuf, errlen, "availability_engine: nfId has no availability");
        return NSSF_AVAIL_ERR_NOT_FOUND;
    }
    if (rc != 0) {
        cJSON_Delete(working);
        cJSON_Delete(before);
        set_err(errbuf, errlen, "availability_engine: patch commit failed");
        return NSSF_AVAIL_ERR_INTERNAL;
    }

    /* Commit succeeded → publish the PATCHED change event (before → after). */
    publish_change(engine, nf_id, tai, NSSF_AVAIL_CHANGE_PATCHED, before, working);

    nssf_availability_result_e result;
    if (cJSON_GetArraySize(working) == 0) {
        result = NSSF_AVAIL_NO_CONTENT;  /* no slices after update → 204 */
    } else if (out_authorized != NULL) {
        /* supportedFeatures is unchanged by a snssais patch — omit from the echo. */
        cJSON *authorized = make_authorized_info(working, tai, NULL);
        if (authorized == NULL) {
            cJSON_Delete(working);
            cJSON_Delete(before);
            set_err(errbuf, errlen, "availability_engine: OOM building response");
            return NSSF_AVAIL_ERR_INTERNAL;
        }
        *out_authorized = authorized;
        result = NSSF_AVAIL_OK;
    } else {
        result = NSSF_AVAIL_OK;
    }

    cJSON_Delete(working);
    cJSON_Delete(before);
    return result;
}

/* ── delete (NSSAIAvailabilityDelete) ─────────────────────────────────────── */

nssf_availability_result_e nssf_availability_engine_delete(
    nssf_availability_engine_t *engine,
    const char *nf_id,
    char *errbuf,
    size_t errlen)
{
    if (engine == NULL) {
        set_err(errbuf, errlen, "availability_engine: null argument");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }
    if (!nf_id_valid(nf_id)) {
        set_err(errbuf, errlen, "availability_engine: invalid nfId");
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }

    int rc = nssf_availability_repo_delete(engine->repo, nf_id);
    if (rc == 1) {
        set_err(errbuf, errlen, "availability_engine: nfId has no availability");
        return NSSF_AVAIL_ERR_NOT_FOUND;
    }
    if (rc != 0) {
        set_err(errbuf, errlen, "availability_engine: delete failed");
        return NSSF_AVAIL_ERR_INTERNAL;
    }

    /* Commit succeeded → publish the DELETED change event. This is the cascade
     * seam by which SubscriptionStore/NotificationDispatcher (later) tombstone
     * affected subscriptions; with a NULL seam it is a no-op. `after` is NULL
     * (availability removed); `before`/`tai` are unknown by id alone here. */
    publish_change(engine, nf_id, NULL, NSSF_AVAIL_CHANGE_DELETED, NULL, NULL);

    return NSSF_AVAIL_NO_CONTENT;
}

/* ── options (NSSAIAvailabilityOptions) ───────────────────────────────────── */

nssf_availability_result_e nssf_availability_engine_options(
    nssf_availability_engine_t *engine,
    char *allow_out,
    size_t allow_len)
{
    (void)engine;  /* static metadata — no engine state consulted. */
    if (allow_out == NULL || allow_len == 0) {
        return NSSF_AVAIL_ERR_BAD_REQUEST;
    }
    /* Supported methods on the NSSAIAvailability resource (api matrix rows). */
    static const char ALLOW[] = "PUT, PATCH, DELETE, OPTIONS";
    if (allow_len <= sizeof(ALLOW) - 1) {
        allow_out[0] = '\0';
        return NSSF_AVAIL_ERR_BAD_REQUEST;  /* buffer too small */
    }
    memcpy(allow_out, ALLOW, sizeof(ALLOW));
    return NSSF_AVAIL_OK;
}
