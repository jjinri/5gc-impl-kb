/*
 * test_jwks_fetch.c — WI-tls-bootstrap inbound OAuth2 JWKS fetch acceptance.
 *
 * In-process mock JWKS server. 테스트 런타임에 RSA 키쌍을 생성하고, public key
 * 를 RS256 JWK (kid + n + e + alg) 로 직렬화한 JWKS JSON 을 127.0.0.1 의
 * ephemeral 포트에 bind 된 최소 HTTP/1.1 서버 (pthread) 가 /jwks 에서 plain
 * http 로 제공한다. libjwt jwks_load_fromurl (libcurl) 이 http://127.0.0.1:PORT
 * 에서 가져온다.
 *
 *  fetch path: nssf_jwks_cache_create(url) → nssf_jwks_cache_refresh →
 *              keyring populated.
 *  valid:      private key 로 RS256 서명한 JWT (matching kid, 유효 exp, scope
 *              "nnssf-nsselection") → nssf_oauth2_validate_bearer == 0,
 *              out_scope == "nnssf-nsselection".
 *  invalid:    (a) 서명 tamper, (b) wrong-kid, (c) expired exp → validate != 0
 *              (signature + exp 강제 증명).
 *
 * HTTP bind 가 진짜로 실패하는 경우에만 SKIP — 그 경우 명시적으로 출력한다.
 */

#define _POSIX_C_SOURCE 200809L

#include "oauth2_jwks.h"
#include "oauth2_scope_check.h"
#include "unity.h"

#include <jwt.h>

#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* ---- base64url (no padding) of a binary buffer ---- */

static char *b64url(const unsigned char *in, size_t len)
{
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    char *out = malloc(4 * ((len + 2) / 3) + 1);
    if (out == NULL) {
        return NULL;
    }
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

/* base64url of a BIGNUM (big-endian, minimal length). */
static char *b64url_bn(const BIGNUM *bn)
{
    int n = BN_num_bytes(bn);
    unsigned char *buf = malloc((size_t)(n > 0 ? n : 1));
    TEST_ASSERT_NOT_NULL(buf);
    int wrote = BN_bn2bin(bn, buf);
    char *s = b64url(buf, (size_t)wrote);
    free(buf);
    return s;
}

static BIGNUM *get_bn(EVP_PKEY *key, const char *param)
{
    BIGNUM *bn = NULL;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, EVP_PKEY_get_bn_param(key, param, &bn),
                                  param);
    return bn;
}

/* ---- key material + JWK serialisation ---- */

static EVP_PKEY *g_key = NULL;
static const char *kKid = "nssf-test-kid-1";

/* Build the *public* JWKS JSON that the mock server serves over HTTP. */
static char *make_public_jwks_json(EVP_PKEY *key)
{
    BIGNUM *n = get_bn(key, OSSL_PKEY_PARAM_RSA_N);
    BIGNUM *e = get_bn(key, OSSL_PKEY_PARAM_RSA_E);
    char *n_b = b64url_bn(n);
    char *e_b = b64url_bn(e);
    BN_free(n);
    BN_free(e);

    char *json = malloc(4096);
    TEST_ASSERT_NOT_NULL(json);
    snprintf(json, 4096,
             "{\"keys\":[{"
             "\"kty\":\"RSA\","
             "\"use\":\"sig\","
             "\"alg\":\"RS256\","
             "\"kid\":\"%s\","
             "\"n\":\"%s\","
             "\"e\":\"%s\""
             "}]}",
             kKid, n_b, e_b);
    free(n_b);
    free(e_b);
    return json;
}

/* Build a *private* single-JWK JSON for signing via the libjwt builder. */
static char *make_private_jwk_json(EVP_PKEY *key)
{
    BIGNUM *n = get_bn(key, OSSL_PKEY_PARAM_RSA_N);
    BIGNUM *e = get_bn(key, OSSL_PKEY_PARAM_RSA_E);
    BIGNUM *d = get_bn(key, OSSL_PKEY_PARAM_RSA_D);
    BIGNUM *p = get_bn(key, OSSL_PKEY_PARAM_RSA_FACTOR1);
    BIGNUM *q = get_bn(key, OSSL_PKEY_PARAM_RSA_FACTOR2);
    BIGNUM *dp = get_bn(key, OSSL_PKEY_PARAM_RSA_EXPONENT1);
    BIGNUM *dq = get_bn(key, OSSL_PKEY_PARAM_RSA_EXPONENT2);
    BIGNUM *qi = get_bn(key, OSSL_PKEY_PARAM_RSA_COEFFICIENT1);

    char *n_b = b64url_bn(n);
    char *e_b = b64url_bn(e);
    char *d_b = b64url_bn(d);
    char *p_b = b64url_bn(p);
    char *q_b = b64url_bn(q);
    char *dp_b = b64url_bn(dp);
    char *dq_b = b64url_bn(dq);
    char *qi_b = b64url_bn(qi);
    BN_free(n);
    BN_free(e);
    BN_free(d);
    BN_free(p);
    BN_free(q);
    BN_free(dp);
    BN_free(dq);
    BN_free(qi);

    char *json = malloc(8192);
    TEST_ASSERT_NOT_NULL(json);
    snprintf(json, 8192,
             "{"
             "\"kty\":\"RSA\","
             "\"use\":\"sig\","
             "\"alg\":\"RS256\","
             "\"kid\":\"%s\","
             "\"n\":\"%s\",\"e\":\"%s\",\"d\":\"%s\","
             "\"p\":\"%s\",\"q\":\"%s\","
             "\"dp\":\"%s\",\"dq\":\"%s\",\"qi\":\"%s\""
             "}",
             kKid, n_b, e_b, d_b, p_b, q_b, dp_b, dq_b, qi_b);
    free(n_b);
    free(e_b);
    free(d_b);
    free(p_b);
    free(q_b);
    free(dp_b);
    free(dq_b);
    free(qi_b);
    return json;
}

/*
 * Sign an RS256 JWT with our private key, carrying the given kid (so we can
 * exercise the wrong-kid path) and exp offset (so we can exercise expiry).
 * scope is set as a string claim. Returns malloc'd token (caller frees) or NULL.
 */
static char *sign_jwt(const char *kid, const char *scope, time_t exp_offset_sec)
{
    char *jwk_json = make_private_jwk_json(g_key);
    jwk_set_t *set = jwks_create(jwk_json);
    free(jwk_json);
    if (set == NULL || jwks_error(set)) {
        if (set != NULL) {
            jwks_free(set);
        }
        return NULL;
    }
    const jwk_item_t *item = jwks_item_get(set, 0);
    if (item == NULL) {
        jwks_free(set);
        return NULL;
    }

    jwt_builder_t *b = jwt_builder_new();
    if (b == NULL) {
        jwks_free(set);
        return NULL;
    }

    char *token = NULL;
    if (jwt_builder_setkey(b, JWT_ALG_RS256, item) == 0) {
        jwt_value_t v;
        /* kid header — production verify_cb resolves the key strictly by kid. */
        jwt_set_SET_STR(&v, "kid", kid);
        v.replace = 1;
        (void)jwt_builder_header_set(b, &v);

        jwt_set_SET_STR(&v, "scope", scope);
        (void)jwt_builder_claim_set(b, &v);

        /* exp = now + offset (negative offset → already expired). */
        jwt_set_SET_INT(&v, "exp", (jwt_long_t)(time(NULL) + exp_offset_sec));
        (void)jwt_builder_claim_set(b, &v);

        token = jwt_builder_generate(b);
    }

    jwt_builder_free(b);
    jwks_free(set);
    return token;
}

/* ---- minimal in-process HTTP/1.1 JWKS server on 127.0.0.1:0 ---- */

typedef struct {
    int listen_fd;
    int port;
    char *jwks_body;
    pthread_t thread;
    bool running;
} mock_server_t;

static mock_server_t g_srv = {.listen_fd = -1, .port = 0, .jwks_body = NULL,
                              .running = false};

static void *server_thread(void *arg)
{
    mock_server_t *s = (mock_server_t *)arg;
    for (;;) {
        int fd = accept(s->listen_fd, NULL, NULL);
        if (fd < 0) {
            break; /* listen socket closed at teardown → accept fails → exit. */
        }
        /* Drain the request (we serve the same body for any path). */
        char reqbuf[2048];
        (void)recv(fd, reqbuf, sizeof(reqbuf), 0);

        char header[256];
        int hlen = snprintf(header, sizeof(header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %zu\r\n"
                            "Connection: close\r\n\r\n",
                            strlen(s->jwks_body));
        /* Best-effort writes; the client (libcurl) reads until close. */
        ssize_t w = send(fd, header, (size_t)hlen, MSG_NOSIGNAL);
        if (w > 0) {
            send(fd, s->jwks_body, strlen(s->jwks_body), MSG_NOSIGNAL);
        }
        close(fd);
    }
    return NULL;
}

/* Returns true if the server bound + started; false (with reason) on bind fail. */
static bool mock_server_start(mock_server_t *s, char *jwks_body)
{
    s->jwks_body = jwks_body;
    s->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->listen_fd < 0) {
        return false;
    }
    int one = 1;
    setsockopt(s->listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* 127.0.0.1 */
    addr.sin_port = 0;                             /* ephemeral. */
    if (bind(s->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(s->listen_fd);
        s->listen_fd = -1;
        return false;
    }
    socklen_t alen = sizeof(addr);
    if (getsockname(s->listen_fd, (struct sockaddr *)&addr, &alen) != 0) {
        close(s->listen_fd);
        s->listen_fd = -1;
        return false;
    }
    s->port = ntohs(addr.sin_port);
    if (listen(s->listen_fd, 8) != 0) {
        close(s->listen_fd);
        s->listen_fd = -1;
        return false;
    }
    /* Socket is bound + listening BEFORE we signal readiness via return. The
     * accept thread can lag, but connect() will queue in the listen backlog. */
    if (pthread_create(&s->thread, NULL, server_thread, s) != 0) {
        close(s->listen_fd);
        s->listen_fd = -1;
        return false;
    }
    s->running = true;
    return true;
}

static void mock_server_stop(mock_server_t *s)
{
    if (!s->running) {
        return;
    }
    /* shutdown + close unblocks accept() → thread returns → join. */
    shutdown(s->listen_fd, SHUT_RDWR);
    close(s->listen_fd);
    pthread_join(s->thread, NULL);
    s->listen_fd = -1;
    s->running = false;
}

void setUp(void) {}
void tearDown(void) {}

/* ---- tests ---- */

static char g_url[128];

static void test_fetch_populates_keyring(void)
{
    if (!g_srv.running) {
        TEST_FAIL_MESSAGE("mock JWKS server failed to bind 127.0.0.1 — see SKIP");
        return;
    }
    nssf_jwks_cache_t *cache = nssf_jwks_cache_create(g_url, 60);
    TEST_ASSERT_NOT_NULL(cache);

    int rc = nssf_jwks_cache_refresh(cache);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "refresh must fetch JWKS over http");

    nssf_jwks_cache_free(cache);
}

static void test_validate_valid_token(void)
{
    if (!g_srv.running) {
        TEST_FAIL_MESSAGE("mock JWKS server unavailable");
        return;
    }
    nssf_jwks_cache_t *cache = nssf_jwks_cache_create(g_url, 60);
    TEST_ASSERT_NOT_NULL(cache);

    char *token = sign_jwt(kKid, NSSF_SCOPE_NSSELECTION, 300);
    TEST_ASSERT_NOT_NULL_MESSAGE(token, "RS256 sign failed");

    char *scope = NULL;
    char errbuf[256] = {0};
    int rc = nssf_oauth2_validate_bearer(cache, token, &scope, errbuf,
                                         sizeof(errbuf));
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, errbuf);
    TEST_ASSERT_NOT_NULL(scope);
    TEST_ASSERT_EQUAL_STRING(NSSF_SCOPE_NSSELECTION, scope);
    /* The captured scope claim must satisfy the per-operation scope check. */
    TEST_ASSERT_TRUE(nssf_oauth2_scope_match(scope, NSSF_SCOPE_NSSELECTION));

    free(scope);
    free(token);
    nssf_jwks_cache_free(cache);
}

static void test_validate_tampered_signature_rejected(void)
{
    if (!g_srv.running) {
        TEST_FAIL_MESSAGE("mock JWKS server unavailable");
        return;
    }
    nssf_jwks_cache_t *cache = nssf_jwks_cache_create(g_url, 60);
    TEST_ASSERT_NOT_NULL(cache);

    char *token = sign_jwt(kKid, NSSF_SCOPE_NSSELECTION, 300);
    TEST_ASSERT_NOT_NULL(token);
    /*
     * Tamper the signature deterministically. The JWT signature is the third
     * dot-separated segment (after the second '.'). We must NOT flip the LAST
     * base64url char: an RS256 signature is 256 bytes → 342 base64url chars,
     * and the final char only carries 2 meaningful signature bits + 4 padding
     * bits, so a low-bit flip there is often discarded by the decoder → the
     * 256-byte signature stays identical → verify still passes (flaky leak).
     *
     * Instead we perturb several chars well inside the signature segment, where
     * every base64url char encodes 6 fully-meaningful signature bits. Changing
     * those alters the decoded signature bytes, so RSA verify deterministically
     * fails on every run.
     */
    char *sig = strrchr(token, '.'); /* points at the second '.' */
    TEST_ASSERT_NOT_NULL_MESSAGE(sig, "JWT must have a signature segment");
    sig += 1; /* first char of the signature segment */
    size_t sig_len = strlen(sig);
    /* Need enough room that the perturbed indices are away from padding tail. */
    TEST_ASSERT_TRUE_MESSAGE(sig_len > 40,
                             "RS256 signature segment unexpectedly short");
    /* Flip a spread of middle-of-signature chars to a different base64url char. */
    const size_t kOffsets[] = {10, 20, 30};
    for (size_t i = 0; i < sizeof(kOffsets) / sizeof(kOffsets[0]); i++) {
        char *p = &sig[kOffsets[i]];
        *p = (*p == 'A') ? 'B' : 'A';
    }

    char *scope = NULL;
    char errbuf[256] = {0};
    int rc = nssf_oauth2_validate_bearer(cache, token, &scope, errbuf,
                                         sizeof(errbuf));
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, rc,
                                  "tampered signature must be rejected");
    TEST_ASSERT_NULL_MESSAGE(scope, "no scope must leak on rejection");

    free(token);
    nssf_jwks_cache_free(cache);
}

static void test_validate_wrong_kid_rejected(void)
{
    if (!g_srv.running) {
        TEST_FAIL_MESSAGE("mock JWKS server unavailable");
        return;
    }
    nssf_jwks_cache_t *cache = nssf_jwks_cache_create(g_url, 60);
    TEST_ASSERT_NOT_NULL(cache);

    /* Correct signature but a kid the keyring does not contain → no key found. */
    char *token = sign_jwt("unknown-kid", NSSF_SCOPE_NSSELECTION, 300);
    TEST_ASSERT_NOT_NULL(token);

    char *scope = NULL;
    char errbuf[256] = {0};
    int rc = nssf_oauth2_validate_bearer(cache, token, &scope, errbuf,
                                         sizeof(errbuf));
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, rc, "unknown kid must be rejected");

    free(token);
    nssf_jwks_cache_free(cache);
}

static void test_validate_expired_token_rejected(void)
{
    if (!g_srv.running) {
        TEST_FAIL_MESSAGE("mock JWKS server unavailable");
        return;
    }
    nssf_jwks_cache_t *cache = nssf_jwks_cache_create(g_url, 60);
    TEST_ASSERT_NOT_NULL(cache);

    /* exp set 600s in the past → expired. */
    char *token = sign_jwt(kKid, NSSF_SCOPE_NSSELECTION, -600);
    TEST_ASSERT_NOT_NULL(token);

    char *scope = NULL;
    char errbuf[256] = {0};
    int rc = nssf_oauth2_validate_bearer(cache, token, &scope, errbuf,
                                         sizeof(errbuf));
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, rc, "expired exp must be rejected");

    free(token);
    nssf_jwks_cache_free(cache);
}

int main(void)
{
    g_key = EVP_RSA_gen(2048);
    if (g_key == NULL) {
        fprintf(stderr, "[test_jwks_fetch] RSA keygen failed\n");
        return 1;
    }

    char *jwks_json = make_public_jwks_json(g_key);
    if (!mock_server_start(&g_srv, jwks_json)) {
        fprintf(stderr,
                "[SKIP] test_jwks_fetch — could not bind 127.0.0.1 mock JWKS "
                "server (sandboxed network?). Fetch path not exercised.\n");
        /* Genuine bind failure → do not fail the suite, but make it explicit. */
        free(jwks_json);
        EVP_PKEY_free(g_key);
        return 0;
    }
    snprintf(g_url, sizeof(g_url), "http://127.0.0.1:%d/jwks", g_srv.port);
    fprintf(stderr, "[test_jwks_fetch] mock JWKS server at %s\n", g_url);

    UNITY_BEGIN();
    RUN_TEST(test_fetch_populates_keyring);
    RUN_TEST(test_validate_valid_token);
    RUN_TEST(test_validate_tampered_signature_rejected);
    RUN_TEST(test_validate_wrong_kid_rejected);
    RUN_TEST(test_validate_expired_token_rejected);
    int rc = UNITY_END();

    mock_server_stop(&g_srv);
    free(jwks_json);
    EVP_PKEY_free(g_key);
    return rc;
}
