/*
 * test_availability_engine_delete_cascade.c — WI-availability-engine unit
 * acceptance (delete / NSSAIAvailabilityDelete + cascade-event seam).
 *
 * AvailabilityEngine.delete() over the in-memory AvailabilityRepository seam + a
 * STUB change-event seam. No DB, no network, no env gating — runs unconditionally
 * in CI. ctest -R nssf_availability_engine_ 매칭
 * (nssf_availability_engine_delete_cascade).
 *
 * 검증 항목 (availability_engine.h 계약 + AvailabilityEngine.md §cascade):
 *   - seeded id delete → NO_CONTENT + stub seam 이 change_type=DELETED,
 *     after=NULL 로 호출됨. 이것이 cascade-event 단언 — 실제 SubscriptionStore 가
 *     아니라 phase2 의 in-process 발행 seam 을 stub 으로 관측.
 *   - absent id delete → NOT_FOUND (이벤트 미발행 — commit 실패시 no event).
 *   - 미존재 id (한 번도 seed 안 됨) → NOT_FOUND.
 *
 * `after` 는 borrowed — call 동안만 유효. delete 의 after 는 NULL 이므로
 * pointer-was-NULL 사실만 capture (deep-copy 불필요). LSan-clean 의무.
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

/* availability row 1건 seed (delete 대상). */
static void seed_row(nssf_availability_repo_t *repo, const char *id,
                     const char *mcc, const char *mnc, const char *tac)
{
    nssf_availability_record_t rec;
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s", id);
    rec.plmn_id = make_plmn(mcc, mnc);
    rec.tai = make_tai(mcc, mnc, tac);
    rec.snssais = cJSON_CreateArray();
    cJSON_AddItemToArray(rec.snssais, make_snssai(1, "000001"));
    rec.amf_set_id = NULL;
    rec.reallocation_indication = false;
    TEST_ASSERT_EQUAL_INT(0, nssf_availability_repo_upsert(repo, &rec));
    nssf_availability_record_clear(&rec);
}

/* ── stub change-event seam — capture DELETED + after-was-NULL fact ───────── */

typedef struct {
    int calls;
    nssf_availability_change_e last_type;
    char last_nf_id[64];
    bool last_after_was_null;
    bool last_before_was_null;
} stub_capture_t;

static void stub_publish(void *ctx, const nssf_availability_change_event_t *ev)
{
    stub_capture_t *cap = (stub_capture_t *)ctx;
    cap->calls++;
    cap->last_type = ev->change_type;
    snprintf(cap->last_nf_id, sizeof(cap->last_nf_id), "%s",
             ev->nf_id != NULL ? ev->nf_id : "");
    cap->last_after_was_null = (ev->after == NULL);
    cap->last_before_was_null = (ev->before == NULL);
}

/* ── test 1: seeded id delete → NO_CONTENT + DELETED event (after=NULL) ───── */

static void test_delete_seeded_no_content_cascade_event(void)
{
    stub_capture_t cap;
    memset(&cap, 0, sizeof(cap));

    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_seams_t seams = {.publish = stub_publish, .ctx = &cap};
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    const char *nf_id = "11111111-dddd-3333-4444-555555555555";
    seed_row(repo, nf_id, "001", "01", "000001");

    char err[128] = {0};
    nssf_availability_result_e r =
        nssf_availability_engine_delete(engine, nf_id, err, sizeof(err));

    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_NO_CONTENT, r,
                                  "seeded id delete 가 NO_CONTENT 아님");

    /* cascade-event 단언 — stub seam 이 DELETED, after=NULL 로 1회 호출. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, cap.calls, "DELETED 이벤트 미발행");
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_CHANGE_DELETED, cap.last_type,
                                  "change_type != DELETED");
    TEST_ASSERT_EQUAL_STRING(nf_id, cap.last_nf_id);
    TEST_ASSERT_TRUE_MESSAGE(cap.last_after_was_null,
                             "DELETED 이벤트의 after 가 NULL 아님 (cascade 규약)");

    /* 실제 row 도 사라졌는지 read-back 으로 확인 — find empty. */
    cJSON *q_plmn = make_plmn("001", "01");
    cJSON *q_tai = make_tai("001", "01", "000001");
    nssf_availability_record_list_t list;
    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(0, list.count, "delete 후 row 가 남아있음");
    nssf_availability_record_list_free(&list);
    cJSON_Delete(q_plmn);
    cJSON_Delete(q_tai);

    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── test 2: absent id delete → NOT_FOUND, 이벤트 미발행 ───────────────────── */

static void test_delete_absent_not_found(void)
{
    stub_capture_t cap;
    memset(&cap, 0, sizeof(cap));

    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_seams_t seams = {.publish = stub_publish, .ctx = &cap};
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    /* 한 번도 seed 안 된 id → repo delete 가 no-match → NOT_FOUND. */
    char err[128] = {0};
    nssf_availability_result_e r = nssf_availability_engine_delete(
        engine, "deadbeef-0000-0000-0000-000000000000", err, sizeof(err));

    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_ERR_NOT_FOUND, r,
                                  "absent id delete 가 NOT_FOUND 아님");
    /* commit 실패(no-match)시 no event — cascade 미발행. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, cap.calls, "NOT_FOUND delete 가 cascade 이벤트를 발행");

    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── test 3: seed → delete (성공) → 재 delete (이미 삭제) → NOT_FOUND ──────── */

static void test_delete_then_redelete_not_found(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    /* NULL seam — phase2 reality (no consumer). delete 는 NULL seam 에서도 동작. */
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, NULL);
    TEST_ASSERT_NOT_NULL(engine);

    const char *nf_id = "22222222-dddd-3333-4444-555555555555";
    seed_row(repo, nf_id, "002", "02", "000002");

    char err[128] = {0};
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_AVAIL_NO_CONTENT,
        nssf_availability_engine_delete(engine, nf_id, err, sizeof(err)),
        "첫 delete 가 NO_CONTENT 아님");

    /* 같은 id 재 delete → 이제 없으므로 NOT_FOUND (NULL seam path 도 안전). */
    err[0] = '\0';
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_AVAIL_ERR_NOT_FOUND,
        nssf_availability_engine_delete(engine, nf_id, err, sizeof(err)),
        "이미 삭제된 id 재 delete 가 NOT_FOUND 아님");

    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_delete_seeded_no_content_cascade_event);
    RUN_TEST(test_delete_absent_not_found);
    RUN_TEST(test_delete_then_redelete_not_found);
    return UNITY_END();
}
