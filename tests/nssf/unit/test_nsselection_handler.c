#include "nsselection_handler.h"
#include "unity.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_returns_501(void)
{
    char body[256];
    int status = nsselection_get_handle(NULL, body, sizeof(body));
    TEST_ASSERT_EQUAL_INT(501, status);
}

static void test_body_contains_status_501(void)
{
    char body[256];
    nsselection_get_handle(NULL, body, sizeof(body));
    TEST_ASSERT_NOT_NULL(strstr(body, "\"status\":501"));
}

static void test_body_contains_problem_type(void)
{
    char body[256];
    nsselection_get_handle(NULL, body, sizeof(body));
    TEST_ASSERT_NOT_NULL(strstr(body, "\"type\":\"urn:5gc:nssf:tracer-bullet-stub\""));
}

static void test_body_contains_phase1_detail(void)
{
    char body[256];
    nsselection_get_handle(NULL, body, sizeof(body));
    TEST_ASSERT_NOT_NULL(strstr(body, "\"detail\":\"Phase 1 tracer-bullet stub\""));
}

static void test_null_body_does_not_crash(void)
{
    int status = nsselection_get_handle(NULL, NULL, 0);
    TEST_ASSERT_EQUAL_INT(501, status);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_returns_501);
    RUN_TEST(test_body_contains_status_501);
    RUN_TEST(test_body_contains_problem_type);
    RUN_TEST(test_body_contains_phase1_detail);
    RUN_TEST(test_null_body_does_not_crash);
    return UNITY_END();
}
