/*
 * test_selection_engine_registration.c — WI-selection-engine unit acceptance.
 *
 * SliceInfoForRegistration path of the SelectionEngine decision
 *   allowed = requested ∩ subscribed ∩ plmn_policy ∩ availability[tai].
 *
 * 검증 항목 (selection_engine.h 계약 + SelectionEngine.md §Decisions):
 *   - full-allow  — requested ⊆ subscribed ∩ availability[tai] → FULL_ALLOW,
 *                   allowedNssaiList populated, rejectedNssaiInTa absent.
 *   - partial     — 일부 requested 가 availability 에 없음 → PARTIAL (200-class),
 *                   allowed 는 survivors, rejectedNssaiInTa 는 dropped.
 *   - full-reject — 살아남는 requested 없음 → FULL_REJECT, rejected 가 전부 보유.
 *   - (sst,sd) whole-value 동등 — sd 유무가 match 를 구분 (substring 아님).
 *   - plmn_policy allow-list 가 추가 gate 로 동작.
 *
 * Engine 은 ASSEMBLED query cJSON 을 소비 (raw HTTP 아님). availability 는
 * nssf_availability_repo_new_inmemory() 로 seed — DB/network 없음. 모든 path 에서
 * out_authorized_info + repo + engine 해제. LSan-clean 의무.
 */

#include "unity.h"

#include "availability_repository.h"
#include "cJSON.h"
#include "selection_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* {"mcc":..,"mnc":..} plmn_id. caller owns. */
static cJSON *make_plmn(const char *mcc, const char *mnc)
{
    cJSON *p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "mcc", mcc);
    cJSON_AddStringToObject(p, "mnc", mnc);
    return p;
}

/* {"plmnId":{...},"tac":..} tai. caller owns. */
static cJSON *make_tai(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *t = cJSON_CreateObject();
    cJSON_AddItemToObject(t, "plmnId", make_plmn(mcc, mnc));
    cJSON_AddStringToObject(t, "tac", tac);
    return t;
}

/* {"sst":..[,"sd":..]} Snssai. caller owns. sd NULL → sd 생략. */
static cJSON *make_snssai(int sst, const char *sd)
{
    cJSON *s = cJSON_CreateObject();
    cJSON_AddNumberToObject(s, "sst", sst);
    if (sd != NULL) {
        cJSON_AddStringToObject(s, "sd", sd);
    }
    return s;
}

/* availability record 1건 upsert. snssais 는 caller 가 build 해 ownership 양도용 dup. */
static void seed_availability(nssf_availability_repo_t *repo, const char *id,
                              const char *mcc, const char *mnc, const char *tac,
                              cJSON *snssais_owned /* 소비됨 */)
{
    nssf_availability_record_t rec;
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s", id);
    rec.plmn_id = make_plmn(mcc, mnc);
    rec.tai = make_tai(mcc, mnc, tac);
    rec.snssais = snssais_owned;
    rec.amf_set_id = NULL;
    rec.reallocation_indication = false;
    TEST_ASSERT_EQUAL_INT(0, nssf_availability_repo_upsert(repo, &rec));
    nssf_availability_record_clear(&rec);
}

/*
 * Registration query 조립. requested/subscribed 는 Snssai 배열 (caller owns,
 * 소비됨). subscribed 는 SubscribedSnssai 래퍼 ({"subscribedSnssai":{...}}) 로 감싼다.
 */
static cJSON *make_registration_query(const char *mcc, const char *mnc,
                                      const char *tac,
                                      cJSON *requested_arr /* 소비 */,
                                      cJSON *subscribed_snssai_arr /* 소비, NULL 가능 */)
{
    cJSON *query = cJSON_CreateObject();
    cJSON *reg = cJSON_CreateObject();

    cJSON_AddItemToObject(reg, "requestedNssai", requested_arr);

    if (subscribed_snssai_arr != NULL) {
        cJSON *sub_list = cJSON_CreateArray();
        cJSON *it = NULL;
        cJSON_ArrayForEach(it, subscribed_snssai_arr) {
            cJSON *wrap = cJSON_CreateObject();
            cJSON_AddItemToObject(wrap, "subscribedSnssai",
                                  cJSON_Duplicate(it, true));
            cJSON_AddItemToArray(sub_list, wrap);
        }
        cJSON_AddItemToObject(reg, "subscribedNssai", sub_list);
        cJSON_Delete(subscribed_snssai_arr);
    }

    cJSON_AddItemToObject(query, "sliceInfoForRegistration", reg);
    cJSON_AddItemToObject(query, "tai", make_tai(mcc, mnc, tac));
    cJSON_AddItemToObject(query, "homePlmnId", make_plmn(mcc, mnc));
    return query;
}

/* allowedNssaiList[0].allowedSnssaiList 추출 (없으면 NULL). */
static cJSON *allowed_snssai_list(const cJSON *authorized)
{
    cJSON *anl = cJSON_GetObjectItemCaseSensitive(authorized, "allowedNssaiList");
    if (!cJSON_IsArray(anl) || cJSON_GetArraySize(anl) == 0) {
        return NULL;
    }
    cJSON *grp = cJSON_GetArrayItem(anl, 0);
    return cJSON_GetObjectItemCaseSensitive(grp, "allowedSnssaiList");
}

/* allowedSnssaiList 안에 (sst,sd) 가 allowedSnssai 로 존재? */
static bool allowed_has(const cJSON *authorized, int sst, const char *sd)
{
    cJSON *asl = allowed_snssai_list(authorized);
    if (asl == NULL) {
        return false;
    }
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, asl) {
        cJSON *s = cJSON_GetObjectItemCaseSensitive(entry, "allowedSnssai");
        cJSON *sst_j = cJSON_GetObjectItemCaseSensitive(s, "sst");
        cJSON *sd_j = cJSON_GetObjectItemCaseSensitive(s, "sd");
        if (!cJSON_IsNumber(sst_j) || sst_j->valueint != sst) {
            continue;
        }
        bool has_sd = cJSON_IsString(sd_j) && sd_j->valuestring != NULL;
        if ((sd == NULL) != (!has_sd)) {
            continue;
        }
        if (sd == NULL) {
            return true;
        }
        if (strcmp(sd_j->valuestring, sd) == 0) {
            return true;
        }
    }
    return false;
}

/* rejectedNssaiInTa 안에 (sst,sd) 존재? */
static bool rejected_has(const cJSON *authorized, int sst, const char *sd)
{
    cJSON *rej = cJSON_GetObjectItemCaseSensitive(authorized, "rejectedNssaiInTa");
    if (!cJSON_IsArray(rej)) {
        return false;
    }
    cJSON *s = NULL;
    cJSON_ArrayForEach(s, rej) {
        cJSON *sst_j = cJSON_GetObjectItemCaseSensitive(s, "sst");
        cJSON *sd_j = cJSON_GetObjectItemCaseSensitive(s, "sd");
        if (!cJSON_IsNumber(sst_j) || sst_j->valueint != sst) {
            continue;
        }
        bool has_sd = cJSON_IsString(sd_j) && sd_j->valuestring != NULL;
        if ((sd == NULL) != (!has_sd)) {
            continue;
        }
        if (sd == NULL) {
            return true;
        }
        if (strcmp(sd_j->valuestring, sd) == 0) {
            return true;
        }
    }
    return false;
}

static int allowed_count(const cJSON *authorized)
{
    cJSON *asl = allowed_snssai_list(authorized);
    return asl == NULL ? 0 : cJSON_GetArraySize(asl);
}

static int rejected_count(const cJSON *authorized)
{
    cJSON *rej = cJSON_GetObjectItemCaseSensitive(authorized, "rejectedNssaiInTa");
    return cJSON_IsArray(rej) ? cJSON_GetArraySize(rej) : 0;
}

/* ── full-allow ─────────────────────────────────────────────────────────── */
static void test_registration_full_allow(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    cJSON_AddItemToArray(avail, make_snssai(2, "0000ab"));
    seed_availability(repo, "11111111-0000-0000-0000-000000000001",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON_AddItemToArray(requested, make_snssai(2, "0000ab"));

    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));
    cJSON_AddItemToArray(subscribed, make_snssai(2, "0000ab"));

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_REJECT;
    char err[128] = {0};
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          err, sizeof(err));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, err);
    TEST_ASSERT_NOT_NULL(authorized);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_FULL_ALLOW, result);
    TEST_ASSERT_EQUAL_INT(2, allowed_count(authorized));
    TEST_ASSERT_TRUE(allowed_has(authorized, 1, "000001"));
    TEST_ASSERT_TRUE(allowed_has(authorized, 2, "0000ab"));
    /* full-allow → rejected 없음. */
    TEST_ASSERT_EQUAL_INT(0, rejected_count(authorized));
    /* allowedNssai group 은 accessType 을 보유. */
    {
        cJSON *anl = cJSON_GetObjectItemCaseSensitive(authorized, "allowedNssaiList");
        cJSON *grp = cJSON_GetArrayItem(anl, 0);
        cJSON *at = cJSON_GetObjectItemCaseSensitive(grp, "accessType");
        TEST_ASSERT_TRUE(cJSON_IsString(at));
    }

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── partial (availability gate drops one) ─────────────────────────────── */
static void test_registration_partial(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    /* availability 에는 (1,000001) 만 — (2,0000ab) 는 TAI 에 없음. */
    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    seed_availability(repo, "22222222-0000-0000-0000-000000000002",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON_AddItemToArray(requested, make_snssai(2, "0000ab"));

    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));
    cJSON_AddItemToArray(subscribed, make_snssai(2, "0000ab"));

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_ALLOW;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_PARTIAL, result);
    /* survivor = (1,000001), dropped = (2,0000ab). */
    TEST_ASSERT_EQUAL_INT(1, allowed_count(authorized));
    TEST_ASSERT_TRUE(allowed_has(authorized, 1, "000001"));
    TEST_ASSERT_FALSE(allowed_has(authorized, 2, "0000ab"));
    TEST_ASSERT_EQUAL_INT(1, rejected_count(authorized));
    TEST_ASSERT_TRUE(rejected_has(authorized, 2, "0000ab"));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── full-reject (nothing survives) ─────────────────────────────────────── */
static void test_registration_full_reject(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    /* availability 에 requested 와 겹치는 게 전혀 없음. */
    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(9, "0000ff"));
    seed_availability(repo, "33333333-0000-0000-0000-000000000003",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON_AddItemToArray(requested, make_snssai(2, "0000ab"));

    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));
    cJSON_AddItemToArray(subscribed, make_snssai(2, "0000ab"));

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_ALLOW;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_FULL_REJECT, result);
    /* full-reject → allowedNssaiList 부재, rejected 가 둘 다 보유. */
    TEST_ASSERT_NULL(allowed_snssai_list(authorized));
    TEST_ASSERT_EQUAL_INT(2, rejected_count(authorized));
    TEST_ASSERT_TRUE(rejected_has(authorized, 1, "000001"));
    TEST_ASSERT_TRUE(rejected_has(authorized, 2, "0000ab"));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/*
 * (sst,sd) whole-value 동등 — sd 유무가 match 를 구분.
 * requested (1, sd="000001") vs availability (1, no sd). sst 는 같지만 sd 유무가
 * 다르므로 match 아님 → reject. 반대로 requested (3, no sd) 는 availability (3, no sd)
 * 와 정확히 match → allow. substring/sst-only match 가 아님을 증명.
 */
static void test_registration_snssai_sd_distinguishes_match(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, NULL));   /* sd 없음 */
    cJSON_AddItemToArray(avail, make_snssai(3, NULL));   /* sd 없음 */
    seed_availability(repo, "44444444-0000-0000-0000-000000000004",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001")); /* sd 있음 → mismatch */
    cJSON_AddItemToArray(requested, make_snssai(3, NULL));     /* sd 없음 → match */

    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));
    cJSON_AddItemToArray(subscribed, make_snssai(3, NULL));

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_ALLOW;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_PARTIAL, result);
    /* (3, no sd) 만 allow, (1, sd) 는 whole-value mismatch 로 reject. */
    TEST_ASSERT_EQUAL_INT(1, allowed_count(authorized));
    TEST_ASSERT_TRUE(allowed_has(authorized, 3, NULL));
    TEST_ASSERT_FALSE(allowed_has(authorized, 1, "000001"));
    TEST_ASSERT_TRUE(rejected_has(authorized, 1, "000001"));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/*
 * plmn_policy allow-list 가 추가 gate — availability+subscribed 통과해도 policy
 * 밖이면 reject. requested 둘 다 availability/subscribed 통과하지만 policy 는
 * (1,000001) 만 허용 → (2,0000ab) reject → PARTIAL.
 */
static void test_registration_plmn_policy_gate(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    cJSON_AddItemToArray(avail, make_snssai(2, "0000ab"));
    seed_availability(repo, "55555555-0000-0000-0000-000000000005",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON_AddItemToArray(requested, make_snssai(2, "0000ab"));

    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));
    cJSON_AddItemToArray(subscribed, make_snssai(2, "0000ab"));

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    /* policy allow-list — borrowed by engine. (1,000001) 만 허용. */
    cJSON *policy = cJSON_CreateArray();
    cJSON_AddItemToArray(policy, make_snssai(1, "000001"));

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    seams.plmn_policy_allow_list = policy;
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_ALLOW;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_PARTIAL, result);
    TEST_ASSERT_EQUAL_INT(1, allowed_count(authorized));
    TEST_ASSERT_TRUE(allowed_has(authorized, 1, "000001"));
    TEST_ASSERT_TRUE(rejected_has(authorized, 2, "0000ab"));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    cJSON_Delete(policy);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_registration_full_allow);
    RUN_TEST(test_registration_partial);
    RUN_TEST(test_registration_full_reject);
    RUN_TEST(test_registration_snssai_sd_distinguishes_match);
    RUN_TEST(test_registration_plmn_policy_gate);
    return UNITY_END();
}
