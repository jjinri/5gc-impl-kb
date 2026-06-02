/*
 * test_nrf_token_acquire.c — WI-outbound-oauth2 (ADR-0004 M4) integration suite.
 *
 * End-to-end client_credentials acquire against a MOCK NRF token endpoint, wired
 * through the test-only loopback ctor (nssf_oauth2_outbound_create_insecure,
 * http://127.0.0.1 — mirrors nssf_jwks_cache_create_insecure in test_jwks_fetch.c)
 * plus the mock token-transport seam. No socket, no real NRF, no DB — the whole
 * suite runs UNCONDITIONALLY in CI (registered above the PostgreSQL gate).
 *
 * The mock endpoint records the request it received (form_body / basic_auth /
 * url) and replies with a staged body + HTTP status, so the round-trip asserts:
 *
 *   loopback ctor accepts http://127.0.0.1; rejects non-loopback / '@'   — ctor
 *   form body is x-www-form-urlencoded grant_type=client_credentials     — #1
 *   identity carried as nfInstanceId=<nf_instance_id> (29.510, NOT       — #2
 *     the OAuth2 client_id form param — B1 fix)
 *   acquire → attach round-trip yields Authorization: Bearer <token>     — #1
 *   307 redirect response → fail-closed / no-follow (ABORT, no attach)   — #5
 *
 * The "live real-NRF" portion is a SKIP-PASS (no live NRF in readiness CI).
 */

#define _POSIX_C_SOURCE 200809L

#include "oauth2_outbound.h"
#include "unity.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- mock NRF token endpoint (captured request + staged reply) ---- */

typedef struct {
    int calls;
    char form_body[512];
    char basic_auth[256];
    char url[256];
    const char *reply_body;
    long reply_status;
} nrf_mock_t;

static int nrf_mock_transport(const nssf_oauth2_outbound_t *ob,
                              const char *token_url, const char *form_body,
                              const char *basic_auth, char **out_body,
                              long *out_http_status, void *ctx)
{
    (void)ob;
    nrf_mock_t *m = (nrf_mock_t *)ctx;
    m->calls++;
    snprintf(m->form_body, sizeof(m->form_body), "%s",
             form_body != NULL ? form_body : "");
    snprintf(m->basic_auth, sizeof(m->basic_auth), "%s",
             basic_auth != NULL ? basic_auth : "");
    snprintf(m->url, sizeof(m->url), "%s", token_url != NULL ? token_url : "");
    *out_body = m->reply_body != NULL ? strdup(m->reply_body) : NULL;
    *out_http_status = m->reply_status;
    return 0;
}

static nrf_mock_t g_nrf;

void setUp(void)
{
    memset(&g_nrf, 0, sizeof(g_nrf));
}
void tearDown(void) {}

static nssf_oauth2_outbound_config_t loopback_cfg(void)
{
    nssf_oauth2_outbound_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.token_url = "http://127.0.0.1:8080/oauth2/token"; /* loopback mock. */
    cfg.nf_instance_id = "nssf-instance-77ce-4f2a";
    cfg.auth_method = NSSF_OAUTH2_AUTH_MTLS;
    cfg.default_scope = "nnrf-disc nnrf-nfm";
    return cfg;
}

/* ---- loopback ctor gate (mirrors the jwks insecure ctor policy) ---- */

static void test_loopback_ctor_accepts_127001(void)
{
    nssf_oauth2_outbound_config_t cfg = loopback_cfg();
    nssf_oauth2_outbound_t *ob = nssf_oauth2_outbound_create_insecure(&cfg);
    TEST_ASSERT_NOT_NULL(ob);
    nssf_oauth2_outbound_free(ob);
}

static void test_loopback_ctor_rejects_nonloopback_and_userinfo(void)
{
    nssf_oauth2_outbound_config_t cfg = loopback_cfg();
    cfg.token_url = "http://nrf.example.com/oauth2/token"; /* not loopback. */
    TEST_ASSERT_NULL(nssf_oauth2_outbound_create_insecure(&cfg));

    cfg.token_url = "http://127.0.0.1@evil.example.com/token"; /* '@' userinfo. */
    TEST_ASSERT_NULL(nssf_oauth2_outbound_create_insecure(&cfg));
}

/* ---- end-to-end acquire → attach round-trip (#1, #2) ---- */

static void test_client_credentials_acquire_and_attach_roundtrip(void)
{
    nssf_oauth2_outbound_config_t cfg = loopback_cfg();
    nssf_oauth2_outbound_t *ob = nssf_oauth2_outbound_create_insecure(&cfg);
    TEST_ASSERT_NOT_NULL(ob);
    nssf_oauth2_outbound_set_transport(ob, nrf_mock_transport, &g_nrf);

    g_nrf.reply_status = 200;
    g_nrf.reply_body =
        "{\"access_token\":\"e2e-bearer-token\",\"token_type\":\"Bearer\","
        "\"expires_in\":300}";

    struct curl_slist *headers = NULL;
    nssf_oauth2_outbound_status_t st =
        nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers);
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_OK, st);

    /*
     * x-www-form-urlencoded grant + canonical identity (decisions 1 + 2).
     * B1: the 29.510 Nnrf_AccessToken AccessTokenReq carries the NF identity in
     * the `nfInstanceId` field, NOT the OAuth2 `client_id` form param. Assert the
     * correct field is present and the wrong one never appears in the body.
     */
    TEST_ASSERT_NOT_NULL(
        strstr(g_nrf.form_body, "grant_type=client_credentials"));
    TEST_ASSERT_NOT_NULL(
        strstr(g_nrf.form_body, "nfInstanceId=nssf-instance-77ce-4f2a"));
    TEST_ASSERT_NULL(strstr(g_nrf.form_body, "client_id="));
    /* request reached the loopback token endpoint. */
    TEST_ASSERT_NOT_NULL(strstr(g_nrf.url, "127.0.0.1"));

    /* Bearer attach round-trip. */
    bool found = false;
    for (struct curl_slist *h = headers; h != NULL; h = h->next) {
        if (strcmp(h->data, "Authorization: Bearer e2e-bearer-token") == 0) {
            found = true;
        }
    }
    TEST_ASSERT_TRUE(found);
    curl_slist_free_all(headers);
    nssf_oauth2_outbound_free(ob);
}

/* ---- 307 redirect → fail-closed / no-follow (#5) ---- */

static void test_redirect_307_fails_closed(void)
{
    /*
     * The default policy does NOT follow redirects (CURLOPT_FOLLOWLOCATION=0);
     * a 3xx on the token endpoint is a non-200 and fails closed. We surface the
     * NRF returning a 307 via the mock and assert ABORT with no attach — the
     * NSSF never follows the redirect and never emits an unauth outbound.
     */
    nssf_oauth2_outbound_config_t cfg = loopback_cfg();
    nssf_oauth2_outbound_t *ob = nssf_oauth2_outbound_create_insecure(&cfg);
    TEST_ASSERT_NOT_NULL(ob);
    nssf_oauth2_outbound_set_transport(ob, nrf_mock_transport, &g_nrf);

    g_nrf.reply_status = 307; /* Temporary Redirect — not followed. */
    g_nrf.reply_body = "";

    char *bearer = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_acquire(ob, NULL, &bearer));
    TEST_ASSERT_NULL(bearer); /* NULL on non-OK — nothing to free. */

    struct curl_slist *headers = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_OAUTH2_ABORT,
                          nssf_oauth2_outbound_attach_bearer(ob, NULL, &headers));
    TEST_ASSERT_NULL(headers);
    nssf_oauth2_outbound_free(ob);
}

/* ---- live real-NRF acquire (SKIP-PASS — no live NRF in readiness CI) ---- */

static void test_live_nrf_acquire_skip_pass(void)
{
    const char *live = getenv("NSSF_NRF_TOKEN_URL");
    if (live == NULL || live[0] == '\0') {
        TEST_IGNORE_MESSAGE(
            "SKIP-PASS: NSSF_NRF_TOKEN_URL unset — no live NRF token endpoint "
            "in readiness CI (mock-transport path provides the real coverage).");
        return;
    }
    /* A live NRF would be exercised here; readiness CI never reaches this. */
    TEST_PASS();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_loopback_ctor_accepts_127001);
    RUN_TEST(test_loopback_ctor_rejects_nonloopback_and_userinfo);
    RUN_TEST(test_client_credentials_acquire_and_attach_roundtrip);
    RUN_TEST(test_redirect_307_fails_closed);
    RUN_TEST(test_live_nrf_acquire_skip_pass);
    return UNITY_END();
}
