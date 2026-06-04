/*
 * test_subscription_store_patch.c — WI-subscription-store SubModifyPatch
 * acceptance (PR-phase3-subscription-store, SubscriptionStore.md §SubModifyPatch,
 * 29.531 §6.1.6 + TS29571 PatchItem, subscription_store.h Acceptance #3).
 *
 * Driven through the in-memory test seam (nssf_subscription_store_new_inmemory)
 * — no DB, no network — runs UNCONDITIONALLY in CI. A subscription is seeded via
 * create(); read-back via nssf_subscription_store_get asserts the resource view.
 *
 *   - RFC 6902 patch (op/path/value) applied ATOMICALLY: a multi-op patch whose
 *     LATER op fails aborts the WHOLE patch → 400 BAD_PATCH, NO partial mutation
 *     (the earlier ops' effect is rolled back — the stored row is untouched).    — #3
 *   - a single valid replace op mutates exactly the addressed field.             — #3
 *   - non-application/json-patch+json content-type → 415 UNSUPPORTED, the body
 *     is NOT touched (the stored resource is unchanged).                          — #3/415
 *   - patch of an absent id → 404 NOT_FOUND.                                       — #3/404
 *
 * cJSON comes transitively from nssf_store → nssf_generated.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "subscription_store.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static const char *const JSON_PATCH_CT = "application/json-patch+json";

/* {"plmnId":{...},"label":"orig"} filter — "label" is the mutable target. */
static cJSON *make_filter(const char *label)
{
    cJSON *f = cJSON_CreateObject();
    cJSON *plmn = cJSON_CreateObject();
    cJSON_AddStringToObject(plmn, "mcc", "001");
    cJSON_AddStringToObject(plmn, "mnc", "01");
    cJSON_AddItemToObject(f, "plmnId", plmn);
    cJSON_AddStringToObject(f, "label", label);
    return f;
}

/* One RFC 6902 op object {"op":..,"path":..,["value":..]}. caller adds to array. */
static cJSON *make_op(const char *op, const char *path, cJSON *value)
{
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "op", op);
    cJSON_AddStringToObject(o, "path", path);
    if (value != NULL) {
        cJSON_AddItemToObject(o, "value", value);
    }
    return o;
}

/* create a seeded subscription, return its id in out_id[37]. */
static void seed_subscription(nssf_subscription_store_t *store, char out_id[37],
                              const char *label)
{
    cJSON *filter = make_filter(label);
    nssf_sub_result_e r = nssf_subscription_store_create(
        store, "https://amf.example.com/cb", filter, NULL, NULL,
        NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS, out_id);
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK, r);
    cJSON_Delete(filter);
}

static const char *filter_label(const cJSON *filter)
{
    const cJSON *l = cJSON_GetObjectItemCaseSensitive(filter, "label");
    return cJSON_IsString(l) ? l->valuestring : NULL;
}

/* ===================================================================
 * #3 — a valid single replace op mutates exactly the addressed field
 * =================================================================== */
static void test_patch_replace_applies(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    char id[37];
    seed_subscription(store, id, "orig");

    cJSON *patch = cJSON_CreateArray();
    cJSON_AddItemToArray(
        patch,
        make_op("replace", "/filter/label", cJSON_CreateString("patched")));

    nssf_sub_result_e r =
        nssf_subscription_store_patch(store, id, JSON_PATCH_CT, patch);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_SUB_OK, r,
                                  "valid replace patch 가 OK 반환 실패");
    cJSON_Delete(patch);

    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, id, &rec));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("patched", filter_label(rec.filter),
                                     "replace 가 addressed field 미반영");
    nssf_subscription_record_clear(&rec);

    nssf_subscription_store_free(store);
}

/* ===================================================================
 * #3 — atomicity: a multi-op patch whose LATER op fails aborts the WHOLE patch
 *      (no partial mutation — the first op's effect is NOT persisted)
 * =================================================================== */
static void test_patch_atomic_failing_op_aborts_all(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    char id[37];
    seed_subscription(store, id, "orig");

    /* op0: valid replace of /filter/label → would set "halfway".
     * op1: replace of a NON-EXISTENT target (/filter/doesNotExist) → INVALID
     *      (replace requires an existing target). The whole patch must abort and
     *      op0's "halfway" must NOT survive. */
    cJSON *patch = cJSON_CreateArray();
    cJSON_AddItemToArray(
        patch,
        make_op("replace", "/filter/label", cJSON_CreateString("halfway")));
    cJSON_AddItemToArray(
        patch, make_op("replace", "/filter/doesNotExist",
                       cJSON_CreateString("boom")));

    nssf_sub_result_e r =
        nssf_subscription_store_patch(store, id, JSON_PATCH_CT, patch);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_BAD_PATCH, r,
        "failing op 가 400 BAD_PATCH 로 분류되지 않음");
    cJSON_Delete(patch);

    /* NO partial mutation — the stored label is still the original. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, id, &rec));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "orig", filter_label(rec.filter),
        "atomicity 위반 — 실패한 patch 가 earlier op 를 부분 반영함");
    nssf_subscription_record_clear(&rec);

    nssf_subscription_store_free(store);
}

/* ===================================================================
 * #3/415 — wrong content-type → 415 UNSUPPORTED, body NOT touched
 * =================================================================== */
static void test_patch_wrong_content_type_415_body_untouched(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    char id[37];
    seed_subscription(store, id, "orig");

    /* a body that WOULD succeed under the right content-type — must be ignored. */
    cJSON *patch = cJSON_CreateArray();
    cJSON_AddItemToArray(
        patch,
        make_op("replace", "/filter/label", cJSON_CreateString("should-not")));

    /* application/json (NOT application/json-patch+json) → 415 before body. */
    nssf_sub_result_e r =
        nssf_subscription_store_patch(store, id, "application/json", patch);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_UNSUPPORTED, r,
        "wrong content-type 가 415 UNSUPPORTED 로 분류되지 않음");

    /* a NULL content-type is also unsupported (defensive). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_UNSUPPORTED,
        nssf_subscription_store_patch(store, id, NULL, patch),
        "NULL content-type 가 415 로 분류되지 않음");
    cJSON_Delete(patch);

    /* body untouched — the stored label is still the original. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, id, &rec));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "orig", filter_label(rec.filter),
        "415 인데 body 가 변경됨 (415 는 body touch 전 reject 여야)");
    nssf_subscription_record_clear(&rec);

    nssf_subscription_store_free(store);
}

/* the correct content-type with a trailing charset parameter is accepted (the
 * store matches the media-type prefix). */
static void test_patch_content_type_with_charset_accepted(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    char id[37];
    seed_subscription(store, id, "orig");

    cJSON *patch = cJSON_CreateArray();
    cJSON_AddItemToArray(
        patch,
        make_op("replace", "/filter/label", cJSON_CreateString("charset-ok")));
    nssf_sub_result_e r = nssf_subscription_store_patch(
        store, id, "application/json-patch+json; charset=utf-8", patch);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, r,
        "charset 파라미터가 붙은 정상 content-type 이 reject 됨");
    cJSON_Delete(patch);

    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, id, &rec));
    TEST_ASSERT_EQUAL_STRING("charset-ok", filter_label(rec.filter));
    nssf_subscription_record_clear(&rec);

    nssf_subscription_store_free(store);
}

/* ===================================================================
 * #3/404 — patch of an absent id → NOT_FOUND
 * =================================================================== */
static void test_patch_absent_id_not_found(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    cJSON *patch = cJSON_CreateArray();
    cJSON_AddItemToArray(
        patch, make_op("replace", "/filter/label", cJSON_CreateString("x")));

    nssf_sub_result_e r = nssf_subscription_store_patch(
        store, "deadbeef-0000-4000-8000-000000000000", JSON_PATCH_CT, patch);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_NOT_FOUND, r,
        "미존재 id patch 가 404 NOT_FOUND 로 분류되지 않음");
    cJSON_Delete(patch);

    nssf_subscription_store_free(store);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_patch_replace_applies);
    RUN_TEST(test_patch_atomic_failing_op_aborts_all);
    RUN_TEST(test_patch_wrong_content_type_415_body_untouched);
    RUN_TEST(test_patch_content_type_with_charset_accepted);
    RUN_TEST(test_patch_absent_id_not_found);
    return UNITY_END();
}
