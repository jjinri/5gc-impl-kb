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

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_snssai_roundtrip);
    RUN_TEST(test_snssai_parse_from_json);
    RUN_TEST(test_plmn_id_roundtrip);
    RUN_TEST(test_cjson_vendored_works);
    return UNITY_END();
}
