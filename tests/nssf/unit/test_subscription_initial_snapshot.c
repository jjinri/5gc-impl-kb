/*
 * test_subscription_initial_snapshot.c — REQUIRED CHECK for
 * PR-phase3-subscription-store (SubscriptionStore.md §Subscribe,
 * subscription_store.h create() contract, G-08 addendum 2026-06-04).
 *
 * proves: 신규 subscription 진입 시 initial snapshot 을 EXACTLY ONCE dispatch.
 * On create() with the snapshot seam installed the store reads the current
 * availability snapshot, enqueues EXACTLY ONE retry_queue row (the real
 * subscription UUID + its callbackUri), and triggers ONE call-driven dispatch.
 * There is NO worker thread / polling loop — so there is NO repeated/looped
 * dispatch.
 *
 * Seams used (no network / no DB / no PostgreSQL — runs UNCONDITIONALLY in CI):
 *   - nssf_subscription_store_new_inmemory      (the store under test),
 *   - nssf_retry_store_new_inmemory             (the dispatcher's in-memory
 *                                                retry-queue backend — the
 *                                                enqueue assertion target),
 *   - nssf_availability_repo_new_inmemory       (a STUB availability snapshot
 *                                                seeded so the snapshot read
 *                                                has a non-empty result),
 *   - nssf_notification_dispatcher_new_insecure + a MOCK transport that counts
 *     POSTs (the dispatch-invoked-exactly-once assertion target).
 *
 * WHY a counting mock transport == "dispatch invoked exactly once": the store's
 * dispatch_initial_snapshot() enqueues one row then calls
 * nssf_notification_dispatcher_dispatch_pending() ONE time. That one call
 * dequeues the one due row and POSTs it once → the mock sees EXACTLY ONE call.
 * A looped/worker re-dispatch would either POST again (mock.calls > 1) or leave
 * the row enqueued; we assert neither happens.
 *
 * The ctest target name MUST match the regex nssf_subscription_initial_snapshot.
 *
 * notification_dispatcher.h + availability_repository.h + cJSON come
 * transitively from nssf_store (PUBLIC nssf_dispatcher / nssf_repository /
 * nssf_generated). curl/curl.h rides in via the dispatcher transport seam type.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "subscription_store.h"
#include "notification_dispatcher.h"
#include "availability_repository.h"
#include "cJSON.h"

#include <curl/curl.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- mock AMF callback transport: counts POSTs, replies a staged status ---- */
typedef struct {
    int calls;            /* total transport invocations across the dispatch. */
    char last_uri[256];   /* callback_uri of the last POST. */
    char last_body[512];   /* body of the last POST. */
    long reply_status;    /* staged HTTP status returned to the dispatcher. */
} amf_mock_t;

static amf_mock_t g_amf;

static int amf_mock_transport(const nssf_notification_dispatcher_t *disp,
                              const char *callback_uri, const char *body,
                              struct curl_slist *headers, long *out_http_status,
                              void *ctx)
{
    (void)disp;
    (void)headers;
    amf_mock_t *m = (amf_mock_t *)ctx;
    m->calls++;
    snprintf(m->last_uri, sizeof(m->last_uri), "%s",
             callback_uri != NULL ? callback_uri : "");
    snprintf(m->last_body, sizeof(m->last_body), "%s", body != NULL ? body : "");
    *out_http_status = m->reply_status;
    return 0;
}

void setUp(void)
{
    memset(&g_amf, 0, sizeof(g_amf));
    g_amf.reply_status = 200; /* AMF accepts → SENT (row completed/removed). */
}
void tearDown(void) {}

/* {"plmnId":{...},"tai":{...}} filter carrying a snapshot key. caller owns. */
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

/* Seed a STUB availability record so the snapshot read returns a non-empty set
 * for the filter's plmn/tai key (proves the snapshot-read branch is taken). */
static void seed_availability(nssf_availability_repo_t *repo, const char *mcc,
                              const char *mnc, const char *tac)
{
    nssf_availability_record_t rec;
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s",
             "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee");
    rec.plmn_id = cJSON_CreateObject();
    cJSON_AddStringToObject(rec.plmn_id, "mcc", mcc);
    cJSON_AddStringToObject(rec.plmn_id, "mnc", mnc);
    rec.tai = cJSON_CreateObject();
    cJSON *tplmn = cJSON_CreateObject();
    cJSON_AddStringToObject(tplmn, "mcc", mcc);
    cJSON_AddStringToObject(tplmn, "mnc", mnc);
    cJSON_AddItemToObject(rec.tai, "plmnId", tplmn);
    cJSON_AddStringToObject(rec.tai, "tac", tac);
    rec.snssais = cJSON_CreateArray();
    cJSON *s = cJSON_CreateObject();
    cJSON_AddNumberToObject(s, "sst", 1);
    cJSON_AddItemToArray(rec.snssais, s);
    rec.amf_set_id = strdup("amfset-snap");
    rec.reallocation_indication = false;
    TEST_ASSERT_EQUAL_INT(0, nssf_availability_repo_upsert(repo, &rec));
    nssf_availability_record_clear(&rec);
}

/* ===================================================================
 * REQUIRED CHECK — create with seam → EXACTLY ONE enqueue + EXACTLY ONE dispatch
 * =================================================================== */
static void test_create_dispatches_initial_snapshot_exactly_once(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    nssf_availability_repo_t *avail = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(avail);
    seed_availability(avail, "001", "01", "000001");

    /* insecure ctor: relaxes the https-only gate is NOT needed here (we use an
     * https callback so the production gate passes too) — but the store fires a
     * REAL dispatch, so install the mock transport that counts POSTs. NULL oauth
     * → no bearer attach (dev no-attach path). */
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(retry, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    /* install all three collaborators: avail repo (snapshot read), retry store
     * (enqueue target), dispatcher (the single call-driven dispatch). */
    nssf_subscription_store_set_snapshot_seam(store, avail, retry, disp);

    cJSON *filter = make_filter("001", "01", "000001");
    char id[37];
    nssf_sub_result_e r = nssf_subscription_store_create(
        store, "https://amf.example.com/nssai-avail/notify", filter,
        NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS, id);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_SUB_OK, r, "create 가 OK 반환 실패");
    TEST_ASSERT_EQUAL_size_t(36, strlen(id));

    /* (A) dispatch was invoked EXACTLY ONCE → the mock saw EXACTLY ONE POST. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls,
        "initial snapshot dispatch 가 정확히 1회가 아님 "
        "(0=미실행 / >1=loop/worker 재실행 금지 위반)");
    /* the one POST carried the new subscription's callback URI. */
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "https://amf.example.com/nssai-avail/notify", g_amf.last_uri,
        "dispatch 된 POST 가 신규 subscription 의 callbackUri 가 아님");
    /* the payload carried the REAL subscription UUID (not fabricated). */
    TEST_ASSERT_NOT_NULL_MESSAGE(
        strstr(g_amf.last_body, id),
        "initial snapshot payload 가 실제 subscription UUID 를 담지 않음");

    /* (B) EXACTLY ONE row was enqueued AND it has now been consumed (SENT →
     * completed/removed). The queue is empty → no leftover row to re-dispatch.
     * A second dequeue returning 0 (no due row) proves there was no duplicate
     * enqueue and no re-armed loop. */
    nssf_retry_item_t leftover;
    int dq = nssf_retry_store_dequeue(retry, &leftover);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, dq,
        "initial snapshot 후 retry_queue 에 row 가 남음 "
        "(중복 enqueue 또는 dispatch 후 미완료 — 정확히 1회 위반)");
    nssf_retry_item_clear(&leftover);

    /* (C) NO repeated/looped dispatch: even after create() returns, calling the
     * dispatcher again finds NOTHING due (NONE) and emits NO further POST — the
     * store fired exactly once and left no self-perpetuating work. */
    int before = g_amf.calls;
    nssf_dispatch_result_e again =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE, again,
        "create 후 queue 에 due row 가 남아있음 (looped dispatch 위험)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        before, g_amf.calls,
        "create 후 추가 POST 발생 (dispatch 가 1회를 초과)");

    /* the subscription itself is durable regardless of dispatch outcome. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, id, &rec));
    nssf_subscription_record_clear(&rec);

    cJSON_Delete(filter);
    nssf_subscription_store_free(store);
    nssf_notification_dispatcher_free(disp);
    nssf_availability_repo_free(avail);
    nssf_retry_store_free(retry);
}

/* ===================================================================
 * no seam installed → create persists, dispatch is SKIPPED (no crash, no POST)
 * =================================================================== */
static void test_create_without_seam_skips_dispatch(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    /* NO set_snapshot_seam → retry_store NULL → dispatch path returns early. */

    cJSON *filter = make_filter("002", "02", "000002");
    char id[37];
    nssf_sub_result_e r = nssf_subscription_store_create(
        store, "https://amf.example.com/cb", filter,
        NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS, id);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, r,
        "seam 미설치 create 가 OK 반환 실패 (persist, skip dispatch)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls, "seam 미설치인데 POST 가 emit 됨 (dispatch skip 위반)");

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
    RUN_TEST(test_create_dispatches_initial_snapshot_exactly_once);
    RUN_TEST(test_create_without_seam_skips_dispatch);
    return UNITY_END();
}
