/*
 * test_availability_repo_inmemory.c — WI-availability-repo unit acceptance.
 *
 * AvailabilityRepository in-memory test seam round-trip. No DB —
 * nssf_availability_repo_new_inmemory() backend only. ctest -R
 * nssf_availability_repo_ 매칭 (nssf_availability_repo_inmemory).
 *
 * Round-trip coverage:
 *   - upsert → find_for_plmn_tai 가 matching record 반환 (모든 field 일치).
 *   - 동일 id 재 upsert 가 UPDATE (중복 없음) — find 가 갱신값 반환.
 *   - patch (partial) 가 지정 field 만 변경 — 나머지 보존.
 *   - delete by id → affected=0 return + find empty.
 *   - 미존재 id delete → no-match return (header 규약 = 1).
 *
 * Ownership 규약 (header) — upsert 에 넘긴 record 는 caller 가 clear; find 결과
 * list 는 nssf_availability_record_list_free 로 해제. LSan-clean 의무.
 */

#include "unity.h"

#include "availability_repository.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* {"mcc":"001","mnc":"01"} 형태 plmn_id 생성. caller owns. */
static cJSON *make_plmn(const char *mcc, const char *mnc)
{
    cJSON *p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "mcc", mcc);
    cJSON_AddStringToObject(p, "mnc", mnc);
    return p;
}

/* {"plmnId":{...},"tac":"000001"} 형태 tai 생성. caller owns. */
static cJSON *make_tai(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *t = cJSON_CreateObject();
    cJSON *p = make_plmn(mcc, mnc);
    cJSON_AddItemToObject(t, "plmnId", p);
    cJSON_AddStringToObject(t, "tac", tac);
    return t;
}

/* [{"sst":1,"sd":"000001"}] 형태 snssais 배열 생성. caller owns. */
static cJSON *make_snssais(int sst, const char *sd)
{
    cJSON *arr = cJSON_CreateArray();
    cJSON *s = cJSON_CreateObject();
    cJSON_AddNumberToObject(s, "sst", sst);
    if (sd != NULL) {
        cJSON_AddStringToObject(s, "sd", sd);
    }
    cJSON_AddItemToArray(arr, s);
    return arr;
}

/* string field 읽기 (없으면 NULL). */
static const char *json_str(const cJSON *obj, const char *key)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsString(it) ? it->valuestring : NULL;
}

/*
 * 전체 round-trip: upsert → find → re-upsert(update) → patch → delete →
 * 미존재 delete. 하나의 repo lifecycle 에서 모든 단계 검증.
 */
static void test_inmemory_full_round_trip(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL_MESSAGE(repo, "in-memory repo 생성 실패");

    const char *id = "11111111-2222-3333-4444-555555555555";

    /* ── 1. upsert insert ─────────────────────────────────────────────── */
    nssf_availability_record_t rec;
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s", id);
    rec.plmn_id = make_plmn("001", "01");
    rec.tai = make_tai("001", "01", "000001");
    rec.snssais = make_snssais(1, "000001");
    rec.amf_set_id = strdup("amfset-001");
    TEST_ASSERT_NOT_NULL(rec.amf_set_id);
    rec.reallocation_indication = false;

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_availability_repo_upsert(repo, &rec),
                                  "upsert insert 가 0 반환 실패");

    /* ── 2. find → record 반환 + field 일치 ──────────────────────────── */
    cJSON *q_plmn = make_plmn("001", "01");
    cJSON *q_tai = make_tai("001", "01", "000001");

    nssf_availability_record_list_t list;
    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list),
        "find 가 0 반환 실패");
    TEST_ASSERT_EQUAL_size_t_MESSAGE(1, list.count, "insert 후 find count != 1");

    TEST_ASSERT_EQUAL_STRING(id, list.records[0].id);
    TEST_ASSERT_EQUAL_STRING("001", json_str(list.records[0].plmn_id, "mcc"));
    TEST_ASSERT_EQUAL_STRING("01", json_str(list.records[0].plmn_id, "mnc"));
    TEST_ASSERT_EQUAL_STRING("000001", json_str(list.records[0].tai, "tac"));
    TEST_ASSERT_NOT_NULL(list.records[0].amf_set_id);
    TEST_ASSERT_EQUAL_STRING("amfset-001", list.records[0].amf_set_id);
    TEST_ASSERT_FALSE(list.records[0].reallocation_indication);
    TEST_ASSERT_TRUE(cJSON_IsArray(list.records[0].snssais));
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(list.records[0].snssais));

    nssf_availability_record_list_free(&list);

    /* ── 3. 동일 id 재 upsert → UPDATE (중복 없음, 갱신값) ────────────── */
    nssf_availability_record_clear(&rec);
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s", id);
    rec.plmn_id = make_plmn("001", "01");     /* same plmn/tai → still findable */
    rec.tai = make_tai("001", "01", "000001");
    rec.snssais = make_snssais(2, "0000ab");  /* changed snssais */
    rec.amf_set_id = strdup("amfset-002");    /* changed amf */
    TEST_ASSERT_NOT_NULL(rec.amf_set_id);
    rec.reallocation_indication = true;       /* changed flag */

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_availability_repo_upsert(repo, &rec),
                                  "re-upsert (update) 가 0 반환 실패");
    nssf_availability_record_clear(&rec);

    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
        1, list.count, "re-upsert 가 row 중복 (UPDATE 아님)");
    TEST_ASSERT_EQUAL_STRING("amfset-002", list.records[0].amf_set_id);
    TEST_ASSERT_TRUE_MESSAGE(list.records[0].reallocation_indication,
                             "re-upsert 후 reallocation_indication 미갱신");
    /* sst 갱신 확인. */
    {
        cJSON *s0 = cJSON_GetArrayItem(list.records[0].snssais, 0);
        cJSON *sst = cJSON_GetObjectItemCaseSensitive(s0, "sst");
        TEST_ASSERT_TRUE(cJSON_IsNumber(sst));
        TEST_ASSERT_EQUAL_INT(2, sst->valueint);
    }
    nssf_availability_record_list_free(&list);

    /* ── 4. patch — amf_set_id 만 변경, reallocation 미변경 ───────────── */
    char *patched_amf = "amfset-PATCHED";
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0,
        nssf_availability_repo_patch(repo, id,
                                     NULL, NULL, NULL, /* plmn/tai/snssais 보존 */
                                     patched_amf,
                                     false, /* set_reallocation=false → 보존 */
                                     false),
        "patch 가 0 (updated) 반환 실패");

    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list));
    TEST_ASSERT_EQUAL_size_t(1, list.count);
    /* 패치 field 변경. */
    TEST_ASSERT_EQUAL_STRING("amfset-PATCHED", list.records[0].amf_set_id);
    /* 미패치 field 보존 — reallocation 은 직전 upsert 의 true 유지. */
    TEST_ASSERT_TRUE_MESSAGE(
        list.records[0].reallocation_indication,
        "patch (set_reallocation=false) 가 reallocation 을 변경");
    /* snssais 도 보존 (sst=2). */
    {
        cJSON *s0 = cJSON_GetArrayItem(list.records[0].snssais, 0);
        cJSON *sst = cJSON_GetObjectItemCaseSensitive(s0, "sst");
        TEST_ASSERT_EQUAL_INT(2, sst->valueint);
    }
    nssf_availability_record_list_free(&list);

    /* patch 의 set_reallocation=true path 도 검증 — flag 만 false 로 토글. */
    TEST_ASSERT_EQUAL_INT(
        0,
        nssf_availability_repo_patch(repo, id, NULL, NULL, NULL, NULL,
                                     true,  /* set_reallocation */
                                     false  /* → false */));
    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list));
    TEST_ASSERT_EQUAL_size_t(1, list.count);
    TEST_ASSERT_FALSE_MESSAGE(
        list.records[0].reallocation_indication,
        "patch (set_reallocation=true) 가 flag 를 false 로 토글 실패");
    /* amf 는 이번 patch 에서 NULL → 보존. */
    TEST_ASSERT_EQUAL_STRING("amfset-PATCHED", list.records[0].amf_set_id);
    nssf_availability_record_list_free(&list);

    /* ── 5. patch 미존재 id → no-match return = 1 (header 규약) ────────── */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1,
        nssf_availability_repo_patch(repo, "deadbeef-0000-0000-0000-000000000000",
                                     NULL, NULL, NULL, "x", false, false),
        "미존재 id patch 가 1 (no-match) 반환 실패");

    /* ── 6. delete by id → 0 (deleted), find empty ───────────────────── */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_availability_repo_delete(repo, id),
                                  "delete 가 0 (deleted) 반환 실패");

    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list),
        "delete 후 find 가 0 반환 실패");
    TEST_ASSERT_EQUAL_size_t_MESSAGE(0, list.count,
                                     "delete 후 find count != 0");
    nssf_availability_record_list_free(&list); /* count=0 — no-op free, LSan-safe */

    /* ── 7. 미존재 id delete → no-match return = 1 (header 규약) ──────── */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, nssf_availability_repo_delete(repo, id),
        "이미 삭제된 id delete 가 1 (no-match) 반환 실패");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1,
        nssf_availability_repo_delete(repo,
                                      "00000000-0000-0000-0000-000000000000"),
        "미존재 id delete 가 1 (no-match) 반환 실패");

    cJSON_Delete(q_plmn);
    cJSON_Delete(q_tai);
    nssf_availability_repo_free(repo);
}

/*
 * 분리된 plmn/tai 두 record 가 find 에서 서로 격리 — find 필터가 일치 row 만
 * 반환함을 확인.
 */
static void test_inmemory_find_isolation(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    nssf_availability_record_t a;
    memset(&a, 0, sizeof(a));
    snprintf(a.id, sizeof(a.id), "%s", "aaaaaaaa-0000-0000-0000-000000000001");
    a.plmn_id = make_plmn("001", "01");
    a.tai = make_tai("001", "01", "000001");
    a.snssais = make_snssais(1, NULL);
    a.amf_set_id = NULL; /* SQL NULL path */
    a.reallocation_indication = false;
    TEST_ASSERT_EQUAL_INT(0, nssf_availability_repo_upsert(repo, &a));
    nssf_availability_record_clear(&a);

    nssf_availability_record_t b;
    memset(&b, 0, sizeof(b));
    snprintf(b.id, sizeof(b.id), "%s", "bbbbbbbb-0000-0000-0000-000000000002");
    b.plmn_id = make_plmn("002", "02");
    b.tai = make_tai("002", "02", "000002");
    b.snssais = make_snssais(2, NULL);
    b.amf_set_id = strdup("amfset-b");
    TEST_ASSERT_NOT_NULL(b.amf_set_id);
    b.reallocation_indication = true;
    TEST_ASSERT_EQUAL_INT(0, nssf_availability_repo_upsert(repo, &b));
    nssf_availability_record_clear(&b);

    /* a 의 키로 find → a 만, amf_set_id == NULL. */
    cJSON *qa_plmn = make_plmn("001", "01");
    cJSON *qa_tai = make_tai("001", "01", "000001");
    nssf_availability_record_list_t list;
    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, qa_plmn, qa_tai, &list));
    TEST_ASSERT_EQUAL_size_t(1, list.count);
    TEST_ASSERT_EQUAL_STRING("aaaaaaaa-0000-0000-0000-000000000001",
                             list.records[0].id);
    TEST_ASSERT_NULL_MESSAGE(list.records[0].amf_set_id,
                             "amf_set_id NULL 보존 실패");
    nssf_availability_record_list_free(&list);

    /* 미일치 키 → count 0. */
    cJSON *qc_plmn = make_plmn("999", "99");
    cJSON *qc_tai = make_tai("999", "99", "0000ff");
    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, qc_plmn, qc_tai, &list));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(0, list.count, "미일치 find count != 0");
    nssf_availability_record_list_free(&list);

    cJSON_Delete(qa_plmn);
    cJSON_Delete(qa_tai);
    cJSON_Delete(qc_plmn);
    cJSON_Delete(qc_tai);
    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_inmemory_full_round_trip);
    RUN_TEST(test_inmemory_find_isolation);
    return UNITY_END();
}
