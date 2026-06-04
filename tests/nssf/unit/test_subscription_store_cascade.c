/*
 * test_subscription_store_cascade.c — WI-subscription-store cascade + delete
 * idempotency acceptance (PR-phase3-subscription-store, SubscriptionStore.md
 * §Unsubscribe + cascade, subscription_store.h Acceptance #4 + #2).
 *
 * Driven through the in-memory test seam (nssf_subscription_store_new_inmemory)
 * — no DB, no network — runs UNCONDITIONALLY in CI.
 *
 *   - on_availability_deleted() cascade-tombstones the AFFECTED subscription so
 *     a subsequent get() returns NOT_FOUND (tombstoned rows are not live);
 *     an UNAFFECTED subscription remains live.                                    — #4
 *   - on_availability_deleted() is IDEMPOTENT on an absent / already-tombstoned
 *     subscription_id (no error).                                                 — #4
 *   - delete() of an absent subscription is success / no-op (idempotent
 *     Unsubscribe at this layer — not a 404).                                     — #2
 *
 * cJSON comes transitively from nssf_store → nssf_generated.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "subscription_store.h"
#include "cJSON.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static cJSON *make_filter(const char *tac)
{
    cJSON *f = cJSON_CreateObject();
    cJSON *plmn = cJSON_CreateObject();
    cJSON_AddStringToObject(plmn, "mcc", "001");
    cJSON_AddStringToObject(plmn, "mnc", "01");
    cJSON_AddItemToObject(f, "plmnId", plmn);
    cJSON_AddStringToObject(f, "tac", tac);
    return f;
}

static void seed(nssf_subscription_store_t *store, char out_id[37],
                 const char *tac)
{
    cJSON *filter = make_filter(tac);
    TEST_ASSERT_EQUAL_INT(
        NSSF_SUB_OK,
        nssf_subscription_store_create(store, "https://amf.example.com/cb",
                                       filter, NULL, NULL,
                                       NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS,
                                       out_id));
    cJSON_Delete(filter);
}

/* ===================================================================
 * #4 — cascade tombstones the AFFECTED sub; the UNAFFECTED one survives
 * =================================================================== */
static void test_cascade_tombstones_affected_only(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    char affected[37];
    char other[37];
    seed(store, affected, "000001");
    seed(store, other, "000002");

    /* sanity — both live before the cascade. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, affected, &rec));
    nssf_subscription_record_clear(&rec);
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, other, &rec));
    nssf_subscription_record_clear(&rec);

    /* the engine's deleted-availability event cascades onto `affected`. */
    nssf_sub_result_e r =
        nssf_subscription_store_on_availability_deleted(store, affected);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, r, "on_availability_deleted 가 OK 반환 실패");

    /* the affected sub is now tombstoned → not a live row → NOT_FOUND. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_NOT_FOUND,
        nssf_subscription_store_get(store, affected, &rec),
        "cascade 후 affected subscription 이 여전히 live (tombstone 실패)");

    /* the unaffected sub remains live. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_get(store, other, &rec),
        "cascade 가 unaffected subscription 까지 tombstone 함");
    nssf_subscription_record_clear(&rec);

    nssf_subscription_store_free(store);
}

/* ===================================================================
 * #4 — cascade is idempotent on absent / already-tombstoned id
 * =================================================================== */
static void test_cascade_idempotent(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    char id[37];
    seed(store, id, "000001");

    /* first cascade — tombstones. */
    TEST_ASSERT_EQUAL_INT(
        NSSF_SUB_OK, nssf_subscription_store_on_availability_deleted(store, id));
    /* second cascade on the already-tombstoned id — still OK (idempotent). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK,
        nssf_subscription_store_on_availability_deleted(store, id),
        "이미 tombstone 된 id 의 cascade 가 idempotent OK 아님");
    /* cascade on a never-existed id — also OK (idempotent, no error). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK,
        nssf_subscription_store_on_availability_deleted(
            store, "00000000-0000-4000-8000-000000000000"),
        "미존재 id 의 cascade 가 idempotent OK 아님");

    nssf_subscription_store_free(store);
}

/* ===================================================================
 * #2 — delete() idempotency: deleting an absent subscription is success/no-op
 * =================================================================== */
static void test_delete_idempotent(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    char id[37];
    seed(store, id, "000001");

    /* delete a live row → success. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_delete(store, id),
        "live row delete 가 OK 반환 실패");
    /* it is gone. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_NOT_FOUND,
                          nssf_subscription_store_get(store, id, &rec));

    /* delete the SAME (now-absent) id again → still success (idempotent). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_delete(store, id),
        "이미 삭제된 id delete 가 idempotent OK 아님 (404 금지)");
    /* delete a never-existed id → success (idempotent Unsubscribe). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK,
        nssf_subscription_store_delete(
            store, "11111111-0000-4000-8000-000000000000"),
        "미존재 id delete 가 idempotent OK 아님");

    nssf_subscription_store_free(store);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_cascade_tombstones_affected_only);
    RUN_TEST(test_cascade_idempotent);
    RUN_TEST(test_delete_idempotent);
    return UNITY_END();
}
