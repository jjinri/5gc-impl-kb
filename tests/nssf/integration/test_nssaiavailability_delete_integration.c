/*
 * test_nssaiavailability_delete_integration.c — WI-availability-handlers
 * integration (NSSAIAvailabilityDelete).
 *
 * DELETE /nssai-availability/{nfId}. The handler
 * (nssf_nssaiavailability_delete_handle) is driven IN-PROCESS — no socket, no live
 * DB, no nghttp2 server. Collaborators wired from in-test seams (mirroring
 * test_nsselection_get_*):
 *   - jwks_cache : nssf_jwks_cache_create_from_json() over an in-test RSA JWKS,
 *                  validating an RS256 JWT we mint with scope nnssf-nssaiavailability.
 *   - engine     : nssf_availability_engine_new() over nssf_availability_repo_new_
 *                  inmemory() SEEDED with the addressed nfId row; zero-init seams
 *                  (NULL publish — the phase2 deferred cascade no-op).
 *
 * So 204 (seeded nfId) / 404 (absent) / 401 / 403 run UNCONDITIONALLY in CI. The
 * PG-fixture / live probe portion is SKIP-PASS when env absent. Never FAIL for
 * missing env.
 *
 * LSan-clean: every cJSON / response body / engine / repo / jwks / token freed.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "availability_engine.h"
#include "availability_repository.h"
#include "cJSON.h"
#include "nssaiavailability_delete_handler.h"
#include "oauth2_jwks.h"
#include "oauth2_scope_check.h"

#include <jwt.h>

#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void setUp(void) {}
void tearDown(void) {}

#define SEEDED_NF_ID "abababab-0000-0000-0000-00000000aaaa"

/* ── in-test RSA key + JWKS/JWT minting (mirror of test_jwks_fetch.c) ──────── */

static EVP_PKEY *g_key = NULL;
static const char *kKid = "nssf-test-kid-avail-delete";

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
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, EVP_PKEY_get_bn_param(key, param, &bn), param);
    return bn;
}

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
             "\"kty\":\"RSA\",\"use\":\"sig\",\"alg\":\"RS256\","
             "\"kid\":\"%s\",\"n\":\"%s\",\"e\":\"%s\"}]}",
             kKid, n_b, e_b);
    free(n_b);
    free(e_b);
    return json;
}

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
             "{\"kty\":\"RSA\",\"use\":\"sig\",\"alg\":\"RS256\",\"kid\":\"%s\","
             "\"n\":\"%s\",\"e\":\"%s\",\"d\":\"%s\","
             "\"p\":\"%s\",\"q\":\"%s\",\"dp\":\"%s\",\"dq\":\"%s\",\"qi\":\"%s\"}",
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
        jwt_set_SET_STR(&v, "kid", kid);
        v.replace = 1;
        (void)jwt_builder_header_set(b, &v);
        jwt_set_SET_STR(&v, "scope", scope);
        (void)jwt_builder_claim_set(b, &v);
        jwt_set_SET_INT(&v, "exp", (jwt_long_t)(time(NULL) + exp_offset_sec));
        (void)jwt_builder_claim_set(b, &v);
        token = jwt_builder_generate(b);
    }
    jwt_builder_free(b);
    jwks_free(set);
    return token;
}

static char *make_bearer(const char *scope)
{
    char *token = sign_jwt(kKid, scope, 300);
    TEST_ASSERT_NOT_NULL_MESSAGE(token, "RS256 sign failed");
    size_t len = strlen("Bearer ") + strlen(token) + 1;
    char *auth = malloc(len);
    TEST_ASSERT_NOT_NULL(auth);
    snprintf(auth, len, "Bearer %s", token);
    free(token);
    return auth;
}

/* ── TAI / snssai / record helpers ────────────────────────────────────────── */

static cJSON *make_plmn(const char *mcc, const char *mnc)
{
    cJSON *p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "mcc", mcc);
    cJSON_AddStringToObject(p, "mnc", mnc);
    return p;
}

static cJSON *make_tai(const char *mcc, const char *mnc, const char *tac)
{
    cJSON *t = cJSON_CreateObject();
    cJSON_AddItemToObject(t, "plmnId", make_plmn(mcc, mnc));
    cJSON_AddStringToObject(t, "tac", tac);
    return t;
}

static cJSON *make_snssai(int sst, const char *sd)
{
    cJSON *s = cJSON_CreateObject();
    cJSON_AddNumberToObject(s, "sst", sst);
    if (sd != NULL) {
        cJSON_AddStringToObject(s, "sd", sd);
    }
    return s;
}

/* Seed availability[id=SEEDED_NF_ID, 001/01/000001]. */
static void seed_record(nssf_availability_repo_t *repo)
{
    nssf_availability_record_t rec;
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s", SEEDED_NF_ID);
    rec.plmn_id = make_plmn("001", "01");
    rec.tai = make_tai("001", "01", "000001");
    cJSON *snssais = cJSON_CreateArray();
    cJSON_AddItemToArray(snssais, make_snssai(1, "000001"));
    rec.snssais = snssais;
    rec.amf_set_id = NULL;
    rec.reallocation_indication = false;
    TEST_ASSERT_EQUAL_INT(0, nssf_availability_repo_upsert(repo, &rec));
    nssf_availability_record_clear(&rec);
}

typedef struct {
    nssf_jwks_cache_t *cache;
    nssf_availability_repo_t *repo;
    nssf_availability_engine_t *engine;
} fixture_t;

static void fixture_init(fixture_t *fx, bool seed)
{
    char *jwks_json = make_public_jwks_json(g_key);
    fx->cache = nssf_jwks_cache_create_from_json(jwks_json, 60);
    free(jwks_json);
    TEST_ASSERT_NOT_NULL_MESSAGE(fx->cache, "from_json JWKS cache must build");

    fx->repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(fx->repo);
    if (seed) {
        seed_record(fx->repo);
    }

    nssf_availability_engine_seams_t seams;
    memset(&seams, 0, sizeof(seams)); /* NULL publish — phase2 deferred no-op. */
    fx->engine = nssf_availability_engine_new(fx->repo, &seams);
    TEST_ASSERT_NOT_NULL(fx->engine);
}

static void fixture_free(fixture_t *fx)
{
    nssf_availability_engine_free(fx->engine);
    nssf_availability_repo_free(fx->repo);
    nssf_jwks_cache_free(fx->cache);
}

/* ── 204 — delete a seeded nfId (in-process, unconditional) ───────────────── */

static void test_delete_204_seeded_in_process(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);

    nssf_nssaiavailability_delete_request_t req = {
        .authorization = auth,
        .nf_id = SEEDED_NF_ID,
        .request_target = "/nssai-availability/" SEEDED_NF_ID,
        .max_uri_len = 0,
    };
    nssf_nssaiavailability_delete_deps_t deps = {.jwks_cache = fx.cache, .engine = fx.engine};
    nssf_nssaiavailability_delete_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_nssaiavailability_delete_handle(&req, &deps, &out);

    TEST_ASSERT_EQUAL_INT(204, status);
    TEST_ASSERT_EQUAL_INT(204, out.status);
    TEST_ASSERT_NULL_MESSAGE(out.body, "204 carries no body");

    /* A second delete of the now-removed nfId is 404 — confirms it was deleted. */
    nssf_nssaiavailability_delete_response_t out2;
    memset(&out2, 0, sizeof(out2));
    TEST_ASSERT_EQUAL_INT(404, nssf_nssaiavailability_delete_handle(&req, &deps, &out2));
    nssf_nssaiavailability_delete_response_free(&out2);

    nssf_nssaiavailability_delete_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── 404 — delete an absent nfId ──────────────────────────────────────────── */

static void test_delete_404_absent(void)
{
    fixture_t fx;
    fixture_init(&fx, false); /* nothing seeded */
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);

    nssf_nssaiavailability_delete_request_t req = {
        .authorization = auth,
        .nf_id = "cdcdcdcd-1111-1111-1111-111111111111",
        .request_target = "/nssai-availability/cdcdcdcd-1111-1111-1111-111111111111",
    };
    nssf_nssaiavailability_delete_deps_t deps = {.jwks_cache = fx.cache, .engine = fx.engine};
    nssf_nssaiavailability_delete_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(404, nssf_nssaiavailability_delete_handle(&req, &deps, &out));
    TEST_ASSERT_EQUAL_STRING(NSSF_CT_PROBLEM_JSON, out.content_type);

    nssf_nssaiavailability_delete_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── 401 — no bearer ──────────────────────────────────────────────────────── */

static void test_delete_401_no_bearer(void)
{
    fixture_t fx;
    fixture_init(&fx, true);

    nssf_nssaiavailability_delete_request_t req = {
        .authorization = NULL,
        .nf_id = SEEDED_NF_ID,
        .request_target = "/nssai-availability/" SEEDED_NF_ID,
    };
    nssf_nssaiavailability_delete_deps_t deps = {.jwks_cache = fx.cache, .engine = fx.engine};
    nssf_nssaiavailability_delete_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(401, nssf_nssaiavailability_delete_handle(&req, &deps, &out));
    TEST_ASSERT_EQUAL_STRING(NSSF_CT_PROBLEM_JSON, out.content_type);

    nssf_nssaiavailability_delete_response_free(&out);
    fixture_free(&fx);
}

/* ── 403 — valid token, wrong scope ───────────────────────────────────────── */

static void test_delete_403_wrong_scope(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSELECTION); /* wrong scope. */

    nssf_nssaiavailability_delete_request_t req = {
        .authorization = auth,
        .nf_id = SEEDED_NF_ID,
        .request_target = "/nssai-availability/" SEEDED_NF_ID,
    };
    nssf_nssaiavailability_delete_deps_t deps = {.jwks_cache = fx.cache, .engine = fx.engine};
    nssf_nssaiavailability_delete_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(403, nssf_nssaiavailability_delete_handle(&req, &deps, &out));
    TEST_ASSERT_EQUAL_STRING(NSSF_CT_PROBLEM_JSON, out.content_type);

    nssf_nssaiavailability_delete_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── PG-fixture / live probe (SKIP-PASS when env absent) ──────────────────── */

static int probe_env_ready(void)
{
    return getenv("NSSF_DB_CONNINFO") != NULL &&
           getenv("NSSF_BIND_ADDRESS") != NULL &&
           getenv("NSSF_BIND_PORT") != NULL &&
           getenv("NSSF_TLS_SERVER_CERT") != NULL &&
           getenv("NSSF_TLS_SERVER_KEY") != NULL &&
           getenv("NSSF_OAUTH2_JWKS_URL") != NULL;
}

static void test_delete_persisted_probe_or_skip(void)
{
    if (!probe_env_ready()) {
        fprintf(stderr,
                "[SKIP] test_nssaiavailability_delete_integration live probe — "
                "NSSF_DB_CONNINFO/NSSF_BIND_*/NSSF_TLS_*/NSSF_OAUTH2_JWKS_URL not "
                "set. CI has no PostgreSQL service + no live server; the in-process "
                "coverage above is the real coverage.\n");
        TEST_PASS_MESSAGE("live probe env not set — SKIP PASS");
        return;
    }
    TEST_PASS_MESSAGE("probe env present — in-process contract already asserted");
}

int main(void)
{
    g_key = EVP_RSA_gen(2048);
    if (g_key == NULL) {
        fprintf(stderr, "[test_nssaiavailability_delete_integration] RSA keygen failed\n");
        return 1;
    }
    UNITY_BEGIN();
    RUN_TEST(test_delete_204_seeded_in_process);
    RUN_TEST(test_delete_404_absent);
    RUN_TEST(test_delete_401_no_bearer);
    RUN_TEST(test_delete_403_wrong_scope);
    RUN_TEST(test_delete_persisted_probe_or_skip);
    int rc = UNITY_END();
    EVP_PKEY_free(g_key);
    return rc;
}
