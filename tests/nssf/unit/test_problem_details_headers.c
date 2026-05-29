/*
 * test_problem_details_headers.c — mandatory_headers catalog 정합성.
 *
 * 본 test 는 wrapper metadata 의 mandatory_headers list 가
 * error-cause-catalog.yaml 의 각 cause 와 1:1 일치하는지 검증한다. 실제
 * header 송신 layer (sbi_server_stack) 가 본 metadata 를 그대로 transport 에
 * 부착하므로 본 metadata 의 정확성이 송신 정합성의 단일 진실 출처다.
 */

#define _POSIX_C_SOURCE 200809L

#include "nf_problem_details_wrapper.h"
#include "unity.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static int header_list_contains(const nssf_pd_metadata_t *meta,
                                const char *needle)
{
    if (meta == NULL || meta->mandatory_headers == NULL) {
        return 0;
    }
    for (size_t i = 0; i < meta->mandatory_header_count; ++i) {
        if (strstr(meta->mandatory_headers[i], needle) != NULL) {
            return 1;
        }
    }
    return 0;
}

static void test_401_token_missing_has_www_authenticate(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_UNAUTHORIZED_TOKEN_MISSING);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(1, meta->mandatory_header_count);
    TEST_ASSERT_TRUE(header_list_contains(meta, "WWW-Authenticate"));
    TEST_ASSERT_TRUE(header_list_contains(meta, "Bearer"));
}

static void test_401_token_invalid_has_www_authenticate(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_UNAUTHORIZED_TOKEN_INVALID);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(1, meta->mandatory_header_count);
    TEST_ASSERT_TRUE(header_list_contains(meta, "WWW-Authenticate"));
}

static void test_405_method_not_allowed_has_allow(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_METHOD_NOT_ALLOWED);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(1, meta->mandatory_header_count);
    TEST_ASSERT_TRUE(header_list_contains(meta, "Allow"));
}

static void test_415_json_patch_has_accept_patch(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON_PATCH);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(1, meta->mandatory_header_count);
    TEST_ASSERT_TRUE(header_list_contains(meta, "Accept-Patch"));
    TEST_ASSERT_TRUE(header_list_contains(meta, "application/json-patch+json"));
}

static void test_429_too_many_requests_has_retry_after(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_TOO_MANY_REQUESTS);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(1, meta->mandatory_header_count);
    TEST_ASSERT_TRUE(header_list_contains(meta, "Retry-After"));
}

static void test_503_db_has_retry_after(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_SERVICE_UNAVAILABLE_DB);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(1, meta->mandatory_header_count);
    TEST_ASSERT_TRUE(header_list_contains(meta, "Retry-After"));
}

static void test_503_upstream_has_retry_after(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_SERVICE_UNAVAILABLE_UPSTREAM);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(1, meta->mandatory_header_count);
    TEST_ASSERT_TRUE(header_list_contains(meta, "Retry-After"));
}

static void test_415_json_has_no_mandatory_header(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_UNSUPPORTED_MEDIA_TYPE_JSON);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(0, meta->mandatory_header_count);
    TEST_ASSERT_NULL(meta->mandatory_headers);
}

static void test_400_invalid_body_has_no_mandatory_header(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_BAD_REQUEST_INVALID_BODY);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(0, meta->mandatory_header_count);
}

static void test_500_internal_server_error_has_no_mandatory_header(void)
{
    const nssf_pd_metadata_t *meta =
        nssf_pd_metadata_for(NSSF_PD_INTERNAL_SERVER_ERROR);
    TEST_ASSERT_NOT_NULL(meta);
    TEST_ASSERT_EQUAL_size_t(0, meta->mandatory_header_count);
}

static void test_cause_names_distinct(void)
{
    for (int i = 0; i < NSSF_PD_CAUSE_COUNT; ++i) {
        const char *name_a = nssf_pd_cause_name((nssf_pd_cause_e)i);
        TEST_ASSERT_NOT_NULL(name_a);
        for (int j = i + 1; j < NSSF_PD_CAUSE_COUNT; ++j) {
            const char *name_b = nssf_pd_cause_name((nssf_pd_cause_e)j);
            TEST_ASSERT_NOT_NULL(name_b);
            TEST_ASSERT_NOT_EQUAL(0, strcmp(name_a, name_b));
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_401_token_missing_has_www_authenticate);
    RUN_TEST(test_401_token_invalid_has_www_authenticate);
    RUN_TEST(test_405_method_not_allowed_has_allow);
    RUN_TEST(test_415_json_patch_has_accept_patch);
    RUN_TEST(test_429_too_many_requests_has_retry_after);
    RUN_TEST(test_503_db_has_retry_after);
    RUN_TEST(test_503_upstream_has_retry_after);
    RUN_TEST(test_415_json_has_no_mandatory_header);
    RUN_TEST(test_400_invalid_body_has_no_mandatory_header);
    RUN_TEST(test_500_internal_server_error_has_no_mandatory_header);
    RUN_TEST(test_cause_names_distinct);
    return UNITY_END();
}
