/*
 * test_notification_dispatcher_queue.c — WI-notification-dispatcher unit suite.
 *
 * NotificationDispatcher retry-queue STORE seam + PRODUCER seam, driven ENTIRELY
 * through the in-memory store backend (nssf_retry_store_new_inmemory) — no DB, no
 * libpq, no network. So the enqueue-on-change acceptance + dequeue ordering /
 * SKIP-LOCKED claim / complete-vs-requeue semantics run UNCONDITIONALLY in CI.
 *
 * The required_check core (proves: Put/Patch committed mutation →
 * NotificationDispatcher retry_queue 에 row enqueue):
 *   - test_change_publish_enqueues_one_row drives the PRODUCER seam
 *     (nssf_notification_dispatcher_change_publish, the
 *      nssf_availability_change_publish_fn-compatible entry the AvailabilityEngine
 *      invokes after a committed REPLACED/PATCHED/DELETED mutation) and asserts
 *      EXACTLY one row lands in the in-memory store (dequeue==1 then dequeue==0).
 *
 * Store-seam coverage (header §store):
 *   - dequeue returns rows lowest-id-first (FIFO insertion order = the
 *     ORDER BY ... id ASC the libpq backend uses) and a CLAIMED row is hidden from
 *     a second dequeue (FOR UPDATE SKIP LOCKED mirror via the `claimed` flag).
 *   - complete() removes the claimed row terminally; requeue() re-arms it
 *     (claimed=false, attempt_count++) so the next dequeue hands it back.
 *
 * Ownership: each dequeued nssf_retry_item_t is owned by the caller and cleared
 * with nssf_retry_item_clear (LSan-clean).
 */

#define _POSIX_C_SOURCE 200809L

#include "notification_dispatcher.h"
#include "unity.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

void setUp(void) {}
void tearDown(void) {}

/* {"plmnId":{...},"tac":"..."} 형태 tai 생성. caller owns. */
static cJSON *make_tai(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *t = cJSON_CreateObject();
    cJSON *p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "mcc", mcc);
    cJSON_AddStringToObject(p, "mnc", mnc);
    cJSON_AddItemToObject(t, "plmnId", p);
    cJSON_AddStringToObject(t, "tac", tac);
    return t;
}

/* Build a borrowed change event mirroring what the engine publishes post-commit. */
static nssf_availability_change_event_t make_event(const char *nf_id,
                                                   const cJSON *tai,
                                                   nssf_availability_change_e kind,
                                                   const cJSON *after)
{
    nssf_availability_change_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.nf_id = nf_id;
    ev.tai = tai;
    ev.change_type = kind;
    ev.before = NULL;
    ev.after = after;
    return ev;
}

/* ===================================================================
 * 1. enqueue-on-change — the required_check core (Put/Patch → enqueue)
 * =================================================================== */

static void test_change_publish_enqueues_one_row(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL_MESSAGE(store, "in-memory retry store 생성 실패");

    /* The dispatcher is the PRODUCER ctx the engine seam carries; no oauth handle
     * is needed for the enqueue path (it never POSTs here). */
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);

    cJSON *tai = make_tai("001", "01", "000001");
    cJSON *after = cJSON_CreateArray(); /* authorized snssais snapshot. */
    nssf_availability_change_event_t ev = make_event(
        "nf-11111111-2222-3333-4444-555555555555", tai,
        NSSF_AVAIL_CHANGE_REPLACED, after);

    /* PRODUCER seam — signature-compatible with nssf_availability_change_publish_fn
     * (ctx == the dispatcher). This is the "Put/Patch 후 retry_queue 에 row
     * enqueue" acceptance: a committed mutation enqueues exactly one row. */
    nssf_notification_dispatcher_change_publish(disp, &ev);

    /* Exactly one row is now pending: first dequeue claims it, second finds none. */
    nssf_retry_item_t item;
    int dq1 = nssf_retry_store_dequeue(store, &item);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, dq1,
                                  "change_publish 후 첫 dequeue 가 row 반환 실패");
    TEST_ASSERT_NOT_NULL(item.id);
    /* the enqueued row keys on the changed nfId (Phase 2 producer). */
    TEST_ASSERT_EQUAL_STRING("nf-11111111-2222-3333-4444-555555555555",
                             item.subscription_id);
    /* payload carries the serialized change body (REPLACED + nfId). */
    TEST_ASSERT_NOT_NULL(item.payload_json);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(item.payload_json, "REPLACED"),
                                 "payload 가 changeType 미포함");
    nssf_retry_store_complete(store, item.id);
    nssf_retry_item_clear(&item);

    nssf_retry_item_t item2;
    int dq2 = nssf_retry_store_dequeue(store, &item2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, dq2, "change_publish 가 1개 초과 enqueue (정확히 1 row 이어야)");
    nssf_retry_item_clear(&item2);

    cJSON_Delete(tai);
    cJSON_Delete(after);
    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* A PATCHED event (the Patch acceptance half) also enqueues exactly one row. */
static void test_change_publish_patched_enqueues_one_row(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);

    cJSON *tai = make_tai("002", "02", "000002");
    nssf_availability_change_event_t ev =
        make_event("nf-patched-0001", tai, NSSF_AVAIL_CHANGE_PATCHED, NULL);
    nssf_notification_dispatcher_change_publish(disp, &ev);

    nssf_retry_item_t item;
    TEST_ASSERT_EQUAL_INT(1, nssf_retry_store_dequeue(store, &item));
    TEST_ASSERT_EQUAL_STRING("nf-patched-0001", item.subscription_id);
    TEST_ASSERT_NOT_NULL(strstr(item.payload_json, "PATCHED"));
    nssf_retry_store_complete(store, item.id);
    nssf_retry_item_clear(&item);

    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_dequeue(store, &none));
    nssf_retry_item_clear(&none);

    cJSON_Delete(tai);
    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * 2. dequeue ordering + SKIP-LOCKED claim
 * =================================================================== */

static void test_dequeue_fifo_order_and_claim_once(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    /* Enqueue three rows in order; dequeue must hand them back lowest-id-first
     * (FIFO = the libpq ORDER BY next_attempt_at ASC, id ASC). */
    TEST_ASSERT_EQUAL_INT(
        0, nssf_retry_store_enqueue(store, "sub-A", "https://amf.example/cb-A",
                                    "{\"seq\":1}", NULL));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_retry_store_enqueue(store, "sub-B", "https://amf.example/cb-B",
                                    "{\"seq\":2}", NULL));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_retry_store_enqueue(store, "sub-C", "https://amf.example/cb-C",
                                    "{\"seq\":3}", NULL));

    nssf_retry_item_t a;
    TEST_ASSERT_EQUAL_INT(1, nssf_retry_store_dequeue(store, &a));
    TEST_ASSERT_EQUAL_STRING("sub-A", a.subscription_id);
    TEST_ASSERT_EQUAL_STRING("https://amf.example/cb-A", a.callback_uri);

    /* A is now CLAIMED (held under the SKIP-LOCKED mirror). The next dequeue must
     * skip it and hand back B, NOT re-deal A. */
    nssf_retry_item_t b;
    TEST_ASSERT_EQUAL_INT(1, nssf_retry_store_dequeue(store, &b));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "sub-B", b.subscription_id,
        "claimed row 가 두 번 배포됨 (SKIP LOCKED 위반)");

    nssf_retry_item_t c;
    TEST_ASSERT_EQUAL_INT(1, nssf_retry_store_dequeue(store, &c));
    TEST_ASSERT_EQUAL_STRING("sub-C", c.subscription_id);

    /* all three claimed → no further due row. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none),
        "모든 row claimed 후 dequeue 가 0 반환 실패");

    nssf_retry_item_clear(&a);
    nssf_retry_item_clear(&b);
    nssf_retry_item_clear(&c);
    nssf_retry_item_clear(&none);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * 3. complete removes; requeue re-arms
 * =================================================================== */

static void test_complete_removes_row(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    TEST_ASSERT_EQUAL_INT(
        0, nssf_retry_store_enqueue(store, "sub-done",
                                    "https://amf.example/cb", "{}", NULL));

    nssf_retry_item_t item;
    TEST_ASSERT_EQUAL_INT(1, nssf_retry_store_dequeue(store, &item));
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_complete(store, item.id));
    nssf_retry_item_clear(&item);

    /* completed row is gone — not merely hidden. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none),
                                  "complete 후에도 row 가 dequeue 됨");
    nssf_retry_item_clear(&none);
    nssf_retry_store_free(store);
}

static void test_requeue_rearms_row(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    TEST_ASSERT_EQUAL_INT(
        0, nssf_retry_store_enqueue(store, "sub-retry",
                                    "https://amf.example/cb", "{}", NULL));

    nssf_retry_item_t first;
    TEST_ASSERT_EQUAL_INT(1, nssf_retry_store_dequeue(store, &first));
    TEST_ASSERT_EQUAL_INT(0, first.attempt_count); /* fresh row, no attempts yet. */

    /* while CLAIMED a second dequeue sees nothing. */
    nssf_retry_item_t hidden;
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_dequeue(store, &hidden));
    nssf_retry_item_clear(&hidden);

    /* requeue re-arms it for an immediate next attempt + bumps attempt_count. */
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_requeue(store, first.id));
    nssf_retry_item_clear(&first);

    nssf_retry_item_t again;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, nssf_retry_store_dequeue(store, &again),
                                  "requeue 후 row 가 재배포되지 않음");
    TEST_ASSERT_EQUAL_STRING("sub-retry", again.subscription_id);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, again.attempt_count,
                                  "requeue 가 attempt_count 증가 실패");
    nssf_retry_store_complete(store, again.id);
    nssf_retry_item_clear(&again);
    nssf_retry_store_free(store);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_change_publish_enqueues_one_row);
    RUN_TEST(test_change_publish_patched_enqueues_one_row);
    RUN_TEST(test_dequeue_fifo_order_and_claim_once);
    RUN_TEST(test_complete_removes_row);
    RUN_TEST(test_requeue_rearms_row);
    return UNITY_END();
}
