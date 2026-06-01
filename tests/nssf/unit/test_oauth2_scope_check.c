/*
 * test_oauth2_scope_check.c — WI-tls-bootstrap OAuth2 scope-check acceptance.
 *
 * Pure, libc-only. nssf_oauth2_scope_match 의 whole-token (space-delimited)
 * 의미론과 nssf_oauth2_scope_for_operation 의 operation→scope 매핑을 검증.
 * substring-but-not-token trap 은 401/403 deny path 의 핵심 — token claim 안에
 * required scope 가 부분 문자열로 들어있어도 *whole token* 이 아니면 deny 다.
 */

#define _POSIX_C_SOURCE 200809L

#include "oauth2_scope_check.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

/* ---- nssf_oauth2_scope_match — positive (200/authorized path) ---- */

static void test_match_single_token_present(void)
{
    TEST_ASSERT_TRUE(nssf_oauth2_scope_match(NSSF_SCOPE_NSSELECTION,
                                             NSSF_SCOPE_NSSELECTION));
}

static void test_match_multi_token_claim_first(void)
{
    TEST_ASSERT_TRUE(nssf_oauth2_scope_match(
        "nnssf-nsselection nnssf-nssaiavailability", "nnssf-nsselection"));
}

static void test_match_multi_token_claim_second(void)
{
    TEST_ASSERT_TRUE(nssf_oauth2_scope_match(
        "nnssf-nsselection nnssf-nssaiavailability", "nnssf-nssaiavailability"));
}

static void test_match_tolerates_extra_spaces(void)
{
    /* Leading / interior multiple spaces must still resolve whole tokens. */
    TEST_ASSERT_TRUE(nssf_oauth2_scope_match(
        "  nnssf-nsselection   nnssf-nssaiavailability  ",
        "nnssf-nssaiavailability"));
}

/* ---- nssf_oauth2_scope_match — negative (deny path = 401/403) ---- */

static void test_match_absent_scope_denied(void)
{
    TEST_ASSERT_FALSE(nssf_oauth2_scope_match(
        "nnssf-nsselection nnssf-nssaiavailability", "nnssf-foo"));
}

static void test_match_substring_prefix_trap_denied(void)
{
    /*
     * required "nssf-nsselection" is a strict substring of the claim token
     * "nnssf-nsselection" but NOT a whole space-delimited token → deny.
     */
    TEST_ASSERT_FALSE(
        nssf_oauth2_scope_match("nnssf-nsselection", "nssf-nsselection"));
}

static void test_match_substring_truncated_trap_denied(void)
{
    /* required "nnssf-nssel" is a prefix-substring of the claim token → deny. */
    TEST_ASSERT_FALSE(
        nssf_oauth2_scope_match("nnssf-nsselection", "nnssf-nssel"));
}

static void test_match_required_superstring_denied(void)
{
    /* required longer than the claim token → never a whole-token match. */
    TEST_ASSERT_FALSE(nssf_oauth2_scope_match(
        "nnssf-nsselection", "nnssf-nsselection-extra"));
}

static void test_match_null_and_empty_denied(void)
{
    TEST_ASSERT_FALSE(nssf_oauth2_scope_match(NULL, NSSF_SCOPE_NSSELECTION));
    TEST_ASSERT_FALSE(
        nssf_oauth2_scope_match("nnssf-nsselection nnssf-nssaiavailability",
                                NULL));
    TEST_ASSERT_FALSE(
        nssf_oauth2_scope_match("nnssf-nsselection", ""));
    TEST_ASSERT_FALSE(nssf_oauth2_scope_match("", "nnssf-nsselection"));
}

/* ---- nssf_oauth2_scope_for_operation — operation→scope partition ---- */

static void test_scope_for_nsselection_family(void)
{
    TEST_ASSERT_EQUAL_STRING(NSSF_SCOPE_NSSELECTION,
                             nssf_oauth2_scope_for_operation("NSSelectionGet"));
    TEST_ASSERT_EQUAL_STRING(
        NSSF_SCOPE_NSSELECTION,
        nssf_oauth2_scope_for_operation("Nnssf_NSSelection"));
    TEST_ASSERT_EQUAL_STRING(
        NSSF_SCOPE_NSSELECTION,
        nssf_oauth2_scope_for_operation("nnssf-nsselection"));
}

static void test_scope_for_nssaiavailability_family(void)
{
    TEST_ASSERT_EQUAL_STRING(
        NSSF_SCOPE_NSSAIAVAILABILITY,
        nssf_oauth2_scope_for_operation("NSSAIAvailabilityPut"));
    TEST_ASSERT_EQUAL_STRING(
        NSSF_SCOPE_NSSAIAVAILABILITY,
        nssf_oauth2_scope_for_operation("NSSAIAvailabilityPostSubscriptions"));
    TEST_ASSERT_EQUAL_STRING(
        NSSF_SCOPE_NSSAIAVAILABILITY,
        nssf_oauth2_scope_for_operation("Nnssf_NSSAIAvailability"));
    TEST_ASSERT_EQUAL_STRING(
        NSSF_SCOPE_NSSAIAVAILABILITY,
        nssf_oauth2_scope_for_operation("nnssf-nssaiavailability"));
}

static void test_scope_for_unknown_is_null(void)
{
    TEST_ASSERT_NULL(nssf_oauth2_scope_for_operation("Nudm_SDM"));
    TEST_ASSERT_NULL(nssf_oauth2_scope_for_operation("totally-unknown-op"));
    TEST_ASSERT_NULL(nssf_oauth2_scope_for_operation(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_match_single_token_present);
    RUN_TEST(test_match_multi_token_claim_first);
    RUN_TEST(test_match_multi_token_claim_second);
    RUN_TEST(test_match_tolerates_extra_spaces);
    RUN_TEST(test_match_absent_scope_denied);
    RUN_TEST(test_match_substring_prefix_trap_denied);
    RUN_TEST(test_match_substring_truncated_trap_denied);
    RUN_TEST(test_match_required_superstring_denied);
    RUN_TEST(test_match_null_and_empty_denied);
    RUN_TEST(test_scope_for_nsselection_family);
    RUN_TEST(test_scope_for_nssaiavailability_family);
    RUN_TEST(test_scope_for_unknown_is_null);
    return UNITY_END();
}
