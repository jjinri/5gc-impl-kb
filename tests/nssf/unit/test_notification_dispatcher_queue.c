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
    RUN_TEST(test_dequeue_fifo_order_and_claim_once);
    RUN_TEST(test_complete_removes_row);
    RUN_TEST(test_requeue_rearms_row);
    RUN_TEST(test_pg_enqueue_invalid_subscription_fails);
    return UNITY_END();
}
