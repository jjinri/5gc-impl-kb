/*
 * test_nsselection_response_schema.c — WI-nsselection-handler contract test.
 *
 * NSSelectionGet 200 response-schema conformance. The handler
 * (nssf_nsselection_get_handle) is driven IN-PROCESS — same path the nghttp2
 * server takes — to produce the real serialized 200 body, then the body is
 * asserted byte/structure-level against the AuthorizedNetworkSliceInfo response
 * schema the contract names for NSSelectionGet:
 *
 *   handoff/nssf/contract.yaml
 *     api/NSSelectionGet.responses.'200'.schema = AuthorizedNetworkSliceInfo
 *     data-model/AuthorizedNetworkSliceInfo.machine_file =
 *       design/nssf/contract/data-model/AuthorizedNetworkSliceInfo.json
 *
 * From that schema the load-bearing 200 shape is:
 *   AuthorizedNetworkSliceInfo.allowedNssaiList[] : AllowedNssai
 *     .allowedSnssaiList[] (required) : AllowedSnssai
 *       .allowedSnssai (required) : Snssai { sst (int, required), sd (string?) }
 *     .accessType (required)
 * and the response media type is application/json (NSSF_CT_JSON), NOT
 * application/problem+json.
 *
 * The contract.yaml + the generated AuthorizedNetworkSliceInfo model are the
 * sources of truth — no spec/OpenAPI re-reading (ADR-0005). This test runs
 * UNCONDITIONALLY (no env, no DB, no socket): the JWKS is seeded offline from an
 * in-test RSA key and the availability comes from the in-memory repo.
 *
 * LSan-clean: every cJSON / response body / engine / repo / jwks freed.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "availability_repository.h"
#include "cJSON.h"
#include "nsselection_get_handler.h"
#include "oauth2_jwks.h"
#include "oauth2_scope_check.h"
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

/* ── in-test RSA key + JWKS/JWT minting (offline, no network) ──────────────── */

static EVP_PKEY *g_key = NULL;
static const char *kKid = "nssf-test-kid-contract";

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

/* ── NSSelectionGet query assembly ────────────────────────────────────────── */

static const char hexdig[] = "0123456789ABCDEF";

static char *url_encode(const char *s)
{
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
            out[w++] = hexdig[(c >> 4) & 0xf];
            out[w++] = hexdig[c & 0xf];
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

/* requested == subscribed == {(1,000001),(2,0000ab)} → full-allow. */
static char *build_query_string(void)
{
    cJSON *reg = cJSON_CreateObject();
    cJSON *req_arr = cJSON_CreateArray();
    cJSON_AddItemToArray(req_arr, make_snssai(1, "000001"));
    cJSON_AddItemToArray(req_arr, make_snssai(2, "0000ab"));
    cJSON_AddItemToObject(reg, "requestedNssai", req_arr);
    cJSON *sub_list = cJSON_CreateArray();
    {
        cJSON *w1 = cJSON_CreateObject();
        cJSON_AddItemToObject(w1, "subscribedSnssai", make_snssai(1, "000001"));
        cJSON_AddItemToArray(sub_list, w1);
        cJSON *w2 = cJSON_CreateObject();
        cJSON_AddItemToObject(w2, "subscribedSnssai", make_snssai(2, "0000ab"));
        cJSON_AddItemToArray(sub_list, w2);
    }
    cJSON_AddItemToObject(reg, "subscribedNssai", sub_list);

    char *p_reg = kv_param("slice-info-for-registration", reg);
    char *p_tai = kv_param("tai", make_tai("001", "01", "000001"));
    char *p_plmn = kv_param("home-plmn-id", make_plmn("001", "01"));
    size_t need = strlen(p_reg) + strlen(p_tai) + strlen(p_plmn) + 3;
    char *q = malloc(need);
    TEST_ASSERT_NOT_NULL(q);
    snprintf(q, need, "%s&%s&%s", p_reg, p_tai, p_plmn);
    free(p_reg);
    free(p_tai);
    free(p_plmn);
    return q;
}

static void seed_availability(nssf_availability_repo_t *repo)
{
    nssf_availability_record_t rec;
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s", "efefefef-0000-0000-0000-00000000eeee");
    rec.plmn_id = make_plmn("001", "01");
    rec.tai = make_tai("001", "01", "000001");
    cJSON *snssais = cJSON_CreateArray();
    cJSON_AddItemToArray(snssais, make_snssai(1, "000001"));
    cJSON_AddItemToArray(snssais, make_snssai(2, "0000ab"));
    rec.snssais = snssais;
    rec.amf_set_id = NULL;
    rec.reallocation_indication = false;
    TEST_ASSERT_EQUAL_INT(0, nssf_availability_repo_upsert(repo, &rec));
    nssf_availability_record_clear(&rec);
}

/*
 * Drive the handler once and hand the parsed 200 body back to the caller (caller
 * cJSON_Deletes *out_body). Asserts the 200 + application/json envelope here.
 */
static void produce_200_body(cJSON **out_body)
{
    char *jwks_json = make_public_jwks_json(g_key);
    nssf_jwks_cache_t *cache = nssf_jwks_cache_create_from_json(jwks_json, 60);
    free(jwks_json);
    TEST_ASSERT_NOT_NULL(cache);

    nssf_availability_repo_t *repo = nssf_availability_repo_new_inmemory();
    TEST_ASSERT_NOT_NULL(repo);
    seed_availability(repo);
    nssf_selection_seams_t seams;
    memset(&seams, 0, sizeof(seams));
    nssf_selection_engine_t *engine = nssf_selection_engine_new(repo, &seams);
    TEST_ASSERT_NOT_NULL(engine);

    char *token = sign_jwt(kKid, NSSF_SCOPE_NSSELECTION, 300);
    TEST_ASSERT_NOT_NULL(token);
    size_t auth_len = strlen("Bearer ") + strlen(token) + 1;
    char *authorization = malloc(auth_len);
    TEST_ASSERT_NOT_NULL(authorization);
    snprintf(authorization, auth_len, "Bearer %s", token);

    char *raw_query = build_query_string();
    nssf_nsselection_request_t req = {
        .authorization = authorization,
        .raw_query = raw_query,
        .request_target = "/network-slice-information",
        .max_uri_len = 0,
    };
    nssf_nsselection_deps_t deps = {.jwks_cache = cache, .engine = engine};
    nssf_nsselection_response_t out;
    memset(&out, 0, sizeof(out));
    int status = nssf_nsselection_get_handle(&req, &deps, &out);

    /* envelope conformance: 200 + application/json (NOT problem+json). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(200, status, out.body ? out.body : "(null)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(NSSF_CT_JSON, out.content_type,
                                     "200 must be application/json");
    TEST_ASSERT_NOT_NULL(out.body);

    cJSON *body = cJSON_Parse(out.body);
    TEST_ASSERT_NOT_NULL_MESSAGE(body, "200 body must be valid JSON");
    *out_body = body;

    nssf_nsselection_response_free(&out);
    free(raw_query);
    free(authorization);
    free(token);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
    nssf_jwks_cache_free(cache);
}

/* ── schema conformance assertions (AuthorizedNetworkSliceInfo.json) ───────── */

/*
 * The 200 body is an OBJECT (AuthorizedNetworkSliceInfo). The contract marks
 * every top-level property optional, so the conformance rule for an object that
 * IS present is: no unexpected/foreign top-level keys, and any present key has
 * the schema-declared JSON type.
 */
static void test_200_is_authorized_network_slice_info_object(void)
{
    cJSON *body = NULL;
    produce_200_body(&body);
    TEST_ASSERT_TRUE_MESSAGE(cJSON_IsObject(body),
                             "AuthorizedNetworkSliceInfo is a JSON object");

    /* Known top-level property names from AuthorizedNetworkSliceInfo.json. */
    static const char *const KNOWN[] = {
        "allowedNssaiList", "configuredNssai", "targetAmfSet", "candidateAmfList",
        "rejectedNssaiInPlmn", "rejectedNssaiInTa", "nsiInformation",
        "supportedFeatures", "nrfAmfSet", "nrfAmfSetNfMgtUri",
        "nrfAmfSetAccessTokenUri", "nrfOauth2Required", "targetAmfServiceSet",
        "targetNssai", "nsagInfos", "mappingOfNssai", "snssaiInfoRspData",
    };
    const size_t known_n = sizeof(KNOWN) / sizeof(KNOWN[0]);

    cJSON *child = NULL;
    cJSON_ArrayForEach(child, body) {
        bool found = false;
        for (size_t i = 0; i < known_n; i++) {
            if (strcmp(child->string, KNOWN[i]) == 0) {
                found = true;
                break;
            }
        }
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "unexpected top-level key '%s' not in AuthorizedNetworkSliceInfo",
                 child->string != NULL ? child->string : "(null)");
        TEST_ASSERT_TRUE_MESSAGE(found, msg);
    }
    cJSON_Delete(body);
}

/*
 * allowedNssaiList[] : AllowedNssai. Each entry MUST carry:
 *   allowedSnssaiList (required, array of AllowedSnssai)
 *   accessType        (required)
 * and each AllowedSnssai MUST carry allowedSnssai : Snssai { sst:int [, sd:str] }.
 */
static void test_allowed_nssai_list_conforms(void)
{
    cJSON *body = NULL;
    produce_200_body(&body);

    cJSON *anl = cJSON_GetObjectItemCaseSensitive(body, "allowedNssaiList");
    TEST_ASSERT_TRUE_MESSAGE(cJSON_IsArray(anl), "allowedNssaiList is an array");
    TEST_ASSERT_TRUE_MESSAGE(cJSON_GetArraySize(anl) >= 1,
                             "full-allow yields >= 1 AllowedNssai group");

    cJSON *grp = NULL;
    cJSON_ArrayForEach(grp, anl) {
        TEST_ASSERT_TRUE_MESSAGE(cJSON_IsObject(grp), "AllowedNssai is an object");

        /* accessType required + a string. */
        cJSON *access = cJSON_GetObjectItemCaseSensitive(grp, "accessType");
        TEST_ASSERT_NOT_NULL_MESSAGE(access, "AllowedNssai.accessType required");
        TEST_ASSERT_TRUE_MESSAGE(cJSON_IsString(access),
                                 "accessType serialises as a string");

        /* allowedSnssaiList required + array of AllowedSnssai. */
        cJSON *asl = cJSON_GetObjectItemCaseSensitive(grp, "allowedSnssaiList");
        TEST_ASSERT_NOT_NULL_MESSAGE(asl, "AllowedNssai.allowedSnssaiList required");
        TEST_ASSERT_TRUE_MESSAGE(cJSON_IsArray(asl), "allowedSnssaiList is an array");
        TEST_ASSERT_TRUE_MESSAGE(cJSON_GetArraySize(asl) >= 1,
                                 "allowedSnssaiList non-empty on full-allow");

        cJSON *entry = NULL;
        cJSON_ArrayForEach(entry, asl) {
            TEST_ASSERT_TRUE_MESSAGE(cJSON_IsObject(entry),
                                     "AllowedSnssai is an object");
            cJSON *snssai = cJSON_GetObjectItemCaseSensitive(entry, "allowedSnssai");
            TEST_ASSERT_NOT_NULL_MESSAGE(snssai,
                                         "AllowedSnssai.allowedSnssai required");
            TEST_ASSERT_TRUE_MESSAGE(cJSON_IsObject(snssai),
                                     "allowedSnssai is a Snssai object");

            /* Snssai: sst (required integer), sd (optional string). */
            cJSON *sst = cJSON_GetObjectItemCaseSensitive(snssai, "sst");
            TEST_ASSERT_NOT_NULL_MESSAGE(sst, "Snssai.sst required");
            TEST_ASSERT_TRUE_MESSAGE(cJSON_IsNumber(sst), "Snssai.sst is integer");
            TEST_ASSERT_TRUE_MESSAGE(sst->valueint >= 0 && sst->valueint <= 255,
                                     "Snssai.sst within uint8 range");
            cJSON *sd = cJSON_GetObjectItemCaseSensitive(snssai, "sd");
            if (sd != NULL) {
                TEST_ASSERT_TRUE_MESSAGE(cJSON_IsString(sd),
                                         "Snssai.sd, when present, is a string");
            }
        }
    }
    cJSON_Delete(body);
}

/*
 * Byte-level envelope: the 200 serialisation parses back to a single object and
 * round-trips (no trailing garbage, valid JSON document). cJSON_Parse already
 * proves well-formedness; re-serialise + re-parse pins idempotent structure.
 */
static void test_200_body_round_trips(void)
{
    cJSON *body = NULL;
    produce_200_body(&body);
    char *re = cJSON_PrintUnformatted(body);
    TEST_ASSERT_NOT_NULL(re);
    cJSON *again = cJSON_Parse(re);
    TEST_ASSERT_NOT_NULL_MESSAGE(again, "re-serialised 200 body must re-parse");
    TEST_ASSERT_TRUE(cJSON_IsObject(again));
    cJSON_Delete(again);
    free(re);
    cJSON_Delete(body);
}

int main(void)
{
    g_key = EVP_RSA_gen(2048);
    if (g_key == NULL) {
        fprintf(stderr, "[test_nsselection_response_schema] RSA keygen failed\n");
        return 1;
    }
    UNITY_BEGIN();
    RUN_TEST(test_200_is_authorized_network_slice_info_object);
    RUN_TEST(test_allowed_nssai_list_conforms);
    RUN_TEST(test_200_body_round_trips);
    int rc = UNITY_END();
    EVP_PKEY_free(g_key);
    return rc;
}
