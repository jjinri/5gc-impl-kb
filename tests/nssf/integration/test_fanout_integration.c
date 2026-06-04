/*
 * test_fanout_integration.c — PR-phase3-fanout-integration end-to-end cascade
 * (engine change event → SubscriptionStore fan-out match → retry_queue enqueue →
 * ONE call-driven dispatch → outbound POST). Closes the B3 deferred fan-out.
 *
 * This suite asserts the LIVE wired cascade purely through the supported
 * integration seams — NO DB, NO socket, NO live AMF — so it runs UNCONDITIONALLY
 * in CI (registered above the PostgreSQL gate):
 *
 *   - in-memory SubscriptionStore (nssf_subscription_store_new_inmemory),
 *   - in-memory retry store (nssf_retry_store_new_inmemory),
 *   - a counting MOCK outbound transport on the dispatcher
 *     (nssf_notification_dispatcher_set_transport — same pattern as
 *     test_notification_dispatcher_outbound.c) replying 200.
 *
 * main.c's fanout_publish() trampoline is `static` (not unit-reachable), so the
 * cascade is reproduced through the SUPPORTED integration seam the trampoline
 * itself calls: nssf_subscription_store_fanout_change(store, retry, ev) for the
 * store-layer match+enqueue, then ONE nssf_notification_dispatcher_dispatch_pending
 * for the single call-driven dispatch — exactly the two-step single-shot contract
 * documented on fanout_publish() (NO loop, NO worker thread).
 *
 * NOTE on the create() snapshot seam: these tests deliberately DO NOT install the
 * initial-snapshot seam on the store (nssf_subscription_store_set_snapshot_seam),
 * so create() persists each subscription WITHOUT enqueueing an initial-snapshot
 * row. That isolates the fan-out enqueue under test — every retry_queue row
 * observed here is produced by fanout_change, not by create().
 *
 * Cases (PR-phase3-fanout-integration acceptance):
 *   1. broad subscriber (no-tai filter) → ONE row (real UUID + callbackUri) →    — #1
 *      ONE dispatch → mock POST count == 1.
 *   2. TAI-scoped match vs non-match → only the matching sub's UUID enqueued.    — #2
 *   3. multi-subscriber fan-out → N rows, each its OWN distinct UUID+callbackUri. — #3
 *   4. callbackUri B1 gate (http:// non-loopback / userinfo@ / fragment#) →      — #4
 *      NOT enqueued (no un-postable row).
 *   5. DELETED change → matched sub tombstoned (store_get → NOT_FOUND).          — #5
 *   6. call-driven / single-dispatch → 2 due rows from 2 matches, ONE publish    — #6
 *      step drives at most ONE dispatch (bounded; not an unbounded drain).
 *
 * No spec re-derivation: inputs are this prompt + subscription_store.h/.c +
 * notification_dispatcher.h + the existing integration test precedents on-branch.
 */

#define _POSIX_C_SOURCE 200809L

#include "cJSON.h"
#include "notification_dispatcher.h"
#include "subscription_store.h"
#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

/* ---- counting mock AMF outbound transport (replies 200, records each POST) ---- */

typedef struct {
    int calls;            /* total outbound POSTs this dispatch run. */
    char last_uri[256];   /* callback_uri of the last POST. */
    char last_body[512];  /* body of the last POST. */
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
    *out_http_status = 200; /* AMF accepts → SENT → row completed/removed. */
    return 0;
}

void setUp(void)
{
    memset(&g_amf, 0, sizeof(g_amf));
}
void tearDown(void) {}

/* ---- filter / event builders (the persisted-filter shape: top-level `tai`) ---- */

/* The serving-TAI cJSON shape the change event carries and a tai-constrained
 * filter must deep-equal (cJSON_Compare, case-sensitive). */
static cJSON *make_tai(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *t = cJSON_CreateObject();
    cJSON *plmn = cJSON_CreateObject();
    cJSON_AddStringToObject(plmn, "mcc", mcc);
    cJSON_AddStringToObject(plmn, "mnc", mnc);
    cJSON_AddItemToObject(t, "plmnId", plmn);
    cJSON_AddStringToObject(t, "tac", tac);
    return t;
}

/* A broad-subscriber filter: NO `tai` constraint → matches any TAI. */
static cJSON *make_broad_filter(void)
{
    return cJSON_CreateObject();
}

/* A TAI-scoped filter: a top-level `tai` the fan-out matches against. */
static cJSON *make_tai_filter(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *f = cJSON_CreateObject();
    cJSON_AddItemToObject(f, "tai", make_tai(mcc, mnc, tac));
    return f;
}

/* Persist a subscription via the store API (NO snapshot seam installed → no
 * initial-snapshot enqueue). Fills out_id with the real generated UUID. The
 * filter is borrowed (the store deep-copies what it keeps); the caller frees it.
 * amf_id/amf_set_id are the DOCX-GAP-001 subscriber identity — borrowed, either
 * may be NULL (legacy/null-identity → no self-suppression). */
static void create_sub_id(nssf_subscription_store_t *store,
                          const char *callback_uri, const cJSON *filter,
                          const char *amf_id, const char *amf_set_id,
                          char out_id[37])
{
    nssf_sub_result_e r = nssf_subscription_store_create(
        store, callback_uri, filter, amf_id, amf_set_id,
        NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS, out_id);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_SUB_OK, r,
                                  "subscription create 가 실패함");
    TEST_ASSERT_TRUE_MESSAGE(out_id[0] != '\0',
                             "create 가 real UUID 를 out_id 에 채우지 않음");
}

/* Identity-agnostic create (NULL,NULL identity) — preserves the prior behavior
 * of the pre-GAP-001 call sites (null identity = no self-suppression). */
static void create_sub(nssf_subscription_store_t *store, const char *callback_uri,
                       const cJSON *filter, char out_id[37])
{
    create_sub_id(store, callback_uri, filter, NULL, NULL, out_id);
}

/* Build a borrowed change event. `tai` may be NULL (no-tai event). */
static nssf_availability_change_event_t make_event(
    const char *nf_id, nssf_availability_change_e change_type, const cJSON *tai)
{
    nssf_availability_change_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.nf_id = nf_id;
    ev.change_type = change_type;
    ev.tai = tai;
    ev.before = NULL;
    ev.after = NULL;
    return ev;
}

/* Dequeue every pending row, recording each (subscription_id, callback_uri) pair.
 * Bounded by `cap` so a runaway enqueue can never hang the test. Returns count. */
typedef struct {
    char sub_id[37];
    char callback[256];
} drained_row_t;

static int drain_rows(nssf_retry_store_t *retry, drained_row_t *out, int cap)
{
    int n = 0;
    nssf_retry_item_t item;
    /* dequeue claims (hides) each row; complete() by the row's opaque id removes
     * it terminally so the queue ends empty and nothing leaks. */
    while (n < cap && nssf_retry_store_dequeue(retry, &item) == 1) {
        snprintf(out[n].sub_id, sizeof(out[n].sub_id), "%s",
                 item.subscription_id != NULL ? item.subscription_id : "");
        snprintf(out[n].callback, sizeof(out[n].callback), "%s",
                 item.callback_uri != NULL ? item.callback_uri : "");
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, nssf_retry_store_complete(retry, item.id),
            "dequeued row complete (row id 로 제거) 실패");
        nssf_retry_item_clear(&item);
        n++;
    }
    /* If we hit the cap there were MORE rows than expected — surface it. */
    TEST_ASSERT_TRUE_MESSAGE(n < cap,
                             "enqueue 된 row 수가 예상 상한을 초과 (runaway fan-out)");
    return n;
}

/* ===================================================================
 * 1. broad subscriber → ONE row (real UUID + callbackUri) → ONE dispatch → POST 1
 * =================================================================== */

static void test_broad_subscriber_one_row_one_post(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    char sub_id[37];
    cJSON *filter = make_broad_filter();
    const char *cb = "https://amf.example.com/n1n2/notify";
    create_sub(store, cb, filter, sub_id);
    cJSON_Delete(filter);

    /* a REPLACED (Put) committed change with a serving TAI. */
    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-nf-001", NSSF_AVAIL_CHANGE_REPLACED, ev_tai);

    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, enq, "broad subscriber 가 정확히 1 row 로 fan-out 되지 않음");

    /* the enqueued row carries the REAL subscription UUID + its callbackUri. */
    nssf_retry_item_t item;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, nssf_retry_store_dequeue(retry, &item),
        "fan-out row 가 큐에 enqueue 되지 않음");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(sub_id, item.subscription_id,
                                     "row 가 real subscription UUID 를 담지 않음");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(cb, item.callback_uri,
                                     "row 가 sub 의 callbackUri 를 담지 않음");
    /* requeue so the dispatcher (built below) can drive the actual POST. */
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_requeue(retry, item.id));
    nssf_retry_item_clear(&item);

    /* ONE call-driven dispatch → mock POST count == 1 (the wired cascade tail). */
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(retry, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    nssf_dispatch_result_e dr =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_SENT, dr,
                                  "fan-out row 의 dispatch 가 SENT 되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls, "ONE dispatch 가 정확히 1 outbound POST 가 아님");
    TEST_ASSERT_EQUAL_STRING(cb, g_amf.last_uri);

    cJSON_Delete(ev_tai);
    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * 2. TAI-scoped match vs non-match → only the matching sub's UUID enqueued
 * =================================================================== */

static void test_tai_scoped_match_vs_nonmatch(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    /* sub A: filter.tai == event.tai → MATCH. */
    char id_match[37];
    cJSON *f_match = make_tai_filter("001", "01", "000001");
    create_sub(store, "https://amf-a.example.com/cb", f_match, id_match);
    cJSON_Delete(f_match);

    /* sub B: filter.tai != event.tai (different tac) → NO match. */
    char id_nonmatch[37];
    cJSON *f_nonmatch = make_tai_filter("001", "01", "00FFFF");
    create_sub(store, "https://amf-b.example.com/cb", f_nonmatch, id_nonmatch);
    cJSON_Delete(f_nonmatch);

    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-nf-002", NSSF_AVAIL_CHANGE_PATCHED, ev_tai);

    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, enq, "TAI-scoped fan-out 이 정확히 1 match (matching sub only) 가 아님");

    drained_row_t rows[8];
    int n = drain_rows(retry, rows, 8);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, n, "enqueue 된 row 가 정확히 1 개가 아님");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        id_match, rows[0].sub_id,
        "matching sub 의 UUID 가 아닌 row 가 enqueue 됨");
    TEST_ASSERT_EQUAL_STRING("https://amf-a.example.com/cb", rows[0].callback);

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * 3. multi-subscriber fan-out → N rows, each its OWN distinct UUID+callbackUri
 * =================================================================== */

static void test_multi_subscriber_distinct_rows(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    /* three matching subscribers: one broad + two TAI-scoped onto the same TAI. */
    char id0[37], id1[37], id2[37];
    cJSON *f0 = make_broad_filter();
    create_sub(store, "https://amf-0.example.com/cb", f0, id0);
    cJSON_Delete(f0);
    cJSON *f1 = make_tai_filter("001", "01", "000001");
    create_sub(store, "https://amf-1.example.com/cb", f1, id1);
    cJSON_Delete(f1);
    cJSON *f2 = make_tai_filter("001", "01", "000001");
    create_sub(store, "https://amf-2.example.com/cb", f2, id2);
    cJSON_Delete(f2);

    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-nf-003", NSSF_AVAIL_CHANGE_REPLACED, ev_tai);

    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        3, enq, "3 matching subs 가 정확히 3 row 로 fan-out 되지 않음");

    drained_row_t rows[8];
    int n = drain_rows(retry, rows, 8);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, n, "enqueue 된 row 가 3 개가 아님");

    /* each row carries its OWN distinct UUID + callbackUri (no cross-talk). */
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            TEST_ASSERT_FALSE_MESSAGE(
                strcmp(rows[i].sub_id, rows[j].sub_id) == 0,
                "두 row 가 같은 subscription UUID 를 공유 (fan-out cross-talk)");
            TEST_ASSERT_FALSE_MESSAGE(
                strcmp(rows[i].callback, rows[j].callback) == 0,
                "두 row 가 같은 callbackUri 를 공유 (fan-out cross-talk)");
        }
    }
    /* and every persisted sub UUID is represented exactly once. */
    const char *want[3] = {id0, id1, id2};
    for (int w = 0; w < 3; w++) {
        int hits = 0;
        for (int i = 0; i < n; i++) {
            if (strcmp(rows[i].sub_id, want[w]) == 0) {
                hits++;
            }
        }
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            1, hits, "각 matching sub 의 UUID 가 정확히 1 row 로 나타나지 않음");
    }

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * 4. callbackUri B1 gate → an un-postable callbackUri is NEVER enqueued
 * =================================================================== */

static void test_callback_gate_unpostable_not_enqueued(void)
{
    /* Each hostile callbackUri is rejected by the shared B1 gate at enqueue time:
     * http:// non-loopback, userinfo('@'), fragment('#'). NONE may enqueue a row. */
    const char *hostile[] = {
        "http://amf.example.com/cb",              /* plaintext, non-loopback. */
        "https://user:pass@evil.example.com/cb",  /* userinfo '@'. */
        "https://amf.example.com/cb#frag",        /* fragment '#'. */
    };

    for (size_t i = 0; i < sizeof(hostile) / sizeof(hostile[0]); i++) {
        nssf_subscription_store_t *store =
            nssf_subscription_store_new_inmemory();
        TEST_ASSERT_NOT_NULL(store);
        nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
        TEST_ASSERT_NOT_NULL(retry);

        char sub_id[37];
        cJSON *filter = make_broad_filter(); /* matches the event. */
        create_sub(store, hostile[i], filter, sub_id);
        cJSON_Delete(filter);

        cJSON *ev_tai = make_tai("001", "01", "000001");
        nssf_availability_change_event_t ev =
            make_event("amf-nf-004", NSSF_AVAIL_CHANGE_REPLACED, ev_tai);

        int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, enq,
            "un-postable callbackUri (B1 gate fail) 가 fan-out 에서 enqueue 됨");

        /* queue is empty — no un-postable row was persisted. */
        nssf_retry_item_t none;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, nssf_retry_store_dequeue(retry, &none),
            "B1 gate 가 막은 callbackUri 인데 retry row 가 enqueue 됨");
        nssf_retry_item_clear(&none);

        cJSON_Delete(ev_tai);
        nssf_retry_store_free(retry);
        nssf_subscription_store_free(store);
    }
}

/* A matching broad subscriber alongside a hostile-callback subscriber → exactly
 * ONE row (the postable one); the hostile sub is skipped, scan continues. */
static void test_callback_gate_skips_only_unpostable(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    char id_good[37], id_bad[37];
    cJSON *fg = make_broad_filter();
    create_sub(store, "https://amf-good.example.com/cb", fg, id_good);
    cJSON_Delete(fg);
    cJSON *fb = make_broad_filter();
    create_sub(store, "http://amf-bad.example.com/cb", fb, id_bad); /* plaintext */
    cJSON_Delete(fb);

    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-nf-005", NSSF_AVAIL_CHANGE_REPLACED, ev_tai);

    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, enq, "postable sub 1 개만 enqueue (hostile sub skip) 되지 않음");

    drained_row_t rows[8];
    int n = drain_rows(retry, rows, 8);
    TEST_ASSERT_EQUAL_INT(1, n);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        id_good, rows[0].sub_id, "skip 되어야 할 hostile sub 가 enqueue 됨");

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * 5. DELETED change → matched sub tombstoned (store_get → NOT_FOUND)
 * =================================================================== */

static void test_deleted_change_tombstones_matched_sub(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    char sub_id[37];
    cJSON *filter = make_broad_filter();
    create_sub(store, "https://amf.example.com/cb", filter, sub_id);
    cJSON_Delete(filter);

    /* the sub is live BEFORE the DELETED change. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_get(store, sub_id, &rec),
        "DELETED 전에 sub 가 live 로 읽히지 않음");
    nssf_subscription_record_clear(&rec);

    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-nf-006", NSSF_AVAIL_CHANGE_DELETED, ev_tai);

    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, enq, "DELETED fan-out 이 matched sub 에 대해 1 row 를 enqueue 하지 않음");

    /* per Acceptance #4: the matched sub is now tombstoned → store_get NOT_FOUND. */
    nssf_subscription_record_t gone;
    memset(&gone, 0, sizeof(gone));
    nssf_sub_result_e gr = nssf_subscription_store_get(store, sub_id, &gone);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_NOT_FOUND, gr,
        "DELETED change 후 matched sub 가 tombstone 되지 않음 (store_get != NOT_FOUND)");
    nssf_subscription_record_clear(&gone);

    /* a second DELETED fan-out finds no live match (the sub is gone) → 0 rows. */
    int enq2 = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, enq2, "tombstoned sub 가 후속 fan-out 에서 다시 매치됨");

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * 6. call-driven / single-dispatch — ONE publish step drives at most ONE
 *    dispatch (bounded), NOT an unbounded drain of the queue
 * =================================================================== */

/*
 * Reproduce the trampoline's single-shot contract: fanout_change enqueues N due
 * rows (here 2, from 2 matches), then ONE publish step performs EXACTLY ONE
 * dispatch_pending call (the documented at-most-one-row-per-publish contract on
 * main.c's fanout_publish). One publish must NOT drain both rows — proven by the
 * second due row remaining queued after the single dispatch.
 */
static void test_single_dispatch_per_publish_no_drain(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    char id0[37], id1[37];
    cJSON *f0 = make_broad_filter();
    create_sub(store, "https://amf-0.example.com/cb", f0, id0);
    cJSON_Delete(f0);
    cJSON *f1 = make_broad_filter();
    create_sub(store, "https://amf-1.example.com/cb", f1, id1);
    cJSON_Delete(f1);

    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-nf-007", NSSF_AVAIL_CHANGE_REPLACED, ev_tai);

    /* publish step 1 — store-layer match + enqueue: 2 due rows. */
    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, enq, "2 matches 가 2 due row 로 enqueue 안 됨");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(retry, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    /* publish step 2 — EXACTLY ONE call-driven dispatch (the trampoline's single
     * shot). It dispatches AT MOST one row; it does NOT loop to drain the queue. */
    nssf_dispatch_result_e dr =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_SENT, dr, "single dispatch 가 한 row 를 SENT 하지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls,
        "ONE publish 가 1 POST 초과를 emit (single-shot 위반 — busy drain loop)");

    /* the SECOND due row is STILL queued — one publish did NOT drain it (no loop). */
    nssf_retry_item_t leftover;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, nssf_retry_store_dequeue(retry, &leftover),
        "두 번째 due row 가 single dispatch 로 drain 됨 (unbounded drain 의심)");
    nssf_retry_item_clear(&leftover);

    cJSON_Delete(ev_tai);
    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * DOCX-GAP-001 self-notification suppression (5 mandatory cases)
 *
 * The NSSF must NOT echo a change notification back to the AMF that caused the
 * availability update. The suppression predicate (subscription_store.c):
 *   suppress ⇔ ev->nf_id != NULL && subscriber_amf_id != NULL &&
 *              strcmp(subscriber_amf_id, ev->nf_id) == 0
 * Suppression is ENQUEUE-SKIP ONLY — never a match/cleanup skip. amfSetId-only
 * subscribers (amf_id == NULL) are NEVER suppressed. The change event's nf_id is
 * the originating AMF identity (make_event's first arg).
 * =================================================================== */

/* GAP-001 #1 — same amf_id → SUPPRESSED. A broad/matching subscriber whose
 * persisted amf_id == the change event's nf_id is the originator → its
 * notification enqueue is SKIPPED (0 rows / 0 POSTs for that sub). */
static void test_gap001_same_amf_id_suppressed(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    /* the subscriber IS the AMF that will cause the change (same nfId). */
    const char *self_amf = "amf-self-001";
    char sub_id[37];
    cJSON *filter = make_broad_filter(); /* broad → matches the event. */
    create_sub_id(store, "https://amf.example.com/cb", filter, self_amf, NULL,
                  sub_id);
    cJSON_Delete(filter);

    /* a REPLACED change ORIGINATING from that same AMF (ev->nf_id == self_amf). */
    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event(self_amf, NSSF_AVAIL_CHANGE_REPLACED, ev_tai);

    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, enq, "self-AMF (same amf_id) 가 suppress 되지 않고 enqueue 됨 (echo)");

    /* the queue is EMPTY — no row, hence no POST can ever fire for the self-AMF. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(retry, &none),
        "self-suppress 됐어야 하는데 retry row 가 enqueue 됨 (mock POST count != 0)");
    nssf_retry_item_clear(&none);

    /* the subscription is STILL live (suppress is enqueue-skip, NOT a delete). */
    nssf_subscription_record_t rec;
    memset(&rec, 0, sizeof(rec));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_get(store, sub_id, &rec),
        "REPLACED self-suppress 가 sub 를 잘못 제거함 (enqueue-skip 이어야 함)");
    nssf_subscription_record_clear(&rec);

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* GAP-001 #2 — different amf_id → NOTIFIED. A subscriber whose amf_id differs
 * from the change originator's nfId must be enqueued and POSTed (no suppression
 * of foreign AMFs). */
static void test_gap001_different_amf_id_notified(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    /* subscriber is a DIFFERENT AMF than the change originator. */
    char sub_id[37];
    cJSON *filter = make_broad_filter();
    const char *cb = "https://amf-other.example.com/cb";
    create_sub_id(store, cb, filter, "amf-subscriber-A", NULL, sub_id);
    cJSON_Delete(filter);

    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-originator-B", NSSF_AVAIL_CHANGE_REPLACED, ev_tai);

    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, enq, "foreign AMF (different amf_id) 가 enqueue 되지 않음 (over-suppress)");

    /* the enqueued row carries the REAL sub UUID + callbackUri; requeue it so the
     * dispatcher can drive the actual POST (same pattern as the broad-sub case). */
    nssf_retry_item_t item;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, nssf_retry_store_dequeue(retry, &item),
        "foreign-AMF fan-out row 가 큐에 enqueue 되지 않음");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(sub_id, item.subscription_id,
                                     "row 가 real subscription UUID 를 담지 않음");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(cb, item.callback_uri,
                                     "row 가 sub 의 callbackUri 를 담지 않음");
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_requeue(retry, item.id));
    nssf_retry_item_clear(&item);

    /* a single dispatch fires exactly one outbound POST (the wired cascade tail). */
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(retry, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);
    nssf_dispatch_result_e dr =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_SENT, dr,
                                  "foreign-AMF fan-out row 의 dispatch 가 SENT 아님");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls, "different amf_id 인데 POST 가 1 회 발사되지 않음");
    TEST_ASSERT_EQUAL_STRING(cb, g_amf.last_uri);

    cJSON_Delete(ev_tai);
    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* GAP-001 #3 — explicit filter subtree + amf_id persisted INDEPENDENT of the
 * filter. A subscription created WITH a tai filter AND an amf_id must persist the
 * amf_id (store_get round-trip), and self-suppression still applies. Guards the
 * "explicit-filter under-suppression" risk: identity is a SEPARATE truth from
 * the filter JSON. */
static void test_gap001_explicit_filter_amf_id_persisted(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    const char *self_amf = "amf-explicit-filter-001";
    char sub_id[37];
    /* an EXPLICIT tai filter subtree (not broad) AND a subscriber identity. */
    cJSON *filter = make_tai_filter("001", "01", "000001");
    create_sub_id(store, "https://amf.example.com/cb", filter, self_amf, NULL,
                  sub_id);
    cJSON_Delete(filter);

    /* identity is persisted independent of (and alongside) the filter subtree. */
    nssf_subscription_record_t rec;
    memset(&rec, 0, sizeof(rec));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_get(store, sub_id, &rec),
        "explicit-filter sub 의 store_get 실패");
    TEST_ASSERT_NOT_NULL_MESSAGE(
        rec.amf_id, "filter subtree 가 있으면 amf_id 가 persist 안 됨 (under-suppress)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        self_amf, rec.amf_id, "persisted amf_id 가 입력과 다름");
    /* the filter subtree itself also round-trips (matching truth unchanged). */
    TEST_ASSERT_NOT_NULL_MESSAGE(rec.filter, "explicit filter 가 persist 안 됨");
    TEST_ASSERT_NOT_NULL_MESSAGE(
        cJSON_GetObjectItemCaseSensitive(rec.filter, "tai"),
        "filter 의 tai subtree 미보존");
    nssf_subscription_record_clear(&rec);

    /* and self-suppression STILL applies on a matching event from that AMF. */
    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event(self_amf, NSSF_AVAIL_CHANGE_PATCHED, ev_tai);
    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, enq,
        "explicit-filter self-AMF 가 suppress 안 됨 (identity 가 filter 와 묶임)");
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(retry, &none),
        "explicit-filter self-suppress 인데 row 가 enqueue 됨");
    nssf_retry_item_clear(&none);

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* GAP-001 #4 — amfSetId-only → NOT over-suppressed. A subscriber that carries
 * only an amf_set_id (amf_id == NULL) can never be confirmed as the originating
 * AMF, so it must STILL be notified even when the change is in that set. Guards
 * the "set-level over-suppression" risk. */
static void test_gap001_amf_set_id_only_not_suppressed(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    /* amf_set_id set, amf_id NULL → identity insufficient to confirm same-AMF. */
    char sub_id[37];
    cJSON *filter = make_broad_filter();
    const char *cb = "https://amf-set.example.com/cb";
    create_sub_id(store, cb, filter, NULL, "amf-set-7", sub_id);
    cJSON_Delete(filter);

    /* persisted: amf_set_id present, amf_id NULL. */
    nssf_subscription_record_t rec;
    memset(&rec, 0, sizeof(rec));
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(store, sub_id, &rec));
    TEST_ASSERT_NULL_MESSAGE(rec.amf_id, "amfSetId-only 인데 amf_id 가 채워짐");
    TEST_ASSERT_NOT_NULL_MESSAGE(rec.amf_set_id, "amf_set_id 가 persist 안 됨");
    TEST_ASSERT_EQUAL_STRING("amf-set-7", rec.amf_set_id);
    nssf_subscription_record_clear(&rec);

    /* event originates from SOME AMF (its set could include the subscriber) —
     * with amf_id NULL the predicate cannot suppress → STILL notified. */
    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-in-set-7", NSSF_AVAIL_CHANGE_REPLACED, ev_tai);
    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, enq, "amfSetId-only subscriber 가 과도하게 suppress 됨 (set-wide drop)");

    drained_row_t rows[4];
    int n = drain_rows(retry, rows, 4);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, n, "amfSetId-only → 정확히 1 row notify 아님");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        sub_id, rows[0].sub_id, "notify row 가 해당 sub UUID 가 아님");
    TEST_ASSERT_EQUAL_STRING(cb, rows[0].callback);

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* GAP-001 #5 — DELETE on self-matched → tombstone RETAINED (the critical proof:
 * suppress = enqueue-skip, NOT cleanup-skip). A self-matching subscriber
 * (amf_id == ev->nf_id) on a DELETED change has its enqueue SUPPRESSED, yet the
 * subscription MUST still be tombstoned (store_get → NOT_FOUND). Guards the
 * "suppress mis-implemented as match/cleanup-skip" risk. */
static void test_gap001_delete_self_matched_tombstone_retained(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    const char *self_amf = "amf-self-delete-001";
    char sub_id[37];
    cJSON *filter = make_broad_filter(); /* broad → matches the DELETED event. */
    create_sub_id(store, "https://amf.example.com/cb", filter, self_amf, NULL,
                  sub_id);
    cJSON_Delete(filter);

    /* live before the change. */
    nssf_subscription_record_t live;
    memset(&live, 0, sizeof(live));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_get(store, sub_id, &live),
        "DELETED 전에 self-AMF sub 가 live 로 읽히지 않음");
    nssf_subscription_record_clear(&live);

    /* a DELETED change ORIGINATING from that same AMF → enqueue suppressed. */
    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event(self_amf, NSSF_AVAIL_CHANGE_DELETED, ev_tai);
    int enq = nssf_subscription_store_fanout_change(store, retry, &ev);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, enq,
        "DELETED self-AMF 의 enqueue 가 suppress 되지 않음 (echo on delete)");

    /* queue empty — the self-notification was NOT enqueued. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(retry, &none),
        "DELETED self-suppress 인데 retry row 가 enqueue 됨");
    nssf_retry_item_clear(&none);

    /* CRITICAL: the cleanup MUST have run anyway — the sub is now tombstoned.
     * This proves suppression is enqueue-skip, NOT match/cleanup-skip. */
    nssf_subscription_record_t gone;
    memset(&gone, 0, sizeof(gone));
    nssf_sub_result_e gr = nssf_subscription_store_get(store, sub_id, &gone);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_NOT_FOUND, gr,
        "self-suppress 가 tombstone(cleanup) 까지 skip 함 (enqueue-skip 위반)");
    nssf_subscription_record_clear(&gone);

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

/* ===================================================================
 * defensive: NULL args → -1, no-match event → 0 (the documented contract)
 * =================================================================== */

static void test_null_and_nomatch_contract(void)
{
    nssf_subscription_store_t *store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);

    cJSON *ev_tai = make_tai("001", "01", "000001");
    nssf_availability_change_event_t ev =
        make_event("amf-nf-008", NSSF_AVAIL_CHANGE_REPLACED, ev_tai);

    /* NULL store / retry / ev → -1. */
    TEST_ASSERT_EQUAL_INT(-1,
                          nssf_subscription_store_fanout_change(NULL, retry, &ev));
    TEST_ASSERT_EQUAL_INT(-1,
                          nssf_subscription_store_fanout_change(store, NULL, &ev));
    TEST_ASSERT_EQUAL_INT(
        -1, nssf_subscription_store_fanout_change(store, retry, NULL));

    /* empty store → no live subscription → 0 rows. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_subscription_store_fanout_change(store, retry, &ev),
        "subscriber 없는 store 의 fan-out 이 0 이 아님");

    /* a TAI-scoped sub vs a NO-tai event → tai-constrained filter cannot match. */
    char id[37];
    cJSON *f = make_tai_filter("001", "01", "000001");
    create_sub(store, "https://amf.example.com/cb", f, id);
    cJSON_Delete(f);
    nssf_availability_change_event_t ev_notai =
        make_event("amf-nf-008b", NSSF_AVAIL_CHANGE_REPLACED, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_subscription_store_fanout_change(store, retry, &ev_notai),
        "tai-constrained filter 가 no-tai event 에 매치됨");

    cJSON_Delete(ev_tai);
    nssf_retry_store_free(retry);
    nssf_subscription_store_free(store);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_broad_subscriber_one_row_one_post);
    RUN_TEST(test_tai_scoped_match_vs_nonmatch);
    RUN_TEST(test_multi_subscriber_distinct_rows);
    RUN_TEST(test_callback_gate_unpostable_not_enqueued);
    RUN_TEST(test_callback_gate_skips_only_unpostable);
    RUN_TEST(test_deleted_change_tombstones_matched_sub);
    RUN_TEST(test_single_dispatch_per_publish_no_drain);
    /* DOCX-GAP-001 self-notification suppression (5 mandatory cases). */
    RUN_TEST(test_gap001_same_amf_id_suppressed);
    RUN_TEST(test_gap001_different_amf_id_notified);
    RUN_TEST(test_gap001_explicit_filter_amf_id_persisted);
    RUN_TEST(test_gap001_amf_set_id_only_not_suppressed);
    RUN_TEST(test_gap001_delete_self_matched_tombstone_retained);
    RUN_TEST(test_null_and_nomatch_contract);
    return UNITY_END();
}
