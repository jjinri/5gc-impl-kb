/*
 * test_nssaiavailability_route_dispatch.c — WI-availability-route-registration
 * router-driven integration.
 *
 * Drives nssf_router_dispatch() DIRECTLY — no socket, no nghttp2 server, no live
 * DB. The router is built over the combined nssf_router_deps_t (jwks_cache +
 * selection_engine + availability_engine), all wired from in-test seams
 * (mirroring test_nsselection_get_* / test_nssaiavailability_*_integration):
 *   - jwks_cache         : nssf_jwks_cache_create_from_json() over an in-test RSA
 *                          JWKS, validating RS256 JWTs we mint (scope
 *                          nnssf-nsselection / nnssf-nssaiavailability).
 *   - selection_engine   : nssf_selection_engine_new() over an in-memory repo —
 *                          for GET /network-slice-information.
 *   - availability_engine: nssf_availability_engine_new() over the SAME in-memory
 *                          repo — for the NSSAIAvailability routes.
 *
 * This proves the (method, path) dispatch table: each registered route reaches
 * its handler (security gate + status), wrong method on a known path answers 405,
 * an unknown path answers 404, a missing {nfId} answers 404, and OPTIONS answers
 * 200 + an Allow header listing PUT/PATCH/DELETE/OPTIONS. The live-socket nghttp2
 * variant is covered by the in-process dispatch here (no SKIP needed — the router
 * is the same TU the server calls).
 *
 * LSan-clean: every router response (nssf_router_response_free) + repo / engines /
 * jwks / token freed.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "availability_engine.h"
#include "availability_repository.h"
#include "cJSON.h"
#include "oauth2_jwks.h"
#include "oauth2_scope_check.h"
#include "router.h"
#include "selection_engine.h"

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
#define ABSENT_NF_ID "cdcdcdcd-1111-1111-1111-111111111111"

/* ── in-test RSA key + JWKS/JWT minting (mirror of test_jwks_fetch.c) ──────── */

static EVP_PKEY *g_key = NULL;
static const char *kKid = "nssf-test-kid-route-dispatch";

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

/* Wrap a minted token into a "Bearer <jwt>" header value (caller frees). */
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

/* ── cJSON helpers for the seeded availability row + PUT body ──────────────── */

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

/* Seed availability[id=SEEDED_NF_ID, 001/01/000001] = {(1,000001)}. */
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

/* A well-formed NssaiAvailabilityInfo PUT body (caller frees). */
static char *make_put_body(void)
{
    cJSON *info = cJSON_CreateObject();
    cJSON *data_list = cJSON_CreateArray();
    cJSON *entry = cJSON_CreateObject();
    cJSON_AddItemToObject(entry, "tai", make_tai("001", "01", "000001"));
    cJSON *snssais = cJSON_CreateArray();
    cJSON_AddItemToArray(snssais, make_snssai(1, "000001"));
    cJSON_AddItemToObject(entry, "supportedSnssaiList", snssais);
    cJSON_AddItemToArray(data_list, entry);
    cJSON_AddItemToObject(info, "supportedNssaiAvailabilityData", data_list);

    char *s = cJSON_PrintUnformatted(info);
    cJSON_Delete(info);
    TEST_ASSERT_NOT_NULL(s);
    return s;
}

/* A minimal RFC 6902 patch document (one no-op replace). Caller frees. */
static char *make_patch_body(void)
{
    cJSON *arr = cJSON_CreateArray();
    cJSON *op = cJSON_CreateObject();
    cJSON_AddStringToObject(op, "op", "replace");
    cJSON_AddStringToObject(op, "path", "/0/supportedSnssaiList");
    cJSON *val = cJSON_CreateArray();
    cJSON_AddItemToArray(val, make_snssai(1, "000001"));
    cJSON_AddItemToObject(op, "value", val);
    cJSON_AddItemToArray(arr, op);

    char *s = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    TEST_ASSERT_NOT_NULL(s);
    return s;
}

/* The router under test + its borrowed deps. One in-memory repo backs both
 * engines (the selection_engine + availability_engine share it). */
typedef struct {
    nssf_jwks_cache_t *cache;
    nssf_availability_repo_t *repo;
    nssf_selection_engine_t *sel_engine;
    nssf_availability_engine_t *avail_engine;
    nssf_router_t *router;
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

    nssf_selection_seams_t sel_seams;
    memset(&sel_seams, 0, sizeof(sel_seams)); /* body-sourced subscribed, no AMF. */
    fx->sel_engine = nssf_selection_engine_new(fx->repo, &sel_seams);
    TEST_ASSERT_NOT_NULL(fx->sel_engine);

    nssf_availability_engine_seams_t avail_seams;
    memset(&avail_seams, 0, sizeof(avail_seams)); /* NULL publish — phase2 no-op. */
    fx->avail_engine = nssf_availability_engine_new(fx->repo, &avail_seams);
    TEST_ASSERT_NOT_NULL(fx->avail_engine);

    nssf_router_deps_t deps = {
        .jwks_cache = fx->cache,
        .selection_engine = fx->sel_engine,
        .availability_engine = fx->avail_engine,
    };
    fx->router = nssf_router_create(&deps);
    TEST_ASSERT_NOT_NULL(fx->router);
}

static void fixture_free(fixture_t *fx)
{
    nssf_router_free(fx->router);
    nssf_availability_engine_free(fx->avail_engine);
    nssf_selection_engine_free(fx->sel_engine);
    nssf_availability_repo_free(fx->repo);
    nssf_jwks_cache_free(fx->cache);
}

/* ── percent-encode + NSSelectionGet query (server-side encoding) ──────────── */

static char *url_encode(const char *s)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t len = strlen(s);
    char *out = malloc(len * 3 + 1);
    TEST_ASSERT_NOT_NULL(out);
    size_t w = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        bool unreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                          (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                          c == '.' || c == '~';
        if (unreserved) {
            out[w++] = (char)c;
        } else {
            out[w++] = '%';
            out[w++] = hex[(c >> 4) & 0xf];
            out[w++] = hex[c & 0xf];
        }
    }
    out[w] = '\0';
    return out;
}

static char *kv_param(const char *name, cJSON *value_owned)
{
    char *json = cJSON_PrintUnformatted(value_owned);
    cJSON_Delete(value_owned);
    TEST_ASSERT_NOT_NULL(json);
    char *enc = url_encode(json);
    free(json);
    size_t need = strlen(name) + 1 + strlen(enc) + 1;
    char *out = malloc(need);
    TEST_ASSERT_NOT_NULL(out);
    snprintf(out, need, "%s=%s", name, enc);
    free(enc);
    return out;
}

/*
 * Build the full :path the server would hand the router for GET
 * /network-slice-information — route + '?' + encoded slice-info/tai/home-plmn.
 * requested == subscribed so the body carries its own subscribed set. The TAI is
 * the seeded 001/01/000001. Caller frees.
 */
static char *build_nsselection_path(void)
{
    cJSON *reg = cJSON_CreateObject();
    cJSON *req_arr = cJSON_CreateArray();
    cJSON_AddItemToArray(req_arr, make_snssai(1, "000001"));
    cJSON_AddItemToObject(reg, "requestedNssai", req_arr);

    cJSON *sub_list = cJSON_CreateArray();
    cJSON *w1 = cJSON_CreateObject();
    cJSON_AddItemToObject(w1, "subscribedSnssai", make_snssai(1, "000001"));
    cJSON_AddItemToArray(sub_list, w1);
    cJSON_AddItemToObject(reg, "subscribedNssai", sub_list);

    char *p_reg = kv_param("slice-info-for-registration", reg);
    char *p_tai = kv_param("tai", make_tai("001", "01", "000001"));
    char *p_plmn = kv_param("home-plmn-id", make_plmn("001", "01"));

    const char *route = "/network-slice-information";
    size_t need = strlen(route) + 1 + strlen(p_reg) + strlen(p_tai) +
                  strlen(p_plmn) + 3;
    char *path = malloc(need);
    TEST_ASSERT_NOT_NULL(path);
    snprintf(path, need, "%s?%s&%s&%s", route, p_reg, p_tai, p_plmn);
    free(p_reg);
    free(p_tai);
    free(p_plmn);
    return path;
}

/* ── GET /network-slice-information → routed to NSSelectionGet → 200 ────────── */

static void test_route_get_nsselection_200(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSELECTION);
    char *path = build_nsselection_path();

    nssf_router_request_t req = {
        .method = "GET",
        .path = path,
        .authorization = auth,
    };
    nssf_router_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_router_dispatch(fx.router, &req, &out);

    TEST_ASSERT_EQUAL_INT_MESSAGE(200, status, out.body ? out.body : "(null body)");
    TEST_ASSERT_EQUAL_INT(200, out.status);
    TEST_ASSERT_NOT_NULL(out.body);
    cJSON *body = cJSON_Parse(out.body);
    TEST_ASSERT_NOT_NULL_MESSAGE(body, "200 body must be valid JSON");
    cJSON_Delete(body);

    nssf_router_response_free(&out);
    free(path);
    free(auth);
    fixture_free(&fx);
}

/* ── PUT /nssai-availability/{nfId} — security gate + route match ──────────── */

static void test_route_put_security_gate(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *body = make_put_body();
    const char *path = "/nssai-availability/" SEEDED_NF_ID;

    /* 401 — no bearer (route matched, handler ran its gate). */
    {
        nssf_router_request_t req = {
            .method = "PUT",
            .path = path,
            .authorization = NULL,
            .content_type = "application/json",
            .body = body,
            .content_length = strlen(body),
            .has_content_length = true,
        };
        nssf_router_response_t out;
        memset(&out, 0, sizeof(out));
        TEST_ASSERT_EQUAL_INT(401, nssf_router_dispatch(fx.router, &req, &out));
        nssf_router_response_free(&out);
    }

    /* 403 — wrong scope (nnssf-nsselection on a PUT). */
    {
        char *bad = make_bearer(NSSF_SCOPE_NSSELECTION);
        nssf_router_request_t req = {
            .method = "PUT",
            .path = path,
            .authorization = bad,
            .content_type = "application/json",
            .body = body,
            .content_length = strlen(body),
            .has_content_length = true,
        };
        nssf_router_response_t out;
        memset(&out, 0, sizeof(out));
        TEST_ASSERT_EQUAL_INT(403, nssf_router_dispatch(fx.router, &req, &out));
        nssf_router_response_free(&out);
        free(bad);
    }

    free(body);
    fixture_free(&fx);
}

/* ── PUT /nssai-availability/{nfId} valid → handler answer (NOT 404/405) ───── */

static void test_route_put_valid_reaches_handler(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    char *body = make_put_body();

    nssf_router_request_t req = {
        .method = "PUT",
        .path = "/nssai-availability/" SEEDED_NF_ID,
        .authorization = auth,
        .content_type = "application/json",
        .body = body,
        .content_length = strlen(body),
        .has_content_length = true,
    };
    nssf_router_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_router_dispatch(fx.router, &req, &out);

    /* The route matched + the handler ran: a valid PUT commits (200) or clears
     * (204). What it must NOT be is a routing miss (404) or method miss (405). */
    TEST_ASSERT_TRUE_MESSAGE(status == 200 || status == 204,
                             "valid PUT must reach the handler (200/204)");
    TEST_ASSERT_NOT_EQUAL(404, status);
    TEST_ASSERT_NOT_EQUAL(405, status);
    TEST_ASSERT_EQUAL_INT(status, out.status);

    nssf_router_response_free(&out);
    free(body);
    free(auth);
    fixture_free(&fx);
}

/* ── PATCH /nssai-availability/{nfId} → route matched (router tai=NULL → 400) ─ */

static void test_route_patch_reaches_handler(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);
    char *body = make_patch_body();

    nssf_router_request_t req = {
        .method = "PATCH",
        .path = "/nssai-availability/" SEEDED_NF_ID,
        .authorization = auth,
        .content_type = "application/json-patch+json",
        .body = body,
        .content_length = strlen(body),
        .has_content_length = true,
    };
    nssf_router_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_router_dispatch(fx.router, &req, &out);

    /* The router passes tai=NULL (no transport Tai source yet), so the engine
     * classifies the patch as 400 — but that is the HANDLER's answer, proving the
     * route matched. It must NOT be a routing miss (404) or method miss (405). */
    TEST_ASSERT_NOT_EQUAL_MESSAGE(404, status, "PATCH route must match (not 404)");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(405, status, "PATCH is a known method (not 405)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(400, status,
                                  "router tai=NULL → handler classifies 400");
    TEST_ASSERT_EQUAL_INT(400, out.status);

    /* Wrong scope on PATCH is the handler's 403 — still a matched route. */
    nssf_router_response_free(&out);
    char *bad = make_bearer(NSSF_SCOPE_NSSELECTION);
    req.authorization = bad;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(403, nssf_router_dispatch(fx.router, &req, &out));

    nssf_router_response_free(&out);
    free(bad);
    free(body);
    free(auth);
    fixture_free(&fx);
}

/* ── DELETE /nssai-availability/{nfId} → 204 seeded / 404 absent / 401 ─────── */

static void test_route_delete_dispatch(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);

    /* 204 — delete the seeded nfId. */
    {
        nssf_router_request_t req = {
            .method = "DELETE",
            .path = "/nssai-availability/" SEEDED_NF_ID,
            .authorization = auth,
        };
        nssf_router_response_t out;
        memset(&out, 0, sizeof(out));
        TEST_ASSERT_EQUAL_INT(204, nssf_router_dispatch(fx.router, &req, &out));
        TEST_ASSERT_NULL_MESSAGE(out.body, "204 carries no body");
        nssf_router_response_free(&out);
    }

    /* 404 — the row is now gone (and ABSENT_NF_ID never existed). */
    {
        nssf_router_request_t req = {
            .method = "DELETE",
            .path = "/nssai-availability/" ABSENT_NF_ID,
            .authorization = auth,
        };
        nssf_router_response_t out;
        memset(&out, 0, sizeof(out));
        TEST_ASSERT_EQUAL_INT(404, nssf_router_dispatch(fx.router, &req, &out));
        nssf_router_response_free(&out);
    }

    /* 401 — no bearer (route matched, handler gate denies). */
    {
        nssf_router_request_t req = {
            .method = "DELETE",
            .path = "/nssai-availability/" SEEDED_NF_ID,
            .authorization = NULL,
        };
        nssf_router_response_t out;
        memset(&out, 0, sizeof(out));
        TEST_ASSERT_EQUAL_INT(401, nssf_router_dispatch(fx.router, &req, &out));
        nssf_router_response_free(&out);
    }

    free(auth);
    fixture_free(&fx);
}

/* ── OPTIONS /nssai-availability → 200 + Allow PUT/PATCH/DELETE/OPTIONS ─────── */

static void test_route_options_200_allow(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);

    nssf_router_request_t req = {
        .method = "OPTIONS",
        .path = "/nssai-availability",
        .authorization = auth,
    };
    nssf_router_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_router_dispatch(fx.router, &req, &out);

    TEST_ASSERT_EQUAL_INT(200, status);
    TEST_ASSERT_EQUAL_INT(200, out.status);
    TEST_ASSERT_NULL_MESSAGE(out.body, "OPTIONS carries no body");
    TEST_ASSERT_TRUE_MESSAGE(strstr(out.allow, "PUT") != NULL, "Allow must list PUT");
    TEST_ASSERT_TRUE_MESSAGE(strstr(out.allow, "PATCH") != NULL, "Allow must list PATCH");
    TEST_ASSERT_TRUE_MESSAGE(strstr(out.allow, "DELETE") != NULL, "Allow must list DELETE");
    TEST_ASSERT_TRUE_MESSAGE(strstr(out.allow, "OPTIONS") != NULL, "Allow must list OPTIONS");

    nssf_router_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── wrong method on a known {nfId} path → 405 ─────────────────────────────── */

static void test_route_wrong_method_405(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);

    /* POST is not a registered verb on /nssai-availability/{nfId}. */
    nssf_router_request_t req = {
        .method = "POST",
        .path = "/nssai-availability/" SEEDED_NF_ID,
        .authorization = auth,
    };
    nssf_router_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(405, nssf_router_dispatch(fx.router, &req, &out));
    TEST_ASSERT_EQUAL_INT(405, out.status);

    nssf_router_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── wrong method on the OPTIONS path → 405 ────────────────────────────────── */

static void test_route_options_path_wrong_method_405(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);

    /* GET on the collection path (only OPTIONS is registered there) → 405. */
    nssf_router_request_t req = {
        .method = "GET",
        .path = "/nssai-availability",
        .authorization = auth,
    };
    nssf_router_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(405, nssf_router_dispatch(fx.router, &req, &out));

    nssf_router_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── unknown path → 404 ────────────────────────────────────────────────────── */

static void test_route_unknown_path_404(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);

    nssf_router_request_t req = {
        .method = "GET",
        .path = "/no-such-resource",
        .authorization = auth,
    };
    nssf_router_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(404, nssf_router_dispatch(fx.router, &req, &out));
    TEST_ASSERT_EQUAL_INT(404, out.status);

    nssf_router_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

/* ── missing {nfId} (`/nssai-availability/`) → 404 ─────────────────────────── */

static void test_route_missing_nfid_404(void)
{
    fixture_t fx;
    fixture_init(&fx, true);
    char *auth = make_bearer(NSSF_SCOPE_NSSAIAVAILABILITY);

    /* The empty trailing segment is not a valid {nfId} → no route → 404. */
    nssf_router_request_t req = {
        .method = "DELETE",
        .path = "/nssai-availability/",
        .authorization = auth,
    };
    nssf_router_response_t out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(404, nssf_router_dispatch(fx.router, &req, &out));

    nssf_router_response_free(&out);
    free(auth);
    fixture_free(&fx);
}

int main(void)
{
    g_key = EVP_RSA_gen(2048);
    if (g_key == NULL) {
        fprintf(stderr, "[test_nssaiavailability_route_dispatch] RSA keygen failed\n");
        return 1;
    }
    UNITY_BEGIN();
    RUN_TEST(test_route_get_nsselection_200);
    RUN_TEST(test_route_put_security_gate);
    RUN_TEST(test_route_put_valid_reaches_handler);
    RUN_TEST(test_route_patch_reaches_handler);
    RUN_TEST(test_route_delete_dispatch);
    RUN_TEST(test_route_options_200_allow);
    RUN_TEST(test_route_wrong_method_405);
    RUN_TEST(test_route_options_path_wrong_method_405);
    RUN_TEST(test_route_unknown_path_404);
    RUN_TEST(test_route_missing_nfid_404);
    int rc = UNITY_END();
    EVP_PKEY_free(g_key);
    return rc;
}
