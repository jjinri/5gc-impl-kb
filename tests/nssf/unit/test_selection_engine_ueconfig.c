/*
 * test_selection_engine_ueconfig.c — WI-selection-engine unit acceptance.
 *
 * SliceInfoForUEConfigurationUpdate path — requestedNssai[] (배열). registration
 * 과 동일한 requested ∩ subscribed ∩ availability 교집합 로직을 변형 키
 * (sliceInfoForUEConfigurationUpdate) 로 검증. 본 변형도 body subscribedNssai 를
 * 운반하므로 NULL seam 으로 body 경로를 검증한다.
 *
 * availability 는 in-memory repo seed. 모든 path 에서 out + repo + engine 해제.
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

static cJSON *make_plmn(const char *mcc, const char *mnc)
{
    cJSON *p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "mcc", mcc);
    cJSON_AddStringToObject(p, "mnc", mnc);
    return p;
}

static cJSON *make_tai(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *t = cJSON_CreateObject();
    cJSON_AddItemToObject(t, "plmnId", make_plmn(mcc, mnc));
    cJSON_AddStringToObject(t, "tac", tac);
    return t;
}

static cJSON *make_snssai(int sst, const char *sd)
{
    cJSON *s = cJSON_CreateObject();
    cJSON_AddNumberToObject(s, "sst", sst);
    if (sd != NULL) {
        cJSON_AddStringToObject(s, "sd", sd);
    }
    return s;
}

static void seed_availability(nssf_availability_repo_t *repo, const char *id,
                              const char *mcc, const char *mnc, const char *tac,
                              cJSON *snssais_owned)
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
 * UEConfigUpdate query — sliceInfoForUEConfigurationUpdate.requestedNssai[] +
 * subscribedNssai[] (SubscribedSnssai wrapper). requested/subscribed_src 소비.
 */
static cJSON *make_ueconfig_query(const char *mcc, const char *mnc,
                                  const char *tac,
                                  cJSON *requested_arr /* 소비 */,
                                  cJSON *subscribed_src /* 소비 */)
{
    cJSON *query = cJSON_CreateObject();
    cJSON *ue = cJSON_CreateObject();

    cJSON_AddItemToObject(ue, "requestedNssai", requested_arr);

    cJSON *sub_list = cJSON_CreateArray();
    cJSON *it = NULL;
    cJSON_ArrayForEach(it, subscribed_src) {
        cJSON *wrap = cJSON_CreateObject();
        cJSON_AddItemToObject(wrap, "subscribedSnssai", cJSON_Duplicate(it, true));
        cJSON_AddItemToArray(sub_list, wrap);
    }
    cJSON_AddItemToObject(ue, "subscribedNssai", sub_list);
    cJSON_Delete(subscribed_src);

    cJSON_AddItemToObject(query, "sliceInfoForUEConfigurationUpdate", ue);
    cJSON_AddItemToObject(query, "tai", make_tai(mcc, mnc, tac));
    cJSON_AddItemToObject(query, "homePlmnId", make_plmn(mcc, mnc));
    return query;
}

static cJSON *allowed_snssai_list(const cJSON *authorized)
{
    cJSON *anl = cJSON_GetObjectItemCaseSensitive(authorized, "allowedNssaiList");
    if (!cJSON_IsArray(anl) || cJSON_GetArraySize(anl) == 0) {
        return NULL;
    }
    cJSON *grp = cJSON_GetArrayItem(anl, 0);
    return cJSON_GetObjectItemCaseSensitive(grp, "allowedSnssaiList");
}

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
        if (sd == NULL || strcmp(sd_j->valuestring, sd) == 0) {
            return true;
        }
    }
    return false;
}

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
        if (sd == NULL || strcmp(sd_j->valuestring, sd) == 0) {
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

/*
 * requestedNssai[] 교집합 — requested {(1,000001),(2,0000ab),(4,0000cd)},
 * subscribed {(1,..),(2,..),(4,..)}, availability {(1,..),(4,..)}.
 * → (1),(4) allow, (2) reject (availability 부재) → PARTIAL.
 */
static void test_ueconfig_requested_intersect(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    cJSON_AddItemToArray(avail, make_snssai(4, "0000cd"));
    seed_availability(repo, "c1111111-0000-0000-0000-000000000001",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON_AddItemToArray(requested, make_snssai(2, "0000ab"));
    cJSON_AddItemToArray(requested, make_snssai(4, "0000cd"));

    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));
    cJSON_AddItemToArray(subscribed, make_snssai(2, "0000ab"));
    cJSON_AddItemToArray(subscribed, make_snssai(4, "0000cd"));

    cJSON *query = make_ueconfig_query("001", "01", "000001",
                                       requested, subscribed);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams)); /* NULL seam → body subscribedNssai 사용 */
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_ALLOW;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_PARTIAL, result);
    TEST_ASSERT_EQUAL_INT(2, allowed_count(authorized));
    TEST_ASSERT_TRUE(allowed_has(authorized, 1, "000001"));
    TEST_ASSERT_TRUE(allowed_has(authorized, 4, "0000cd"));
    TEST_ASSERT_FALSE(allowed_has(authorized, 2, "0000ab"));
    TEST_ASSERT_TRUE(rejected_has(authorized, 2, "0000ab"));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/*
 * subscribed gate — requested 가 availability 엔 있지만 subscribed 에 없으면 reject.
 * requested (5,0000ee) availability 에 있음, subscribed 에 없음 → reject (FULL_REJECT).
 */
static void test_ueconfig_subscribed_gate(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(5, "0000ee"));
    seed_availability(repo, "c2222222-0000-0000-0000-000000000002",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(5, "0000ee"));

    /* subscribed 에 (5,0000ee) 없음 — 다른 slice 만. */
    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));

    cJSON *query = make_ueconfig_query("001", "01", "000001",
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
    TEST_ASSERT_NULL(allowed_snssai_list(authorized));
    TEST_ASSERT_TRUE(rejected_has(authorized, 5, "0000ee"));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ueconfig_requested_intersect);
    RUN_TEST(test_ueconfig_subscribed_gate);
    return UNITY_END();
}
