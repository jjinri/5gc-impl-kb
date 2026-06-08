/*
 * test_notification_dispatcher_queue.c — WI-notification-dispatcher unit suite.
 *
 * NotificationDispatcher retry-queue STORE seam + PRODUCER seam, driven ENTIRELY
 * through the in-memory store backend (nssf_retry_store_new_inmemory) — no DB, no
 * libpq, no network. So the enqueue-on-commit acceptance + dequeue ordering /
 * SKIP-LOCKED claim / complete-vs-requeue semantics run UNCONDITIONALLY in CI.
 *
 * The required_check core (proves: a committed mutation → NotificationDispatcher
 * retry_queue 에 row enqueue) is the enqueue-on-commit SEAM, NOT the fan-out:
 *   - test_enqueue_seam_one_row drives the public enqueue API
 *     (nssf_retry_store_enqueue) with a REAL subscription UUID + callbackUri and
 *     asserts EXACTLY one row lands in the in-memory store (dequeue==1 carrying
 *     that subscription_id / callback_uri, then dequeue==0). This is the
 *     "Put/Patch committed mutation → retry_queue 에 row enqueue" acceptance: the
 *     enqueue-on-commit seam the AvailabilityEngine will drive once the Phase-3
 *     SubscriptionStore fan-out resolves changed nfId/tai → affected subscriptions.
 *   - test_change_publish_takes_not_ready_path asserts the PRODUCER seam
 *     (nssf_notification_dispatcher_change_publish) currently takes the explicit
 *     NOT-READY / log path (B3): it does NOT fabricate a subscription_id from the
 *     changed nfId, so NO retry_queue row is enqueued until the Phase-3 fan-out
 *     lands. WHY: writing the nfId into retry_queue.subscription_id (a real UUID/FK
 *     column) fabricates a non-existent subscription and produces a poison row with
 *     a NULL callback that can never deliver.
 *
 * Store-seam coverage (header §store):
 *   - dequeue returns rows lowest-id-first (FIFO insertion order = the
 *     ORDER BY ... id ASC the libpq backend uses) and a CLAIMED row is hidden from
 *     a second dequeue (FOR UPDATE SKIP LOCKED mirror via the `claimed` flag).
 *   - complete() removes the claimed row terminally; requeue() re-arms it
 *     (claimed=false, attempt_count++) so the next dequeue hands it back.
 *   - enqueue rejects NULL store / subscription_id / payload (explicit -1, NOT a
 *     silent swallow — B3/PG error-path contract on the seam side).
 *
 * The PG-FK error path (an invalid/missing subscription UUID rejected by the
 * retry_queue.subscription_id FK) runs only under the PostgreSQL gate
 * (POSTGRES_* + NSSF_SCHEMA_SQL_PATH env), mirroring test_availability_repo_pg.c;
 * it is SKIP PASS otherwise.
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

/* A real subscription UUID + callbackUri — the only shape that yields a row. */
static const char *SUB_UUID = "11111111-2222-3333-4444-555555555555";
static const char *CB_URI = "https://amf.example.com/n1n2/callback";

/* ===================================================================
 * 1. enqueue-on-commit SEAM — the required_check core
 *    (committed mutation → retry_queue row, via the public enqueue API)
 * =================================================================== */

static void test_enqueue_seam_one_row(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL_MESSAGE(store, "in-memory retry store 생성 실패");

    /* The enqueue-on-commit seam: a REAL subscription UUID AND a callbackUri are
     * both present (the in-memory test backend supplies them, standing in for the
     * Phase-3 SubscriptionStore fan-out output). This is the slice deliverable —
     * "Put/Patch committed mutation → retry_queue 에 row enqueue". */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0,
        nssf_retry_store_enqueue(store, SUB_UUID, CB_URI,
                                 "{\"changeType\":\"REPLACED\"}",
                                 "corr-fixed-0001"),
        "enqueue seam 가 commit (0) 반환 실패");

    /* Exactly one row is now pending: first dequeue claims it, second finds none. */
    nssf_retry_item_t item;
    int dq1 = nssf_retry_store_dequeue(store, &item);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, dq1, "enqueue 후 첫 dequeue 가 row 반환 실패");
    TEST_ASSERT_NOT_NULL(item.id);
    /* the enqueued row carries the REAL subscription UUID (not a fabricated nfId). */
    TEST_ASSERT_EQUAL_STRING(SUB_UUID, item.subscription_id);
    /* the resolved callbackUri rides in the envelope and is recovered on dequeue. */
    TEST_ASSERT_EQUAL_STRING(CB_URI, item.callback_uri);
    /* payload carries the serialized change body (REPLACED). */
    TEST_ASSERT_NOT_NULL(item.payload_json);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(item.payload_json, "REPLACED"),
                                 "payload 가 changeType 미포함");
    nssf_retry_store_complete(store, item.id);
    nssf_retry_item_clear(&item);

    nssf_retry_item_t item2;
    int dq2 = nssf_retry_store_dequeue(store, &item2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, dq2, "enqueue 가 1개 초과 row 생성 (정확히 1 row 이어야)");
    nssf_retry_item_clear(&item2);

    nssf_retry_store_free(store);
}

/* B3 — the PRODUCER seam (change_publish) takes the NOT-READY path: it does NOT
 * cast the changed nfId into retry_queue.subscription_id, so NO row is enqueued
 * until the Phase-3 SubscriptionStore fan-out exists. */
static void test_change_publish_takes_not_ready_path(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    /* The dispatcher is the PRODUCER ctx the engine seam carries; no oauth handle
     * is needed for the producer path (it never POSTs here). */
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);

    cJSON *tai = make_tai("001", "01", "000001");
    cJSON *after = cJSON_CreateArray(); /* authorized snssais snapshot. */
    nssf_availability_change_event_t ev = make_event(
        "nf-11111111-2222-3333-4444-555555555555", tai,
        NSSF_AVAIL_CHANGE_REPLACED, after);

    /* PRODUCER seam — signature-compatible with nssf_availability_change_publish_fn
     * (ctx == the dispatcher). Phase 2 NOT-READY path: no fan-out, no enqueue. */
    nssf_notification_dispatcher_change_publish(disp, &ev);

    /* The queue MUST still be empty — change_publish enqueued NOTHING (no nfId
     * fabricated into subscription_id, no poison row). */
    nssf_retry_item_t none;
    int dq = nssf_retry_store_dequeue(store, &none);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, dq,
        "change_publish 가 not-ready path 가 아님 — row 가 enqueue 됨 "
        "(nfId-as-subscription_id 금지)");
    nssf_retry_item_clear(&none);

    /* A PATCHED event must likewise enqueue nothing (the Patch half). */
    nssf_availability_change_event_t ev2 =
        make_event("nf-patched-0001", tai, NSSF_AVAIL_CHANGE_PATCHED, NULL);
    nssf_notification_dispatcher_change_publish(disp, &ev2);
    nssf_retry_item_t none2;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none2),
        "PATCHED change_publish 가 row enqueue (not-ready path 위반)");
    nssf_retry_item_clear(&none2);

    cJSON_Delete(tai);
    cJSON_Delete(after);
    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* The enqueue seam REJECTS NULL store / subscription_id / payload with an
 * explicit -1 — an invalid enqueue must NOT be a silent swallow (B3/PG
 * error-path contract on the seam side; the FK-violation path runs under PG). */
static void test_enqueue_rejects_invalid_args(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    TEST_ASSERT_EQUAL_INT_MESSAGE(
        -1, nssf_retry_store_enqueue(NULL, SUB_UUID, CB_URI, "{}", NULL),
        "NULL store enqueue 가 -1 반환 실패 (silent swallow 금지)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        -1, nssf_retry_store_enqueue(store, NULL, CB_URI, "{}", NULL),
        "NULL subscription_id enqueue 가 -1 반환 실패");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        -1, nssf_retry_store_enqueue(store, SUB_UUID, CB_URI, NULL, NULL),
        "NULL payload enqueue 가 -1 반환 실패");

    /* nothing was enqueued by the rejected calls. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_dequeue(store, &none));
    nssf_retry_item_clear(&none);

    nssf_retry_store_free(store);
}

/* ===================================================================
 * 1b. EYEBALL-STOP — enqueue-time STRUCTURAL callback_uri hard-reject
 *
 * PR-phase2-dispatcher-enqueue-hardening 의 핵심. nssf_retry_store_enqueue 가
 * structurally un-dispatchable 한 callback_uri 를 backend 진입 BEFORE 에 -1 로
 * hard-reject 한다 (permissive structural gate, allow_insecure_loopback=true).
 * 이것은 dispatch-time https-only gate + OAuth2 fail-closed 앞단의 추가
 * defense-in-depth 계층 — bad URL 은 애초에 row 가 되지 못한다.
 *
 * REJECT (-1, no row): NULL/empty, userinfo('@'), fragment('#'),
 *   empty-authority, non-loopback plaintext http, un-dispatchable scheme.
 * ACCEPT (0, row lands): production https, 그리고 test loopback http
 *   (127.0.0.1 / [::1] / localhost) — 둘 다 admit 해야 test path 가 생존.
 *
 * 핵심 invariant — reject 된 callback_uri 는 NEVER dispatchable row 가 된다:
 * 매 -1 직후 dequeue 가 0 (no due row) 이어야 한다 (fail-closed/no-row).
 * =================================================================== */

/* helper — enqueue 가 -1 reject 이고 그 직후 큐에 due row 가 없음을 단언한다.
 * 매 케이스마다 "rejected callback_uri 는 dispatchable row 가 되지 않는다"
 * 의 fail-closed invariant 를 함께 검증한다 (no-row invariant). */
static void assert_enqueue_rejected_no_row(nssf_retry_store_t *store,
                                           const char *callback_uri,
                                           const char *label)
{
    char msg[256];
    snprintf(msg, sizeof(msg),
             "enqueue 가 [%s] 를 -1 로 reject 하지 않음 (structural gate)", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        -1,
        nssf_retry_store_enqueue(store, SUB_UUID, callback_uri,
                                 "{\"changeType\":\"REPLACED\"}", "corr-rej"),
        msg);

    /* fail-closed/no-row invariant — rejected URI 는 due row 가 되지 않는다. */
    nssf_retry_item_t none;
    snprintf(msg, sizeof(msg),
             "[%s] reject 후 dequeue 가 due row 반환 — rejected URI 가 "
             "dispatchable row 가 됨 (fail-closed/no-row 위반)", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none), msg);
    nssf_retry_item_clear(&none);
}

/* helper — enqueue 가 0 accept 이고 그 직후 dequeue 가 정확히 해당 row 를
 * 돌려줌을 단언한다 (accepted URI 는 dispatchable row 가 된다). */
static void assert_enqueue_accepted_one_row(nssf_retry_store_t *store,
                                            const char *callback_uri,
                                            const char *label)
{
    char msg[256];
    snprintf(msg, sizeof(msg),
             "enqueue 가 [%s] 를 accept (0) 하지 않음 — production/loopback "
             "path 가 막힘", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0,
        nssf_retry_store_enqueue(store, SUB_UUID, callback_uri,
                                 "{\"changeType\":\"REPLACED\"}", "corr-acc"),
        msg);

    nssf_retry_item_t item;
    snprintf(msg, sizeof(msg), "[%s] accept 후 dequeue 가 row 반환 실패", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, nssf_retry_store_dequeue(store, &item), msg);
    snprintf(msg, sizeof(msg), "[%s] dequeue row 의 callback_uri 불일치", label);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(callback_uri, item.callback_uri, msg);
    nssf_retry_store_complete(store, item.id);
    nssf_retry_item_clear(&item);
}

/* REJECT set (cases 1~7,10) — structurally un-dispatchable callback_uri 는
 * enqueue 에서 -1, 그리고 row 가 되지 않는다 (no-row invariant). 하나의 store
 * 를 공유해 매 reject 직후 큐가 비어있음(0)을 확인한다. */
static void test_enqueue_rejects_structural_callback_uri(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    /* 1. NULL callback_uri → -1, no row. */
    assert_enqueue_rejected_no_row(store, NULL, "NULL");
    /* 2. empty "" → -1. */
    assert_enqueue_rejected_no_row(store, "", "empty");
    /* 3. userinfo('@') — credential-in-URL / host-confusion. */
    assert_enqueue_rejected_no_row(
        store, "https://user:pass@amf.example.com/cb", "userinfo@");
    /* 4. fragment('#') — never sent to server, a config/data error. */
    assert_enqueue_rejected_no_row(
        store, "https://amf.example.com/cb#frag", "fragment#");
    /* 5. empty authority — nothing between :// and the path. */
    assert_enqueue_rejected_no_row(store, "https:///cb", "empty-authority(/cb)");
    assert_enqueue_rejected_no_row(store, "https://", "empty-authority(bare)");
    /* 6. non-loopback plaintext http — no ctor dispatches it (loopback only). */
    assert_enqueue_rejected_no_row(
        store, "http://amf.example.com/cb", "non-loopback-http");
    /* 7. bad / un-dispatchable scheme. */
    assert_enqueue_rejected_no_row(store, "ftp://amf.example.com/cb", "ftp://");
    assert_enqueue_rejected_no_row(store, "garbage", "garbage(no-scheme)");
    assert_enqueue_rejected_no_row(store, "://nohost", "://nohost");

    /* 10. (cumulative) every reject above left the queue empty — re-confirm the
     * store NEVER accumulated a dispatchable poison row across the whole set. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none),
        "rejected URI 들 이후 큐에 dispatchable row 가 남음 (no-row invariant 위반)");
    nssf_retry_item_clear(&none);

    nssf_retry_store_free(store);
}

/* ACCEPT set (cases 8,9) — production https AND the test loopback-http targets
 * MUST survive enqueue (each becomes exactly one dispatchable row). The store
 * is ctor-agnostic, so the permissive gate admits BOTH the https production path
 * and the http://127.0.0.1|[::1]|localhost test path. */
static void test_enqueue_accepts_https_and_loopback(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    /* 8. production https. */
    assert_enqueue_accepted_one_row(
        store, "https://amf.example.com/cb", "https-production");
    /* 9. test loopback http — all three loopback host forms must pass enqueue. */
    assert_enqueue_accepted_one_row(
        store, "http://127.0.0.1:8080/cb", "http-loopback-127");
    assert_enqueue_accepted_one_row(store, "http://[::1]/cb", "http-loopback-v6");
    assert_enqueue_accepted_one_row(
        store, "http://localhost/cb", "http-loopback-localhost");

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

/* RETARGETED (Phase 4 fail-closed bounded retry) — requeue() SEMANTICS changed.
 * The 2-arg signature is retained, but re-arm now applies BACKOFF: attempt_count
 * is bumped AND next_attempt_at is pushed to `now + backoff(attempt_count) +
 * jitter` (status stays pending). Because next_attempt_at moves into the FUTURE,
 * an IMMEDIATE dequeue after requeue returns 0 (the row is NOT yet due) — this is
 * the call-driven, no-in-line-retry property. (Old Phase 1~3 semantics re-armed to
 * NOW() so an immediate dequeue handed the row straight back; that is no longer
 * true and is precisely what the resilience slice changed.) We assert the NEW
 * behaviour from the store seam: requeue bumps attempt_count to 1, and the row is
 * not-due-immediately. We do NOT sleep here — the backoff floor (base=200ms) makes
 * "not due now" deterministic; the due-after-backoff delivery is covered by the
 * integration suite's smallest-window case. */
static void test_requeue_rearms_with_backoff_not_due_immediately(void)
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

    /* requeue re-arms with BACKOFF (next_attempt_at pushed to the future) and bumps
     * attempt_count. The row is now NOT due — backoff has not elapsed. */
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_requeue(store, first.id));
    nssf_retry_item_clear(&first);

    /* NEW SEMANTICS — an IMMEDIATE dequeue returns 0: the re-armed row's backoff
     * (>= base 200ms) has not elapsed, so it is not yet due (no in-line retry). */
    nssf_retry_item_t not_due;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &not_due),
        "requeue 직후 dequeue 가 row 를 반환 — backoff 미적용 (in-line retry 금지 위반)");
    nssf_retry_item_clear(&not_due);

    nssf_retry_store_free(store);
}

/* NEW (#3 dead_letter terminal) — after dead_letter(id) the row is RETAINED (not
 * deleted) but is NEVER re-claimed: dequeue returns 0 even though the row's
 * next_attempt_at is in the past (a fresh row enqueues due-now). dead_letter rows
 * are structurally unclaimable (the claim filters status='pending'). No sleep —
 * the row is due-now and would dequeue but for the dead_letter status. */
static void test_dead_letter_retains_row_but_never_reclaimed(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    TEST_ASSERT_EQUAL_INT(
        0, nssf_retry_store_enqueue(store, "sub-dead",
                                    "https://amf.example/cb", "{}", NULL));

    /* claim it (due now), then move it to the terminal dead_letter status. */
    nssf_retry_item_t item;
    TEST_ASSERT_EQUAL_INT(1, nssf_retry_store_dequeue(store, &item));
    TEST_ASSERT_EQUAL_INT(0, nssf_retry_store_dead_letter(store, item.id));
    nssf_retry_item_clear(&item);

    /* the row is dead-lettered: a subsequent dequeue NEVER re-claims it, even
     * though its next_attempt_at (enqueued due-now) is in the past. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none),
        "dead_letter 후 row 가 재배포됨 — dead_letter 는 unclaimable 이어야");
    nssf_retry_item_clear(&none);

    /* a SECOND dequeue likewise finds nothing — the row stays terminally parked
     * (retained for audit, never re-dealt). */
    nssf_retry_item_t none2;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none2),
        "dead_letter row 가 반복 dequeue 에서 재등장 (terminal 위반)");
    nssf_retry_item_clear(&none2);

    nssf_retry_store_free(store);
}

/* NEW (#1 backoff schedule) — bound nssf_retry_backoff_delay_ms across
 * attempt_count 0..N with rand_unit==0 (the exact `delay==capped` points). The
 * `capped` floor is exponential (base * 2^attempt_count) doubling, monotonic
 * non-decreasing, and saturates at max_ms. base/max mirror the dispatcher's
 * NSSF_DISPATCH_BACKOFF_* defaults (200ms / 30000ms). */
static void test_backoff_schedule_capped_exponential_rand0(void)
{
    const long base = 200;
    const long max = 30000;

    /* rand_unit==0 → delay == capped exactly (the deterministic floor). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        200, nssf_retry_backoff_delay_ms(0, base, max, 0.0),
        "attempt 0 floor 가 base(200) 가 아님");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        400, nssf_retry_backoff_delay_ms(1, base, max, 0.0), "attempt 1 floor != 400");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        800, nssf_retry_backoff_delay_ms(2, base, max, 0.0), "attempt 2 floor != 800");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1600, nssf_retry_backoff_delay_ms(3, base, max, 0.0), "attempt 3 floor != 1600");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        3200, nssf_retry_backoff_delay_ms(4, base, max, 0.0), "attempt 4 floor != 3200");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        6400, nssf_retry_backoff_delay_ms(5, base, max, 0.0), "attempt 5 floor != 6400");

    /* doubling continues until the cap; 200*2^7 = 25600 (still < 30000), and the
     * next double (51200) exceeds max, so the schedule saturates at max=30000. */
    long prev = -1;
    long saw_cap = 0;
    for (int a = 0; a <= 12; ++a) {
        long capped = nssf_retry_backoff_delay_ms(a, base, max, 0.0);
        /* monotonic non-decreasing floor. */
        TEST_ASSERT_TRUE_MESSAGE(capped >= prev,
                                 "capped 가 attempt 증가에 대해 단조 비감소가 아님");
        /* never above the cap (rand_unit==0 → no jitter added). */
        TEST_ASSERT_TRUE_MESSAGE(capped <= max,
                                 "capped(rand0) 가 max_ms 를 초과");
        if (capped == max) {
            saw_cap = 1;
        }
        prev = capped;
    }
    /* the schedule DOES reach the cap within the tested range. */
    TEST_ASSERT_TRUE_MESSAGE(saw_cap,
                             "backoff 가 max_ms cap 에 도달하지 않음 (saturate 실패)");
    /* a pathologically large attempt_count is overflow-safe and pinned at the cap. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        max, nssf_retry_backoff_delay_ms(1000, base, max, 0.0),
        "거대한 attempt_count 가 cap(max) 으로 clamp 되지 않음 (overflow-safe)");
}

/* NEW (#2 jitter bound) — for rand_unit sampled across [0,1) the equal-jitter
 * delay stays in [capped, capped + capped/2): never below the capped floor (and
 * thus never below base), never at/above capped + capped/2, and never above
 * max_ms + max_ms/2. delay==capped iff rand_unit==0. */
static void test_backoff_jitter_bounds(void)
{
    const long base = 200;
    const long max = 30000;

    /* sample rand_unit across [0,1) at several attempt_counts (incl. the capped
     * region) and assert the equal-jitter bounds at every point. */
    const double units[] = {0.0,  0.0001, 0.25, 0.5, 0.75, 0.999, 0.999999};
    const int attempts[] = {0, 1, 2, 4, 7, 8, 20};
    for (size_t ai = 0; ai < sizeof(attempts) / sizeof(attempts[0]); ++ai) {
        int a = attempts[ai];
        long capped = nssf_retry_backoff_delay_ms(a, base, max, 0.0); /* floor. */
        for (size_t ui = 0; ui < sizeof(units) / sizeof(units[0]); ++ui) {
            double u = units[ui];
            long d = nssf_retry_backoff_delay_ms(a, base, max, u);
            /* LOWER edge — never below the capped floor (⇒ never below base). */
            TEST_ASSERT_TRUE_MESSAGE(d >= capped,
                                     "jitter delay 가 capped floor 미만 (하한 위반)");
            TEST_ASSERT_TRUE_MESSAGE(d >= base,
                                     "jitter delay 가 base 미만 (절대 하한 위반)");
            /* UPPER edge — strictly below capped + capped/2 (half-open jitter). */
            TEST_ASSERT_TRUE_MESSAGE(
                d < capped + (capped / 2) + 1,
                "jitter delay 가 capped + capped/2 상한 도달/초과 (상한 위반)");
            /* GLOBAL upper bound — never above max_ms + max_ms/2. */
            TEST_ASSERT_TRUE_MESSAGE(d <= max + max / 2,
                                     "jitter delay 가 max*1.5 전역 상한 초과");
            /* equality edge — delay==capped iff rand_unit==0. */
            if (u == 0.0) {
                TEST_ASSERT_EQUAL_INT_MESSAGE(
                    capped, d, "rand_unit==0 인데 delay != capped");
            } else {
                /* a positive rand_unit on a non-degenerate capped adds >=0 jitter;
                 * for capped>=2 the floor stays exact only at u==0. (capped is
                 * always >= base=200 here, so capped/2 >= 100 and jitter can be >0.) */
                TEST_ASSERT_TRUE_MESSAGE(d >= capped,
                                         "positive jitter 가 floor 아래로 내려감");
            }
        }
    }

    /* defensive clamps — out-of-range rand_unit is clamped, not propagated. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        200, nssf_retry_backoff_delay_ms(0, base, max, -1.0),
        "rand_unit<0 가 0 으로 clamp 되지 않음 (delay != capped)");
    long d_over = nssf_retry_backoff_delay_ms(0, base, max, 5.0);
    TEST_ASSERT_TRUE_MESSAGE(d_over >= 200 && d_over < 200 + 100 + 1,
                             "rand_unit>=1 가 [0,1) 로 clamp 되지 않음 (상한 위반)");
}

/* ===================================================================
 * 4. B3/PG — invalid/missing subscription rejected by the FK (PG-gated)
 *
 * The retry_queue.subscription_id is a real UUID FK; enqueuing a subscription
 * that does not exist (or one not in the FK target) must FAIL EXPLICITLY (the
 * libpq enqueue returns -1), NOT silently swallow a poison row. Runs only when a
 * real Postgres is wired (POSTGRES_* + NSSF_SCHEMA_SQL_PATH env), mirroring
 * test_availability_repo_pg.c; SKIP PASS otherwise.
 * =================================================================== */

static int pg_env_ready(const char **host, const char **port, const char **db,
                        const char **user, const char **pwd,
                        const char **schema)
{
    *host = getenv("POSTGRES_HOST");
    *port = getenv("POSTGRES_PORT");
    *db = getenv("POSTGRES_DB");
    *user = getenv("POSTGRES_USER");
    *pwd = getenv("POSTGRES_PASSWORD");
    *schema = getenv("NSSF_SCHEMA_SQL_PATH");
    return *host != NULL && *db != NULL && *user != NULL && *schema != NULL;
}

static void test_pg_enqueue_invalid_subscription_fails(void)
{
    const char *host, *port, *db, *user, *pwd, *schema;
    if (!pg_env_ready(&host, &port, &db, &user, &pwd, &schema)) {
        fprintf(stderr,
                "[SKIP] test_pg_enqueue_invalid_subscription_fails — POSTGRES_* / "
                "NSSF_SCHEMA_SQL_PATH not set. CI 에 PostgreSQL service container "
                "추가 후 활성.\n");
        TEST_PASS_MESSAGE("env not set — skipping pg FK error-path test");
        return;
    }

    /* schema.sql 적용 (test_availability_repo_pg.c 미러). */
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "PGPASSWORD='%s' psql -h '%s' -p '%s' -U '%s' -d '%s' "
             "-v ON_ERROR_STOP=1 -f '%s' >/dev/null",
             pwd != NULL ? pwd : "", host, port != NULL ? port : "5432", user,
             db, schema);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, system(cmd), "schema.sql 적용 실패");

    char conninfo[1024];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%s dbname=%s user=%s password=%s sslmode=prefer", host,
             port != NULL ? port : "5432", db, user, pwd != NULL ? pwd : "");

    char errbuf[256] = {0};
    nssf_retry_store_t *store =
        nssf_retry_store_new_pg(conninfo, errbuf, sizeof(errbuf));
    TEST_ASSERT_NOT_NULL_MESSAGE(store, errbuf);

    /* A non-castable subscription id (not a UUID) — the $1::uuid cast / FK fails →
     * the libpq enqueue MUST return -1, not silently swallow the row. */
    int rc = nssf_retry_store_enqueue(
        store, "not-a-uuid-and-no-such-subscription", CB_URI, "{}", NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        -1, rc,
        "invalid/missing subscription enqueue 가 -1 (explicit failure) 반환 실패 "
        "— silent swallow 금지 (B3/PG)");

    /* nothing landed — a subsequent dequeue finds no due row. */
    nssf_retry_item_t none;
    int dq = nssf_retry_store_dequeue(store, &none);
    TEST_ASSERT_TRUE_MESSAGE(dq == 0 || dq == -1,
                             "failed enqueue 후 poison row 가 dequeue 됨");
    nssf_retry_item_clear(&none);

    nssf_retry_store_free(store);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_enqueue_seam_one_row);
    RUN_TEST(test_change_publish_takes_not_ready_path);
    RUN_TEST(test_enqueue_rejects_invalid_args);
    RUN_TEST(test_enqueue_rejects_structural_callback_uri);
    RUN_TEST(test_enqueue_accepts_https_and_loopback);
    RUN_TEST(test_dequeue_fifo_order_and_claim_once);
    RUN_TEST(test_complete_removes_row);
    RUN_TEST(test_requeue_rearms_with_backoff_not_due_immediately);
    RUN_TEST(test_dead_letter_retains_row_but_never_reclaimed);
    RUN_TEST(test_backoff_schedule_capped_exponential_rand0);
    RUN_TEST(test_backoff_jitter_bounds);
    RUN_TEST(test_pg_enqueue_invalid_subscription_fails);
    return UNITY_END();
}
