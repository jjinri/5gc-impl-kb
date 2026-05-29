/*
 * test_nftype_wrapper.c — WI-nftype-wrapper acceptance.
 *
 * (a) known enum probe + canonical round-trip (대표 + 전수 67 enum).
 * (b) unknown string → UNKNOWN + raw lossless retain (G-09).
 * (c) NULL / empty 입력 처리.
 * (d) to_string + is_known 일관성.
 */

#define _POSIX_C_SOURCE 200809L

#include "nf_type_wrapper.h"
#include "unity.h"

#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_known_amf_probe_and_to_string(void)
{
    TEST_ASSERT_EQUAL_INT(NSSF_NF_TYPE_AMF, nssf_nf_type_probe("AMF"));
    TEST_ASSERT_EQUAL_STRING("AMF", nssf_nf_type_to_string(NSSF_NF_TYPE_AMF));
    TEST_ASSERT_TRUE(nssf_nf_type_is_known(NSSF_NF_TYPE_AMF));
}

static void test_known_nssf_probe(void)
{
    TEST_ASSERT_EQUAL_INT(NSSF_NF_TYPE_NSSF, nssf_nf_type_probe("NSSF"));
    TEST_ASSERT_EQUAL_STRING("NSSF", nssf_nf_type_to_string(NSSF_NF_TYPE_NSSF));
}

static void test_known_nrf_probe(void)
{
    TEST_ASSERT_EQUAL_INT(NSSF_NF_TYPE_NRF, nssf_nf_type_probe("NRF"));
}

static void test_known_special_5g_eir(void)
{
    /* 5G_EIR — 숫자 prefix enum, NFType.json 의 13번째 (index 12). */
    TEST_ASSERT_EQUAL_INT(NSSF_NF_TYPE_5G_EIR, nssf_nf_type_probe("5G_EIR"));
    TEST_ASSERT_EQUAL_STRING("5G_EIR",
                             nssf_nf_type_to_string(NSSF_NF_TYPE_5G_EIR));
}

static void test_unknown_string_passthrough(void)
{
    /* G-09 — 미래 NFType 가정. */
    nssf_nf_type_t *t = nssf_nf_type_create_from_string("FUTURE_NF_TYPE");
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQUAL_INT(NSSF_NF_TYPE_UNKNOWN, t->probed);
    TEST_ASSERT_FALSE(nssf_nf_type_is_known(t->probed));
    TEST_ASSERT_NOT_NULL(t->raw);
    TEST_ASSERT_EQUAL_STRING("FUTURE_NF_TYPE", t->raw);
    nssf_nf_type_free(t);
}

static void test_lowercase_is_unknown(void)
{
    /* canonical 은 UPPERCASE — lowercase 변형은 unknown 으로 처리. */
    nssf_nf_type_t *t = nssf_nf_type_create_from_string("amf");
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQUAL_INT(NSSF_NF_TYPE_UNKNOWN, t->probed);
    TEST_ASSERT_EQUAL_STRING("amf", t->raw);
    nssf_nf_type_free(t);
}

static void test_null_input_returns_null(void)
{
    TEST_ASSERT_NULL(nssf_nf_type_create_from_string(NULL));
    TEST_ASSERT_EQUAL_INT(NSSF_NF_TYPE_UNKNOWN, nssf_nf_type_probe(NULL));
}

static void test_empty_input_unknown(void)
{
    nssf_nf_type_t *t = nssf_nf_type_create_from_string("");
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQUAL_INT(NSSF_NF_TYPE_UNKNOWN, t->probed);
    TEST_ASSERT_NOT_NULL(t->raw);
    TEST_ASSERT_EQUAL_STRING("", t->raw);
    nssf_nf_type_free(t);
}

static void test_to_string_unknown_label(void)
{
    TEST_ASSERT_EQUAL_STRING("UNKNOWN",
                             nssf_nf_type_to_string(NSSF_NF_TYPE_UNKNOWN));
}

static void test_to_string_out_of_range_returns_unknown_label(void)
{
    TEST_ASSERT_EQUAL_STRING(
        "UNKNOWN",
        nssf_nf_type_to_string((nssf_nf_type_e)NSSF_NF_TYPE_KNOWN_COUNT));
}

static void test_free_null_is_safe(void)
{
    nssf_nf_type_free(NULL);  /* must not crash */
    TEST_PASS();
}

static void test_all_67_known_enum_roundtrip(void)
{
    /* 전수 round-trip — probe(to_string(e)) == e for e in [0, KNOWN_COUNT). */
    for (int i = 0; i < NSSF_NF_TYPE_KNOWN_COUNT; ++i) {
        const char *name = nssf_nf_type_to_string((nssf_nf_type_e)i);
        TEST_ASSERT_NOT_NULL(name);
        TEST_ASSERT_TRUE(nssf_nf_type_is_known((nssf_nf_type_e)i));
        nssf_nf_type_e back = nssf_nf_type_probe(name);
        TEST_ASSERT_EQUAL_INT(i, back);
    }
}

static void test_known_count_matches_spec(void)
{
    TEST_ASSERT_EQUAL_INT(67, NSSF_NF_TYPE_KNOWN_COUNT);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_known_count_matches_spec);
    RUN_TEST(test_known_amf_probe_and_to_string);
    RUN_TEST(test_known_nssf_probe);
    RUN_TEST(test_known_nrf_probe);
    RUN_TEST(test_known_special_5g_eir);
    RUN_TEST(test_unknown_string_passthrough);
    RUN_TEST(test_lowercase_is_unknown);
    RUN_TEST(test_null_input_returns_null);
    RUN_TEST(test_empty_input_unknown);
    RUN_TEST(test_to_string_unknown_label);
    RUN_TEST(test_to_string_out_of_range_returns_unknown_label);
    RUN_TEST(test_free_null_is_safe);
    RUN_TEST(test_all_67_known_enum_roundtrip);
    return UNITY_END();
}
