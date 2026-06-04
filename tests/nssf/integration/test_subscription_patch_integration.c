/*
 * test_subscription_patch_integration.c — WI-subscription-handlers integration
 * (NSSAIAvailabilitySubModifyPatch).
 *
 * ctest target name (verification-matrix regex):
 *     nssf_subscription_submodifypatch_integration
 *
 * PATCH /nssai-availability/subscriptions/{subscriptionId}. The handler
 * (nssf_subscription_patch_handle) is driven IN-PROCESS — no socket, no live DB,
 * no nghttp2 server. The collaborators are wired from in-test seams:
 *   - jwks_cache : nssf_jwks_cache_create_from_json() over an in-test RSA JWKS,
 *                  validating an RS256 JWT we mint with scope nnssf-nssaiavailability.
 *   - store      : nssf_subscription_store_new_inmemory() — the in-memory test seam.
 *
 * Required behaviors run UNCONDITIONALLY in CI:
 *   - valid RFC 6902 patch + content-type application/json-patch+json → 200 OK,
 *     mutation applied (the patched callbackUri is read back from the store).
 *   - non-application/json-patch+json content-type → 415.
 *   - absent id → 404.
 *   - malformed patch (invalid RFC 6902 op) → 400.
 *
 * The existing row is created through the store's own create() (no snapshot seam
 * → persist + skip dispatch) so the patch has a real persisted target.
 *
 * LSan-clean: every cJSON / response body / store / jwks / token freed.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "cJSON.h"
#include "oauth2_jwks.h"
#include "oauth2_scope_check.h"
#include "subscription_patch_handler.h"
#include "subscription_store.h"

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

#define NSSF_CT_JSON_PATCH "application/json-patch+json"

/* ── in-test RSA key + JWKS/JWT minting ─────────────────────────────────────── */

static EVP_PKEY *g_key = NULL;
static const char *kKid = "nssf-test-kid-sub-patch";

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

/* ── fixture: jwks cache + in-memory subscription store ─────────────────────── */

typedef struct {
    nssf_jwks_cache_t *cache;
    nssf_subscription_store_t *store;
} fixture_t;

static void fixture_init(fixture_t *fx)
{
    char *jwks_json = make_public_jwks_json(g_key);
    fx->cache = nssf_jwks_cache_create_from_json(jwks_json, 60);
    free(jwks_json);
    TEST_ASSERT_NOT_NULL_MESSAGE(fx->cache, "from_json JWKS cache must build");

    fx->store = nssf_subscription_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(fx->store);
}

static void fixture_free(fixture_t *fx)
{
    nssf_subscription_store_free(fx->store);
    nssf_jwks_cache_free(fx->cache);
}

static cJSON *make_filter(void)
{
    cJSON *f = cJSON_CreateObject();
    cJSON *plmn = cJSON_CreateObject();
    cJSON_AddStringToObject(plmn, "mcc", "001");
    cJSON_AddStringToObject(plmn, "mnc", "01");
    cJSON_AddItemToObject(f, "plmnId", plmn);
    return f;
}

/* Persist a subscription (no snapshot seam → no dispatch), id via out_id. */
static void seed_subscription(nssf_subscription_store_t *store,
                              const char *callback, char out_id[37])
{
    cJSON *filter = make_filter();
    nssf_sub_result_e r = nssf_subscription_store_create(
        store, callback, filter, NSSF_SUBSCRIPTION_DEFAULT_EXPIRY_SECONDS,
        out_id);
    cJSON_Delete(filter);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NSSF_SUB_OK, r, "seed create failed");
    TEST_ASSERT_EQUAL_size_t(36, strlen(out_id));
}

/* ── 200 — valid RFC 6902 replace callbackUri → mutation applied ────────────── */

static void test_patch_200_replace_callback_applied(void)
{
    fixture_t fx;
    fixture_init(&fx);

    char id[37];
    seed_subscription(fx.store, "https://amf.example.com/old", id);

    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    /* RFC 6902 op array: replace /callbackUri with a new https value. */
    const char *patch =
        "[{\"op\":\"replace\",\"path\":\"/callbackUri\","
        "\"value\":\"https://amf.example.com/new\"}]";
    char target[96];
    snprintf(target, sizeof(target), "/nssai-availability/subscriptions/%s", id);

    nssf_subscription_patch_request_t req = {
        .authorization = auth,
        .subscription_id = id,
        .content_type = NSSF_CT_JSON_PATCH,
        .body = patch,
        .content_length = strlen(patch),
        .has_content_length = true,
        .max_body_len = 0,
        .request_target = target,
        .max_uri_len = 0,
    };
    nssf_subscription_patch_deps_t deps = {.jwks_cache = fx.cache,
                                           .store = fx.store};
    nssf_subscription_patch_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_subscription_patch_handle(&req, &deps, &out);

    TEST_ASSERT_EQUAL_INT_MESSAGE(200, status,
                                  out.body ? out.body : "(null body)");
    TEST_ASSERT_EQUAL_INT(200, out.status);

    /* mutation applied — read the patched callbackUri back from the store. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_get(fx.store, id, &rec),
        "patched subscription must still exist");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "https://amf.example.com/new", rec.callback_uri,
        "RFC 6902 replace /callbackUri not applied to the stored row");
    nssf_subscription_record_clear(&rec);

    nssf_subscription_patch_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── 415 — wrong content-type (not application/json-patch+json) ──────────────── */

static void test_patch_415_wrong_content_type(void)
{
    fixture_t fx;
    fixture_init(&fx);

    char id[37];
    seed_subscription(fx.store, "https://amf.example.com/old", id);

    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    const char *patch =
        "[{\"op\":\"replace\",\"path\":\"/callbackUri\","
        "\"value\":\"https://amf.example.com/new\"}]";

    nssf_subscription_patch_request_t req = {
        .authorization = auth,
        .subscription_id = id,
        .content_type = "application/json", /* NOT json-patch+json → 415 */
        .body = patch,
        .content_length = strlen(patch),
        .has_content_length = true,
        .request_target = "/nssai-availability/subscriptions/x",
    };
    nssf_subscription_patch_deps_t deps = {.jwks_cache = fx.cache,
                                           .store = fx.store};
    nssf_subscription_patch_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_subscription_patch_handle(&req, &deps, &out);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        415, status,
        "non-application/json-patch+json content-type must be 415");

    /* the row must be unchanged (no mutation under a 415). */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(fx.store, id, &rec));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("https://amf.example.com/old",
                                     rec.callback_uri,
                                     "415 must not have mutated the row");
    nssf_subscription_record_clear(&rec);

    nssf_subscription_patch_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── 404 — absent subscription id ───────────────────────────────────────────── */

static void test_patch_404_absent_id(void)
{
    fixture_t fx;
    fixture_init(&fx);

    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    const char *absent = "deadbeef-0000-4000-8000-000000000000";
    const char *patch =
        "[{\"op\":\"replace\",\"path\":\"/callbackUri\","
        "\"value\":\"https://amf.example.com/new\"}]";
    char target[96];
    snprintf(target, sizeof(target), "/nssai-availability/subscriptions/%s",
             absent);

    nssf_subscription_patch_request_t req = {
        .authorization = auth,
        .subscription_id = absent,
        .content_type = NSSF_CT_JSON_PATCH,
        .body = patch,
        .content_length = strlen(patch),
        .has_content_length = true,
        .request_target = target,
    };
    nssf_subscription_patch_deps_t deps = {.jwks_cache = fx.cache,
                                           .store = fx.store};
    nssf_subscription_patch_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_subscription_patch_handle(&req, &deps, &out);
    TEST_ASSERT_EQUAL_INT_MESSAGE(404, status,
                                  "patching an absent subscription must be 404");

    nssf_subscription_patch_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── 400 — malformed patch (invalid RFC 6902 op) ────────────────────────────── */

static void test_patch_400_malformed_patch(void)
{
    fixture_t fx;
    fixture_init(&fx);

    char id[37];
    seed_subscription(fx.store, "https://amf.example.com/old", id);

    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    /* an invalid RFC 6902 op ("frobnicate" is not a JSON Patch operation). */
    const char *bad =
        "[{\"op\":\"frobnicate\",\"path\":\"/callbackUri\","
        "\"value\":\"https://amf.example.com/new\"}]";
    char target[96];
    snprintf(target, sizeof(target), "/nssai-availability/subscriptions/%s", id);

    nssf_subscription_patch_request_t req = {
        .authorization = auth,
        .subscription_id = id,
        .content_type = NSSF_CT_JSON_PATCH,
        .body = bad,
        .content_length = strlen(bad),
        .has_content_length = true,
        .request_target = target,
    };
    nssf_subscription_patch_deps_t deps = {.jwks_cache = fx.cache,
                                           .store = fx.store};
    nssf_subscription_patch_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_subscription_patch_handle(&req, &deps, &out);
    TEST_ASSERT_EQUAL_INT_MESSAGE(400, status,
                                  "an invalid RFC 6902 op must be 400");
    TEST_ASSERT_EQUAL_STRING(NSSF_CT_PROBLEM_JSON, out.content_type);

    /* atomic — the bad patch left the row unchanged. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT(NSSF_SUB_OK,
                          nssf_subscription_store_get(fx.store, id, &rec));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("https://amf.example.com/old",
                                     rec.callback_uri,
                                     "400 (bad patch) must not have mutated the row");
    nssf_subscription_record_clear(&rec);

    nssf_subscription_patch_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── 401 — no bearer (security gate runs BEFORE business logic) ──────────────── */

static void test_patch_401_no_bearer(void)
{
    fixture_t fx;
    fixture_init(&fx);

    const char *patch =
        "[{\"op\":\"replace\",\"path\":\"/callbackUri\","
        "\"value\":\"https://amf.example.com/new\"}]";
    nssf_subscription_patch_request_t req = {
        .authorization = NULL,
        .subscription_id = "x",
        .content_type = NSSF_CT_JSON_PATCH,
        .body = patch,
        .content_length = strlen(patch),
        .has_content_length = true,
        .request_target = "/nssai-availability/subscriptions/x",
    };
    nssf_subscription_patch_deps_t deps = {.jwks_cache = fx.cache,
                                           .store = fx.store};
    nssf_subscription_patch_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(401,
                          nssf_subscription_patch_handle(&req, &deps, &out));
    TEST_ASSERT_EQUAL_STRING(NSSF_CT_PROBLEM_JSON, out.content_type);

    nssf_subscription_patch_response_free(&out);
    fixture_free(&fx);
}

int main(void)
{
    g_key = EVP_RSA_gen(2048);
    if (g_key == NULL) {
        fprintf(stderr,
                "[test_subscription_patch_integration] RSA keygen failed\n");
        return 1;
    }
    UNITY_BEGIN();
    RUN_TEST(test_patch_200_replace_callback_applied);
    RUN_TEST(test_patch_415_wrong_content_type);
    RUN_TEST(test_patch_404_absent_id);
    RUN_TEST(test_patch_400_malformed_patch);
    RUN_TEST(test_patch_401_no_bearer);
    int rc = UNITY_END();
    EVP_PKEY_free(g_key);
    return rc;
}
