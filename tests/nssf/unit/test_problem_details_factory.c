/*
 * test_problem_details_factory.c — 18 production cause round-trip.
 *
 * 각 factory 호출 → problem_details_t 검증 (status/title/type_urn 일치) →
 * cJSON serialize → parseFromJSON 후 동일성 검증 (round-trip 무결성).
 */

#define _POSIX_C_SOURCE 200809L

#include "nf_problem_details_wrapper.h"
#include "problem_details.h"
#include "cJSON.h"
#include "unity.h"

#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void assert_factory_matches_metadata(problem_details_t *pd,
                                            nssf_pd_cause_e cause)
{
    const nssf_pd_metadata_t *meta = nssf_pd_metadata_for(cause);
    const char *expected_cause = nssf_pd_cause_name(cause);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_NOT_NULL(expected_cause);
    TEST_ASSERT_NOT_NULL(pd);
    TEST_ASSERT_EQUAL_INT(meta->status, pd->status);
    TEST_ASSERT_EQUAL_STRING(meta->title, pd->title);
    TEST_ASSERT_EQUAL_STRING(meta->type_urn, pd->type);
    TEST_ASSERT_NOT_NULL(pd->detail);
    TEST_ASSERT_NOT_NULL(pd->cause);
    TEST_ASSERT_EQUAL_STRING(expected_cause, pd->cause);
}

static void roundtrip_cause(nssf_pd_cause_e cause)
{
    problem_details_t *pd = nf_problem_details_make_by_cause(cause, NULL, "/nnssf-nsselection/v2/foo");
    assert_factory_matches_metadata(pd, cause);

    cJSON *json = problem_details_convertToJSON(pd);
    TEST_ASSERT_NOT_NULL(json);

    char *txt = cJSON_PrintUnformatted(json);
    TEST_ASSERT_NOT_NULL(txt);

    cJSON *parsed_json = cJSON_Parse(txt);
    TEST_ASSERT_NOT_NULL(parsed_json);
    problem_details_t *parsed = problem_details_parseFromJSON(parsed_json);
    TEST_ASSERT_NOT_NULL(parsed);

    const nssf_pd_metadata_t *meta = nssf_pd_metadata_for(cause);
    const char *expected_cause = nssf_pd_cause_name(cause);
    TEST_ASSERT_EQUAL_INT(meta->status, parsed->status);
    TEST_ASSERT_EQUAL_STRING(meta->title, parsed->title);
    TEST_ASSERT_EQUAL_STRING(meta->type_urn, parsed->type);
    TEST_ASSERT_EQUAL_STRING("/nnssf-nsselection/v2/foo", parsed->instance);
    TEST_ASSERT_NOT_NULL(parsed->cause);
    TEST_ASSERT_EQUAL_STRING(expected_cause, parsed->cause);

    problem_details_free(parsed);
    cJSON_Delete(parsed_json);
    free(txt);
    cJSON_Delete(json);
    problem_details_free(pd);
}

static void test_cause_count_matches_catalog(void)
{
    TEST_ASSERT_EQUAL_INT(18, NSSF_PD_CAUSE_COUNT);
}

static void test_cause_metadata_for_invalid_returns_null(void)
{
    TEST_ASSERT_NULL(nssf_pd_metadata_for((nssf_pd_cause_e)-1));
    TEST_ASSERT_NULL(nssf_pd_metadata_for((nssf_pd_cause_e)NSSF_PD_CAUSE_COUNT));
    TEST_ASSERT_NULL(nssf_pd_cause_name((nssf_pd_cause_e)-1));
    TEST_ASSERT_NULL(nssf_pd_cause_name((nssf_pd_cause_e)NSSF_PD_CAUSE_COUNT));
}

static void test_roundtrip_400_invalid_body(void)
{
    roundtrip_cause(NSSF_PD_BAD_REQUEST_INVALID_BODY);
}
static void test_roundtrip_400_invalid_param(void)
{
    roundtrip_cause(NSSF_PD_BAD_REQUEST_INVALID_PARAM);
}
static void test_roundtrip_401_token_missing(void)
{
    roundtrip_cause(NSSF_PD_UNAUTHORIZED_TOKEN_MISSING);
}
static void test_roundtrip_401_token_invalid(void)
{
    roundtrip_cause(NSSF_PD_UNAUTHORIZED_TOKEN_INVALID);
}
static void test_roundtrip_403(void)
{
    roundtrip_cause(NSSF_PD_FORBIDDEN_SCOPE_MISMATCH);
}
static void test_roundtrip_404_no_availability(void)
{
    roundtrip_cause(NSSF_PD_NOT_FOUND_NO_AVAILABILITY);
}
static void test_roundtrip_404_subscription(void)
{
    roundtrip_cause(NSSF_PD_NOT_FOUND_SUBSCRIPTION);
}
static void test_roundtrip_405(void)
{
    roundtrip_cause(NSSF_PD_METHOD_NOT_ALLOWED);
}
static void test_roundtrip_408(void)
{
    roundtrip_cause(NSSF_PD_REQUEST_TIMEOUT);
}
static void test_roundtrip_411(void)
{
    roundtrip_cause(NSSF_PD_PAYLOAD_LENGTH_REQUIRED);
}
static void test_roundtrip_413(void)
{
    roundtrip_cause(NSSF_PD_PAYLOAD_TOO_LARGE);
}
static void test_roundtrip_414(void)
{
    roundtrip_cause(NSSF_PD_URI_TOO_LONG);
}
static void test_roundtrip_415_json(void)
{
    roundtrip_cause(NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON);
}
static void test_roundtrip_415_json_patch(void)
{
    roundtrip_cause(NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON_PATCH);
}
static void test_roundtrip_429(void)
{
    roundtrip_cause(NSSF_PD_TOO_MANY_REQUESTS);
}
static void test_roundtrip_500(void)
{
    roundtrip_cause(NSSF_PD_INTERNAL_SERVER_ERROR);
}
static void test_roundtrip_503_db(void)
{
    roundtrip_cause(NSSF_PD_SERVICE_UNAVAILABLE_DB);
}
static void test_roundtrip_503_upstream(void)
{
    roundtrip_cause(NSSF_PD_SERVICE_UNAVAILABLE_UPSTREAM);
}

static void test_variant_factory_makes_same_result_as_generic(void)
{
    problem_details_t *via_generic = nf_problem_details_make_by_cause(NSSF_PD_BAD_REQUEST_INVALID_BODY,
                                                  NULL, NULL);
    problem_details_t *via_variant = nf_problem_details_make_400_invalid_body(NULL, NULL);
    TEST_ASSERT_NOT_NULL(via_generic);
    TEST_ASSERT_NOT_NULL(via_variant);
    TEST_ASSERT_EQUAL_INT(via_generic->status, via_variant->status);
    TEST_ASSERT_EQUAL_STRING(via_generic->title, via_variant->title);
    TEST_ASSERT_EQUAL_STRING(via_generic->type, via_variant->type);
    TEST_ASSERT_EQUAL_STRING(via_generic->detail, via_variant->detail);
    TEST_ASSERT_NOT_NULL(via_generic->cause);
    TEST_ASSERT_NOT_NULL(via_variant->cause);
    TEST_ASSERT_EQUAL_STRING(via_generic->cause, via_variant->cause);
    problem_details_free(via_generic);
    problem_details_free(via_variant);
}

static void test_custom_detail_overrides_template(void)
{
    const char *detail = "Custom error reason 42";
    problem_details_t *pd = nf_problem_details_make_by_cause(NSSF_PD_INTERNAL_SERVER_ERROR,
                                         detail, NULL);
    TEST_ASSERT_NOT_NULL(pd);
    TEST_ASSERT_EQUAL_STRING(detail, pd->detail);
    problem_details_free(pd);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_cause_count_matches_catalog);
    RUN_TEST(test_cause_metadata_for_invalid_returns_null);
    RUN_TEST(test_roundtrip_400_invalid_body);
    RUN_TEST(test_roundtrip_400_invalid_param);
    RUN_TEST(test_roundtrip_401_token_missing);
    RUN_TEST(test_roundtrip_401_token_invalid);
    RUN_TEST(test_roundtrip_403);
    RUN_TEST(test_roundtrip_404_no_availability);
    RUN_TEST(test_roundtrip_404_subscription);
    RUN_TEST(test_roundtrip_405);
    RUN_TEST(test_roundtrip_408);
    RUN_TEST(test_roundtrip_411);
    RUN_TEST(test_roundtrip_413);
    RUN_TEST(test_roundtrip_414);
    RUN_TEST(test_roundtrip_415_json);
    RUN_TEST(test_roundtrip_415_json_patch);
    RUN_TEST(test_roundtrip_429);
    RUN_TEST(test_roundtrip_500);
    RUN_TEST(test_roundtrip_503_db);
    RUN_TEST(test_roundtrip_503_upstream);
    RUN_TEST(test_variant_factory_makes_same_result_as_generic);
    RUN_TEST(test_custom_detail_overrides_template);
    return UNITY_END();
}
