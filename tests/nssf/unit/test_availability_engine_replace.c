/*
 * test_availability_engine_replace.c — WI-availability-engine unit acceptance
 * (replace / NSSAIAvailabilityPut path).
 *
 * AvailabilityEngine.replace() over the in-memory AvailabilityRepository seam +
 * a STUB change-event seam. No DB, no network, no env gating — runs
 * unconditionally in CI. ctest -R nssf_availability_engine_ 매칭
 * (nssf_availability_engine_replace).
 *
 * 검증 항목 (availability_engine.h 계약 + AvailabilityEngine.md):
 *   - full-replace OK + echoed authorizedNssaiAvailabilityData[0].
 *     supportedSnssaiList 가 dedup (set 의미). stub seam 이 REPLACED 관측.
 *   - supportedNssaiAvailabilityData 의 supportedSnssaiList 가 비면 → NO_CONTENT
 *     (committed, 204), out_authorized 미설정.
 *   - tai 누락 / plmnId 누락 → BAD_REQUEST.
 *   - amfSetReallocationInd 생략 → persisted reallocation_indication=false (G-11),
 *     find_for_plmn_tai 로 read-back.
 *
 * Ownership — 모든 cJSON (info / query / out_authorized) 는 본 test 가 cJSON_Delete.
 * repo / engine 은 각 path 끝에서 free. stub seam 이 capture 한 deep-copy 도 free.
 * LSan-clean 의무.
 */

#include "unity.h"

#include "availability_engine.h"
#include "availability_repository.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* ── JSON builders (caller owns) ──────────────────────────────────────────── */

/* {"mcc":..,"mnc":..} plmn_id. */
static cJSON *make_plmn(const char *mcc, const char *mnc)
{
    cJSON *p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "mcc", mcc);
    cJSON_AddStringToObject(p, "mnc", mnc);
    return p;
}

/* {"plmnId":{...},"tac":..} tai. */
static cJSON *make_tai(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *t = cJSON_CreateObject();
    cJSON_AddItemToObject(t, "plmnId", make_plmn(mcc, mnc));
    cJSON_AddStringToObject(t, "tac", tac);
    return t;
}

/* {"sst":..[,"sd":..]} Snssai. sd NULL → sd 생략. caller owns. */
static cJSON *make_snssai(int sst, const char *sd)
{
    cJSON *s = cJSON_CreateObject();
    cJSON_AddNumberToObject(s, "sst", sst);
    if (sd != NULL) {
        cJSON_AddStringToObject(s, "sd", sd);
    }
    return s;
}

/*
 * NssaiAvailabilityInfo 한 TA-entry 조립:
 *   { "supportedNssaiAvailabilityData": [ {"tai":{...},"supportedSnssaiList":[...]} ],
 *     [ "amfSetReallocationInd": <bool> ] }
 * snssais 배열은 caller 가 build → 본 함수가 소유권 가져감(소비).
 * include_realloc=false 면 amfSetReallocationInd 키를 아예 생략(G-11 default 경로).
 */
static cJSON *make_avail_info(const char *mcc, const char *mnc, const char *tac,
                              cJSON *snssais_owned, bool include_realloc,
                              bool realloc_value)
{
    cJSON *info = cJSON_CreateObject();
    cJSON *data_list = cJSON_CreateArray();
    cJSON_AddItemToObject(info, "supportedNssaiAvailabilityData", data_list);

    cJSON *entry = cJSON_CreateObject();
    cJSON_AddItemToArray(data_list, entry);
    cJSON_AddItemToObject(entry, "tai", make_tai(mcc, mnc, tac));
    cJSON_AddItemToObject(entry, "supportedSnssaiList", snssais_owned);

    if (include_realloc) {
        cJSON_AddBoolToObject(info, "amfSetReallocationInd", realloc_value);
    }
    return info;
}

/* ── stub change-event seam — capture last event (deep-copied) ────────────── */

typedef struct {
    int calls;
    nssf_availability_change_e last_type;
    char last_nf_id[64];
    cJSON *last_after;   /* deep-copy (or NULL) — test owns, freed in reset */
    bool last_after_was_null;
} stub_capture_t;

static void stub_reset(stub_capture_t *cap)
{
    if (cap->last_after != NULL) {
        cJSON_Delete(cap->last_after);
    }
    memset(cap, 0, sizeof(*cap));
}

static void stub_publish(void *ctx, const nssf_availability_change_event_t *ev)
{
    stub_capture_t *cap = (stub_capture_t *)ctx;
    cap->calls++;
    cap->last_type = ev->change_type;
    snprintf(cap->last_nf_id, sizeof(cap->last_nf_id), "%s",
             ev->nf_id != NULL ? ev->nf_id : "");
    /* event cJSON 은 borrowed — call 동안만 유효. 보존하려면 deep-copy. */
    if (cap->last_after != NULL) {
        cJSON_Delete(cap->last_after);
        cap->last_after = NULL;
    }
    cap->last_after_was_null = (ev->after == NULL);
    if (ev->after != NULL) {
        cap->last_after = cJSON_Duplicate(ev->after, true);
    }
}

/* ── test 1: full-replace OK + dedup echo + stub saw REPLACED ─────────────── */

static void test_replace_ok_dedup_and_event(void)
{
    stub_capture_t cap;
    memset(&cap, 0, sizeof(cap));

    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    nssf_availability_engine_seams_t seams = {.publish = stub_publish, .ctx = &cap};
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    const char *nf_id = "11111111-2222-3333-4444-555555555555";

    /* supportedSnssaiList 에 (1,"000001") 가 중복으로 두 번 — dedup 으로 1개여야. */
    cJSON *snssais = cJSON_CreateArray();
    cJSON_AddItemToArray(snssais, make_snssai(1, "000001"));
    cJSON_AddItemToArray(snssais, make_snssai(1, "000001")); /* duplicate */
    cJSON_AddItemToArray(snssais, make_snssai(2, NULL));     /* distinct (no sd) */
    cJSON *info = make_avail_info("001", "01", "000001", snssais, false, false);

    cJSON *out = NULL;
    char err[128] = {0};
    nssf_availability_result_e r = nssf_availability_engine_replace(
        engine, nf_id, info, &out, err, sizeof(err));

    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_OK, r, err);
    TEST_ASSERT_NOT_NULL_MESSAGE(out, "OK replace 가 authorized body 미생성");

    /* echoed authorizedNssaiAvailabilityData[0].supportedSnssaiList 가 dedup. */
    cJSON *auth_data = cJSON_GetObjectItemCaseSensitive(
        out, "authorizedNssaiAvailabilityData");
    TEST_ASSERT_TRUE(cJSON_IsArray(auth_data));
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(auth_data));
    cJSON *d0 = cJSON_GetArrayItem(auth_data, 0);
    cJSON *echoed = cJSON_GetObjectItemCaseSensitive(d0, "supportedSnssaiList");
    TEST_ASSERT_TRUE(cJSON_IsArray(echoed));
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, cJSON_GetArraySize(echoed),
                                  "supportedSnssaiList dedup 실패 (중복 제거 안 됨)");

    /* stub seam 이 REPLACED 1회 관측, after = 새 snssais. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, cap.calls, "REPLACED 이벤트 미발행");
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_CHANGE_REPLACED, cap.last_type,
                                  "change_type != REPLACED");
    TEST_ASSERT_EQUAL_STRING(nf_id, cap.last_nf_id);
    TEST_ASSERT_NOT_NULL_MESSAGE(cap.last_after, "REPLACED after snapshot NULL");
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(cap.last_after));

    cJSON_Delete(out);
    cJSON_Delete(info);
    stub_reset(&cap);
    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── test 2: empty supported list ⇒ NO_CONTENT, no body ───────────────────── */

static void test_replace_empty_list_no_content(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, NULL);
    TEST_ASSERT_NOT_NULL(engine);

    /* tai 는 있으나 supportedSnssaiList 가 빈 배열 → no slices → 204. */
    cJSON *empty = cJSON_CreateArray();
    cJSON *info = make_avail_info("001", "01", "000001", empty, false, false);

    cJSON *out = NULL;
    char err[128] = {0};
    nssf_availability_result_e r = nssf_availability_engine_replace(
        engine, "22222222-2222-3333-4444-555555555555", info, &out, err,
        sizeof(err));

    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_NO_CONTENT, r,
                                  "빈 supportedSnssaiList 가 NO_CONTENT 아님");
    TEST_ASSERT_NULL_MESSAGE(out, "NO_CONTENT 가 body 를 생성");

    cJSON_Delete(info);
    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── test 3: missing tai / missing plmnId ⇒ BAD_REQUEST ───────────────────── */

static void test_replace_missing_tai_bad_request(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, NULL);
    TEST_ASSERT_NOT_NULL(engine);

    const char *nf_id = "33333333-2222-3333-4444-555555555555";
    char err[128];

    /* (a) supportedNssaiAvailabilityData entry 에 tai 누락 → BAD_REQUEST. */
    {
        cJSON *info = cJSON_CreateObject();
        cJSON *data_list = cJSON_CreateArray();
        cJSON_AddItemToObject(info, "supportedNssaiAvailabilityData", data_list);
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddItemToArray(data_list, entry);
        /* tai 의도적 누락 */
        cJSON *snssais = cJSON_CreateArray();
        cJSON_AddItemToArray(snssais, make_snssai(1, "000001"));
        cJSON_AddItemToObject(entry, "supportedSnssaiList", snssais);

        cJSON *out = NULL;
        err[0] = '\0';
        nssf_availability_result_e r = nssf_availability_engine_replace(
            engine, nf_id, info, &out, err, sizeof(err));
        TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_ERR_BAD_REQUEST, r,
                                      "tai 누락이 BAD_REQUEST 아님");
        TEST_ASSERT_NULL(out);
        cJSON_Delete(info);
    }

    /* (b) tai 는 있으나 plmnId 누락 → BAD_REQUEST. */
    {
        cJSON *info = cJSON_CreateObject();
        cJSON *data_list = cJSON_CreateArray();
        cJSON_AddItemToObject(info, "supportedNssaiAvailabilityData", data_list);
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddItemToArray(data_list, entry);
        cJSON *tai = cJSON_CreateObject();           /* plmnId 없는 tai */
        cJSON_AddStringToObject(tai, "tac", "000001");
        cJSON_AddItemToObject(entry, "tai", tai);
        cJSON *snssais = cJSON_CreateArray();
        cJSON_AddItemToArray(snssais, make_snssai(1, "000001"));
        cJSON_AddItemToObject(entry, "supportedSnssaiList", snssais);

        cJSON *out = NULL;
        err[0] = '\0';
        nssf_availability_result_e r = nssf_availability_engine_replace(
            engine, nf_id, info, &out, err, sizeof(err));
        TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_ERR_BAD_REQUEST, r,
                                      "plmnId 누락이 BAD_REQUEST 아님");
        TEST_ASSERT_NULL(out);
        cJSON_Delete(info);
    }

    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── test 4: amfSetReallocationInd 생략 ⇒ persisted false (G-11) ───────────── */

static void test_replace_realloc_default_false(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, NULL);
    TEST_ASSERT_NOT_NULL(engine);

    const char *nf_id = "44444444-2222-3333-4444-555555555555";

    cJSON *snssais = cJSON_CreateArray();
    cJSON_AddItemToArray(snssais, make_snssai(1, "000001"));
    /* include_realloc=false → amfSetReallocationInd 키 자체를 생략. */
    cJSON *info = make_avail_info("001", "01", "000001", snssais, false, false);

    cJSON *out = NULL;
    char err[128] = {0};
    nssf_availability_result_e r = nssf_availability_engine_replace(
        engine, nf_id, info, &out, err, sizeof(err));
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_OK, r, err);

    /* read-back via find_for_plmn_tai — persisted reallocation_indication=false. */
    cJSON *q_plmn = make_plmn("001", "01");
    cJSON *q_tai = make_tai("001", "01", "000001");
    nssf_availability_record_list_t list;
    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(1, list.count, "replace 후 find count != 1");
    TEST_ASSERT_EQUAL_STRING(nf_id, list.records[0].id);
    TEST_ASSERT_FALSE_MESSAGE(
        list.records[0].reallocation_indication,
        "amfSetReallocationInd 생략 시 reallocation_indication 이 false 아님 (G-11)");

    nssf_availability_record_list_free(&list);
    cJSON_Delete(q_plmn);
    cJSON_Delete(q_tai);
    cJSON_Delete(out);
    cJSON_Delete(info);
    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_replace_ok_dedup_and_event);
    RUN_TEST(test_replace_empty_list_no_content);
    RUN_TEST(test_replace_missing_tai_bad_request);
    RUN_TEST(test_replace_realloc_default_false);
    return UNITY_END();
}
