/*
 * test_oauth2_outbound_attach.c — WI-outbound-oauth2 (ADR-0004 M4) unit suite.
 *
 * Outbound OAuth2 client_credentials token acquire + bearer attach, driven
 * ENTIRELY through the mock token-transport seam (nssf_oauth2_outbound_set_
 * transport). No network, no DB, no live NRF — runs unconditionally in CI.
 *
 * The mock NRF token endpoint is a captured-context transport: each invocation
 * records the form_body / basic_auth / token_url it received and replies with a
 * caller-staged body + HTTP status (or a forced transport error). This lets the
 * suite assert the 12 ratified decisions purely from observable behavior:
 *
 *   happy acquire + attach (Authorization: Bearer <token>)        — #1
 *   TTL cache reuse + refresh-buffer + server-TTL cap ≤ max_ttl    — #6
 *   token_type must be Bearer; missing access_token → fail-closed  — #10
 *   JWS exp already past → not cached / fail-closed                — #10
 *   non-200 / transport error while enabled → ABORT (no attach)    — #7,#11
 *   enabled==false → NO_ATTACH (dev path), never ABORT             — #7
 *   default scope present; per-call scope_override changes request — #4
 *   https-only ctor reject (http / '@' / '#'); valid https → ok    — #9
 *   mTLS + secret_ref simultaneously → create NULL                 — #12
 *   secret-auth production gate; SECRET_BASIC via HTTP Basic       — #3,#2
 */

#define _POSIX_C_SOURCE 200809L

#include "oauth2_outbound.h"
#include "unity.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- base64url (no padding), for synthesizing a JWS payload ---- */

static char *b64url(const unsigned char *in, size_t len)
{
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    char *out = malloc(4 * ((len + 2) / 3) + 1);
    TEST_ASSERT_NOT_NULL(out);
    size_t o = 0;
    size_t i = 0;
    while (i + 3 <= len) {
        unsigned v = (unsigned)in[i] << 16 | (unsigned)in[i + 1] << 8 | in[i + 2];
        out[o++] = tbl[(v >> 18) & 0x3f];
        out[o++] = tbl[(v >> 12) & 0x3f];
        out[o++] = tbl[(v >> 6) & 0x3f];
        out[o++] = tbl[v & 0x3f];
        i += 3;
    }
    if (len - i == 1) {
        unsigned v = (unsigned)in[i] << 16;
        out[o++] = tbl[(v >> 18) & 0x3f];
        out[o++] = tbl[(v >> 12) & 0x3f];
    } else if (len - i == 2) {
        unsigned v = (unsigned)in[i] << 16 | (unsigned)in[i + 1] << 8;
        out[o++] = tbl[(v >> 18) & 0x3f];
        out[o++] = tbl[(v >> 12) & 0x3f];
        out[o++] = tbl[(v >> 6) & 0x3f];
    }
    out[o] = '\0';
    return out;
}

/*
 * Build a 3-part JWS "header.payload.signature" whose payload JSON carries the
 * given exp (seconds since epoch). header/signature are opaque placeholders —
 * the production exp parse is a non-crypto base64url+JSON read (decision 10),
 * so the signature is never verified here.
 */
static char *make_jws_with_exp(long long exp)
{
    char payload[96];
    int pn = snprintf(payload, sizeof(payload), "{\"exp\":%lld}", exp);
    char *enc = b64url((const unsigned char *)payload, (size_t)pn);
    /* fixed dummy header/sig segments (must be base64url-decodable for payload
     * only — header/sig are not decoded by jws_exp). */
    char *jws = malloc(strlen("eyJhbGciOiJSUzI1NiJ9.") + strlen(enc) +
                       strlen(".c2ln") + 1);
    TEST_ASSERT_NOT_NULL(jws);
    sprintf(jws, "eyJhbGciOiJSUzI1NiJ9.%s.c2ln", enc);
    free(enc);
    return jws;
}

/* ---- mock NRF token transport (captured context) ---- */

typedef struct {
    int calls;                /* number of transport invocations. */
    char last_form_body[512]; /* captured form_body of the last call. */
    char last_basic_auth[256];/* captured basic_auth ("" when NULL). */
    char last_token_url[256]; /* captured token_url. */

    /* staged reply (consumed each call). */
    const char *reply_body;   /* body JSON to return; transport copies it. */
    long reply_status;        /* HTTP status to report. */
    int force_error;          /* nonzero → simulate transport/TLS failure. */
} mock_t;

static int mock_transport(const nssf_oauth2_outbound_t *ob, const char *token_url,
                          const char *form_body, const char *basic_auth,
                          char **out_body, long *out_http_status, void *ctx)
{
    (void)ob;
    mock_t *m = (mock_t *)ctx;
    m->calls++;
    snprintf(m->last_form_body, sizeof(m->last_form_body), "%s",
             form_body != NULL ? form_body : "");
    snprintf(m->last_basic_auth, sizeof(m->last_basic_auth), "%s",
             basic_auth != NULL ? basic_auth : "");
    snprintf(m->last_token_url, sizeof(m->last_token_url), "%s",
             token_url != NULL ? token_url : "");

    if (m->force_error) {
        return -1; /* network/TLS/timeout failure → fail-closed. */
    }
    /* duplicate the staged body (production frees *out_body). */
    *out_body = m->reply_body != NULL ? strdup(m->reply_body) : NULL;
    *out_http_status = m->reply_status;
    return 0;
}

/* ---- config + handle helpers ---- */

static mock_t g_mock;

void setUp(void)
{
    memset(&g_mock, 0, sizeof(g_mock));
}
void tearDown(void) {}

static nssf_oauth2_outbound_config_t base_cfg(void)
{
    nssf_oauth2_outbound_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.token_url = "https://nrf.example.com:443/oauth2/token";
    cfg.nf_instance_id = "nssf-0a1b2c3d-instance";
    cfg.auth_method = NSSF_OAUTH2_AUTH_MTLS;
    cfg.default_scope = "nnrf-disc nnrf-nfm";
    /* numeric fields left 0 → decision defaults (refresh 60 / max_ttl 3600). */
    return cfg;
}

static nssf_oauth2_outbound_t *make_handle(const nssf_oauth2_outbound_config_t *cfg)
{
    nssf_oauth2_outbound_t *ob = nssf_oauth2_outbound_create(cfg);
    TEST_ASSERT_NOT_NULL(ob);
    nssf_oauth2_outbound_set_transport(ob, mock_transport, &g_mock);
    return ob;
}

/* ============================ acquire happy (#1) ============================ */

static void test_acquire_happy_and_attach_bearer(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"opaque-bearer-aaa\",\"token_type\":\"Bearer\","
        "\"expires_in\":300}";

    const char *bearer = NULL;
    nssf_oauth2_outbound_status_t st =
        nssf_oauth2_outbound_acquire(ob, NULL, &bearer);
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK, st);
    TEST_ASSERT_NOT_NULL(bearer);
    TEST_ASSERT_EQUAL_STRING("opaque-bearer-aaa", bearer);
    TEST_ASSERT_EQUAL_INT(1, g_mock.calls);

    /* attach appends exactly "Authorization: Bearer <token>". */
    struct curl_slist *headers = NULL;
    nssf_oauth2_outbound_status_t ast =
        nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers);
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK, ast);
    TEST_ASSERT_NOT_NULL(headers);
    bool found = false;
    for (struct curl_slist *h = headers; h != NULL; h = h->next) {
        if (strcmp(h->data, "Authorization: Bearer opaque-bearer-aaa") == 0) {
            found = true;
        }
    }
    TEST_ASSERT_TRUE(found);
    curl_slist_free_all(headers);
    nssf_oauth2_outbound_free(ob);
}

/* ====================== TTL cache + refresh-buffer (#6) ===================== */

static void test_ttl_cache_reuse_within_ttl(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"tok-1\",\"token_type\":\"Bearer\","
        "\"expires_in\":300}"; /* well outside the 60s refresh buffer. */

    const char *b1 = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &b1));
    const char *b2 = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &b2));
    /* second acquire reused the cache — transport called exactly once. */
    TEST_ASSERT_EQUAL_INT(1, g_mock.calls);
    TEST_ASSERT_EQUAL_STRING("tok-1", b2);
    nssf_oauth2_outbound_free(ob);
}

static void test_refresh_buffer_triggers_refresh_near_expiry(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.refresh_buffer_seconds = 60;
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    /* expires_in within the refresh buffer → cache is "stale on arrival": the
     * NEXT acquire must refresh (cached_token_fresh = now+60 < now+30 → false). */
    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"tok-near\",\"token_type\":\"Bearer\","
        "\"expires_in\":30}";
    const char *b1 = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &b1));
    TEST_ASSERT_EQUAL_INT(1, g_mock.calls);

    g_mock.reply_body =
        "{\"access_token\":\"tok-fresh\",\"token_type\":\"Bearer\","
        "\"expires_in\":300}";
    const char *b2 = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &b2));
    /* within-buffer token forced a refresh on the second call. */
    TEST_ASSERT_EQUAL_INT(2, g_mock.calls);
    TEST_ASSERT_EQUAL_STRING("tok-fresh", b2);
    nssf_oauth2_outbound_free(ob);
}

static void test_server_ttl_capped_to_max_ttl(void)
{
    /*
     * Server advertises expires_in=7200 but max_ttl caps it. With max_ttl=100
     * and refresh_buffer=200, freshness = now+200 < cached_expiry. If capped
     * (cached_expiry=now+100) → not fresh → refresh on 2nd acquire (calls==2).
     * If NOT capped (cached_expiry=now+7200) → fresh → reuse (calls==1).
     * calls==2 proves the cap was applied (decision 6).
     */
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.max_ttl_seconds = 100;
    cfg.refresh_buffer_seconds = 200;
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"tok-cap\",\"token_type\":\"Bearer\","
        "\"expires_in\":7200}";
    const char *b1 = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &b1));
    const char *b2 = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &b2));
    TEST_ASSERT_EQUAL_INT(2, g_mock.calls);
    nssf_oauth2_outbound_free(ob);
}

/* ======================= token_type must be Bearer (#10) =================== */

static void test_token_type_not_bearer_fail_closed(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"opaque\",\"token_type\":\"mac\","
        "\"expires_in\":300}";
    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    /* attach must NOT append anything on fail-closed. */
    struct curl_slist *headers = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers));
    TEST_ASSERT_NULL(headers);
    nssf_oauth2_outbound_free(ob);
}

static void test_token_type_missing_fail_closed(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"opaque\",\"expires_in\":300}"; /* no token_type. */
    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);
    nssf_oauth2_outbound_free(ob);
}

/* ===================== missing access_token (#10) ========================== */

static void test_missing_access_token_fail_closed(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"token_type\":\"Bearer\",\"expires_in\":300}"; /* no access_token. */
    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);
    nssf_oauth2_outbound_free(ob);
}

/* ========================= JWS exp already past (#10) ====================== */

static void test_jws_exp_past_not_cached(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    /* exp well beyond the 60s clock-skew leeway in the past. */
    char *jws = make_jws_with_exp((long long)time(NULL) - 3600);
    char body[512];
    snprintf(body, sizeof(body),
             "{\"access_token\":\"%s\",\"token_type\":\"Bearer\","
             "\"expires_in\":300}",
             jws);
    g_mock.reply_status = 200;
    g_mock.reply_body = body;

    const char *bearer = NULL;
    /* JWS exp in the past → resolve_expiry returns false → not cached. */
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);
    free(jws);
    nssf_oauth2_outbound_free(ob);
}

static void test_jws_exp_future_is_cached(void)
{
    /* control: a future-exp JWS DOES cache (proves the past-exp reject above is
     * about exp, not about JWS shape). */
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    char *jws = make_jws_with_exp((long long)time(NULL) + 600);
    char body[512];
    snprintf(body, sizeof(body),
             "{\"access_token\":\"%s\",\"token_type\":\"Bearer\","
             "\"expires_in\":300}",
             jws);
    g_mock.reply_status = 200;
    g_mock.reply_body = body;

    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NOT_NULL(bearer);
    free(jws);
    nssf_oauth2_outbound_free(ob);
}

/* =============== fail-closed on transport / non-200 (#7,#11) =============== */

static void test_non200_status_fail_closed(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 500;
    g_mock.reply_body = "{\"error\":\"server_error\"}";
    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);

    /* never an unauthenticated attach. */
    struct curl_slist *headers = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers));
    TEST_ASSERT_NULL(headers);
    nssf_oauth2_outbound_free(ob);
}

static void test_transport_error_fail_closed(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.force_error = 1; /* simulate network/TLS failure. */
    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer);
    nssf_oauth2_outbound_free(ob);
}

/* ========================= disabled → NO_ATTACH (#7) ====================== */

static void test_disabled_no_attach_no_abort(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.enabled = false; /* dev no-attach path. */
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    const char *bearer = NULL;
    nssf_oauth2_outbound_status_t st =
        nssf_oauth2_outbound_acquire(ob, NULL, &bearer);
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_NO_ATTACH, st);
    TEST_ASSERT_NULL(bearer);
    /* transport never invoked when disabled. */
    TEST_ASSERT_EQUAL_INT(0, g_mock.calls);

    /* attach returns NO_ATTACH and leaves headers untouched (caller may emit
     * without a bearer — dev path). */
    struct curl_slist *headers = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_NO_ATTACH,
                          nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers));
    TEST_ASSERT_NULL(headers);
    nssf_oauth2_outbound_free(ob);
}

/* ============================ scope override (#4) ========================== */

static void test_default_scope_in_form_body(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"tok\",\"token_type\":\"Bearer\","
        "\"expires_in\":300}";
    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));

    /* grant_type=client_credentials (decision 1) + url-encoded default scope
     * "nnrf-disc nnrf-nfm" (space → %20). */
    TEST_ASSERT_NOT_NULL(strstr(g_mock.last_form_body,
                                "grant_type=client_credentials"));
    TEST_ASSERT_NOT_NULL(
        strstr(g_mock.last_form_body, "scope=nnrf-disc%20nnrf-nfm"));
    nssf_oauth2_outbound_free(ob);
}

static void test_scope_override_changes_request(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"tok-ov\",\"token_type\":\"Bearer\","
        "\"expires_in\":300}";
    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(
        NSSF_OAUTH2_OK,
        nssf_oauth2_outbound_acquire(ob, "nnrf-disc", &bearer));

    /* per-call override is the requested scope, NOT the default. */
    TEST_ASSERT_NOT_NULL(strstr(g_mock.last_form_body, "scope=nnrf-disc"));
    TEST_ASSERT_NULL(
        strstr(g_mock.last_form_body, "scope=nnrf-disc%20nnrf-nfm"));
    nssf_oauth2_outbound_free(ob);
}

/* ========================== https-only reject (#9) ======================== */

static void test_create_rejects_http_scheme(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.token_url = "http://nrf.example.com/oauth2/token";
    TEST_ASSERT_NULL(nssf_oauth2_outbound_create(&cfg));
}

static void test_create_rejects_userinfo_at(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.token_url = "https://user@nrf.example.com/oauth2/token";
    TEST_ASSERT_NULL(nssf_oauth2_outbound_create(&cfg));
}

static void test_create_rejects_fragment_hash(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.token_url = "https://nrf.example.com/oauth2/token#frag";
    TEST_ASSERT_NULL(nssf_oauth2_outbound_create(&cfg));
}

static void test_create_accepts_valid_https(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.token_url = "https://nrf.example.com:443/oauth2/token";
    nssf_oauth2_outbound_t *ob = nssf_oauth2_outbound_create(&cfg);
    TEST_ASSERT_NOT_NULL(ob);
    nssf_oauth2_outbound_free(ob);
}

/* ========================= mTLS + secret reject (#12) ===================== */

static void test_create_rejects_mtls_with_secret(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.auth_method = NSSF_OAUTH2_AUTH_MTLS;
    cfg.secret_ref = "vault://nssf/nrf-client-secret"; /* incoherent with mTLS. */
    TEST_ASSERT_NULL(nssf_oauth2_outbound_create(&cfg));
}

/* ====================== secret-auth production gate (#3) ================== */

static void test_secret_auth_rejected_when_production_forbids(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.auth_method = NSSF_OAUTH2_AUTH_SECRET_BASIC;
    cfg.secret_ref = "vault://nssf/nrf-client-secret";
    cfg.allow_secret_auth_in_production = false; /* production forbids. */
    TEST_ASSERT_NULL(nssf_oauth2_outbound_create(&cfg));
}

static void test_secret_basic_uses_http_basic_not_body(void)
{
    nssf_oauth2_outbound_config_t cfg = base_cfg();
    cfg.auth_method = NSSF_OAUTH2_AUTH_SECRET_BASIC;
    cfg.secret_ref = "s3cr3t-ref";
    cfg.allow_secret_auth_in_production = true; /* explicit opt-in. */
    nssf_oauth2_outbound_t *ob = make_handle(&cfg);

    g_mock.reply_status = 200;
    g_mock.reply_body =
        "{\"access_token\":\"tok\",\"token_type\":\"Bearer\","
        "\"expires_in\":300}";
    const char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));

    /* basic_auth = "<nf_instance_id>:<secret_ref>" handed to the transport
     * (HTTP Basic), and the secret is NEVER in the form body (decision 3). */
    TEST_ASSERT_EQUAL_STRING("nssf-0a1b2c3d-instance:s3cr3t-ref",
                             g_mock.last_basic_auth);
    TEST_ASSERT_NULL(strstr(g_mock.last_form_body, "s3cr3t-ref"));
    TEST_ASSERT_NULL(strstr(g_mock.last_form_body, "client_secret"));
    nssf_oauth2_outbound_free(ob);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_acquire_happy_and_attach_bearer);
    RUN_TEST(test_ttl_cache_reuse_within_ttl);
    RUN_TEST(test_refresh_buffer_triggers_refresh_near_expiry);
    RUN_TEST(test_server_ttl_capped_to_max_ttl);
    RUN_TEST(test_token_type_not_bearer_fail_closed);
    RUN_TEST(test_token_type_missing_fail_closed);
    RUN_TEST(test_missing_access_token_fail_closed);
    RUN_TEST(test_jws_exp_past_not_cached);
    RUN_TEST(test_jws_exp_future_is_cached);
    RUN_TEST(test_non200_status_fail_closed);
    RUN_TEST(test_transport_error_fail_closed);
    RUN_TEST(test_disabled_no_attach_no_abort);
    RUN_TEST(test_default_scope_in_form_body);
    RUN_TEST(test_scope_override_changes_request);
    RUN_TEST(test_create_rejects_http_scheme);
    RUN_TEST(test_create_rejects_userinfo_at);
    RUN_TEST(test_create_rejects_fragment_hash);
    RUN_TEST(test_create_accepts_valid_https);
    RUN_TEST(test_create_rejects_mtls_with_secret);
    RUN_TEST(test_secret_auth_rejected_when_production_forbids);
    RUN_TEST(test_secret_basic_uses_http_basic_not_body);
    return UNITY_END();
}
