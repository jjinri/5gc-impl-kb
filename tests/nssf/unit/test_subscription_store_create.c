/*
 * test_subscription_store_create.c — WI-subscription-store create() acceptance
 * (PR-phase3-subscription-store, SubscriptionStore.md §Subscribe).
 *
 * Acceptance #1 of the store: nssf_subscription_store_create() persists a new
 * subscription with its JSONB filter and yields a fresh UUID for the Location
 * header, and applies the operator/G-10 default expiry the caller passes in.
 * Driven ENTIRELY through the in-memory test seam
 * (nssf_subscription_store_new_inmemory) — no libpq, no PostgreSQL, no network —
 * so it runs UNCONDITIONALLY in CI. Read-back uses nssf_subscription_store_get.
 *
 *   - create persists → get returns the row (id from out_id, filter round-trips). — #1
 *   - default 24h expiry applied when the caller passes
 *     NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS (G-10): the stored expiry_at is
 *     present and ~24h ahead of now.                                              — #1/G-10
 *   - create with NO snapshot seam installed persists fine (no crash, no dispatch
 *     path entered) — the no-collaborator unit path.
 *
 * ctest -R nssf_subscription_store_ 매칭 (nssf_subscription_store_create).
 * cJSON comes transitively from nssf_store → nssf_generated.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "subscription_store.h"
#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void setUp(void) {}
void tearDown(void) {}

/* A minimal well-formed subscription filter carrying a plmnId/tai snapshot key
 * + a callbackUri-adjacent eventSubscriptions placeholder. caller owns. */
static cJSON *make_filter(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *f = cJSON_CreateObject();
    cJSON *plmn = cJSON_CreateObject();
    cJSON_AddStringToObject(plmn, "mcc", mcc);
    cJSON_AddStringToObject(plmn, "mnc", mnc);
    cJSON_AddItemToObject(f, "plmnId", plmn);
    cJSON *tai = cJSON_CreateObject();
    cJSON *tai_plmn = cJSON_CreateObject();
    cJSON_AddStringToObject(tai_plmn, "mcc", mcc);
    cJSON_AddStringToObject(tai_plmn, "mnc", mnc);
    cJSON_AddItemToObject(tai, "plmnId", tai_plmn);
    cJSON_AddStringToObject(tai, "tac", tac);
    cJSON_AddItemToObject(f, "tai", tai);
    return f;
}

/* string field of a cJSON object (NULL when absent / not a string). */
static const char *json_str(const cJSON *obj, const char *key)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsString(it) ? it->valuestring : NULL;
}

/* Days from the civil (proleptic Gregorian) epoch 1970-01-01 to y-m-d. A
 * self-contained, dependency-free conversion (Howard Hinnant's days_from_civil)
 * so the test needs neither strptime nor the glibc-only timegm under the strict
 * c_std_11 / _POSIX_C_SOURCE feature-test macros. */
static long days_from_civil(long y, unsigned m, unsigned d)
{
    y -= (m <= 2);
    long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153u * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}

/* Parse an RFC 3339 "YYYY-MM-DDTHH:MM:SSZ" UTC text into a unix epoch (seconds).
 * Returns -1 when the text does not match. */
static long parse_rfc3339_utc(const char *s)
{
    int y, mo, d, h, mi, se;
    if (sscanf(s, "%d-%d-%dT%d:%d:%dZ", &y, &mo, &d, &h, &mi, &se) != 6) {
        return -1;
    }
    long days = days_from_civil(y, (unsigned)mo, (unsigned)d);
    return days * 86400L + h * 3600L + mi * 60L + se;
}

/* ===================================================================
 * #1 — create persists a subscription with its filter + yields an id
 * =================================================================== */
static void test_create_persists_filter_and_yields_id(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL_MESSAGE(store, "in-memory store 생성 실패");

    cJSON *filter = make_filter("001", "01", "000001");

    char id[37];
    memset(id, 0xAB, sizeof(id)); /* prove create overwrites with a real id. */
    nssf_sub_result_e r = nssf_subscription_store_create(
        store, "https://amf.example.com/nssai-avail/notify", filter,
        NULL, NULL, NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS, id);

    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_SUB_OK, r, "create 가 OK 반환 실패");
    /* a v4 UUID text is exactly 36 chars + NUL → Location header source. */
    TEST_ASSERT_EQUAL_size_t_MESSAGE(36, strlen(id),
                                     "out_id 가 36-char UUID 가 아님");
    TEST_ASSERT_EQUAL_CHAR_MESSAGE('4', id[14],
                                   "UUID version nibble (pos 14) != '4'");

    /* read-back: the row exists, the filter round-trips (plmnId/tai preserved). */
    nssf_subscription_record_t rec;
    nssf_sub_result_e g = nssf_subscription_store_get(store, id, &rec);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_SUB_OK, g,
                                  "create 후 get 이 row 를 못 찾음");
    TEST_ASSERT_EQUAL_STRING(id, rec.id);
    TEST_ASSERT_EQUAL_STRING("https://amf.example.com/nssai-avail/notify",
                             rec.callback_uri);
    TEST_ASSERT_NOT_NULL_MESSAGE(rec.filter, "저장된 filter 가 NULL");
    {
        const cJSON *plmn =
            cJSON_GetObjectItemCaseSensitive(rec.filter, "plmnId");
        TEST_ASSERT_NOT_NULL_MESSAGE(plmn, "filter 의 plmnId 미보존");
        TEST_ASSERT_EQUAL_STRING("001", json_str(plmn, "mcc"));
        TEST_ASSERT_EQUAL_STRING("01", json_str(plmn, "mnc"));
        const cJSON *tai = cJSON_GetObjectItemCaseSensitive(rec.filter, "tai");
        TEST_ASSERT_NOT_NULL_MESSAGE(tai, "filter 의 tai 미보존");
        TEST_ASSERT_EQUAL_STRING("000001", json_str(tai, "tac"));
    }
    TEST_ASSERT_FALSE_MESSAGE(rec.tombstone, "신규 row 가 tombstone 상태");
    nssf_subscription_record_clear(&rec);

    cJSON_Delete(filter);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * #1/G-10 — default 24h expiry applied when the caller passes the default
 * =================================================================== */
static void test_create_applies_default_24h_expiry(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    cJSON *filter = make_filter("002", "02", "000002");

    time_t before = time(NULL);
    char id[37];
    nssf_sub_result_e r = nssf_subscription_store_create(
        store, "https://amf.example.com/cb", filter,
        NULL, NULL, NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS, id);
    time_t after = time(NULL);
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK, r);

    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, id, &rec));
    TEST_ASSERT_NOT_NULL_MESSAGE(
        rec.expiry_at, "G-10: default expiry 가 stored expiry_at 으로 적용 안 됨");

    long expiry = parse_rfc3339_utc(rec.expiry_at);
    TEST_ASSERT_TRUE_MESSAGE(expiry != -1, "expiry_at RFC3339 파싱 실패");
    /* expiry ≈ now + 24h, bracketed by the create() wall-clock window. A small
     * 2s slack absorbs the second-boundary between before/after. */
    long lo = (long)before + NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS - 2;
    long hi = (long)after + NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS + 2;
    TEST_ASSERT_TRUE_MESSAGE(expiry >= lo && expiry <= hi,
                             "default expiry 가 now+24h 창 밖 (G-10 위반)");
    nssf_subscription_record_clear(&rec);

    cJSON_Delete(filter);
    nssf_subscription_store_free(store);
}

/* A non-default operator/request expiry is honored verbatim (not overridden by
 * the 24h default) — proves expiry_seconds is the effective lifetime. */
static void test_create_honors_explicit_expiry(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    cJSON *filter = make_filter("003", "03", "000003");
    const long one_hour = 3600;

    time_t before = time(NULL);
    char id[37];
    TEST_ASSERT_EQUAL_INT(
        NSSF_SUB_OK, nssf_subscription_store_create(
                         store, "https://amf.example.com/cb", filter,
                         NULL, NULL, one_hour, id));
    time_t after = time(NULL);

    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, id, &rec));
    TEST_ASSERT_NOT_NULL(rec.expiry_at);
    long expiry = parse_rfc3339_utc(rec.expiry_at);
    TEST_ASSERT_TRUE(expiry != -1);
    TEST_ASSERT_TRUE_MESSAGE(
        expiry >= (long)before + one_hour - 2 &&
            expiry <= (long)after + one_hour + 2,
        "explicit 1h expiry 가 24h default 로 덮어써짐");
    /* and it is NOT the 24h default. */
    TEST_ASSERT_TRUE_MESSAGE(
        expiry < (long)before + NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS,
        "explicit expiry 가 default 24h 와 구분되지 않음");
    nssf_subscription_record_clear(&rec);

    cJSON_Delete(filter);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * no-collaborator unit path — create WITHOUT a snapshot seam persists, no crash
 * =================================================================== */
static void test_create_without_seam_persists_no_dispatch(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    /* deliberately DO NOT call set_snapshot_seam → retry_store NULL → the
     * initial-snapshot dispatch path is skipped entirely. */

    cJSON *filter = make_filter("004", "04", "000004");
    char id[37];
    nssf_sub_result_e r = nssf_subscription_store_create(
        store, "https://amf.example.com/cb", filter,
        NULL, NULL, NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS, id);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, r, "seam 미설치 create 가 OK 반환 실패 (no-collaborator path)");
    TEST_ASSERT_EQUAL_size_t(36, strlen(id));

    /* the row is still durable. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, id, &rec));
    nssf_subscription_record_clear(&rec);

    cJSON_Delete(filter);
    nssf_subscription_store_free(store);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_persists_filter_and_yields_id);
    RUN_TEST(test_create_applies_default_24h_expiry);
    RUN_TEST(test_create_honors_explicit_expiry);
    RUN_TEST(test_create_without_seam_persists_no_dispatch);
    return UNITY_END();
}
