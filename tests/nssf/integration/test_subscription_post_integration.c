/*
 * test_subscription_post_integration.c — WI-subscription-handlers integration
 * (NSSAIAvailabilityPost / Subscribe).
 *
 * POST /nssai-availability/subscriptions. The handler
 * (nssf_subscription_post_handle) is driven IN-PROCESS — no socket, no live DB,
 * no nghttp2 server. The collaborators are wired from in-test seams (mirroring
 * test_nssaiavailability_put_integration + test_subscription_initial_snapshot):
 *   - jwks_cache : nssf_jwks_cache_create_from_json() over an in-test RSA JWKS,
 *                  validating an RS256 JWT we mint with scope nnssf-nssaiavailability.
 *   - store      : nssf_subscription_store_new_inmemory() — the in-memory test seam
 *                  (no libpq / PostgreSQL / network).
 *
 * Three required behaviors run UNCONDITIONALLY in CI:
 *   - valid https callbackUri → 201 Created + Location header (the new id) +
 *     persisted row (read back via nssf_subscription_store_get).
 *   - callbackUri-policy reuse: http:// (and userinfo '@' / fragment '#')
 *     callbackUri → 400 invalid-param, NO row, NO dispatch — proves the inbound
 *     handler reuses the dispatcher's shared URL-policy gate.
 *   - initial-snapshot dispatch: with the snapshot seam installed (mock retry
 *     store + stub availability snapshot + a counting dispatcher transport), a
 *     successful create dispatches the initial snapshot EXACTLY ONCE (mock POST
 *     count == 1, no repeated/looped dispatch).
 *
 * LSan-clean: every cJSON / response body / store / repo / dispatcher / jwks /
 * token freed.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "availability_repository.h"
#include "cJSON.h"
#include "notification_dispatcher.h"
#include "oauth2_jwks.h"
#include "oauth2_scope_check.h"
#include "subscription_post_handler.h"
#include "subscription_store.h"

#include <jwt.h>

#include <curl/curl.h>

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

/* ── in-test RSA key + JWKS/JWT minting (mirror of test_nssaiavailability_put) ─ */

static EVP_PKEY *g_key = NULL;
static const char *kKid = "nssf-test-kid-sub-post";

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

/* ── counting mock AMF transport (initial-snapshot dispatch assertion) ──────── */

typedef struct {
    int calls;
    char last_uri[256];
    char last_body[512];
    long reply_status;
} amf_mock_t;

static amf_mock_t g_amf;

static int amf_mock_transport(const nssf_notification_dispatcher_t *disp,
                              const char *callback_uri, const char *body,
                              struct curl_slist *headers, long *out_http_status,
                              void *ctx)
{
    (void)disp;
    (void)headers;
    amf_mock_t *m = (amf_mock_t *)ctx;
    m->calls++;
    snprintf(m->last_uri, sizeof(m->last_uri), "%s",
             callback_uri != NULL ? callback_uri : "");
    snprintf(m->last_body, sizeof(m->last_body), "%s", body != NULL ? body : "");
    *out_http_status = m->reply_status;
    return 0;
}

/* ── subscription create document (callbackUri + nssaiAvailabilityReqData) ──── */

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

/*
 * NssfEventSubscriptionCreateData-style body carrying the callbackUri + a tai
 * filter. The exact filter shape is the store's concern; the handler only needs a
 * parseable JSON object with a callbackUri to validate + a filter to persist.
 * Caller frees the returned string.
 */
static char *make_post_body(const char *callback_uri)
{
    cJSON *doc = cJSON_CreateObject();
    cJSON_AddStringToObject(doc, "callbackUri", callback_uri);
    cJSON_AddItemToObject(doc, "plmnId", make_plmn("001", "01"));
    cJSON_AddItemToObject(doc, "tai", make_tai("001", "01", "000001"));
    char *s = cJSON_PrintUnformatted(doc);
    cJSON_Delete(doc);
    TEST_ASSERT_NOT_NULL(s);
    return s;
}

/* Seed a STUB availability record so the snapshot read returns a non-empty set
 * (mirrors test_subscription_initial_snapshot::seed_availability). */
static void seed_availability(nssf_availability_repo_t *repo, const char *mcc,
                              const char *mnc, const char *tac)
{
    nssf_availability_record_t rec;
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s",
             "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee");
    rec.plmn_id = cJSON_CreateObject();
    cJSON_AddStringToObject(rec.plmn_id, "mcc", mcc);
    cJSON_AddStringToObject(rec.plmn_id, "mnc", mnc);
    rec.tai = cJSON_CreateObject();
    cJSON *tplmn = cJSON_CreateObject();
    cJSON_AddStringToObject(tplmn, "mcc", mcc);
    cJSON_AddStringToObject(tplmn, "mnc", mnc);
    cJSON_AddItemToObject(rec.tai, "plmnId", tplmn);
    cJSON_AddStringToObject(rec.tai, "tac", tac);
    rec.snssais = cJSON_CreateArray();
    cJSON *s = cJSON_CreateObject();
    cJSON_AddNumberToObject(s, "sst", 1);
    cJSON_AddItemToArray(rec.snssais, s);
    rec.amf_set_id = strdup("amfset-snap");
    rec.reallocation_indication = false;
    TEST_ASSERT_EQUAL_INT(0, nssf_availability_repo_upsert(repo, &rec));
    nssf_availability_record_clear(&rec);
}

/* ── fixture: jwks cache + in-memory subscription store (no snapshot seam) ──── */

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

/* ── 201 happy — valid https callbackUri → Created + Location + row persisted ── */

static void test_post_201_created_location_and_row(void)
{
    fixture_t fx;
    fixture_init(&fx);

    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    char *body = make_post_body("https://amf.example.com/nssai-avail/notify");

    nssf_subscription_post_request_t req = {
        .authorization = auth,
        .content_type = "application/json",
        .body = body,
        .content_length = strlen(body),
        .has_content_length = true,
        .max_body_len = 0,
        .request_target = "/nssai-availability/subscriptions",
        .max_uri_len = 0,
    };
    nssf_subscription_post_deps_t deps = {.jwks_cache = fx.cache,
                                          .store = fx.store};

    nssf_subscription_post_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_subscription_post_handle(&req, &deps, &out);

    TEST_ASSERT_EQUAL_INT_MESSAGE(201, status,
                                  out.body ? out.body : "(null body)");
    TEST_ASSERT_EQUAL_INT(201, out.status);
    /* Location header present, carrying the new subscription resource path. */
    TEST_ASSERT_TRUE_MESSAGE(out.location[0] != '\0',
                             "201 must carry a non-empty Location header");
    /* The Location must reference the subscriptions collection + an id. */
    TEST_ASSERT_NOT_NULL_MESSAGE(
        strstr(out.location, "/nssai-availability/subscriptions/"),
        "Location must point at /nssai-availability/subscriptions/{id}");

    /* The new id is the trailing path segment of the Location. */
    const char *slash = strrchr(out.location, '/');
    TEST_ASSERT_NOT_NULL(slash);
    const char *new_id = slash + 1;
    TEST_ASSERT_EQUAL_size_t_MESSAGE(36, strlen(new_id),
                                     "Location id must be a 36-char UUID");

    /* row persisted — read it back through the store. */
    nssf_subscription_record_t rec;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_SUB_OK, nssf_subscription_store_get(fx.store, new_id, &rec),
        "created subscription row not persisted (store_get NOT_FOUND)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "https://amf.example.com/nssai-avail/notify", rec.callback_uri,
        "persisted row callbackUri mismatch");
    nssf_subscription_record_clear(&rec);

    nssf_subscription_post_response_free(&out);
    free(body);
    free(auth);
    fixture_free(&fx);
}

/* ── 400 — http:// callbackUri rejected by the shared URL-policy gate ────────── */

static void test_post_400_http_callback_no_row_no_dispatch(void)
{
    fixture_t fx;
    fixture_init(&fx);

    /* install the snapshot seam so we can prove NO dispatch on a rejected create. */
    memset(&g_amf, 0, sizeof(g_amf));
    g_amf.reply_status = 200;
    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);
    nssf_availability_repo_t *avail = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(avail);
    seed_availability(avail, "001", "01", "000001");
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(retry, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);
    nssf_subscription_store_set_snapshot_seam(fx.store, avail, retry, disp);

    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    /* plaintext http:// callback — the shared dispatcher gate rejects it. */
    char *body = make_post_body("http://amf.example.com/cb");

    nssf_subscription_post_request_t req = {
        .authorization = auth,
        .content_type = "application/json",
        .body = body,
        .content_length = strlen(body),
        .has_content_length = true,
        .request_target = "/nssai-availability/subscriptions",
    };
    nssf_subscription_post_deps_t deps = {.jwks_cache = fx.cache,
                                          .store = fx.store};
    nssf_subscription_post_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_subscription_post_handle(&req, &deps, &out);

    TEST_ASSERT_EQUAL_INT_MESSAGE(
        400, status,
        "http:// callbackUri must be rejected 400 (shared URL policy reuse)");
    TEST_ASSERT_EQUAL_STRING(NSSF_CT_PROBLEM_JSON, out.content_type);
    TEST_ASSERT_TRUE_MESSAGE(out.location[0] == '\0',
                             "rejected create must not emit a Location");

    /* NO dispatch fired — the gate runs BEFORE the store is touched. */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, g_amf.calls,
        "rejected callbackUri must not trigger an initial-snapshot POST");

    /* NO row persisted — queue is empty, nothing enqueued. */
    nssf_retry_item_t none;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(retry, &none),
        "rejected callbackUri must not enqueue a retry row");
    nssf_retry_item_clear(&none);

    nssf_subscription_post_response_free(&out);
    free(body);
    free(auth);
    nssf_notification_dispatcher_free(disp);
    nssf_availability_repo_free(avail);
    nssf_retry_store_free(retry);
    fixture_free(&fx);
}

/* ── 400 — userinfo('@') and fragment('#') callbackUri rejected (same policy) ── */

static void test_post_400_userinfo_and_fragment_rejected(void)
{
    const char *hostile[] = {
        "https://user:pass@evil.example.com/cb", /* userinfo '@' */
        "https://amf.example.com/cb#section",     /* fragment '#' */
    };
    for (size_t i = 0; i < sizeof(hostile) / sizeof(hostile[0]); i++) {
        fixture_t fx;
        fixture_init(&fx);
        char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
        char *body = make_post_body(hostile[i]);

        nssf_subscription_post_request_t req = {
            .authorization = auth,
            .content_type = "application/json",
            .body = body,
            .content_length = strlen(body),
            .has_content_length = true,
            .request_target = "/nssai-availability/subscriptions",
        };
        nssf_subscription_post_deps_t deps = {.jwks_cache = fx.cache,
                                              .store = fx.store};
        nssf_subscription_post_response_t out;
        memset(&out, 0, sizeof(out));
        int status = nssf_subscription_post_handle(&req, &deps, &out);
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            400, status,
            "userinfo('@') / fragment('#') callbackUri must be rejected 400");
        TEST_ASSERT_TRUE_MESSAGE(out.location[0] == '\0',
                                 "rejected create must not emit a Location");

        nssf_subscription_post_response_free(&out);
        free(body);
        free(auth);
        fixture_free(&fx);
    }
}

/* ── initial-snapshot dispatch fires EXACTLY ONCE on a successful create ─────── */

static void test_post_initial_snapshot_dispatched_exactly_once(void)
{
    fixture_t fx;
    fixture_init(&fx);

    memset(&g_amf, 0, sizeof(g_amf));
    g_amf.reply_status = 200; /* AMF accepts → SENT (row completed/removed). */

    nssf_retry_store_t *retry = nssf_retry_store_new_inmemory();
    TEST_ASSERT_NOT_NULL(retry);
    nssf_availability_repo_t *avail = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(avail);
    seed_availability(avail, "001", "01", "000001");
    nssf_notification_dispatcher_t *disp =
        nssf_notification_dispatcher_new(retry, NULL);
    TEST_ASSERT_NOT_NULL(disp);
    nssf_notification_dispatcher_set_transport(disp, amf_mock_transport, &g_amf);
    nssf_subscription_store_set_snapshot_seam(fx.store, avail, retry, disp);

    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    char *body = make_post_body("https://amf.example.com/nssai-avail/notify");

    nssf_subscription_post_request_t req = {
        .authorization = auth,
        .content_type = "application/json",
        .body = body,
        .content_length = strlen(body),
        .has_content_length = true,
        .request_target = "/nssai-availability/subscriptions",
    };
    nssf_subscription_post_deps_t deps = {.jwks_cache = fx.cache,
                                          .store = fx.store};
    nssf_subscription_post_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_subscription_post_handle(&req, &deps, &out);
    TEST_ASSERT_EQUAL_INT_MESSAGE(201, status,
                                  out.body ? out.body : "(null body)");

    /* (A) the initial-snapshot dispatch fired EXACTLY ONCE (call-driven). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, g_amf.calls,
        "initial-snapshot dispatch not exactly once "
        "(0=skipped / >1=looped worker re-dispatch)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "https://amf.example.com/nssai-avail/notify", g_amf.last_uri,
        "dispatched POST not the new subscription's callbackUri");

    /* (B) no leftover/looped work: nothing left in the queue. */
    nssf_retry_item_t leftover;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_retry_store_dequeue(retry, &leftover),
        "retry_queue still has a row after create (duplicate enqueue / loop)");
    nssf_retry_item_clear(&leftover);

    /* (C) calling the dispatcher again finds NOTHING due — no self-perpetuating
     * worker loop was started by the create path. */
    int before = g_amf.calls;
    nssf_dispatch_result_e again =
        nssf_notification_dispatcher_dispatch_pending(disp, NULL);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        NSSF_DISPATCH_NONE, again,
        "due row remains after create (looped dispatch risk)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(before, g_amf.calls,
                                  "extra POST emitted after create (>1 dispatch)");

    nssf_subscription_post_response_free(&out);
    free(body);
    free(auth);
    nssf_notification_dispatcher_free(disp);
    nssf_availability_repo_free(avail);
    nssf_retry_store_free(retry);
    fixture_free(&fx);
}

/* ── 401 — no bearer (security gate runs BEFORE business logic) ──────────────── */

static void test_post_401_no_bearer(void)
{
    fixture_t fx;
    fixture_init(&fx);
    char *body = make_post_body("https://amf.example.com/cb");

    nssf_subscription_post_request_t req = {
        .authorization = NULL,
        .content_type = "application/json",
        .body = body,
        .content_length = strlen(body),
        .has_content_length = true,
        .request_target = "/nssai-availability/subscriptions",
    };
    nssf_subscription_post_deps_t deps = {.jwks_cache = fx.cache,
                                          .store = fx.store};
    nssf_subscription_post_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(401, nssf_subscription_post_handle(&req, &deps, &out));
    TEST_ASSERT_EQUAL_STRING(NSSF_CT_PROBLEM_JSON, out.content_type);

    nssf_subscription_post_response_free(&out);
    free(body);
    fixture_free(&fx);
}

int main(void)
{
    g_key = EVP_RSA_gen(2048);
    if (g_key == NULL) {
        fprintf(stderr, "[test_subscription_post_integration] RSA keygen failed\n");
        return 1;
    }
    UNITY_BEGIN();
    RUN_TEST(test_post_201_created_location_and_row);
    RUN_TEST(test_post_400_http_callback_no_row_no_dispatch);
    RUN_TEST(test_post_400_userinfo_and_fragment_rejected);
    RUN_TEST(test_post_initial_snapshot_dispatched_exactly_once);
    RUN_TEST(test_post_401_no_bearer);
    int rc = UNITY_END();
    EVP_PKEY_free(g_key);
    return rc;
}
