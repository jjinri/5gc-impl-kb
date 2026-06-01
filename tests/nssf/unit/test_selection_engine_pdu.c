/*
 * test_selection_engine_pdu.c — WI-selection-engine unit acceptance.
 *
 * SliceInfoForPDUSession path — single `sNssai` (배열 아님). engine 이 단일
 * sNssai 를 1-원소 requested 집합으로 처리하고 availability gate 로 allow/reject 함을
 * 검증. PDU 변형엔 body subscribedNssai 가 없으므로 subscribed 게이트는 UDM seam
 * 로 공급한다 (NULL seam → body subscribed 부재 → empty → 항상 reject 가 되어
 * availability gate 를 단독 검증하지 못하므로).
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

/* PDU query — sliceInfoForPDUSession.sNssai 단일 객체. */
static cJSON *make_pdu_query(const char *mcc, const char *mnc, const char *tac,
                             cJSON *snssai_owned /* 소비 */)
{
    cJSON *query = cJSON_CreateObject();
    cJSON *pdu = cJSON_CreateObject();
    cJSON_AddItemToObject(pdu, "sNssai", snssai_owned);
    cJSON_AddItemToObject(query, "sliceInfoForPDUSession", pdu);
    cJSON_AddItemToObject(query, "tai", make_tai(mcc, mnc, tac));
    cJSON_AddItemToObject(query, "homePlmnId", make_plmn(mcc, mnc));
    return query;
}

/* UDM seam — 호출마다 subscribed 배열 새로 alloc (ownership engine 으로 양도). */
static bool subscribed_two(void *ctx, const cJSON *q, cJSON **out)
{
    (void)ctx;
    (void)q;
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, make_snssai(1, "000001"));
    cJSON_AddItemToArray(arr, make_snssai(2, "0000ab"));
    *out = arr;
    return true;
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

static int rejected_count(const cJSON *authorized)
{
    cJSON *rej = cJSON_GetObjectItemCaseSensitive(authorized, "rejectedNssaiInTa");
    return cJSON_IsArray(rej) ? cJSON_GetArraySize(rej) : 0;
}

/* sNssai 가 availability 에 있음 → FULL_ALLOW (단일이므로 partial 불가). */
static void test_pdu_single_snssai_allowed(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    seed_availability(repo, "a1111111-0000-0000-0000-000000000001",
                      "001", "01", "000001", avail);

    cJSON *query = make_pdu_query("001", "01", "000001", make_snssai(1, "000001"));

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    seams.subscribed_nssai = subscribed_two;
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_REJECT;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_FULL_ALLOW, result);
    TEST_ASSERT_TRUE(allowed_has(authorized, 1, "000001"));
    TEST_ASSERT_EQUAL_INT(0, rejected_count(authorized));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* sNssai 가 availability 에 없음 → FULL_REJECT, rejected 가 보유. */
static void test_pdu_single_snssai_rejected(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(9, "0000ff"));
    seed_availability(repo, "a2222222-0000-0000-0000-000000000002",
                      "001", "01", "000001", avail);

    cJSON *query = make_pdu_query("001", "01", "000001", make_snssai(1, "000001"));

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    seams.subscribed_nssai = subscribed_two;
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_ALLOW;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_FULL_REJECT, result);
    TEST_ASSERT_NULL(allowed_snssai_list(authorized));
    TEST_ASSERT_EQUAL_INT(1, rejected_count(authorized));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pdu_single_snssai_allowed);
    RUN_TEST(test_pdu_single_snssai_rejected);
    return UNITY_END();
}
