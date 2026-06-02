/*
 * test_availability_engine_patch.c — WI-availability-engine unit acceptance
 * (patch / NSSAIAvailabilityPatch, RFC 6902).
 *
 * AvailabilityEngine.patch() over the in-memory AvailabilityRepository seam + a
 * STUB change-event seam. No DB, no network, no env gating — runs unconditionally
 * in CI. ctest -R nssf_availability_engine_ 매칭 (nssf_availability_engine_patch).
 *
 * 검증 항목 (availability_engine.h 계약 + AvailabilityEngine.md + G-04):
 *   - content_type="application/json-patch+json" + routing tai 가 seeded row 매칭:
 *     add /- · remove /0 · replace /0 · test /0 ops → OK, read-back applied.
 *   - 비-json-patch content_type → UNSUPPORTED_MEDIA_TYPE (415 path).
 *   - invalid op / out-of-range index → BAD_REQUEST, row UNCHANGED (atomicity,
 *     read-back 으로 확인).
 *   - unknown nfId @(plmn,tai) → NOT_FOUND.
 *   - success path 에서 stub seam 이 PATCHED 관측.
 *
 * patch 는 (tai.plmnId, tai)+nfId 로 stored row 를 라우팅한다. seed 의 plmn_id /
 * tai 와 engine 에 넘기는 tai (그 plmnId 가 seed plmn_id 와 동일) 가 일치해야
 * find_for_plmn_tai 가 row 를 찾는다.
 *
 * Ownership — 모든 cJSON 은 본 test 가 cJSON_Delete. LSan-clean 의무.
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

/*
 * availability row 1건 seed. snssais 는 caller build (소비). plmn_id/tai 는 동일
 * (mcc,mnc,tac) 로 구성 — patch routing tai 와 일치시키기 위함.
 */
static void seed_row(nssf_availability_repo_t *repo, const char *id,
                     const char *mcc, const char *mnc, const char *tac,
                     cJSON *snssais_owned /* 소비 */)
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

/* (plmn,tai)+id 로 stored snssais 배열을 deep-copy 로 read-back. 없으면 NULL. */
static cJSON *read_back_snssais(nssf_availability_repo_t *repo, const char *id,
                                const char *mcc, const char *mnc, const char *tac)
{
    cJSON *q_plmn = make_plmn(mcc, mnc);
    cJSON *q_tai = make_tai(mcc, mnc, tac);
    nssf_availability_record_list_t list;
    memset(&list, 0, sizeof(list));
    int rc = nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list);
    cJSON *result = NULL;
    if (rc == 0) {
        for (size_t i = 0; i < list.count; i++) {
            if (strcmp(list.records[i].id, id) == 0 &&
                cJSON_IsArray(list.records[i].snssais)) {
                result = cJSON_Duplicate(list.records[i].snssais, true);
                break;
            }
        }
    }
    nssf_availability_record_list_free(&list);
    cJSON_Delete(q_plmn);
    cJSON_Delete(q_tai);
    return result;
}

/* RFC 6902 op object 1건. value NULL → value 키 생략 (remove 등). caller owns. */
static cJSON *make_op(const char *op, const char *path, cJSON *value_owned)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "op", op);
    cJSON_AddStringToObject(o, "path", path);
    if (value_owned != NULL) {
        cJSON_AddItemToObject(o, "value", value_owned);
    }
    return o;
}

/* ── stub change-event seam ───────────────────────────────────────────────── */

typedef struct {
    int calls;
    nssf_availability_change_e last_type;
    char last_nf_id[64];
} stub_capture_t;

static void stub_publish(void *ctx, const nssf_availability_change_event_t *ev)
{
    stub_capture_t *cap = (stub_capture_t *)ctx;
    cap->calls++;
    cap->last_type = ev->change_type;
    snprintf(cap->last_nf_id, sizeof(cap->last_nf_id), "%s",
             ev->nf_id != NULL ? ev->nf_id : "");
}

#define CT_JSON_PATCH "application/json-patch+json"
#define MCC "001"
#define MNC "01"
#define TAC "000001"

/* ── test 1: valid json-patch ops → OK + read-back applied + PATCHED event ── */

static void test_patch_ok_ops_applied_and_event(void)
{
    stub_capture_t cap;
    memset(&cap, 0, sizeof(cap));

    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_seams_t seams = {.publish = stub_publish, .ctx = &cap};
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    const char *nf_id = "11111111-aaaa-3333-4444-555555555555";

    /* seed snssais = [ (1,"000001") ]. */
    cJSON *seed = cJSON_CreateArray();
    cJSON_AddItemToArray(seed, make_snssai(1, "000001"));
    seed_row(repo, nf_id, MCC, MNC, TAC, seed);

    /* patch document — 4 op 종류 모두 행사. snssais 는 idx 0 한 원소로 시작. */
    cJSON *patch = cJSON_CreateArray();
    /* test /0 — 현재 idx0 == (1,"000001") 임을 단언 (RFC 6902 test). */
    cJSON_AddItemToArray(patch, make_op("test", "/0", make_snssai(1, "000001")));
    /* add /- — 끝에 (2,"0000ab") append → [s1, s2]. */
    cJSON_AddItemToArray(patch, make_op("add", "/-", make_snssai(2, "0000ab")));
    /* replace /0 — idx0 을 (3,NULL) 로 교체 → [s3, s2]. */
    cJSON_AddItemToArray(patch, make_op("replace", "/0", make_snssai(3, NULL)));
    /* remove /0 — idx0 (s3) 제거 → [s2]. */
    cJSON_AddItemToArray(patch, make_op("remove", "/0", NULL));

    cJSON *tai = make_tai(MCC, MNC, TAC);
    cJSON *out = NULL;
    char err[128] = {0};
    nssf_availability_result_e r = nssf_availability_engine_patch(
        engine, nf_id, CT_JSON_PATCH, tai, patch, &out, err, sizeof(err));

    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_OK, r, err);

    /* read-back: 최종 snssais == [ (2,"0000ab") ] (1 원소). */
    cJSON *back = read_back_snssais(repo, nf_id, MCC, MNC, TAC);
    TEST_ASSERT_NOT_NULL_MESSAGE(back, "patch 후 row read-back 실패");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, cJSON_GetArraySize(back),
                                  "patch ops 적용 후 snssais 길이 != 1");
    cJSON *s0 = cJSON_GetArrayItem(back, 0);
    cJSON *sst = cJSON_GetObjectItemCaseSensitive(s0, "sst");
    cJSON *sd = cJSON_GetObjectItemCaseSensitive(s0, "sd");
    TEST_ASSERT_TRUE(cJSON_IsNumber(sst));
    TEST_ASSERT_EQUAL_INT(2, sst->valueint);
    TEST_ASSERT_TRUE(cJSON_IsString(sd));
    TEST_ASSERT_EQUAL_STRING("0000ab", sd->valuestring);

    /* stub seam 이 PATCHED 1회 관측. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, cap.calls, "PATCHED 이벤트 미발행");
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_CHANGE_PATCHED, cap.last_type,
                                  "change_type != PATCHED");
    TEST_ASSERT_EQUAL_STRING(nf_id, cap.last_nf_id);

    cJSON_Delete(back);
    cJSON_Delete(out);
    cJSON_Delete(tai);
    cJSON_Delete(patch);
    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── test 2: 비-json-patch content_type → UNSUPPORTED_MEDIA_TYPE (415) ─────── */

static void test_patch_wrong_content_type_415(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, NULL);
    TEST_ASSERT_NOT_NULL(engine);

    const char *nf_id = "22222222-aaaa-3333-4444-555555555555";
    cJSON *seed = cJSON_CreateArray();
    cJSON_AddItemToArray(seed, make_snssai(1, "000001"));
    seed_row(repo, nf_id, MCC, MNC, TAC, seed);

    cJSON *patch = cJSON_CreateArray();
    cJSON_AddItemToArray(patch, make_op("add", "/-", make_snssai(2, NULL)));
    cJSON *tai = make_tai(MCC, MNC, TAC);
    cJSON *out = NULL;
    char err[128] = {0};

    /* application/json (not json-patch+json) → 415 path. */
    nssf_availability_result_e r = nssf_availability_engine_patch(
        engine, nf_id, "application/json", tai, patch, &out, err, sizeof(err));
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_ERR_UNSUPPORTED_MEDIA_TYPE, r,
                                  "비-json-patch content_type 가 415 아님");
    TEST_ASSERT_NULL(out);

    /* row 는 그대로 — 415 는 mutation 전에 거부. */
    cJSON *back = read_back_snssais(repo, nf_id, MCC, MNC, TAC);
    TEST_ASSERT_NOT_NULL(back);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(back));

    cJSON_Delete(back);
    cJSON_Delete(tai);
    cJSON_Delete(patch);
    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── test 3: invalid op / out-of-range index → BAD_REQUEST, row UNCHANGED ─── */

static void test_patch_invalid_atomicity(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, NULL);
    TEST_ASSERT_NOT_NULL(engine);

    const char *nf_id = "33333333-aaaa-3333-4444-555555555555";
    cJSON *seed = cJSON_CreateArray();
    cJSON_AddItemToArray(seed, make_snssai(1, "000001"));   /* len 1 — idx0 only */
    seed_row(repo, nf_id, MCC, MNC, TAC, seed);

    cJSON *tai = make_tai(MCC, MNC, TAC);
    char err[128];

    /* (a) unknown op token → 문서 전체 거부 (validate 단계, mutation 전). */
    {
        cJSON *patch = cJSON_CreateArray();
        cJSON_AddItemToArray(patch,
                             make_op("frobnicate", "/0", make_snssai(9, NULL)));
        cJSON *out = NULL;
        err[0] = '\0';
        nssf_availability_result_e r = nssf_availability_engine_patch(
            engine, nf_id, CT_JSON_PATCH, tai, patch, &out, err, sizeof(err));
        TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_ERR_BAD_REQUEST, r,
                                      "unknown op 이 BAD_REQUEST 아님");
        TEST_ASSERT_NULL(out);
        cJSON_Delete(patch);
    }

    /* (b) out-of-range index → apply 단계 실패 → all-or-nothing 으로 거부. */
    {
        cJSON *patch = cJSON_CreateArray();
        /* 유효 op 하나 (add /-) 뒤에 out-of-range replace /5 — 후자 때문에 전체 실패. */
        cJSON_AddItemToArray(patch, make_op("add", "/-", make_snssai(7, NULL)));
        cJSON_AddItemToArray(patch, make_op("replace", "/5", make_snssai(8, NULL)));
        cJSON *out = NULL;
        err[0] = '\0';
        nssf_availability_result_e r = nssf_availability_engine_patch(
            engine, nf_id, CT_JSON_PATCH, tai, patch, &out, err, sizeof(err));
        TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_ERR_BAD_REQUEST, r,
                                      "out-of-range index 가 BAD_REQUEST 아님");
        TEST_ASSERT_NULL(out);
        cJSON_Delete(patch);
    }

    /* row UNCHANGED — 두 실패 모두 commit 전 거부였음을 read-back 으로 확인. */
    cJSON *back = read_back_snssais(repo, nf_id, MCC, MNC, TAC);
    TEST_ASSERT_NOT_NULL(back);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, cJSON_GetArraySize(back),
                                  "invalid patch 가 row 를 변경 (atomicity 위반)");
    cJSON *s0 = cJSON_GetArrayItem(back, 0);
    cJSON *sst = cJSON_GetObjectItemCaseSensitive(s0, "sst");
    TEST_ASSERT_EQUAL_INT(1, sst->valueint);

    cJSON_Delete(back);
    cJSON_Delete(tai);
    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── test 4: unknown nfId @(plmn,tai) → NOT_FOUND ─────────────────────────── */

static void test_patch_unknown_nfid_not_found(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    nssf_availability_engine_t *engine = nssf_availability_engine_new(repo, NULL);
    TEST_ASSERT_NOT_NULL(engine);

    /* row 를 (MCC,MNC,TAC) 에 seed 하되, patch 는 다른 nfId 로 → 같은 (plmn,tai)
     * 에서 그 nfId row 가 없으므로 NOT_FOUND. */
    cJSON *seed = cJSON_CreateArray();
    cJSON_AddItemToArray(seed, make_snssai(1, "000001"));
    seed_row(repo, "44444444-aaaa-3333-4444-555555555555", MCC, MNC, TAC, seed);

    cJSON *patch = cJSON_CreateArray();
    cJSON_AddItemToArray(patch, make_op("add", "/-", make_snssai(2, NULL)));
    cJSON *tai = make_tai(MCC, MNC, TAC);
    cJSON *out = NULL;
    char err[128] = {0};

    nssf_availability_result_e r = nssf_availability_engine_patch(
        engine, "deadbeef-0000-0000-0000-000000000000", CT_JSON_PATCH, tai,
        patch, &out, err, sizeof(err));
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_AVAIL_ERR_NOT_FOUND, r,
                                  "unknown nfId 가 NOT_FOUND 아님");
    TEST_ASSERT_NULL(out);

    cJSON_Delete(tai);
    cJSON_Delete(patch);
    nssf_availability_engine_free(engine);
    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_patch_ok_ops_applied_and_event);
    RUN_TEST(test_patch_wrong_content_type_415);
    RUN_TEST(test_patch_invalid_atomicity);
    RUN_TEST(test_patch_unknown_nfid_not_found);
    return UNITY_END();
}
