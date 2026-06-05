/*
 * test_oauth2_outbound_retry.c — WI-oauth2-resilience (ADR-0004 M4 decision #11)
 * unit suite. EYEBALL-STOP security-sensitive slice.
 *
 * Bounded retry + exponential-capped backoff + dead-letter classification +
 * the fail-closed invariant for outbound OAuth2 token acquire. Driven ENTIRELY
 * through the mock token-transport seam (nssf_oauth2_outbound_set_transport) and
 * the no-op backoff-sleep seam (nssf_oauth2_outbound_set_backoff_sleep) — NO
 * network, NO real sleeping, NO live NRF, so the suite is fast/deterministic and
 * runs UNCONDITIONALLY in CI.
 *
 * 검증 핵심 (M4 decision #11):
 *   - RETRIABLE (transport error | HTTP 5xx | HTTP 429) → bounded backoff 로
 *     재시도, max_attempts 로 상한.
 *   - DEAD_LETTER (400/401/403 및 그 외 4xx, OAuth2 error body invalid_client /
 *     invalid_scope, 200-with-error, 200-unusable-token) → 첫 시도에서 즉시
 *     fail-closed, 재시도 없음, dead-letter cb 호출.
 *   - 모든 terminal failure path 는 fail-closed — ABORT 반환 + Authorization:
 *     Bearer header 절대 append 안 함 (unauthenticated outbound 금지).
 *   - 재시도는 acquire 호출 안에서 SYNCHRONOUS — worker thread / polling 없음.
 *
 * Classification map (mirror of refresh_locked 의 nf-code 구현):
 *   transport rc!=0               → RETRIABLE
 *   HTTP 429 | HTTP 5xx           → RETRIABLE
 *   HTTP 200 + valid Bearer + TTL → OK
 *   HTTP 400/401/403 + other 4xx  → DEAD_LETTER
 *   OAuth2 body error invalid_client / invalid_scope → DEAD_LETTER
 *   HTTP 200 with "error" field   → DEAD_LETTER
 *   HTTP 200 but unusable token    → DEAD_LETTER
 *
 * Owned-copy contract (B3): nssf_oauth2_outbound_acquire writes an OWNED malloc'd
 * bearer copy into *out_bearer — every successful acquire here free()s it
 * (LeakSan-clean). The loopback ctor (nssf_oauth2_outbound_create_insecure) is
 * used so the cert-less mock config builds; retry behavior is identical to the
 * production ctor (the retry loop is ctor-agnostic).
 */

#define _POSIX_C_SOURCE 200809L

#include "oauth2_outbound.h"
#include "unity.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Mock NRF token transport — staged per-attempt reply SEQUENCE.
 *
 * Each transport invocation consumes the next staged reply from `replies[]`
 * (clamped to the last entry once exhausted, so "always 503" needs only one
 * staged entry). A reply is one of:
 *   force_error != 0  → return nonzero (transport/TLS/timeout) → RETRIABLE.
 *   else              → return 0, set *out_http_status = http_status and
 *                       *out_body = strdup(body) (or NULL when body==NULL).
 * The mock records the running call count so tests assert attempt count and
 * call-driven (synchronous-only) behavior.
 * ========================================================================== */

typedef struct {
    int force_error; /* nonzero → simulate transport/TLS/timeout failure. */
    long http_status;
    const char *body; /* NULL → no body. */
} staged_reply_t;

typedef struct {
    int calls;                 /* total transport invocations. */
    staged_reply_t replies[8]; /* staged sequence. */
    int reply_count;           /* number of valid entries in replies[]. */
} mock_t;

static int mock_transport(const nssf_oauth2_outbound_t *ob, const char *token_url,
                          const char *form_body, const char *basic_auth,
                          char **out_body, long *out_http_status, void *ctx)
{
    (void)ob;
    (void)token_url;
    (void)form_body;
    (void)basic_auth;
    mock_t *m = (mock_t *)ctx;

    /* Pick the reply for THIS attempt; clamp to the last staged entry so a
     * single staged "always retriable" reply covers an exhaustion run. */
    int idx = m->calls;
    if (idx >= m->reply_count) {
        idx = m->reply_count - 1;
    }
    if (idx < 0) {
        idx = 0;
    }
    const staged_reply_t *r = &m->replies[idx];
    m->calls++;

    if (r->force_error) {
        return -1; /* network/TLS/timeout → RETRIABLE. */
    }
    *out_body = r->body != NULL ? strdup(r->body) : NULL;
    *out_http_status = r->http_status;
    return 0;
}

/* ============================================================================
 * Backoff-sleep seam — NO-OP that RECORDS the delay_ms sequence (so the suite
 * never really sleeps and can assert the exponential-capped delay schedule).
 * ========================================================================== */

typedef struct {
    int calls;
    uint32_t delays[16];
} sleep_rec_t;

static void recording_noop_sleep(uint32_t delay_ms, void *ctx)
{
    sleep_rec_t *s = (sleep_rec_t *)ctx;
    if (s->calls < (int)(sizeof(s->delays) / sizeof(s->delays[0]))) {
        s->delays[s->calls] = delay_ms;
    }
    s->calls++;
    /* NO real sleep — deterministic + fast. */
}

/* ============================================================================
 * Dead-letter callback — records each invocation's (http_status, oauth_error).
 * Asserts classification + SECRET HYGIENE: the recorded oauth_error must be a
 * short error CODE only, never a token/secret.
 * ========================================================================== */

#define DL_MAX 8

typedef struct {
    int calls;
    long http_status[DL_MAX];
    char oauth_error[DL_MAX][128]; /* "" when NULL. */
} dl_rec_t;

static void recording_dead_letter_cb(long http_status, const char *oauth_error,
                                     void *ctx)
{
    dl_rec_t *d = (dl_rec_t *)ctx;
    if (d->calls < DL_MAX) {
        d->http_status[d->calls] = http_status;
        snprintf(d->oauth_error[d->calls], sizeof(d->oauth_error[0]), "%s",
                 oauth_error != NULL ? oauth_error : "");
    }
    d->calls++;
}

/* ============================================================================
 * Fixtures.
 * ========================================================================== */

static mock_t g_mock;
static sleep_rec_t g_sleep;
static dl_rec_t g_dl;

void setUp(void)
{
    memset(&g_mock, 0, sizeof(g_mock));
    memset(&g_sleep, 0, sizeof(g_sleep));
    memset(&g_dl, 0, sizeof(g_dl));
}
void tearDown(void) {}

/*
 * Loopback base config — cert-less MTLS over http://127.0.0.1 (the test-only
 * ctor relaxes the cert/key fail-fast). Retry behavior is ctor-independent.
 */
static nssf_oauth2_outbound_config_t base_cfg(void)
{
    nssf_oauth2_outbound_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.token_url = "http://127.0.0.1:8080/oauth2/token";
    cfg.nf_instance_id = "nssf-0a1b2c3d-instance";
    cfg.auth_method = NSSF_OAUTH2_AUTH_MTLS;
    cfg.default_scope = "nnrf-disc nnrf-nfm";
    /* numeric fields left 0 → decision defaults unless a test overrides. */
    return cfg;
}

/*
 * Build a handle wired to the mock transport AND the recording no-op sleep. The
 * no-op sleep is installed on EVERY handle so no test ever sleeps for real.
 */
static nssf_oauth2_outbound_t *make_handle(const nssf_oauth2_outbound_config_t *cfg)
{
    nssf_oauth2_outbound_t *ob = nssf_oauth2_outbound_create_insecure(cfg);
    TEST_ASSERT_NOT_NULL(ob);
    nssf_oauth2_outbound_set_transport(ob, mock_transport, &g_mock);
    nssf_oauth2_outbound_set_backoff_sleep(ob, recording_noop_sleep, &g_sleep);
    return ob;
}

static void stage(int idx, int force_error, long http_status, const char *body)
{
    g_mock.replies[idx].force_error = force_error;
    g_mock.replies[idx].http_status = http_status;
    g_mock.replies[idx].body = body;
    if (idx + 1 > g_mock.reply_count) {
        g_mock.reply_count = idx + 1;
    }
}

static const char *kGoodBody =
    "{\"access_token\":\"opaque-bearer-ok\",\"token_type\":\"Bearer\","
    "\"expires_in\":300}";

/* Assert that an attach attempt on this handle leaves the curl_slist NULL —
 * the fail-closed invariant: NO Authorization header is ever appended. */
static void assert_attach_fail_closed(nssf_oauth2_outbound_t *ob)
{
    struct curl_slist *headers = NULL;
    nssf_oauth2_outbound_status_t st =
        nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers);
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT, st);
    TEST_ASSERT_NULL(headers); /* never an unauthenticated/partial header set. */
}

/* ============================================================================
 * 1. Transient transport error then success → OK, retried (case #1).
 * ========================================================================== */

static void test_transport_error_then_success(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg(); /* default max_attempts=3. */
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    stage(0, /*force_error=*/1, 0, NULL);    /* attempt 1: transport error. */
    stage(1, 0, 200, kGoodBody);             /* attempt 2: success. */

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NOT_NULL(bearer);
    TEST_ASSERT_EQUAL_STRING("opaque-bearer-ok", bearer);
    free(bearer);

    /* Exactly 2 transport calls; exactly 1 backoff sleep between them. */
    TEST_ASSERT_EQUAL_INT(2, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_INT(1, g_sleep.calls);
    /* No dead-letter on a recovered-by-retry path. */
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_dead_letter_count(ob));
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 2. HTTP 503 (5xx) then success → OK, retried (case #2).
 * ========================================================================== */

static void test_http_503_then_success(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    stage(0, 0, 503, "{\"error\":\"server_error\"}"); /* RETRIABLE 5xx. */
    stage(1, 0, 200, kGoodBody);

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NOT_NULL(bearer);
    free(bearer);

    TEST_ASSERT_EQUAL_INT(2, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_retry_count(ob));
    /* A retried 5xx is NOT a dead-letter. */
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_dead_letter_count(ob));
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 3. HTTP 429 then success → OK, retried (case #3). 429 is the one retriable
 *    4xx — everything else in the 4xx range is dead-letter.
 * ========================================================================== */

static void test_http_429_then_success(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    stage(0, 0, 429, "{\"error\":\"rate_limited\"}"); /* RETRIABLE 429. */
    stage(1, 0, 200, kGoodBody);

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NOT_NULL(bearer);
    free(bearer);

    TEST_ASSERT_EQUAL_INT(2, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_dead_letter_count(ob));
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 4. Retry exhaustion: persistent RETRIABLE (always 503) → ABORT (fail-closed),
 *    transport called EXACTLY max_attempts times, NO token cached/attached, and
 *    a subsequent attach leaves headers unmodified (case #4 + #8).
 * ========================================================================== */

static void test_retry_exhaustion_fail_closed(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.max_attempts = 4; /* explicit bound to assert the exact call count. */
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    stage(0, 0, 503, "{\"error\":\"server_error\"}"); /* clamped → always 503. */

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer); /* fail-closed: nothing owned to free. */

    /* Exactly max_attempts transport calls; (max_attempts-1) backoff sleeps. */
    TEST_ASSERT_EQUAL_INT(4, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(3, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_INT(3, g_sleep.calls);
    /* Exhaustion is NOT a dead-letter (it is a retriable that ran out). */
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_dead_letter_count(ob));

    /* fail-closed invariant: subsequent attach appends NO bearer header. The
     * attach itself drives another full (exhausting) acquire — assert it too is
     * call-count-bounded by max_attempts and leaves the slist NULL. */
    int before = g_mock.calls;
    assert_attach_fail_closed(ob);
    TEST_ASSERT_EQUAL_INT(before + 4, g_mock.calls); /* fresh exhausting acquire. */
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 5. Dead-letter non-retriable, NO retry: 400 / 401 / 403 each → ABORT on the
 *    FIRST attempt (transport called exactly once, retry_count==0), dead-letter
 *    cb invoked with the HTTP status (case #5).
 * ========================================================================== */

static void run_dead_letter_status_no_retry(long status)
{
    setUp(); /* fresh fixtures per status (this is a helper, not a RUN_TEST). */
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.max_attempts = 5; /* generous budget — proves NO retry is consumed. */
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);
    nssf_oauth2_outbound_set_dead_letter_cb(ob, recording_dead_letter_cb, &g_dl);

    stage(0, 0, status, "{\"error\":\"access_denied\"}");

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    /* First-attempt fail-closed: exactly ONE transport call, ZERO retries. */
    TEST_ASSERT_EQUAL_INT(1, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_INT(0, g_sleep.calls);

    /* Dead-letter recorded + cb invoked once with the HTTP status. */
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_dead_letter_count(ob));
    TEST_ASSERT_EQUAL_INT(1, g_dl.calls);
    TEST_ASSERT_EQUAL_INT(status, g_dl.http_status[0]);

    /* fail-closed invariant on the dead-letter path: no bearer header. */
    assert_attach_fail_closed(ob);
    nssf_oauth2_outbound_free(ob);
}

static void test_dead_letter_400_no_retry(void) { run_dead_letter_status_no_retry(400); }
static void test_dead_letter_401_no_retry(void) { run_dead_letter_status_no_retry(401); }
static void test_dead_letter_403_no_retry(void) { run_dead_letter_status_no_retry(403); }

/* ============================================================================
 * 6. OAuth2 error body dead-letter: 400 + {"error":"invalid_client"} and
 *    {"error":"invalid_scope"} → ABORT, no retry, cb sees the error CODE, no
 *    secret leaked (case #6 + #12).
 * ========================================================================== */

static void run_oauth_error_body_dead_letter(const char *err_code)
{
    setUp();
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.max_attempts = 5;
    cfg.secret_ref = NULL; /* MTLS handle — no secret material at all. */
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);
    nssf_oauth2_outbound_set_dead_letter_cb(ob, recording_dead_letter_cb, &g_dl);

    char body[128];
    snprintf(body, sizeof(body), "{\"error\":\"%s\"}", err_code);
    stage(0, 0, 400, body);

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    /* No retry on a non-retriable OAuth2 error body. */
    TEST_ASSERT_EQUAL_INT(1, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_dead_letter_count(ob));

    /* cb sees the HTTP status AND the parsed error CODE (non-secret). */
    TEST_ASSERT_EQUAL_INT(1, g_dl.calls);
    TEST_ASSERT_EQUAL_INT(400, g_dl.http_status[0]);
    TEST_ASSERT_EQUAL_STRING(err_code, g_dl.oauth_error[0]);

    assert_attach_fail_closed(ob);
    nssf_oauth2_outbound_free(ob);
}

static void test_dead_letter_invalid_client(void)
{
    run_oauth_error_body_dead_letter("invalid_client");
}
static void test_dead_letter_invalid_scope(void)
{
    run_oauth_error_body_dead_letter("invalid_scope");
}

/* ============================================================================
 * 7. Exponential capped backoff sequence (case #7). base=100, max=250 over
 *    enough RETRIABLE attempts to hit the cap → recorded delays 100,200,250,250.
 *
 *    delay(retry_index): 0→100, 1→200 (<250), 2→400 capped→250, 3→capped→250.
 * ========================================================================== */

static void test_exponential_capped_backoff_sequence(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.backoff_base_ms = 100;
    cfg.backoff_max_ms = 250;
    cfg.max_attempts = 5; /* 5 tries → 4 backoff sleeps → exercises the cap. */
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    stage(0, 0, 503, "{\"error\":\"server_error\"}"); /* always retriable. */

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    TEST_ASSERT_EQUAL_INT(5, g_mock.calls);
    /* 4 backoff sleeps with the exponential-then-capped schedule. */
    TEST_ASSERT_EQUAL_INT(4, g_sleep.calls);
    TEST_ASSERT_EQUAL_UINT32(100, g_sleep.delays[0]);
    TEST_ASSERT_EQUAL_UINT32(200, g_sleep.delays[1]);
    TEST_ASSERT_EQUAL_UINT32(250, g_sleep.delays[2]); /* 400 capped at 250. */
    TEST_ASSERT_EQUAL_UINT32(250, g_sleep.delays[3]); /* stays capped. */
    /* Every recorded delay is within the configured ceiling (bounded). */
    for (int i = 0; i < g_sleep.calls; i++) {
        TEST_ASSERT_TRUE(g_sleep.delays[i] <= 250);
    }
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 8. fail-closed invariant (hard) — already exercised on the exhaustion path
 *    (#4) and each dead-letter path (#5/#6). This test pins it explicitly on a
 *    DEAD_LETTER path: acquire returns ABORT, attach appends NO header, and the
 *    bearer out-param stays NULL.
 * ========================================================================== */

static void test_fail_closed_no_bearer_header_on_dead_letter(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    stage(0, 0, 401, "{\"error\":\"invalid_token\"}"); /* non-retriable. */

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    /* attach over a PRE-POPULATED slist must NOT add a Bearer line, and must
     * leave the pre-existing entry intact (fail-closed never mutates beyond
     * appending — here it appends nothing). */
    struct curl_slist *headers = curl_slist_append(NULL, "X-Pre: keep");
    TEST_ASSERT_NOT_NULL(headers);
    TEST_ASSERT_EQUAL_INT(
        NSSF_OAUTH2_ABORT,
        nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers));
    bool any_bearer = false;
    int n = 0;
    for (struct curl_slist *h = headers; h != NULL; h = h->next) {
        n++;
        if (strstr(h->data, "Authorization") != NULL ||
            strstr(h->data, "Bearer") != NULL) {
            any_bearer = true;
        }
    }
    TEST_ASSERT_FALSE(any_bearer);  /* NEVER an unauthenticated bearer. */
    TEST_ASSERT_EQUAL_INT(1, n);    /* only the pre-existing entry remains. */
    curl_slist_free_all(headers);
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 9. No-worker / call-driven: the transport is invoked ONLY synchronously
 *    within an acquire call — never in the background between calls (case #9).
 * ========================================================================== */

static void test_call_driven_no_background_invocation(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    stage(0, 0, 200, kGoodBody);

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    free(bearer);
    int after_first = g_mock.calls;
    TEST_ASSERT_EQUAL_INT(1, after_first);

    /* No acquire in progress → call count is STABLE. Repeated observation of
     * accessors / sleep-record must not trigger any transport invocation (no
     * worker thread, no timer). */
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_dead_letter_count(ob));
    TEST_ASSERT_EQUAL_INT(0, g_sleep.calls);
    TEST_ASSERT_EQUAL_INT(after_first, g_mock.calls); /* unchanged. */

    /* The SECOND acquire reuses the fresh cache → still no NEW transport call. */
    char *bearer2 = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer2));
    free(bearer2);
    TEST_ASSERT_EQUAL_INT(after_first, g_mock.calls); /* cache reuse, no I/O. */
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 10. Default config: zeroed max_attempts/backoff_* → defaults applied. With a
 *     persistent RETRIABLE (503) the default max_attempts=3 → exactly 3 calls
 *     and the default backoff base=200 surfaces in the recorded delays (case
 *     #10).
 * ========================================================================== */

static void test_default_config_applies_defaults(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg(); /* all retry fields zero. */
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    stage(0, 0, 503, "{\"error\":\"server_error\"}");

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    /* default max_attempts == 3 → exactly 3 transport calls, 2 backoffs. */
    TEST_ASSERT_EQUAL_INT(3, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(2, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_INT(2, g_sleep.calls);
    /* default backoff base == 200 → first delay 200, second doubled to 400
     * (still < default max 2000). */
    TEST_ASSERT_EQUAL_UINT32(200, g_sleep.delays[0]);
    TEST_ASSERT_EQUAL_UINT32(400, g_sleep.delays[1]);
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 11. Disabled handle (enabled==false): still NO_ATTACH — the retry path is
 *     never engaged; the transport is never invoked (regression guard, #11).
 * ========================================================================== */

static void test_disabled_handle_no_attach_no_retry(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.enabled = false;
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    /* Even though a retriable reply is staged, a disabled handle never touches
     * the transport — NO_ATTACH (dev path), not ABORT, not retry. */
    stage(0, 0, 503, "{\"error\":\"server_error\"}");

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_NO_ATTACH,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);
    TEST_ASSERT_EQUAL_INT(0, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_dead_letter_count(ob));

    /* attach is likewise NO_ATTACH and leaves headers untouched (dev path). */
    struct curl_slist *headers = NULL;
    TEST_ASSERT_EQUAL_INT(
        NSSF_OAUTH2_NO_ATTACH,
        nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers));
    TEST_ASSERT_NULL(headers);
    TEST_ASSERT_EQUAL_INT(0, g_mock.calls);
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * 12. Secret hygiene — the dead-letter cb NEVER receives a token/secret. Even
 *     when the secret_ref is a recognizable sentinel and the response body
 *     carries an error, the recorded oauth_error is the error CODE only, with no
 *     bearer/secret substring in any recorded callback argument (case #12).
 * ========================================================================== */

static void test_dead_letter_cb_never_leaks_secret(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    /* Use a secret-auth handle so a (sentinel) secret_ref exists in the config;
     * the dead-letter cb must STILL never see it. */
    cfg.auth_method = NSSF_OAUTH2_AUTH_SECRET_BASIC;
    cfg.secret_ref = "SENTINEL-SECRET-VALUE";
    cfg.allow_secret_auth_in_production = true;
    cfg.client_cert_path = NULL;
    cfg.client_key_path = NULL;
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);
    nssf_oauth2_outbound_set_dead_letter_cb(ob, recording_dead_letter_cb, &g_dl);

    /* 401 with an OAuth2 error code in the body. */
    stage(0, 0, 401, "{\"error\":\"invalid_client\"}");

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    TEST_ASSERT_EQUAL_INT(1, g_dl.calls);
    TEST_ASSERT_EQUAL_INT(401, g_dl.http_status[0]);
    /* The recorded error is the error CODE only — exactly "invalid_client". */
    TEST_ASSERT_EQUAL_STRING("invalid_client", g_dl.oauth_error[0]);
    /* And contains NO trace of the secret sentinel (no secret leak). */
    TEST_ASSERT_NULL(strstr(g_dl.oauth_error[0], "SENTINEL"));
    TEST_ASSERT_NULL(strstr(g_dl.oauth_error[0], "SECRET"));
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * Extra: 200-with-error and 200-unusable-token are DEAD_LETTER (no retry) —
 * pins the "authoritative 200 that is not a usable grant" classification.
 * ========================================================================== */

static void test_200_with_error_is_dead_letter(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.max_attempts = 4;
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);
    nssf_oauth2_outbound_set_dead_letter_cb(ob, recording_dead_letter_cb, &g_dl);

    /* HTTP 200 but the body carries an OAuth2 error → not a usable grant. */
    stage(0, 0, 200, "{\"error\":\"invalid_scope\"}");

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);
    TEST_ASSERT_EQUAL_INT(1, g_mock.calls); /* no retry on an authoritative 200. */
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_dead_letter_count(ob));
    TEST_ASSERT_EQUAL_INT(1, g_dl.calls);
    TEST_ASSERT_EQUAL_INT(200, g_dl.http_status[0]);
    TEST_ASSERT_EQUAL_STRING("invalid_scope", g_dl.oauth_error[0]);
    nssf_oauth2_outbound_free(ob);
}

static void test_200_unusable_token_is_dead_letter(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.max_attempts = 4;
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);
    nssf_oauth2_outbound_set_dead_letter_cb(ob, recording_dead_letter_cb, &g_dl);

    /* HTTP 200 but token_type is not Bearer → unusable → DEAD_LETTER, no retry. */
    stage(0, 0, 200,
          "{\"access_token\":\"opaque\",\"token_type\":\"mac\","
          "\"expires_in\":300}");

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);
    TEST_ASSERT_EQUAL_INT(1, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(0, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_dead_letter_count(ob));
    /* No OAuth2 error code in this body → cb sees status + NULL error code. */
    TEST_ASSERT_EQUAL_INT(1, g_dl.calls);
    TEST_ASSERT_EQUAL_INT(200, g_dl.http_status[0]);
    TEST_ASSERT_EQUAL_STRING("", g_dl.oauth_error[0]); /* NULL → "" recorded. */
    nssf_oauth2_outbound_free(ob);
}

/* ============================================================================
 * Extra: retriable-then-dead-letter STOPS at the dead-letter (a 5xx that turns
 * into a 401 on retry must record the dead-letter and stop immediately). Proves
 * the dead-letter short-circuits a partially-consumed retry budget.
 * ========================================================================== */

static void test_retriable_then_dead_letter_stops(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.max_attempts = 5;
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);
    nssf_oauth2_outbound_set_dead_letter_cb(ob, recording_dead_letter_cb, &g_dl);

    stage(0, 0, 503, "{\"error\":\"server_error\"}"); /* attempt 1: retriable. */
    stage(1, 0, 403, "{\"error\":\"access_denied\"}"); /* attempt 2: dead-letter. */

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    /* Stopped after exactly 2 attempts (1 retriable + 1 dead-letter), 1 backoff
     * sleep before the dead-letter, then immediate fail-closed. */
    TEST_ASSERT_EQUAL_INT(2, g_mock.calls);
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_retry_count(ob));
    TEST_ASSERT_EQUAL_INT(1, g_sleep.calls);
    TEST_ASSERT_EQUAL_UINT32(1, nssf_oauth2_outbound_dead_letter_count(ob));
    TEST_ASSERT_EQUAL_INT(403, g_dl.http_status[0]);
    nssf_oauth2_outbound_free(ob);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_transport_error_then_success);            /* #1  */
    RUN_TEST(test_http_503_then_success);                   /* #2  */
    RUN_TEST(test_http_429_then_success);                   /* #3  */
    RUN_TEST(test_retry_exhaustion_fail_closed);            /* #4 + #8 */
    RUN_TEST(test_dead_letter_400_no_retry);                /* #5  */
    RUN_TEST(test_dead_letter_401_no_retry);                /* #5  */
    RUN_TEST(test_dead_letter_403_no_retry);                /* #5  */
    RUN_TEST(test_dead_letter_invalid_client);              /* #6  */
    RUN_TEST(test_dead_letter_invalid_scope);               /* #6  */
    RUN_TEST(test_exponential_capped_backoff_sequence);     /* #7  */
    RUN_TEST(test_fail_closed_no_bearer_header_on_dead_letter); /* #8 */
    RUN_TEST(test_call_driven_no_background_invocation);    /* #9  */
    RUN_TEST(test_default_config_applies_defaults);         /* #10 */
    RUN_TEST(test_disabled_handle_no_attach_no_retry);      /* #11 */
    RUN_TEST(test_dead_letter_cb_never_leaks_secret);       /* #12 */
    RUN_TEST(test_200_with_error_is_dead_letter);           /* extra classification */
    RUN_TEST(test_200_unusable_token_is_dead_letter);       /* extra classification */
    RUN_TEST(test_retriable_then_dead_letter_stops);        /* extra short-circuit */
    return UNITY_END();
}
