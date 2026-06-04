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
 *   retry-once: 5xx then 200 → SENT after EXACTLY 2 transport calls (G-08 one     — #2
 *     retry, no exponential/multi-attempt loop)
 *   5xx twice  → DROPPED after EXACTLY 2 calls (retry budget spent, no            — #2
 *     dead-letter, no third attempt)
 *   4xx no-retry: 404 → DROPPED after EXACTLY ONE call (terminal client error)    — #3
 *   oauth fail-closed: attach ABORT → NSSF_DISPATCH_FORBIDDEN, ZERO POSTs, and    — #4
 *     the row is re-armed (still dequeuable later — not lost)
 *   TLS hardening is exercised THROUGH the seam: the mock cannot read curl opts,  — #5
 *     so the dispatch-status contract (a forced transport error == a TLS/conn
 *     failure → retriable) stands in for the not-directly-observable curl flags.
 *
 * REWORK round-1 regression blockers (this suite's new cases):
 *   B1: an http:// callback URL is DROPPED by the production https-only gate       — #B1
 *     BEFORE the transport seam runs — no POST emitted, no bearer attached.
 *   B1: a callback URL with userinfo('@') or fragment('#') is rejected — no POST.  — #B1
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

#include <curl/curl.h>

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
 * 2. retry-once (G-08 minimal): 5xx then 200 → SENT after EXACTLY 2 calls
 * =================================================================== */

static void test_retry_once_5xx_then_200_sent(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-retry", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    /* call0 → 503 (retriable), call1 → 200. */
    g_amf.reply[0] = 503;
    g_amf.reply[1] = 200;
    g_amf.reply_len = 2;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_SENT, r,
                                  "5xx→200 이 SENT 로 분류되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        2, g_amf.calls,
        "G-08: 정확히 1회 retry (총 2 attempt) 가 아님 — exponential/multi-loop 금지");

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* retriable failure twice → DROPPED after EXACTLY 2 calls (budget spent, no
 * dead-letter, no third attempt). */
static void test_retry_exhausted_5xx_twice_dropped(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-exhaust", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 502;
    g_amf.reply[1] = 502;
    g_amf.reply[2] = 502; /* would be a 3rd attempt — must NEVER be reached. */
    g_amf.reply_len = 3;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_DROPPED, r,
                                  "retry 소진이 DROPPED 로 분류되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        2, g_amf.calls,
        "G-08: retry budget 2 attempt 초과 (3rd attempt 발생) — no dead-letter loop");

    /* no dead-letter table this phase — the row is removed terminally. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_retry_store_dequeue(store, &none),
                                  "retry 소진 후 row 가 제거되지 않음 (dead-letter 없음)");
    nssf_retry_item_clear(&none);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* transport error (TLS/conn/timeout) then 200 → retriable, SENT after 2 calls.
 * This is the TLS-hardening-through-the-seam case (#5): a forced transport error
 * stands in for a TLS-handshake/connect failure, classified retriable. */
static void test_transport_error_then_200_retried(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-tls", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = -1;  /* transport/TLS failure → retriable. */
    g_amf.reply[1] = 200; /* recovery on the single retry. */
    g_amf.reply_len = 2;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_SENT, r,
        "TLS/conn 실패가 retriable 로 분류되어 retry 후 SENT 되지 않음");
    TEST_ASSERT_EQUAL_INT(2, g_amf.calls);

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * 3. 4xx no-retry: 404 → DROPPED after EXACTLY ONE call
 * =================================================================== */

static void test_4xx_no_retry_dropped(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-4xx", "https://amf.example.com/cb");

    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(store, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);

    g_amf.reply[0] = 404;
    g_amf.reply[1] = 200; /* would succeed IF a retry happened — it must NOT. */
    g_amf.reply_len = 2;

    nssf_dispatch_result_e r =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_DISPATCH_DROPPED, r,
                                  "4xx 가 DROPPED (terminal) 로 분류되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls, "4xx 가 retry 됨 — 4xx 는 terminal, no retry");

    nssf_notification_dispatcher_free(disp);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * 4. oauth fail-closed: attach ABORT → FORBIDDEN, ZERO POSTs, row re-armed
 * =================================================================== */

static void test_oauth_fail_closed_no_post_row_preserved(void)
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

    /* the row must NOT be lost — re-armed for a later attempt once M4 recovers. */
    nssf_retry_item_t again;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, nssf_retry_store_dequeue(store, &again),
        "fail-closed 후 row 가 보존되지 않음 (재시도 불가)");
    TEST_ASSERT_EQUAL_STRING("sub-failclosed", again.subscription_id);
    nssf_retry_item_clear(&again);

    nssf_notification_dispatcher_free(disp);
    nssf_oauth2_outbound_free(oauth);
    nssf_retry_store_free(store);
}

/* ===================================================================
 * B1 — https-only URL gate runs BEFORE the transport seam
 * =================================================================== */

/* http:// callback under the PRODUCTION ctor (gate active) → row DROPPED, NO POST
 * emitted, NO bearer attached, terminal classification. The mock would have
 * accepted (200) — but the gate fires before the transport and before any token
 * attach, so the mock is NEVER invoked. */
static void test_http_callback_dropped_no_post_no_bearer(void)
{
    nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(store);
    seed_row(store, "sub-http", "http://amf.example.com/cb"); /* plaintext! */

    /* oauth ENABLED so we can prove no bearer is attached on the dropped row. */
    nssf_oauth2_outbound_config_t cfg = oauth_loopback_cfg(true);
    nssf_oauth2_outbound_t *oauth = nssf_oauth2_outbound_create_insecure(&cfg);
    TEST_ASSERT_NOT_NULL(oauth);
    nrf_stub_t nrf = {.reply_status = 200,
                      .reply_body =
                          "{\"access_token\":\"amf-bearer-xyz\","
                          "\"token_type\":\"Bearer\",\"expires_in\":300}"};
    nssf_oauth2_outbound_set_transport(oauth, nrf_stub_transport, &nrf);

    /* PRODUCTION ctor — https-only gate ACTIVE. */
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
        "http:// callback 이 https-only gate 로 DROPPED (terminal) 되지 않음");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls,
        "https-only gate 가 transport 앞에서 막지 못함 — POST 가 emit 됨");
    TEST_ASSERT_FALSE_MESSAGE(
        g_amf.saw_bearer,
        "dropped row 에 bearer 가 부착됨 (gate 가 token attach 앞에서 막아야)");

    /* terminal — the poison row is removed (not re-armed to loop forever). */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(store, &none),
        "http:// DROPPED 후 row 가 terminal 제거되지 않음");
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

/* B1 — userinfo('@') and fragment('#') are rejected on BOTH ctors → NO POST. */
static void test_userinfo_and_fragment_callback_rejected(void)
{
    /* userinfo: an '@' shifts the real host (credential-in-URL / host confusion). */
    {
        nssf_retry_store_t *store = nssf_retry_store_new_inmemory();
        TEST_ASSERT_NOT_NULL(store);
        seed_row(store, "sub-userinfo",
                 "https://user:pass@evil.example.com/cb");
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
            NSSF_DISPATCH_DROPPED, r,
            "userinfo('@') callback 이 DROPPED 되지 않음");
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
        seed_row(store, "sub-fragment",
                 "https://amf.example.com/cb#section");
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
            NSSF_DISPATCH_DROPPED, r,
            "fragment('#') callback 이 DROPPED 되지 않음");
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
    RUN_TEST(test_retry_once_5xx_then_200_sent);
    RUN_TEST(test_retry_exhausted_5xx_twice_dropped);
    RUN_TEST(test_transport_error_then_200_retried);
    RUN_TEST(test_4xx_no_retry_dropped);
    RUN_TEST(test_oauth_fail_closed_no_post_row_preserved);
    RUN_TEST(test_http_callback_dropped_no_post_no_bearer);
    RUN_TEST(test_http_loopback_insecure_ctor_allows_post);
    RUN_TEST(test_userinfo_and_fragment_callback_rejected);
    RUN_TEST(test_crlf_inbound_correlation_not_injected);
    RUN_TEST(test_control_and_overlength_stored_correlation_not_injected);
    RUN_TEST(test_empty_body_row_quarantined_no_post);
    RUN_TEST(test_empty_queue_none);
    return UNITY_END();
}
