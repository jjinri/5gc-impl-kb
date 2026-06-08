/*
 * test_notification_dispatcher_outbound.c — WI-notification-dispatcher integration
 * suite (ADR-0004 Layer A outbound POST + G-08 minimal retry).
 *
 * The CONSUMER side of the dispatcher: dequeue a retry_queue row and POST it to a
 * subscription's AMF callback URI, driven ENTIRELY through the mock TRANSPORT seam
 * (nssf_notification_dispatcher_set_transport — same pattern as
 * nssf_oauth2_outbound_set_transport) over the in-memory store. No socket, no real
 * AMF, no DB — the whole suite runs UNCONDITIONALLY in CI (registered above the
 * PostgreSQL gate).
 *
 * The mock AMF callback records each POST it receives (uri / body / the built
 * header slist + the raw correlation/traceparent values) and replies with a
 * staged HTTP status (or a forced transport error → retriable). This asserts,
 * purely from observable behavior:
 *
 *   happy path: 200 → NSSF_DISPATCH_SENT, exactly ONE POST, headers carry        — #1
 *     3gpp-Sbi-Correlation-Info + traceparent (+ Authorization: Bearer w/ oauth)
 *   Phase 4 fail-closed bounded retry (G-08, CALL-DRIVEN, no in-line retry):
 *     5xx → re-arm with backoff (DROPPED, ONE attempt); an IMMEDIATE re-dispatch   — #2
 *       is NONE (not yet due — no inline loop); after the backoff window the SECOND
 *       dispatch delivers 200 → SENT (2 transport attempts across 2 caller calls).
 *     retriable budget = TEST_DISPATCH_MAX_ATTEMPTS (5); on EXHAUSTION the row goes  — #2
 *       to terminal dead_letter, RETAINED (not deleted) but never re-claimed.
 *     transport error (TLS/conn/timeout) is retriable exactly like a 5xx.          — #5
 *   terminal 4xx (≠429): 404/400 → dead_letter terminal (ONE call, no retry,      — #3
 *     row unclaimable). 429 → retriable back-pressure (re-arm with backoff).       — #8
 *   oauth fail-closed: attach ABORT → NSSF_DISPATCH_FORBIDDEN, ZERO POSTs, and    — #4
 *     the row is TERMINAL dead_letter (NOT re-armed — no token-endpoint hot loop),
 *     so it is never re-claimed (a follow-up dequeue is empty).
 *   no-worker / call-driven: with a due row + transport installed but NO dispatch  — #6
 *     call, NO transport fires (nothing happens between caller-driven dispatches).
 *   TLS hardening is exercised THROUGH the seam: the mock cannot read curl opts,
 *     so the dispatch-status contract (a forced transport error == a TLS/conn
 *     failure → retriable) stands in for the not-directly-observable curl flags.
 *
 * REWORK round-1 regression blockers (this suite's new cases):
 *   B1: a callback URL with userinfo('@') or fragment('#') or a non-loopback      — #B1
 *     http:// scheme is HARD-REJECTED at enqueue (PR-phase2-dispatcher-enqueue-
 *     hardening) — nssf_retry_store_enqueue returns -1, NO row ever exists, so NO
 *     POST and NO bearer can ever happen. This is a STRONGER assertion than the
 *     old dispatch-time drop: the bad callbackUri is stopped at the EARLIEST layer
 *     (defense-in-depth), proven from observable behaviour (the queue is empty).
 *   B1: the dispatch-time https-only DROP layer (second defense) is STILL covered  — #B1
 *     with an ENQUEUABLE URI — a loopback http row (passes the permissive enqueue
 *     gate) dispatched through a PRODUCTION https-only dispatcher is DROPPED with
 *     no POST and no bearer.
 *   B2: a correlation id with CR/LF / control chars / over-length does NOT split   — #B2
 *     or inject the 3gpp-Sbi-Correlation-Info / traceparent header — a safe id is
 *     emitted (the outgoing header value is clean).
 *   F7: a row with a null/empty/corrupt body is quarantined — NO null-body POST.   — #F7
 *
 * OAuth2: the dispatcher attaches the bearer via nssf_oauth2_outbound_attach_bearer.
 * The oauth handle is the test-only loopback ctor + mock token-transport (no live
 * NRF). enabled+token-fail → ABORT (fail-closed); a NULL oauth handle → no bearer
 * is ever attached (the dev no-attach path).
 *
 * CTOR choice (F2 split): tests whose intent is the https-only gate use the
 * production nssf_notification_dispatcher_new; tests that just need a mock target
 * with the seam installed also use _new with an https:// URL (the gate passes the
 * URL through to the mock). nssf_notification_dispatcher_new_insecure is exercised
 * by the http-loopback case to show the relaxed gate still admits a loopback http
 * target.
 */

#define _POSIX_C_SOURCE 200809L

#include "notification_dispatcher.h"
#include "oauth2_outbound.h"
#include "unity.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "cJSON.h"

/* Mirror of the production NSSF_DISPATCH_MAX_ATTEMPTS (notification_dispatcher.c)
 * — the retry budget is NOT exported in the public header, so the test pins it as
 * a local constant. This MUST stay equal to the .c #define (ratified design = 5);
 * if nf-code changes the budget, the exhaustion test's attempt walk must be
 * updated in lockstep. We do NOT include the .c — the value is part of the
 * ratified Phase 4 design contract. */
#define TEST_DISPATCH_MAX_ATTEMPTS 5

/* ---- mock AMF callback transport (captured POST + staged reply sequence) ---- */

typedef struct {
    int calls;                 /* total transport invocations this dispatch. */
    char last_uri[256];        /* callback_uri of the last POST. */
    char last_body[512];       /* body of the last POST. */
    bool saw_correlation;      /* 3gpp-Sbi-Correlation-Info header present. */
    bool saw_traceparent;      /* W3C traceparent header present. */
    bool saw_bearer;           /* Authorization: Bearer header present. */
    char corr_value[512];      /* raw 3gpp-Sbi-Correlation-Info VALUE (B2). */
    char traceparent_value[512]; /* raw traceparent VALUE (B2). */

    /* staged reply sequence (per-call): reply[i] used on call i (clamped to last).
     * A negative entry forces a transport error (TLS/conn/timeout → retriable). */
    long reply[4];
    int reply_len;
} amf_mock_t;

static void scan_headers(amf_mock_t *m, struct curl_slist *headers)
{
    m->saw_correlation = false;
    m->saw_traceparent = false;
    m->saw_bearer = false;
    m->corr_value[0] = '\0';
    m->traceparent_value[0] = '\0';
    for (struct curl_slist *h = headers; h != NULL; h = h->next) {
        if (strncmp(h->data, "3gpp-Sbi-Correlation-Info:", 26) == 0) {
            m->saw_correlation = true;
            const char *v = h->data + 26;
            while (*v == ' ') {
                v++;
            }
            snprintf(m->corr_value, sizeof(m->corr_value), "%s", v);
        } else if (strncmp(h->data, "traceparent:", 12) == 0) {
            m->saw_traceparent = true;
            const char *v = h->data + 12;
            while (*v == ' ') {
                v++;
            }
            snprintf(m->traceparent_value, sizeof(m->traceparent_value), "%s", v);
        } else if (strncmp(h->data, "Authorization: Bearer ", 22) == 0) {
            m->saw_bearer = true;
        }
    }
}

static int amf_mock_transport(const nssf_notification_dispatcher_t *disp,
                              const char *callback_uri, const char *body,
                              struct curl_slist *headers, long *out_http_status,
                              void *ctx)
{
    (void)disp;
    amf_mock_t *m = (amf_mock_t *)ctx;
    int idx = m->calls < m->reply_len ? m->calls : m->reply_len - 1;
    m->calls++;
    snprintf(m->last_uri, sizeof(m->last_uri), "%s",
             callback_uri != NULL ? callback_uri : "");
    snprintf(m->last_body, sizeof(m->last_body), "%s", body != NULL ? body : "");
    scan_headers(m, headers);

    long staged = (m->reply_len > 0) ? m->reply[idx] : 200;
    if (staged < 0) {
        return -1; /* transport/TLS/conn/timeout failure → retriable. */
    }
    *out_http_status = staged;
    return 0;
}

static amf_mock_t g_amf;

void setUp(void)
{
    memset(&g_amf, 0, sizeof(g_amf));
}
void tearDown(void) {}

/* Enqueue one ready-to-dispatch row carrying a real callback target. The PRODUCER
 * seam takes the not-ready path (Phase 2 fan-out deferred); the dispatch path
 * needs a resolved target, so the integration suite enqueues directly with one. */
static void seed_row(nssf_retry_store_t *store, const char *sub,
                     const char *callback)
{
    TEST_ASSERT_EQUAL_INT(0,
                          nssf_retry_store_enqueue(store, sub, callback,
                                                   "{\"changeType\":\"REPLACED\"}",
                                                   "corr-fixed-0001"));
}

/* Enqueue a row carrying a specific (possibly hostile) correlation id (B2). */
static void seed_row_corr(nssf_retry_store_t *store, const char *sub,
                          const char *callback, const char *correlation_id)
{
    TEST_ASSERT_EQUAL_INT(
        0, nssf_retry_store_enqueue(store, sub, callback,
                                    "{\"changeType\":\"REPLACED\"}",
                                    correlation_id));
}

/* Sleep `ms` milliseconds — used ONLY to let a re-armed row's BACKOFF window
 * elapse so it becomes due again on the next dispatch (Phase 4 call-driven retry:
 * a re-armed row is not claimable until its next_attempt_at passes). We sleep the
 * SMALLEST necessary window (the attempt_count=0 backoff floor is base=200ms; with
 * equal jitter the worst case is base*1.5=300ms) plus a small slack. This is NOT a
 * spin/poll loop — it is one bounded sleep over the one known backoff window. */
static void sleep_ms(long ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* Drive dispatch_pending in a BOUNDED wait-until-due loop: a re-armed row is not
 * claimable until its backoff elapses, so an immediate dispatch returns NONE. We
 * sleep a short fixed step and re-dispatch, capped at `budget_ms` total, until the
 * call yields a non-NONE result (the row became due and was dispatched once). This
 * is deterministic in OUTCOME (the row WILL become due) and bounded in time; it is
 * the minimal real-time wait the call-driven design forces for an across-backoff
 * delivery assertion. Returns the final dispatch result; the caller asserts it.
 * `step_ms` is the poll granularity; total real sleep <= budget_ms. */
static nssf_dispatch_result_e dispatch_when_due(
    nssf_notification_dispatcher_t *disp, const char *corr, long step_ms,
    long budget_ms)
{
    long waited = 0;
    for (;;) {
        nssf_dispatch_result_e r =
            nssf_notification_dispatcher_dispatch_pending(disp, corr);
        if (r != NSSF_DISPATCH_NONE) {
            return r; /* the row became due and was dispatched this call. */
        }
        if (waited >= budget_ms) {
            return r; /* timed out still NONE — caller's assertion will fail. */
        }
        sleep_ms(step_ms);
        waited += step_ms;
    }
}

/* Assert a structurally-invalid callback_uri is HARD-REJECTED at enqueue
 * (PR-phase2-dispatcher-enqueue-hardening): -1 return AND no dispatchable row
 * lands (a subsequent dequeue finds nothing). The bad callbackUri is stopped at
 * the EARLIEST layer — it can never become a POST or carry a bearer. */
static void assert_seed_rejected_at_enqueue(nssf_retry_store_t *store,
                                            const char *sub, const char *callback,
                                            const char *label)
{
    char msg[256];
    snprintf(msg, sizeof(msg),
             "[%s] callback_uri 가 enqueue 에서 -1 reject 되지 않음 "
             "(enqueue-time hardening)", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        -1,
        nssf_retry_store_enqueue(store, sub, callback,
                                 "{\"changeType\":\"REPLACED\"}", "corr-fixed-0001"),
        msg);
    nssf_retry_item_t none;
    snprintf(msg, sizeof(msg),
             "[%s] reject 후 dispatchable row 가 남음 — bad URI 가 row 가 됨", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none), msg);
    nssf_retry_item_clear(&none);
}

/* loopback oauth handle wired to a mock NRF token transport. staged_status<0 (or a
 * non-Bearer body) makes acquire fail → attach returns ABORT (fail-closed). */
typedef struct {
    long reply_status;
    const char *reply_body;
} nrf_stub_t;

static int nrf_stub_transport(const nssf_oauth2_outbound_t *ob,
                              const char *token_url, const char *form_body,
                              const char *basic_auth, char **out_body,
                              long *out_http_status, void *ctx)
{
    (void)ob;
    (void)token_url;
    (void)form_body;
    (void)basic_auth;
    nrf_stub_t *s = (nrf_stub_t *)ctx;
    *out_body = s->reply_body != NULL ? strdup(s->reply_body) : NULL;
    *out_http_status = s->reply_status;
    return 0;
}

static nssf_oauth2_outbound_config_t oauth_loopback_cfg(bool enabled)
{
    nssf_oauth2_outbound_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = enabled;
    cfg.token_url = "http://127.0.0.1:8080/oauth2/token"; /* loopback mock. */
    cfg.nf_instance_id = "nssf-instance-dispatch";
    cfg.auth_method = NSSF_OAUTH2_AUTH_MTLS;
    cfg.default_scope = "namf-comm";
    return cfg;
}

/* ===================================================================
 * 1. happy path — 200 → SENT, one POST, correlation + traceparent headers
 * =================================================================== */

static void test_happy_path_200_sent_one_post(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-happy", "https://amf.example.com/n1n2/callback");

    /* NULL oauth → dev no-attach path: emitted WITHOUT a bearer. */
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200;
    g_amf.reply_len = 1;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_SENT, r,
                                  "200 응답이 SENT 로 분류되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_amf.calls,
                                  "happy path 가 정확히 1 POST 가 아님");
    TEST_ASSERT_EQUAL_STRING("https://amf.example.com/n1n2/callback",
                             g_amf.last_uri);
    /* mandatory correlation propagation headers. */
    TEST_ASSERT_TRUE_MESSAGE(g_amf.saw_correlation,
                             "3gpp-Sbi-Correlation-Info 헤더 누락");
    TEST_ASSERT_TRUE_MESSAGE(g_amf.saw_traceparent, "traceparent 헤더 누락");
    /* NULL oauth → no bearer attached (dev no-attach path). */
    TEST_ASSERT_FALSE_MESSAGE(g_amf.saw_bearer,
                              "oauth NULL 인데 Bearer 가 부착됨");

    /* SENT row is completed (deleted) — queue is now empty. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none),
                                  "SENT 후 row 가 큐에서 제거되지 않음");
    nssf_retry_item_clear(&none);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* happy path WITH oauth enabled → Authorization: Bearer attached on the POST. */
static void test_happy_path_with_oauth_attaches_bearer(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-oauth", "https://amf.example.com/n1n2/callback");

    nssf_oauth2_outbound_config_t cfg = oauth_loopback_cfg(true);
    nssf_oauth2_outbound_t *oauth = nssf_oauth2_outbound_create_insecure(&cfg);
    TEST_ASSERT_NOT_NULL(oauth);
    nrf_stub_t nrf = {.reply_status = 200,
                      .reply_body =
                          "{\"access_token\":\"amf-bearer-xyz\","
                          "\"token_type\":\"Bearer\",\"expires_in\":300}"};
    nssf_oauth2_outbound_set_transport(oauth, nrf_stub_transport, &nrf);

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, oauth);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200;
    g_amf.reply_len = 1;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT(NSSF_DISPATCH_SENT, r);
    TEST_ASSERT_EQUAL_INT(1, g_amf.calls);
    TEST_ASSERT_TRUE_MESSAGE(g_amf.saw_bearer,
                             "oauth enabled 인데 Authorization: Bearer 누락");

    nssf_notification_dispatcher_free(disp);
    nssf_oauth2_outbound_free(oauth);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * 2. fail-closed bounded retry (Phase 4): retriable failure RE-ARMS with backoff
 *    for a LATER call — NO in-line retry. The first call makes EXACTLY ONE
 *    transport attempt (DROPPED), and the success arrives only on a SECOND
 *    dispatch once the backoff window elapses (call-driven cadence, G-08).
 * =================================================================== */

/* RETARGETED (Phase 4) — 503 then 200 across TWO dispatch calls. There is NO
 * in-line retry: the first dispatch makes EXACTLY ONE transport attempt (503 →
 * re-arm with backoff → DROPPED). An IMMEDIATE re-dispatch returns NONE (the
 * re-armed row is not yet due — proving no inline retry / no internal loop). After
 * the (smallest) backoff window elapses the SECOND dispatch makes the 2nd transport
 * attempt (200 → SENT). Total transport calls == 2, spread across two caller-driven
 * dispatches, never inside one call. */
static void test_retriable_5xx_rearms_then_200_on_second_call(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-retry", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    /* call0 → 503 (retriable, re-arm), call1 → 200 (the recovery). */
    g_amf.reply[0] = 503;
    g_amf.reply[1] = 200;
    g_amf.reply_len = 2;

    /* FIRST dispatch — exactly ONE transport attempt, 503 → re-arm → DROPPED. */
    nssf_dispatch_result_e r1 =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_DROPPED, r1,
                                  "5xx 가 re-arm(DROPPED) 로 분류되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls, "첫 dispatch 가 EXACTLY 1 transport attempt 가 아님 (no in-line retry)");

    /* IMMEDIATE re-dispatch — the re-armed row's backoff has NOT elapsed, so the
     * row is not due → NONE, and NO additional transport attempt fires. This is the
     * no-inline-retry / no-internal-loop proof. */
    nssf_dispatch_result_e rnow =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE, rnow,
        "re-arm 직후 즉시 dispatch 가 NONE 이 아님 — backoff 무시 (in-line retry)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls,
        "not-due dispatch 가 transport 를 호출함 (call-driven 위반)");

    /* SECOND (due) dispatch — after the smallest backoff window (200ms floor, up to
     * 300ms with jitter) the row is due; 200 → SENT, the 2nd transport attempt. */
    nssf_dispatch_result_e r2 = dispatch_when_due(disp, NULL, 50, 2000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_SENT, r2,
        "backoff 경과 후 second dispatch 가 SENT(200) 가 아님");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        2, g_amf.calls,
        "총 transport attempt 가 2 가 아님 (1 fail + 1 success, 각각 별도 call)");

    /* SENT row deleted — queue empty. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none),
                                  "SENT 후 row 가 제거되지 않음");
    nssf_retry_item_clear(&none);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* RETARGETED (Phase 4) — budget is TEST_DISPATCH_MAX_ATTEMPTS (5), and a retriable
 * failure that EXHAUSTS the budget moves the row to terminal dead_letter, RETAINED
 * (not physically deleted) but never re-claimed. We drive each retriable attempt
 * across its backoff window (call-driven, no in-line loop): attempts 1..4 re-arm
 * (DROPPED), and the 5th attempt (attempt_count reaches MAX-1=4) dead-letters
 * (DROPPED). Each dispatch makes EXACTLY ONE transport attempt — so after 5 calls
 * the transport was invoked exactly 5 times, never more (no hidden inline loop).
 * After exhaustion the row is dead_letter: a follow-up dequeue returns 0 (the row
 * is RETAINED but structurally unclaimable — we do NOT assert physical deletion). */
static void test_retriable_exhausts_budget_then_dead_letter(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-exhaust", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    /* every attempt is a retriable 5xx (clamped to the last staged entry). A 6th
     * transport attempt would land here too — it must NEVER be reached. */
    g_amf.reply[0] = 502;
    g_amf.reply_len = 1;

    /* FIRST attempt — due immediately. The remaining attempts each wait out the
     * growing backoff (200, 400, 800, 1600ms floors + jitter). Budget = 5. */
    nssf_dispatch_result_e r0 =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_DROPPED, r0,
                                  "attempt#1 5xx 가 DROPPED(re-arm) 가 아님");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_amf.calls, "attempt#1 가 1 transport call 이 아님");

    /* attempts #2..#5 — each becomes due after its backoff; #2..#4 re-arm, #5
     * exhausts the budget → dead_letter. All DROPPED, EXACTLY one transport call
     * each. The wait budget (~7s cap) generously covers 400+800+1600+3200ms floors
     * plus jitter; the loop returns as soon as each row is due (no over-sleep). */
    for (int attempt = 2; attempt <= TEST_DISPATCH_MAX_ATTEMPTS; ++attempt) {
        int calls_before = g_amf.calls;
        nssf_dispatch_result_e r = dispatch_when_due(disp, NULL, 50, 7000);
        char msg[128];
        snprintf(msg, sizeof(msg), "attempt#%d 가 DROPPED 가 아님", attempt);
        TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_DROPPED, r, msg);
        snprintf(msg, sizeof(msg),
                 "attempt#%d 가 EXACTLY 1 transport call 이 아님 (inline loop?)",
                 attempt);
        TEST_ASSERT_EQUAL_INT_MESSAGE(calls_before + 1, g_amf.calls, msg);
    }

    /* exactly MAX_ATTEMPTS transport invocations total — never a 6th. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        TEST_DISPATCH_MAX_ATTEMPTS, g_amf.calls,
        "총 transport attempt 가 budget(5) 와 불일치 — 6th attempt 발생 (no inline loop)");

    /* the row is now dead_letter: RETAINED (not deleted) but never re-claimed — a
     * follow-up dequeue returns 0. We do NOT assert physical deletion. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none),
        "budget 소진 후 dead_letter row 가 재배포됨 (terminal/unclaimable 위반)");
    nssf_retry_item_clear(&none);

    /* a further dispatch finds nothing due → NONE, ZERO additional transport. */
    nssf_dispatch_result_e rdead =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE, rdead,
        "dead_letter 후 dispatch 가 NONE 이 아님 (dead_letter 가 다시 claim 됨)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        TEST_DISPATCH_MAX_ATTEMPTS, g_amf.calls,
        "dead_letter row 에 대해 추가 transport attempt 가 발생");

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* RETARGETED (Phase 4) — transport error (TLS/conn/timeout) is RETRIABLE and
 * re-arms with backoff (DROPPED), then recovers to 200 → SENT across a backoff
 * window (TWO dispatch calls). The TLS-hardening-through-the-seam case (#5): a
 * forced transport error stands in for a TLS-handshake/connect failure, classified
 * retriable exactly like a 5xx. */
static void test_transport_error_rearms_then_200_on_second_call(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-tls", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = -1;  /* transport/TLS failure → retriable (re-arm). */
    g_amf.reply[1] = 200; /* recovery on the next (later) call. */
    g_amf.reply_len = 2;

    /* FIRST dispatch — one transport attempt, transport error → re-arm → DROPPED. */
    nssf_dispatch_result_e r1 =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_DROPPED, r1,
        "transport error 가 retriable re-arm(DROPPED) 로 분류되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_amf.calls,
                                  "transport error dispatch 가 1 attempt 가 아님");

    /* immediate re-dispatch is not-due → NONE, no extra transport attempt. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE,
        nssf_notification_dispatcher_dispatch_pending(disp, NULL),
        "transport-error re-arm 직후 즉시 dispatch 가 NONE 이 아님");
    TEST_ASSERT_EQUAL_INT(1, g_amf.calls);

    /* SECOND (due) dispatch — recovery 200 → SENT, the 2nd transport attempt. */
    nssf_dispatch_result_e r2 = dispatch_when_due(disp, NULL, 50, 2000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_SENT, r2,
        "TLS/conn 실패 retriable 재시도 후 SENT 되지 않음");
    TEST_ASSERT_EQUAL_INT(2, g_amf.calls);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * 3. terminal 4xx (≠429) vs retriable 429
 *    404/400 → dead_letter terminal (ONE call, no retry, unclaimable);
 *    429 → re-arm with backoff (retriable back-pressure signal)
 * =================================================================== */

/* RETARGETED (Phase 4) — a 4xx other than 429 is TERMINAL: 404 → DROPPED after
 * EXACTLY ONE transport attempt (no retry), and the row is moved to dead_letter,
 * RETAINED (not physically deleted) but never re-claimed — a follow-up dequeue
 * returns 0. We do NOT assert physical deletion (the old "row removed" assertion is
 * replaced by "row unclaimable"). 400 is checked the same way. */
static void test_terminal_4xx_dead_letter_no_retry(void)
{
    const long terminal[] = {404, 400};
    for (size_t i = 0; i < sizeof(terminal) / sizeof(terminal[0]); ++i) {
        memset(&g_amf, 0, sizeof(g_amf));
        nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
        TEST_ASSERT_NOT_NULL(store);
        seed_row(store, "sub-4xx", "https://amf.example.com/cb");

        nssf_notification_dispatcher_t *disp =
            nssf_notification_dispatcher_new(store, NULL);
        TEST_ASSERT_NOT_NULL(disp);
        nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

        g_amf.reply[0] = terminal[i];
        g_amf.reply[1] = 200; /* would succeed IF a retry happened — it must NOT. */
        g_amf.reply_len = 2;

        char msg[128];
        nssf_dispatch_result_e r =
            nssf_notification_dispatcher_dispatch_pending(disp, NULL);
        snprintf(msg, sizeof(msg), "%ld 가 DROPPED(terminal) 로 분류되지 않음",
                 terminal[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_DROPPED, r, msg);
        snprintf(msg, sizeof(msg), "%ld 가 retry 됨 — terminal 4xx 는 no retry",
                 terminal[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_amf.calls, msg);

        /* the row is dead_letter: RETAINED but unclaimable → dequeue returns 0. */
        nssf_retry_item_t none;
        snprintf(msg, sizeof(msg),
                 "%ld 후 dead_letter row 가 재배포됨 (unclaimable 위반)", terminal[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none), msg);
        nssf_retry_item_clear(&none);

        /* a follow-up dispatch finds nothing due → NONE, no extra transport. */
        snprintf(msg, sizeof(msg), "%ld dead_letter 후 dispatch 가 NONE 이 아님",
                 terminal[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            NSSF_DISPATCH_NONE,
            nssf_notification_dispatcher_dispatch_pending(disp, NULL), msg);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_amf.calls,
                                      "dead_letter row 에 추가 transport 발생");

        nssf_notification_dispatcher_free(disp);
        nssf_retry_store_free(store);
    }
}

/* NEW (#8 429 retriable contrast) — HTTP 429 Too Many Requests is RETRIABLE
 * back-pressure (NOT terminal like other 4xx): the first dispatch re-arms with
 * backoff (DROPPED, one transport attempt), an immediate re-dispatch is not-due
 * (NONE, no extra attempt), and after the backoff window the row recovers to 200 →
 * SENT on the second call. This pins the 429-vs-other-4xx split: 429 re-arms, 404/
 * 400 dead_letter. */
static void test_429_retriable_rearms_then_200_on_second_call(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-429", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 429; /* retriable back-pressure → re-arm. */
    g_amf.reply[1] = 200; /* recovery once we back off. */
    g_amf.reply_len = 2;

    /* FIRST dispatch — 429 re-arms with backoff → DROPPED, ONE transport attempt. */
    nssf_dispatch_result_e r1 =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_DROPPED, r1,
        "429 가 retriable re-arm(DROPPED) 로 분류되지 않음 — terminal 처리 오류");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_amf.calls,
                                  "429 dispatch 가 1 transport attempt 가 아님");

    /* immediate re-dispatch is not-due → NONE (backoff active), no extra attempt. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE,
        nssf_notification_dispatcher_dispatch_pending(disp, NULL),
        "429 re-arm 직후 즉시 dispatch 가 NONE 이 아님 (backoff 무시)");
    TEST_ASSERT_EQUAL_INT(1, g_amf.calls);

    /* SECOND (due) dispatch — 200 → SENT, the 2nd transport attempt. */
    nssf_dispatch_result_e r2 = dispatch_when_due(disp, NULL, 50, 2000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_SENT, r2,
        "429 backoff 경과 후 second dispatch 가 SENT(200) 가 아님");
    TEST_ASSERT_EQUAL_INT(2, g_amf.calls);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * 4. oauth fail-closed (Phase 4 TERMINAL): attach ABORT → FORBIDDEN, ZERO POSTs,
 *    row → dead_letter (NOT re-armed into a hot retry against the NRF token
 *    endpoint), and therefore NOT re-claimable on a later call.
 * =================================================================== */

/* RETARGETED (Phase 4 ratified design) — per the ratified design an OAuth2
 * fail-closed ABORT is TERMINAL: the row is moved to dead_letter (NOT re-armed),
 * so it is NEVER re-claimed — a follow-up dequeue returns 0. FORBIDDEN + ZERO POSTs
 * are unchanged (no unauthenticated outbound). The OLD assertion ("row re-armed /
 * still dequeuable later") is INVERTED: re-arming an OAuth2 ABORT would hammer the
 * token endpoint in a hot loop, which the terminal dead_letter routing closes. */
static void test_oauth_fail_closed_no_post_row_dead_lettered(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-failclosed", "https://amf.example.com/cb");

    /* enabled oauth whose token acquire FAILS (500) → attach returns ABORT. */
    nssf_oauth2_outbound_config_t cfg = oauth_loopback_cfg(true);
    nssf_oauth2_outbound_t *oauth = nssf_oauth2_outbound_create_insecure(&cfg);
    TEST_ASSERT_NOT_NULL(oauth);
    nrf_stub_t nrf = {.reply_status = 500,
                      .reply_body = "{\"error\":\"server_error\"}"};
    nssf_oauth2_outbound_set_transport(oauth, nrf_stub_transport, &nrf);

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, oauth);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200; /* the AMF would accept — but no POST must be emitted. */
    g_amf.reply_len = 1;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_FORBIDDEN, r,
        "oauth attach ABORT 가 FORBIDDEN 로 분류되지 않음 (fail-closed)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls, "fail-closed 인데 outbound POST 가 emit 됨");

    /* TERMINAL — the row is dead_letter, NOT re-armed: a follow-up dequeue returns
     * 0 (the row is retained for audit but never re-claimed). This is the precise
     * inversion of the old "row re-armed / still dequeuable" assertion. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none),
        "OAuth2 ABORT 후 row 가 재배포됨 — terminal dead_letter 가 아니라 re-arm 됨 "
        "(token endpoint hot-retry 위험)");
    nssf_retry_item_clear(&none);

    /* a follow-up dispatch likewise finds nothing due → NONE, still ZERO POSTs:
     * the dead-lettered row can never produce another (unauthenticated) outbound. */
    nssf_dispatch_result_e r2 =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE, r2,
        "dead_letter 후 dispatch 가 NONE 이 아님 (ABORT row 가 다시 claim 됨)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls,
        "dead_letter 후에도 POST 가 emit 됨 — fail-closed 0-POST 불변 위반");

    nssf_notification_dispatcher_free(disp);
    nssf_oauth2_outbound_free(oauth);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * B1 — https-only URL gate runs BEFORE the transport seam
 * =================================================================== */

/* RETARGETED (PR-phase2-dispatcher-enqueue-hardening) — a non-loopback plaintext
 * http:// callback is now HARD-REJECTED at ENQUEUE: it never becomes a row, so no
 * POST and no bearer can ever happen. This is a STRONGER assertion than the old
 * dispatch-time drop (the bad URI is stopped at the EARLIEST layer). The mock
 * transport is installed only to prove it is NEVER invoked (g_amf.calls == 0). */
static void test_http_callback_rejected_at_enqueue_no_post_no_bearer(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    /* oauth ENABLED so we could (in principle) prove no bearer — but the row never
     * exists, so the dispatch path that would attach a bearer is never reached. */
    nssf_oauth2_outbound_config_t cfg = oauth_loopback_cfg(true);
    nssf_oauth2_outbound_t *oauth = nssf_oauth2_outbound_create_insecure(&cfg);
    TEST_ASSERT_NOT_NULL(oauth);
    nrf_stub_t nrf = {.reply_status = 200,
                      .reply_body =
                          "{\"access_token\":\"amf-bearer-xyz\","
                          "\"token_type\":\"Bearer\",\"expires_in\":300}"};
    nssf_oauth2_outbound_set_transport(oauth, nrf_stub_transport, &nrf);

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, oauth);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    /* PRIMARY defense — the plaintext non-loopback http callback is rejected at
     * enqueue: -1, and no dispatchable row lands. */
    assert_seed_rejected_at_enqueue(store, "sub-http", "http://amf.example.com/cb",
                                    "non-loopback-http");

    /* dispatch on the (empty) queue finds NOTHING due → NONE, ZERO POSTs, never a
     * bearer — the bad callbackUri can never reach the transport or token attach. */
    g_amf.reply[0] = 200; /* mock would accept — must NEVER be reached. */
    g_amf.reply_len = 1;
    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE, r,
        "enqueue-reject 후 큐에 row 가 없어야 하므로 dispatch 는 NONE");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls,
        "enqueue 에서 reject 되었는데 POST 가 emit 됨 (no-row invariant 위반)");
    TEST_ASSERT_FALSE_MESSAGE(g_amf.saw_bearer,
                              "rejected callback 에 bearer 가 부착됨");

    nssf_notification_dispatcher_free(disp);
    nssf_oauth2_outbound_free(oauth);
    nssf_retry_store_free(store);
}

/* SECOND defense STILL covered — the dispatch-time https-only DROP layer. An
 * ENQUEUABLE loopback http row (passes the permissive enqueue gate) dispatched
 * through a PRODUCTION (https-only) dispatcher is DROPPED at dispatch time: the
 * https-only gate fires BEFORE the transport and BEFORE any token attach, so no
 * POST is emitted and no bearer is attached. This keeps the dispatch-time layer
 * proven WITHOUT relying on a now-rejected enqueue. */
static void test_loopback_http_dropped_by_production_https_only_gate(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    /* loopback http PASSES the permissive enqueue gate (a real row lands). */
    seed_row(store, "sub-loop-drop", "http://127.0.0.1:9090/cb");

    /* oauth ENABLED so we can prove no bearer is attached on the dropped row. */
    nssf_oauth2_outbound_config_t cfg = oauth_loopback_cfg(true);
    nssf_oauth2_outbound_t *oauth = nssf_oauth2_outbound_create_insecure(&cfg);
    TEST_ASSERT_NOT_NULL(oauth);
    nrf_stub_t nrf = {.reply_status = 200,
                      .reply_body =
                          "{\"access_token\":\"amf-bearer-xyz\","
                          "\"token_type\":\"Bearer\",\"expires_in\":300}"};
    nssf_oauth2_outbound_set_transport(oauth, nrf_stub_transport, &nrf);

    /* PRODUCTION ctor — https-only dispatch gate ACTIVE (no loopback relaxation). */
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, oauth);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200; /* mock would accept — must NEVER be reached. */
    g_amf.reply_len = 1;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_DROPPED, r,
        "loopback http row 가 production https-only gate 로 DROPPED 되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls,
        "https-only dispatch gate 가 transport 앞에서 막지 못함 — POST emit 됨");
    TEST_ASSERT_FALSE_MESSAGE(
        g_amf.saw_bearer,
        "dropped row 에 bearer 가 부착됨 (gate 가 token attach 앞에서 막아야)");

    /* terminal — the dropped row is removed (not re-armed to loop forever). */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none),
        "dispatch-time DROPPED 후 row 가 terminal 제거되지 않음");
    nssf_retry_item_clear(&none);

    nssf_notification_dispatcher_free(disp);
    nssf_oauth2_outbound_free(oauth);
    nssf_retry_store_free(store);
}

/* http loopback under the INSECURE ctor (F2) → gate relaxed, POST IS emitted (the
 * intended test-only loopback path). Confirms the _new_insecure ctor admits an
 * http://127.0.0.1 mock target where a loopback http mock is intended. */
static void test_http_loopback_insecure_ctor_allows_post(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-loopback", "http://127.0.0.1:9090/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new_insecure(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200;
    g_amf.reply_len = 1;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_SENT, r,
        "insecure ctor 가 http loopback POST 를 허용하지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_amf.calls,
                                  "insecure ctor loopback 이 POST 1회 emit 실패");
    TEST_ASSERT_EQUAL_STRING("http://127.0.0.1:9090/cb", g_amf.last_uri);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* RETARGETED (PR-phase2-dispatcher-enqueue-hardening) — B1 userinfo('@') and
 * fragment('#') callbacks are now HARD-REJECTED at ENQUEUE: each returns -1, no
 * row ever lands, and a follow-up dispatch finds NOTHING → NONE with ZERO POSTs.
 * STRONGER than the old dispatch-time drop — the hostile callbackUri is stopped at
 * the EARLIEST layer (defense-in-depth), proven from observable behaviour (empty
 * queue + no POST). The store is always torn down (no LeakSanitizer artifact). */
static void test_userinfo_and_fragment_callback_rejected(void)
{
    /* userinfo: an '@' shifts the real host (credential-in-URL / host confusion). */
    {
        nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
        TEST_ASSERT_NOT_NULL(store);
        assert_seed_rejected_at_enqueue(
            store, "sub-userinfo", "https://user:pass@evil.example.com/cb",
            "userinfo@");

        nssf_notification_dispatcher_t *disp =
            nssf_notification_dispatcher_new(store, NULL);
        TEST_ASSERT_NOT_NULL(disp);
        nssf_notification_dispatcher_set_transport(disp, amf_mock_transport,
                                                   &g_amf);
        g_amf.reply[0] = 200;
        g_amf.reply_len = 1;
        nssf_dispatch_result_e r =
            nssf_notification_dispatcher_dispatch_pending(disp, NULL);
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            NSSF_DISPATCH_NONE, r,
            "userinfo('@') enqueue-reject 후 dispatch 가 NONE 이 아님");
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, g_amf.calls, "userinfo('@') callback 에 POST 가 emit 됨");
        nssf_notification_dispatcher_free(disp);
        nssf_retry_store_free(store);
    }

    memset(&g_amf, 0, sizeof(g_amf));

    /* fragment: a '#' is never sent to the server — a data/config error. */
    {
        nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
        TEST_ASSERT_NOT_NULL(store);
        assert_seed_rejected_at_enqueue(
            store, "sub-fragment", "https://amf.example.com/cb#section",
            "fragment#");

        nssf_notification_dispatcher_t *disp =
            nssf_notification_dispatcher_new(store, NULL);
        TEST_ASSERT_NOT_NULL(disp);
        nssf_notification_dispatcher_set_transport(disp, amf_mock_transport,
                                                   &g_amf);
        g_amf.reply[0] = 200;
        g_amf.reply_len = 1;
        nssf_dispatch_result_e r =
            nssf_notification_dispatcher_dispatch_pending(disp, NULL);
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            NSSF_DISPATCH_NONE, r,
            "fragment('#') enqueue-reject 후 dispatch 가 NONE 이 아님");
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            0, g_amf.calls, "fragment('#') callback 에 POST 가 emit 됨");
        nssf_notification_dispatcher_free(disp);
        nssf_retry_store_free(store);
    }
}

/* ===================================================================
 * B2 — header-injection guard: CR/LF / control / over-length correlation id
 *      must NOT split or inject the header — a safe id is emitted instead
 * =================================================================== */

/* The outgoing 3gpp-Sbi-Correlation-Info / traceparent VALUE captured by the mock
 * carries no CR/LF/control chars (no header split) regardless of the source. */
static void assert_emitted_correlation_clean(void)
{
    TEST_ASSERT_TRUE_MESSAGE(g_amf.saw_correlation,
                             "correlation 헤더가 emit 되지 않음");
    TEST_ASSERT_TRUE_MESSAGE(g_amf.saw_traceparent, "traceparent 헤더 누락");
    /* No CR/LF/control char survived into either header VALUE. */
    for (const char *p = g_amf.corr_value; *p != '\0'; p++) {
        unsigned char ch = (unsigned char)*p;
        TEST_ASSERT_TRUE_MESSAGE(
            ch >= 0x20 && ch != 0x7f,
            "3gpp-Sbi-Correlation-Info VALUE 에 control/CRLF 가 주입됨 "
            "(header split/injection)");
    }
    for (const char *p = g_amf.traceparent_value; *p != '\0'; p++) {
        unsigned char ch = (unsigned char)*p;
        TEST_ASSERT_TRUE_MESSAGE(ch >= 0x20 && ch != 0x7f,
                                 "traceparent VALUE 에 control/CRLF 가 주입됨");
    }
    /* The malicious "X-Injected: 1" line must NOT appear as its own header — i.e.
     * the correlation value must not contain the injected token verbatim split. */
    TEST_ASSERT_NULL_MESSAGE(
        strstr(g_amf.corr_value, "X-Injected"),
        "주입된 헤더 토큰이 correlation VALUE 안에 그대로 남음 "
        "(reject 또는 safe-id 재생성 실패)");
}

/* CRLF-injection correlation id passed as the INBOUND override → sanitized
 * (rejected → safe hex id regenerated), POST still SENT with a clean header. */
static void test_crlf_inbound_correlation_not_injected(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-crlf", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200;
    g_amf.reply_len = 1;

    /* hostile inbound correlation id: a CRLF + an injected header line. */
    const char *evil = "abc123\r\nX-Injected: 1";
    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, evil);
    /* the URL is fine, so dispatch still SENDs — the id is just sanitized. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_SENT, r,
        "CRLF correlation id 가 dispatch 를 막음 (reject 대신 sanitize 기대)");
    TEST_ASSERT_EQUAL_INT(1, g_amf.calls);
    assert_emitted_correlation_clean();

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* control-char + over-length correlation id stored in the ROW → also sanitized
 * (the stored-row source is guarded too, not just the inbound override). */
static void test_control_and_overlength_stored_correlation_not_injected(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);

    /* over-length (>128) + an embedded control char + a fake header line. */
    char hostile[300];
    memset(hostile, 'A', sizeof(hostile));
    hostile[200] = '\0';
    /* splice a control char + injected line near the front. */
    hostile[3] = '\n';
    memcpy(hostile + 4, "X-Injected: 1 ", 14);
    seed_row_corr(store, "sub-ctrl", "https://amf.example.com/cb", hostile);

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200;
    g_amf.reply_len = 1;

    /* NULL inbound → the stored (hostile) id is the source — must be sanitized. */
    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT(NSSF_DISPATCH_SENT, r);
    TEST_ASSERT_EQUAL_INT(1, g_amf.calls);
    assert_emitted_correlation_clean();

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * F7 — null/empty/corrupt body → quarantine, NO null-body POST emitted
 * =================================================================== */

/* A row whose payload yields an empty body is quarantined on dequeue (the poison
 * row is dropped), so dispatch finds nothing due → NONE, and ZERO POSTs are
 * emitted (the dispatcher never POSTs a null/empty body). */
static void test_empty_body_row_quarantined_no_post(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    /* empty payload → the envelope's body is empty → quarantine on dequeue. */
    TEST_ASSERT_EQUAL_INT(0,
                          nssf_retry_store_enqueue(store, "sub-emptybody",
                                                   "https://amf.example.com/cb",
                                                   "", NULL));

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200; /* the mock would accept — must NEVER be reached. */
    g_amf.reply_len = 1;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    /* the poison row is quarantined (dropped) on dequeue → no due row → NONE. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE, r,
        "empty-body row 가 quarantine 되지 않음 (NONE 기대)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls,
        "null/empty body 인데 POST 가 emit 됨 (F7 위반)");

    /* the poison row is GONE (quarantined) — not left to loop forever. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none),
        "quarantine 후 poison row 가 남아있음 (무한 loop 위험)");
    nssf_retry_item_clear(&none);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * F7 follow-up — STRICT-valid-JSON body gate (EYEBALL-STOP slice
 *   PR-phase2-dispatcher-body-strict-json)
 *
 * envelope_build 가 cJSON_Parse 에 실패하는 payload_json 을 envelope `body`
 * 에 plain STRING 으로 저장하므로, non-empty NON-JSON payload 는 invalid-JSON
 * 문자열 body 를 가진 row 가 된다. dispatch 시 envelope_split 의 strict-JSON
 * gate (F7 follow-up) 가 이를 기존 null/empty 와 동일하게 quarantine 한다 —
 * Content-Type: application/json 으로 invalid-JSON 을 POST 하는 창을 닫는다.
 * 반대로 valid-JSON body (object/array/literal) 는 gate 를 통과해 그대로
 * 배달된다 ("valid JSON" gate 이지 "must be object" gate 가 아님).
 * =================================================================== */

/* Drive one dispatch with the given payload as the ROW body and assert the
 * quarantine contract: result is NONE (no due row delivered), ZERO POSTs, no
 * bearer, and the poison row is terminally GONE (a follow-up dequeue is empty).
 * Mirrors test_empty_body_row_quarantined_no_post exactly, parameterized over
 * the (invalid-JSON) payload string. */
static void assert_body_quarantined_no_post(const char *payload, const char *label)
{
    char msg[256];
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    /* non-empty NON-JSON payload → envelope body is that invalid string. */
    snprintf(msg, sizeof(msg), "[%s] enqueue 실패", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0,
        nssf_retry_store_enqueue(store, "sub-badjson",
                                 "https://amf.example.com/cb", payload, NULL),
        msg);

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200; /* the mock would accept — must NEVER be reached. */
    g_amf.reply_len = 1;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    /* the invalid-JSON body row is quarantined on dequeue → no due row → NONE. */
    snprintf(msg, sizeof(msg),
             "[%s] invalid-JSON body row 가 quarantine 되지 않음 (NONE 기대)",
             label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_NONE, r, msg);
    snprintf(msg, sizeof(msg),
             "[%s] invalid-JSON body 인데 POST 가 emit 됨 "
             "(application/json 로 비-JSON 전송 — strict-JSON gate 위반)", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, g_amf.calls, msg);
    snprintf(msg, sizeof(msg), "[%s] quarantine 된 row 에 bearer 부착됨", label);
    TEST_ASSERT_FALSE_MESSAGE(g_amf.saw_bearer, msg);

    /* the poison row is GONE (terminally dropped) — not left to loop forever. */
    nssf_retry_item_t none;
    snprintf(msg, sizeof(msg),
             "[%s] quarantine 후 poison row 가 남아있음 (무한 loop 위험)", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none), msg);
    nssf_retry_item_clear(&none);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* (1) invalid-JSON string body → quarantine, NO POST. Several non-JSON shapes:
 * a bare word, an unterminated object, and an unterminated array all enqueue as
 * a plain string body and are dropped by the strict-JSON gate at dispatch. */
static void test_invalid_json_body_row_quarantined_no_post(void)
{
    assert_body_quarantined_no_post("not valid json", "bare-word");
    memset(&g_amf, 0, sizeof(g_amf));
    assert_body_quarantined_no_post("{unterminated", "unterminated-object");
    memset(&g_amf, 0, sizeof(g_amf));
    assert_body_quarantined_no_post("[1,2", "unterminated-array");
    memset(&g_amf, 0, sizeof(g_amf));
    /* a bare reserved word (not a quoted JSON string) is non-JSON text. */
    assert_body_quarantined_no_post("undefined", "bare-reserved-word");
}

/* Drive one dispatch with the given (VALID-JSON) payload and assert it PASSES
 * the strict-JSON gate: it is POSTed (transport called exactly once, 2xx →
 * SENT), and the delivered body round-trips to the same JSON value. */
static void assert_valid_json_body_delivers(const char *payload, const char *label)
{
    char msg[256];
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    snprintf(msg, sizeof(msg), "[%s] enqueue 실패", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0,
        nssf_retry_store_enqueue(store, "sub-validjson",
                                 "https://amf.example.com/cb", payload, NULL),
        msg);

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200;
    g_amf.reply_len = 1;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    snprintf(msg, sizeof(msg),
             "[%s] valid-JSON body 가 SENT 되지 않음 — strict-JSON gate 가 "
             "valid JSON 을 drop 함 (over-strict)", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_SENT, r, msg);
    snprintf(msg, sizeof(msg),
             "[%s] valid-JSON body 가 정확히 1 POST 로 배달되지 않음", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_amf.calls, msg);

    /* the POSTed body must itself be parseable JSON (round-trips to the same
     * value) — the gate let through a deliverable application/json body. */
    cJSON *got = cJSON_Parse(g_amf.last_body);
    snprintf(msg, sizeof(msg),
             "[%s] POST 된 body 가 valid JSON 이 아님 (배달된 body 가 비-JSON)",
             label);
    TEST_ASSERT_NOT_NULL_MESSAGE(got, msg);
    cJSON *want = cJSON_Parse(payload);
    TEST_ASSERT_NOT_NULL(want);
    snprintf(msg, sizeof(msg),
             "[%s] POST 된 body 가 enqueue payload 와 JSON-동등하지 않음", label);
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(got, want, true), msg);
    cJSON_Delete(got);
    cJSON_Delete(want);

    /* SENT row is completed (deleted) — queue is now empty. */
    nssf_retry_item_t none;
    snprintf(msg, sizeof(msg), "[%s] SENT 후 row 가 큐에서 제거되지 않음", label);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none), msg);
    nssf_retry_item_clear(&none);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* (2) valid-JSON OBJECT body still delivers (regression — the gate does NOT
 * drop valid JSON). */
static void test_valid_json_object_body_still_delivers(void)
{
    assert_valid_json_body_delivers("{\"changeType\":\"REPLACED\"}",
                                    "json-object");
    memset(&g_amf, 0, sizeof(g_amf));
    /* a nested object is still valid JSON. */
    assert_valid_json_body_delivers(
        "{\"changeType\":\"REPLACED\",\"snssais\":[{\"sst\":1}]}",
        "json-nested-object");
}

/* (3) valid-JSON NON-OBJECT body still delivers (boundary — proves the strict-
 * JSON gate is "strict-valid JSON", NOT "must be an object"). A JSON array and
 * the scalar JSON literals that round-trip through the envelope as a re-
 * serialized sub-tree (number / boolean) all PASS and POST. If nf-code's gate
 * were object-only these would (wrongly) quarantine — that is the discrepancy
 * this case guards against.
 *
 * SCOPE NOTE — these payloads are stored by envelope_build as a PARSED sub-tree
 * (cJSON_Parse succeeds), then envelope_split RE-SERIALIZES the sub-tree
 * (cJSON_PrintUnformatted) back to canonical JSON, so the body the gate checks
 * is always strict-valid JSON. (The two literals that do NOT survive the
 * envelope round-trip — a top-level JSON string and a top-level JSON null — are
 * an envelope-storage artifact, NOT the strict-JSON gate, and are pinned
 * separately in test_envelope_roundtrip_nondeliverable_literals_quarantined.) */
static void test_valid_json_non_object_body_still_delivers(void)
{
    /* JSON array. */
    assert_valid_json_body_delivers("[1,2,3]", "json-array");
    memset(&g_amf, 0, sizeof(g_amf));
    /* JSON number literal. */
    assert_valid_json_body_delivers("42", "json-number-literal");
    memset(&g_amf, 0, sizeof(g_amf));
    /* JSON boolean literals. */
    assert_valid_json_body_delivers("true", "json-true-literal");
    memset(&g_amf, 0, sizeof(g_amf));
    assert_valid_json_body_delivers("false", "json-false-literal");
}

/* (3b) BOUNDARY DOCUMENTATION — two top-level JSON literals are non-deliverable
 * through the envelope, and the dispatcher correctly quarantines them (NO POST).
 * This is NOT the strict-JSON gate rejecting valid JSON — it is upstream
 * envelope (build/split) round-trip semantics, asserted here so the boundary is
 * pinned rather than silently ambiguous:
 *
 *   - top-level JSON STRING ("a-json-string"): envelope_build stores it as a
 *     cJSON STRING node; envelope_split's string branch extracts valuestring —
 *     the BARE unquoted text "a-json-string" — which is NOT valid JSON, so the
 *     strict-JSON gate quarantines it. (Emitting the bare text as
 *     application/json would be exactly the poison the gate closes.)
 *   - top-level JSON NULL: envelope_split treats a null body as the F7
 *     null/empty-body case (out_body stays NULL) → quarantine, independent of
 *     the new gate.
 *
 * Either way the observable contract holds: a non-deliverable body never POSTs. */
static void test_envelope_roundtrip_nondeliverable_literals_quarantined(void)
{
    assert_body_quarantined_no_post("\"a-json-string\"", "toplevel-json-string");
    memset(&g_amf, 0, sizeof(g_amf));
    assert_body_quarantined_no_post("null", "toplevel-json-null");
}

/* ===================================================================
 * no-worker / call-driven — nothing fires between caller-driven dispatches
 * =================================================================== */

/* NEW (#5 no-worker, #4 call-driven) — there is NO background worker/thread/timer.
 * With a due row sitting in the queue and the transport installed, NO transport
 * attempt fires UNTIL the caller invokes dispatch_pending. We seed a due row, then
 * WAIT past the smallest backoff window WITHOUT dispatching, and assert g_amf.calls
 * stays 0 the whole time — proving nothing happens between calls (no internal
 * loop). Then a single explicit dispatch makes EXACTLY one attempt. */
static void test_no_worker_no_transport_without_dispatch(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-noworker", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 200;
    g_amf.reply_len = 1;

    /* A due row exists and the transport is wired — but NO dispatch was called.
     * Wait past a full backoff floor window; a background worker (if any existed)
     * would have fired by now. The transport MUST stay untouched. */
    sleep_ms(350);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls,
        "dispatch_pending 호출 없이 transport 가 호출됨 — background worker/timer 존재 "
        "(call-driven/no-worker 위반)");

    /* the row is still pending (nothing consumed it) — one explicit dispatch now
     * makes EXACTLY one attempt and delivers. */
    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_SENT, r,
                                  "명시적 dispatch 후 due row 가 SENT 되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls, "명시적 dispatch 가 EXACTLY 1 transport attempt 가 아님");

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * empty queue → NONE (no transport call)
 * =================================================================== */

static void test_empty_queue_none(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT(NSSF_DISPATCH_NONE, r);
    TEST_ASSERT_EQUAL_INT(0, g_amf.calls);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_happy_path_200_sent_one_post);
    RUN_TEST(test_happy_path_with_oauth_attaches_bearer);
    RUN_TEST(test_retriable_5xx_rearms_then_200_on_second_call);
    RUN_TEST(test_retriable_exhausts_budget_then_dead_letter);
    RUN_TEST(test_transport_error_rearms_then_200_on_second_call);
    RUN_TEST(test_terminal_4xx_dead_letter_no_retry);
    RUN_TEST(test_429_retriable_rearms_then_200_on_second_call);
    RUN_TEST(test_oauth_fail_closed_no_post_row_dead_lettered);
    RUN_TEST(test_no_worker_no_transport_without_dispatch);
    RUN_TEST(test_http_callback_rejected_at_enqueue_no_post_no_bearer);
    RUN_TEST(test_loopback_http_dropped_by_production_https_only_gate);
    RUN_TEST(test_http_loopback_insecure_ctor_allows_post);
    RUN_TEST(test_userinfo_and_fragment_callback_rejected);
    RUN_TEST(test_crlf_inbound_correlation_not_injected);
    RUN_TEST(test_control_and_overlength_stored_correlation_not_injected);
    RUN_TEST(test_empty_body_row_quarantined_no_post);
    RUN_TEST(test_invalid_json_body_row_quarantined_no_post);
    RUN_TEST(test_valid_json_object_body_still_delivers);
    RUN_TEST(test_valid_json_non_object_body_still_delivers);
    RUN_TEST(test_envelope_roundtrip_nondeliverable_literals_quarantined);
    RUN_TEST(test_empty_queue_none);
    return UNITY_END();
}
