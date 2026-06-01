/*
 * test_selection_engine_redirect.c — WI-selection-engine unit acceptance.
 *
 * 두 seam 계열을 검증한다.
 *
 *   (a) redirect_table (open-gaps-and-assumptions G-07) — selection_engine.h 계약:
 *       "the in-process engine only reads, never invents, a redirect target".
 *       - redirect_table NULL/absent → 결과에 redirect 가 발명되지 않음.
 *       - redirect_table present 라도 engine 은 read-only — 결과에 redirect 를
 *         날조하지 않는다 (table 존재가 별도 redirect key 를 강제하지 않음).
 *       두 경우 모두 결정은 변함없이 allowed = 교집합.
 *
 *   (b) UDM subscribed seam override — header:
 *       - subscribed_nssai fn-ptr NULL → body 의 subscribedNssai 로 fallback.
 *       - non-NULL seam → body subscribedNssai 를 무시하고 seam 출력으로 override.
 *       동일 query 에 대해 seam 유무가 allow/reject 를 뒤집어 override 를 증명.
 *
 * + NRF amf_set_resolver seam — non-NULL 이고 allow 가 있으면 targetAmfSet 반영,
 *   NULL 이면 targetAmfSet 부재.
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

/* Registration query. subscribed_src NULL → body subscribedNssai 생략. */
static cJSON *make_registration_query(const char *mcc, const char *mnc,
                                      const char *tac, cJSON *requested_arr,
                                      cJSON *subscribed_src /* NULL 가능, 소비 */)
{
    cJSON *query = cJSON_CreateObject();
    cJSON *reg = cJSON_CreateObject();
    cJSON_AddItemToObject(reg, "requestedNssai", requested_arr);
    if (subscribed_src != NULL) {
        cJSON *sub_list = cJSON_CreateArray();
        cJSON *it = NULL;
        cJSON_ArrayForEach(it, subscribed_src) {
            cJSON *wrap = cJSON_CreateObject();
            cJSON_AddItemToObject(wrap, "subscribedSnssai",
                                  cJSON_Duplicate(it, true));
            cJSON_AddItemToArray(sub_list, wrap);
        }
        cJSON_AddItemToObject(reg, "subscribedNssai", sub_list);
        cJSON_Delete(subscribed_src);
    }
    cJSON_AddItemToObject(query, "sliceInfoForRegistration", reg);
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

static int allowed_count(const cJSON *authorized)
{
    cJSON *asl = allowed_snssai_list(authorized);
    return asl == NULL ? 0 : cJSON_GetArraySize(asl);
}

/* 결과에 redirect 계열 key 가 발명되지 않았는지 — engine 은 read-only. */
static void assert_no_invented_redirect(const cJSON *authorized)
{
    /* engine 은 redirect 를 결과에 쓰지 않는다 (header 계약). 어떤 변형 key 도 없음. */
    TEST_ASSERT_NULL(cJSON_GetObjectItemCaseSensitive(authorized, "redirect"));
    TEST_ASSERT_NULL(cJSON_GetObjectItemCaseSensitive(authorized, "redirectTarget"));
    TEST_ASSERT_NULL(cJSON_GetObjectItemCaseSensitive(authorized, "nsiInformation"));
}

/* UDM seam — body subscribed 와 반대로 (9,0000ff) 만 subscribed 로 반환. */
static bool seam_subscribes_nine(void *ctx, const cJSON *q, cJSON **out)
{
    (void)ctx;
    (void)q;
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, make_snssai(9, "0000ff"));
    *out = arr;
    return true;
}

/* NRF seam — allow 가 있으면 고정 AMF set 반환 (heap, engine 이 free). */
static char *seam_amf_set(void *ctx, const cJSON *allowed, const cJSON *q)
{
    (void)ctx;
    (void)q;
    if (allowed == NULL) {
        return NULL;
    }
    return strdup("amfset-redirect-001");
}

/* ── G-07: redirect_table NULL → 발명된 redirect 없음, 결정은 정상 교집합 ── */
static void test_redirect_table_null_no_invented_redirect(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    seed_availability(repo, "d1111111-0000-0000-0000-000000000001",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams)); /* redirect_table NULL */
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_REJECT;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_FULL_ALLOW, result);
    TEST_ASSERT_EQUAL_INT(1, allowed_count(authorized));
    assert_no_invented_redirect(authorized);

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/* ── G-07: redirect_table present 라도 engine 은 read-only — redirect 미발명 ── */
static void test_redirect_table_present_engine_reads_only(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    seed_availability(repo, "d2222222-0000-0000-0000-000000000002",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    /* present redirect table — {"plmnId":{...},"target":"<uri>"} entry. */
    cJSON *redirect_table = cJSON_CreateArray();
    cJSON *rt0 = cJSON_CreateObject();
    cJSON_AddItemToObject(rt0, "plmnId", make_plmn("001", "01"));
    cJSON_AddStringToObject(rt0, "target", "https://nssf.example.org/redirect");
    cJSON_AddItemToArray(redirect_table, rt0);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    seams.redirect_table = redirect_table; /* borrowed */
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_REJECT;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_FULL_ALLOW, result);
    TEST_ASSERT_EQUAL_INT(1, allowed_count(authorized));
    /* table 이 있어도 engine 은 redirect 를 결과에 날조하지 않는다. */
    assert_no_invented_redirect(authorized);

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    cJSON_Delete(redirect_table);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/*
 * UDM seam NULL → body subscribedNssai fallback. body subscribed = (1,000001),
 * availability = (1,000001) → allow. seam 없으니 body 가 진실.
 */
static void test_subscribed_seam_null_uses_body(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    seed_availability(repo, "d3333333-0000-0000-0000-000000000003",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001")); /* body 가 허용 */

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams)); /* subscribed_nssai NULL → body fallback */
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_REJECT;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_FULL_ALLOW, result);
    TEST_ASSERT_EQUAL_INT(1, allowed_count(authorized));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/*
 * UDM seam non-NULL → seam override. 동일 query 의 body subscribed = (1,000001)
 * 이지만 seam 은 (9,0000ff) 만 반환. requested=(1,000001) 은 seam-subscribed 에
 * 없으므로 reject — seam 이 body 를 override 했음을 증명 (NULL-seam 케이스에선 allow
 * 였던 동일 입력이 reject 로 뒤집힘).
 */
static void test_subscribed_seam_overrides_body(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    seed_availability(repo, "d4444444-0000-0000-0000-000000000004",
                      "001", "01", "000001", avail);

    cJSON *requested = cJSON_CreateArray();
    cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
    cJSON *subscribed = cJSON_CreateArray();
    cJSON_AddItemToArray(subscribed, make_snssai(1, "000001")); /* body 는 허용하나 */

    cJSON *query = make_registration_query("001", "01", "000001",
                                           requested, subscribed);

    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    seams.subscribed_nssai = seam_subscribes_nine; /* seam 은 (9,..) 만 → override */
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    cJSON *authorized = NULL;
    nssf_select_result_e result = NSSF_SELECT_FULL_ALLOW;
    int rc = nssf_selection_engine_select(engine, query, &authorized, &result,
                                          NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, rc);
    /* seam override → (1,000001) 은 seam-subscribed 밖 → FULL_REJECT. */
    TEST_ASSERT_EQUAL_INT(NSSF_SELECT_FULL_REJECT, result);
    TEST_ASSERT_NULL(allowed_snssai_list(authorized));

    cJSON_Delete(authorized);
    cJSON_Delete(query);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
}

/*
 * NRF amf_set_resolver seam — non-NULL 이고 allow 있으면 targetAmfSet 반영,
 * resolver NULL 이면 targetAmfSet 부재.
 */
static void test_amf_set_resolver_reflected_and_absent(void)
{
    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);

    cJSON *avail = cJSON_CreateArray();
    cJSON_AddItemToArray(avail, make_snssai(1, "000001"));
    seed_availability(repo, "d5555555-0000-0000-0000-000000000005",
                      "001", "01", "000001", avail);

    /* (A) resolver present → targetAmfSet 반영. */
    {
        cJSON *requested = cJSON_CreateArray();
        cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
        cJSON *subscribed = cJSON_CreateArray();
        cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));
        cJSON *query = make_registration_query("001", "01", "000001",
                                               requested, subscribed);

        nssf_selection_seams_t seams;
        memset(&seams, 0, sizeof(seams));
        seams.amf_set_resolver = seam_amf_set;
        nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
        TEST_ASSERT_NOT_NULL(engine);

        cJSON *authorized = NULL;
        int rc = nssf_selection_engine_select(engine, query, &authorized, NULL,
                                              NULL, 0);
        TEST_ASSERT_EQUAL_INT(0, rc);
        cJSON *amf = cJSON_GetObjectItemCaseSensitive(authorized, "targetAmfSet");
        TEST_ASSERT_TRUE(cJSON_IsString(amf));
        TEST_ASSERT_EQUAL_STRING("amfset-redirect-001", amf->valuestring);

        cJSON_Delete(authorized);
        cJSON_Delete(query);
        nssf_selection_engine_free(engine);
    }

    /* (B) resolver NULL → targetAmfSet 부재. */
    {
        cJSON *requested = cJSON_CreateArray();
        cJSON_AddItemToArray(requested, make_snssai(1, "000001"));
        cJSON *subscribed = cJSON_CreateArray();
        cJSON_AddItemToArray(subscribed, make_snssai(1, "000001"));
        cJSON *query = make_registration_query("001", "01", "000001",
                                               requested, subscribed);

        nssf_selection_seams_t seams;
        memset(&seams, 0, sizeof(seams)); /* amf_set_resolver NULL */
        nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
        TEST_ASSERT_NOT_NULL(engine);

        cJSON *authorized = NULL;
        int rc = nssf_selection_engine_select(engine, query, &authorized, NULL,
                                              NULL, 0);
        TEST_ASSERT_EQUAL_INT(0, rc);
        TEST_ASSERT_NULL(cJSON_GetObjectItemCaseSensitive(authorized, "targetAmfSet"));

        cJSON_Delete(authorized);
        cJSON_Delete(query);
        nssf_selection_engine_free(engine);
    }

    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_redirect_table_null_no_invented_redirect);
    RUN_TEST(test_redirect_table_present_engine_reads_only);
    RUN_TEST(test_subscribed_seam_null_uses_body);
    RUN_TEST(test_subscribed_seam_overrides_body);
    RUN_TEST(test_amf_set_resolver_reflected_and_absent);
    return UNITY_END();
}
