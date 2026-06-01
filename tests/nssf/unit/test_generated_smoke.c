/*
 * Generated codegen smoke test — nssf_generated 가 link/build + 핵심 schema
 * 의 alloc/serialize/parse/free round-trip 동작 증명.
 *
 * 본 test 는 *Phase 1 wave 1 WI-codegen-bootstrap* 의 acceptance evidence.
 * Phase 1 wave 0 의 test_nsselection_handler.c (501 stub) 와 별개.
 *
 * _POSIX_C_SOURCE >= 200809L 가 strdup 노출 (c11 strict 에서 POSIX 함수 부재).
 */

#define _POSIX_C_SOURCE 200809L

#include "snssai.h"
#include "plmn_id.h"
#include "tai.h"
#include "authorized_network_slice_info.h"
#include "nssai_availability_info.h"
#include "authorized_nssai_availability_info.h"
#include "nssf_event_subscription_create_data.h"
#include "nssf_event_subscription_created_data.h"
#include "cJSON.h"
#include "unity.h"

#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* snssai_create + convertToJSON + free round-trip. */
static void test_snssai_roundtrip(void)
{
    char *sd = strdup("000001");
    TEST_ASSERT_NOT_NULL(sd);
    snssai_t *s = snssai_create(1, sd);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL_INT(1, s->sst);
    TEST_ASSERT_EQUAL_STRING("000001", s->sd);

    cJSON *json = snssai_convertToJSON(s);
    TEST_ASSERT_NOT_NULL(json);

    char *txt = cJSON_PrintUnformatted(json);
    TEST_ASSERT_NOT_NULL(txt);
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"sst\":1"));
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"sd\":\"000001\""));

    free(txt);
    cJSON_Delete(json);
    snssai_free(s);  /* sd freed by snssai_free */
}

/* snssai_parseFromJSON. */
static void test_snssai_parse_from_json(void)
{
    const char *json_str = "{\"sst\":2,\"sd\":\"FFFFFF\"}";
    cJSON *json = cJSON_Parse(json_str);
    TEST_ASSERT_NOT_NULL(json);

    snssai_t *s = snssai_parseFromJSON(json);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL_INT(2, s->sst);
    TEST_ASSERT_EQUAL_STRING("FFFFFF", s->sd);

    snssai_free(s);
    cJSON_Delete(json);
}

/* plmn_id round-trip. */
static void test_plmn_id_roundtrip(void)
{
    char *mcc = strdup("450");
    char *mnc = strdup("05");
    plmn_id_t *p = plmn_id_create(mcc, mnc);
    TEST_ASSERT_NOT_NULL(p);

    cJSON *json = plmn_id_convertToJSON(p);
    TEST_ASSERT_NOT_NULL(json);
    char *txt = cJSON_PrintUnformatted(json);
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"mcc\":\"450\""));
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"mnc\":\"05\""));

    free(txt);
    cJSON_Delete(json);
    plmn_id_free(p);
}

/* cJSON 자체 sanity (vendored copy). */
static void test_cjson_vendored_works(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "key", "value");
    char *txt = cJSON_PrintUnformatted(obj);
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"key\":\"value\""));
    free(txt);
    cJSON_Delete(obj);
}

/*
 * NSSAIAvailability extension (PR-codegen-nssaiavailability-extension) —
 * 새 multi-spec emit 으로 추가된 NSSAIAvailability schema 의 build/link +
 * parse/serialize/free round-trip 증명.
 *
 * nssai_availability_info / authorized_nssai_availability_info 는
 * supported|authorized NssaiAvailabilityData array 가 required (convertToJSON
 * 이 NULL 이면 fail). empty array `[]` 가 valid round-trip 이므로 parseFromJSON
 * → convertToJSON 경로로 nested list_t 수동 구성 없이 smoke 한다.
 */

/* nssai_availability_info: JSON round-trip (required array + string field). */
static void test_nssai_availability_info_roundtrip(void)
{
    const char *json_str =
        "{\"supportedNssaiAvailabilityData\":[],"
        "\"supportedFeatures\":\"a1\",\"amfSetId\":\"set-7\"}";
    cJSON *json = cJSON_Parse(json_str);
    TEST_ASSERT_NOT_NULL(json);

    nssai_availability_info_t *info = nssai_availability_info_parseFromJSON(json);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_NOT_NULL(info->supported_nssai_availability_data);
    TEST_ASSERT_EQUAL_STRING("a1", info->supported_features);
    TEST_ASSERT_EQUAL_STRING("set-7", info->amf_set_id);

    cJSON *out = nssai_availability_info_convertToJSON(info);
    TEST_ASSERT_NOT_NULL(out);
    char *txt = cJSON_PrintUnformatted(out);
    TEST_ASSERT_NOT_NULL(txt);
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"supportedNssaiAvailabilityData\":[]"));
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"supportedFeatures\":\"a1\""));
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"amfSetId\":\"set-7\""));

    free(txt);
    cJSON_Delete(out);
    cJSON_Delete(json);
    nssai_availability_info_free(info);
}

/* authorized_nssai_availability_info: JSON round-trip. */
static void test_authorized_nssai_availability_info_roundtrip(void)
{
    const char *json_str =
        "{\"authorizedNssaiAvailabilityData\":[],"
        "\"supportedFeatures\":\"b2\"}";
    cJSON *json = cJSON_Parse(json_str);
    TEST_ASSERT_NOT_NULL(json);

    authorized_nssai_availability_info_t *info =
        authorized_nssai_availability_info_parseFromJSON(json);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_NOT_NULL(info->authorized_nssai_availability_data);
    TEST_ASSERT_EQUAL_STRING("b2", info->supported_features);

    cJSON *out = authorized_nssai_availability_info_convertToJSON(info);
    TEST_ASSERT_NOT_NULL(out);
    char *txt = cJSON_PrintUnformatted(out);
    TEST_ASSERT_NOT_NULL(txt);
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"authorizedNssaiAvailabilityData\":[]"));
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"supportedFeatures\":\"b2\""));

    free(txt);
    cJSON_Delete(out);
    cJSON_Delete(json);
    authorized_nssai_availability_info_free(info);
}

/*
 * nssf_event_subscription_create_data: parseFromJSON-based smoke.
 * _create 는 13-arg + nested model (event, nsrp/nsiun subscribe info) 라
 * parse 가 더 간결하고 안전. required field = nfNssaiAvailabilityUri (string)
 * + event (object). nssf_event_type 은 empty struct 이므로 event={}.
 */
static void test_nssf_event_subscription_create_data_parse(void)
{
    const char *json_str =
        "{\"nfNssaiAvailabilityUri\":\"https://amf.example/nssai-availability\","
        "\"event\":{},\"amfSetId\":\"set-9\"}";
    cJSON *json = cJSON_Parse(json_str);
    TEST_ASSERT_NOT_NULL(json);

    nssf_event_subscription_create_data_t *data =
        nssf_event_subscription_create_data_parseFromJSON(json);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_STRING(
        "https://amf.example/nssai-availability",
        data->nf_nssai_availability_uri);
    TEST_ASSERT_NOT_NULL(data->event);
    TEST_ASSERT_EQUAL_STRING("set-9", data->amf_set_id);

    cJSON *out = nssf_event_subscription_create_data_convertToJSON(data);
    TEST_ASSERT_NOT_NULL(out);
    char *txt = cJSON_PrintUnformatted(out);
    TEST_ASSERT_NOT_NULL(txt);
    TEST_ASSERT_NOT_NULL(strstr(txt,
        "\"nfNssaiAvailabilityUri\":\"https://amf.example/nssai-availability\""));
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"amfSetId\":\"set-9\""));

    free(txt);
    cJSON_Delete(out);
    cJSON_Delete(json);
    nssf_event_subscription_create_data_free(data);
}

/*
 * nssf_event_subscription_created_data: _create round-trip.
 * subscription_id required (convertToJSON 이 NULL 이면 fail). 나머지 list 는
 * NULL 허용 — optional 이므로 직렬화 생략된다.
 */
static void test_nssf_event_subscription_created_data_roundtrip(void)
{
    char *sub_id = strdup("sub-42");
    char *expiry = strdup("2026-12-31T23:59:59Z");
    TEST_ASSERT_NOT_NULL(sub_id);
    TEST_ASSERT_NOT_NULL(expiry);

    nssf_event_subscription_created_data_t *data =
        nssf_event_subscription_created_data_create(
            sub_id, expiry, NULL, NULL, NULL);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_STRING("sub-42", data->subscription_id);

    cJSON *out = nssf_event_subscription_created_data_convertToJSON(data);
    TEST_ASSERT_NOT_NULL(out);
    char *txt = cJSON_PrintUnformatted(out);
    TEST_ASSERT_NOT_NULL(txt);
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"subscriptionId\":\"sub-42\""));
    TEST_ASSERT_NOT_NULL(strstr(txt, "\"expiry\":\"2026-12-31T23:59:59Z\""));

    free(txt);
    cJSON_Delete(out);
    nssf_event_subscription_created_data_free(data);  /* frees sub_id + expiry */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_snssai_roundtrip);
    RUN_TEST(test_snssai_parse_from_json);
    RUN_TEST(test_plmn_id_roundtrip);
    RUN_TEST(test_cjson_vendored_works);
    RUN_TEST(test_nssai_availability_info_roundtrip);
    RUN_TEST(test_authorized_nssai_availability_info_roundtrip);
    RUN_TEST(test_nssf_event_subscription_create_data_parse);
    RUN_TEST(test_nssf_event_subscription_created_data_roundtrip);
    return UNITY_END();
}
