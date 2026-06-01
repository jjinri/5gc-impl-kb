/*
 * test_tls_context_handshake.c — WI-tls-bootstrap mTLS acceptance.
 *
 * 자급식 (self-contained) — fixture 파일 commit 없음. 테스트 런타임에 OpenSSL
 * EVP_PKEY/X509 API 로 in-test PKI 를 생성한다: CA, CA-서명 server cert+key,
 * CA-서명 client cert+key, 그리고 CA 에 서명되지 않은 self-signed untrusted
 * client cert+key. PEM 은 mkstemp 임시 파일로 써서 nssf_tls_config_t 경로가
 * 가리키게 하고 teardown 에서 unlink 한다.
 *
 * 핸드셰이크는 BIO pair (메모리) 로 구동한다 — 소켓·네트워크 없음. server SSL
 * 은 nssf_tls_context_ssl_ctx() 가 만든 SSL_CTX (SSL_VERIFY_PEER |
 * SSL_VERIFY_FAIL_IF_NO_PEER_CERT) 에서 파생.
 *
 *  PASS: CA-서명 client cert 제시 → 핸드셰이크 완료 + SSL_get_verify_result()
 *        == X509_V_OK (mTLS peer 검증 동작 증명).
 *  FAIL-closed (a): client 가 cert 미제시 → server 가 거절.
 *  FAIL-closed (b): client 가 untrusted self-signed cert 제시 → server 거절.
 *  FAIL-fast:       server_cert_path 가 없는 파일 → create == NULL + errbuf.
 */

#define _POSIX_C_SOURCE 200809L

#include "tls_context.h"
#include "unity.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---- in-test PKI material, generated once in main(), torn down at exit ---- */

typedef struct {
    EVP_PKEY *key;
    X509 *cert;
} keypair_t;

static keypair_t g_ca;            /* root CA (self-signed). */
static keypair_t g_server;        /* server leaf, signed by CA. */
static keypair_t g_client;        /* client leaf, signed by CA (trusted). */
static keypair_t g_rogue;         /* client leaf, self-signed (untrusted). */

/* Temp file paths the nssf_tls_config_t points at. */
static char g_ca_pem_path[]        = "/tmp/nssf_test_ca_XXXXXX";
static char g_server_cert_path[]   = "/tmp/nssf_test_srvcert_XXXXXX";
static char g_server_key_path[]    = "/tmp/nssf_test_srvkey_XXXXXX";

void setUp(void) {}
void tearDown(void) {}

static EVP_PKEY *gen_rsa_key(void)
{
    /* RSA 2048 via the high-level keygen API (OpenSSL 3.0). */
    EVP_PKEY *pkey = EVP_RSA_gen(2048);
    TEST_ASSERT_NOT_NULL_MESSAGE(pkey, "EVP_RSA_gen failed");
    return pkey;
}

static void set_name_entry(X509_NAME *name, const char *field, const char *val)
{
    int rc = X509_NAME_add_entry_by_txt(
        name, field, MBSTRING_ASC, (const unsigned char *)val, -1, -1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, rc, "X509_NAME_add_entry_by_txt failed");
}

/*
 * Build an X509 cert for subject_cn, signed by issuer_key with issuer_cert as
 * the issuer name (self-signed when issuer_cert == NULL → use the new cert as
 * its own issuer). is_ca controls the basicConstraints CA flag.
 */
static X509 *make_cert(EVP_PKEY *subject_key, const char *subject_cn,
                       EVP_PKEY *issuer_key, X509 *issuer_cert, bool is_ca)
{
    X509 *cert = X509_new();
    TEST_ASSERT_NOT_NULL(cert);
    TEST_ASSERT_EQUAL_INT(1, X509_set_version(cert, 2)); /* v3 */

    ASN1_INTEGER_set(X509_get_serialNumber(cert), (long)(rand() & 0x7fffffff));
    X509_gmtime_adj(X509_getm_notBefore(cert), 0);
    X509_gmtime_adj(X509_getm_notAfter(cert), 60L * 60L); /* 1h validity. */

    TEST_ASSERT_EQUAL_INT(1, X509_set_pubkey(cert, subject_key));

    X509_NAME *subj = X509_get_subject_name(cert);
    set_name_entry(subj, "CN", subject_cn);

    X509 *issuer_for_name = issuer_cert != NULL ? issuer_cert : cert;
    TEST_ASSERT_EQUAL_INT(
        1, X509_set_issuer_name(cert, X509_get_subject_name(issuer_for_name)));

    /* basicConstraints — CA:TRUE for the root, CA:FALSE for leaves. */
    X509V3_CTX v3ctx;
    X509V3_set_ctx_nodb(&v3ctx);
    X509V3_set_ctx(&v3ctx, issuer_for_name, cert, NULL, NULL, 0);
    X509_EXTENSION *bc = X509V3_EXT_conf_nid(
        NULL, &v3ctx, NID_basic_constraints,
        is_ca ? "critical,CA:TRUE" : "critical,CA:FALSE");
    TEST_ASSERT_NOT_NULL(bc);
    X509_add_ext(cert, bc, -1);
    X509_EXTENSION_free(bc);

    int rc = X509_sign(cert, issuer_key, EVP_sha256());
    TEST_ASSERT_TRUE_MESSAGE(rc > 0, "X509_sign failed");
    return cert;
}

static void write_cert_pem(const char *path, X509 *cert)
{
    FILE *fp = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL_MESSAGE(fp, "fopen cert pem for write failed");
    TEST_ASSERT_EQUAL_INT(1, PEM_write_X509(fp, cert));
    fclose(fp);
}

static void write_key_pem(const char *path, EVP_PKEY *key)
{
    FILE *fp = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL_MESSAGE(fp, "fopen key pem for write failed");
    TEST_ASSERT_EQUAL_INT(
        1, PEM_write_PrivateKey(fp, key, NULL, NULL, 0, NULL, NULL));
    fclose(fp);
}

static void mkstemp_path(char *template_path)
{
    int fd = mkstemp(template_path);
    TEST_ASSERT_TRUE_MESSAGE(fd >= 0, "mkstemp failed");
    close(fd);
}

/* ---- handshake driver over a BIO pair ---- */

/*
 * Drive a TLS handshake between a server SSL (from nssf ctx) and a client SSL
 * built here. client_key/client_cert may be NULL → client presents no cert.
 * Returns true iff BOTH sides report SSL_ERROR_NONE on completion. On true,
 * *out_verify gets the server-side SSL_get_verify_result().
 */
static bool drive_handshake(SSL_CTX *server_ctx, EVP_PKEY *client_key,
                            X509 *client_cert, long *out_verify)
{
    bool handshaked = false;
    SSL_CTX *client_ctx = SSL_CTX_new(TLS_client_method());
    TEST_ASSERT_NOT_NULL(client_ctx);
    /* Client does not verify the server here — we exercise SERVER-side mTLS. */
    SSL_CTX_set_verify(client_ctx, SSL_VERIFY_NONE, NULL);
    if (client_cert != NULL && client_key != NULL) {
        TEST_ASSERT_EQUAL_INT(1, SSL_CTX_use_certificate(client_ctx, client_cert));
        TEST_ASSERT_EQUAL_INT(1, SSL_CTX_use_PrivateKey(client_ctx, client_key));
    }

    SSL *server = SSL_new(server_ctx);
    SSL *client = SSL_new(client_ctx);
    TEST_ASSERT_NOT_NULL(server);
    TEST_ASSERT_NOT_NULL(client);

    /* Memory BIO pair connects the two endpoints — no sockets. */
    BIO *server_rbio = BIO_new(BIO_s_mem());
    BIO *server_wbio = BIO_new(BIO_s_mem());
    BIO *client_rbio = BIO_new(BIO_s_mem());
    BIO *client_wbio = BIO_new(BIO_s_mem());
    TEST_ASSERT_NOT_NULL(server_rbio);
    TEST_ASSERT_NOT_NULL(server_wbio);
    TEST_ASSERT_NOT_NULL(client_rbio);
    TEST_ASSERT_NOT_NULL(client_wbio);

    SSL_set_bio(server, server_rbio, server_wbio);
    SSL_set_bio(client, client_rbio, client_wbio);
    SSL_set_accept_state(server);
    SSL_set_connect_state(client);

    /*
     * Pump bytes between the two write/read mem BIOs until both complete or one
     * fails. A bounded loop guarantees termination even on a fail-closed path.
     */
    char buf[16384];
    bool server_done = false;
    bool client_done = false;
    bool failed = false;
    for (int round = 0; round < 64 && !failed; round++) {
        if (!client_done) {
            int r = SSL_do_handshake(client);
            if (r == 1) {
                client_done = true;
            } else {
                int e = SSL_get_error(client, r);
                if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE) {
                    failed = true;
                }
            }
        }
        if (!server_done && !failed) {
            int r = SSL_do_handshake(server);
            if (r == 1) {
                server_done = true;
            } else {
                int e = SSL_get_error(server, r);
                if (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE) {
                    failed = true;
                }
            }
        }

        /* client_wbio → server_rbio */
        int n;
        while ((n = BIO_read(client_wbio, buf, sizeof(buf))) > 0) {
            BIO_write(server_rbio, buf, n);
        }
        /* server_wbio → client_rbio */
        while ((n = BIO_read(server_wbio, buf, sizeof(buf))) > 0) {
            BIO_write(client_rbio, buf, n);
        }

        if (server_done && client_done) {
            break;
        }
    }

    if (!failed && server_done && client_done) {
        handshaked = true;
        if (out_verify != NULL) {
            *out_verify = SSL_get_verify_result(server);
        }
    }

    SSL_free(server);
    SSL_free(client);
    SSL_CTX_free(client_ctx);
    return handshaked;
}

/* ---- tests ---- */

static nssf_tls_context_t *make_valid_ctx(void)
{
    nssf_tls_config_t cfg = {
        .enabled = true,
        .server_cert_path = g_server_cert_path,
        .server_key_path = g_server_key_path,
        .client_ca_path = g_ca_pem_path,
        .min_version = "TLSv1.2", /* allow 1.2 so the in-mem pump converges. */
        .cipher_suites = NULL,
    };
    char errbuf[256] = {0};
    nssf_tls_context_t *ctx = nssf_tls_context_create(&cfg, errbuf, sizeof(errbuf));
    TEST_ASSERT_NOT_NULL_MESSAGE(ctx, errbuf);
    return ctx;
}

static void test_create_succeeds_and_reports_enabled(void)
{
    nssf_tls_context_t *ctx = make_valid_ctx();
    TEST_ASSERT_NOT_NULL(nssf_tls_context_ssl_ctx(ctx));
    TEST_ASSERT_TRUE(nssf_tls_context_is_enabled(ctx));
    nssf_tls_context_free(ctx);
}

static void test_mtls_trusted_client_handshake_ok(void)
{
    nssf_tls_context_t *ctx = make_valid_ctx();
    SSL_CTX *server_ctx = nssf_tls_context_ssl_ctx(ctx);
    TEST_ASSERT_NOT_NULL(server_ctx);

    long verify = -1;
    bool ok = drive_handshake(server_ctx, g_client.key, g_client.cert, &verify);
    TEST_ASSERT_TRUE_MESSAGE(
        ok, "mTLS handshake with CA-signed client cert must complete");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        X509_V_OK, (int)verify,
        "server-side peer verification must be X509_V_OK");

    nssf_tls_context_free(ctx);
}

static void test_mtls_no_client_cert_rejected(void)
{
    nssf_tls_context_t *ctx = make_valid_ctx();
    SSL_CTX *server_ctx = nssf_tls_context_ssl_ctx(ctx);

    /* No client cert → SSL_VERIFY_FAIL_IF_NO_PEER_CERT must fail the server. */
    bool ok = drive_handshake(server_ctx, NULL, NULL, NULL);
    TEST_ASSERT_FALSE_MESSAGE(
        ok, "server must reject handshake when client presents no cert");

    nssf_tls_context_free(ctx);
}

static void test_mtls_untrusted_client_cert_rejected(void)
{
    nssf_tls_context_t *ctx = make_valid_ctx();
    SSL_CTX *server_ctx = nssf_tls_context_ssl_ctx(ctx);

    /* Self-signed (not CA-signed) client cert → server must reject. */
    bool ok = drive_handshake(server_ctx, g_rogue.key, g_rogue.cert, NULL);
    TEST_ASSERT_FALSE_MESSAGE(
        ok, "server must reject handshake with untrusted self-signed client cert");

    nssf_tls_context_free(ctx);
}

static void test_create_fails_fast_on_missing_cert(void)
{
    nssf_tls_config_t cfg = {
        .enabled = true,
        .server_cert_path = "/nonexistent/path/does/not/exist.pem",
        .server_key_path = g_server_key_path,
        .client_ca_path = g_ca_pem_path,
        .min_version = NULL,
        .cipher_suites = NULL,
    };
    char errbuf[256] = {0};
    nssf_tls_context_t *ctx = nssf_tls_context_create(&cfg, errbuf, sizeof(errbuf));
    TEST_ASSERT_NULL_MESSAGE(
        ctx, "create must fail when server_cert_path does not exist");
    TEST_ASSERT_TRUE_MESSAGE(errbuf[0] != '\0',
                             "create must write a diagnostic into errbuf");
}

/* ---- PKI setup / teardown around the whole suite ---- */

static void pki_setup(void)
{
    g_ca.key = gen_rsa_key();
    g_ca.cert = make_cert(g_ca.key, "nssf-test-ca", g_ca.key, NULL, true);

    g_server.key = gen_rsa_key();
    g_server.cert =
        make_cert(g_server.key, "nssf-server", g_ca.key, g_ca.cert, false);

    g_client.key = gen_rsa_key();
    g_client.cert =
        make_cert(g_client.key, "nssf-client", g_ca.key, g_ca.cert, false);

    /* Untrusted: self-signed, NOT chained to the test CA. */
    g_rogue.key = gen_rsa_key();
    g_rogue.cert =
        make_cert(g_rogue.key, "rogue-client", g_rogue.key, NULL, false);

    mkstemp_path(g_ca_pem_path);
    mkstemp_path(g_server_cert_path);
    mkstemp_path(g_server_key_path);
    write_cert_pem(g_ca_pem_path, g_ca.cert);
    write_cert_pem(g_server_cert_path, g_server.cert);
    write_key_pem(g_server_key_path, g_server.key);
}

static void pki_teardown(void)
{
    unlink(g_ca_pem_path);
    unlink(g_server_cert_path);
    unlink(g_server_key_path);
    X509_free(g_ca.cert);
    EVP_PKEY_free(g_ca.key);
    X509_free(g_server.cert);
    EVP_PKEY_free(g_server.key);
    X509_free(g_client.cert);
    EVP_PKEY_free(g_client.key);
    X509_free(g_rogue.cert);
    EVP_PKEY_free(g_rogue.key);
}

int main(void)
{
    pki_setup();

    UNITY_BEGIN();
    RUN_TEST(test_create_succeeds_and_reports_enabled);
    RUN_TEST(test_mtls_trusted_client_handshake_ok);
    RUN_TEST(test_mtls_no_client_cert_rejected);
    RUN_TEST(test_mtls_untrusted_client_cert_rejected);
    RUN_TEST(test_create_fails_fast_on_missing_cert);
    int rc = UNITY_END();

    pki_teardown();
    return rc;
}
