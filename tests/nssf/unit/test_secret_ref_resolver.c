/*
 * test_secret_ref_resolver.c — WI-config-hardening (ADR-0004 M7) unit suite.
 *
 * secret_ref_resolver 의 env:/file: arm 을 behavioral 하게 검증한다. 실제 secret
 * 저장소(vault) 없이, env 케이스는 setenv/unsetenv, file 케이스는 mkstemp+write
 * 로 로컬 fixture 를 만든다 — no network, no DB, 무조건 CI 실행. resolved buffer 는
 * 전부 nssf_secret_ref_scrub_free 로 해제해 LeakSan/ASan-clean 을 유지한다.
 *
 * Coverage map (verification-plan WI-config-hardening):
 *   env happy / verbatim-no-trim / unset NOT_FOUND / empty-operand BAD_FORMAT /
 *     empty-value OK len 0
 *   file happy / trailing-LF strip / CRLF strip / internal-LF preserved /
 *     double-trailing only-one-stripped / missing NOT_FOUND / >64KiB TOO_LARGE
 *   bad-format matrix (NULL / "" / vault: / noscheme / file: / env:)
 *   fail-fast invariant (*out_secret==NULL && *out_len==0 on every non-OK)
 *   scrub_free NULL-safe
 *   status_str 가 secret 을 담지 않는 generic class word 만 반환 (no-log behavior
 *     proxy — 실제 no-value-logging 은 reviewer lane 의 code review 가 enforce)
 *
 * 주: 이 모듈의 에러 경로는 dzlog_error 를 호출한다. zlog 는 default category 가
 * 미초기화(NULL)면 dzlog 가 early-return 하도록 방어되어 있어 dzlog_init 없이
 * 안전하다 (기존 nssf_runtime 링크 테스트들과 동일하게 zlog 미초기화로 동작).
 */

#define _POSIX_C_SOURCE 200809L

#include "secret_ref_resolver.h"
#include "unity.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---- temp-file fixture: 단일 활성 파일 경로를 teardown 에서 unlink ---- */

static char g_tmp_path[256];
static bool g_tmp_active;

/*
 * mkstemp 로 임시 파일을 만들고 content (len 바이트, NUL 무관 바이너리-safe) 를
 * 그대로 기록한다. 경로를 g_tmp_path 에 저장 → tearDown 이 unlink. 0 성공.
 */
static int write_tmp_secret(const char *content, size_t len)
{
    snprintf(g_tmp_path, sizeof(g_tmp_path), "/tmp/nssf_secret_ref_XXXXXX");
    int fd = mkstemp(g_tmp_path);
    if (fd < 0) {
        return -1;
    }
    g_tmp_active = true;
    size_t off = 0;
    while (off < len) {
        ssize_t w = write(fd, content + off, len - off);
        if (w < 0) {
            close(fd);
            return -1;
        }
        off += (size_t)w;
    }
    close(fd);
    return 0;
}

/* "file:" + g_tmp_path 형태의 ref 를 만들어 caller buffer 에 채운다. */
static void make_file_ref(char *out, size_t out_sz)
{
    snprintf(out, out_sz, "file:%s", g_tmp_path);
}

void setUp(void)
{
    g_tmp_active = false;
    g_tmp_path[0] = '\0';
}

void tearDown(void)
{
    if (g_tmp_active) {
        unlink(g_tmp_path);
        g_tmp_active = false;
    }
}

/* 공통 helper: 비-OK 상태에서 fail-fast 불변식 (out NULL + len 0) 확인. */
static void assert_fail_fast(nssf_secret_ref_status_t st, char *out, size_t len)
{
    TEST_ASSERT_NOT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)len);
}

/* ============================ env happy (#1) ============================== */

static void test_env_happy(void)
{
    TEST_ASSERT_EQUAL_INT(0, setenv("NSSF_TEST_SECRET_HAPPY", "topsecret", 1));

    char *out = NULL;
    size_t len = 0;
    nssf_secret_ref_status_t st =
        nssf_secret_ref_resolve("env:NSSF_TEST_SECRET_HAPPY", &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(strlen("topsecret"), (unsigned)len);
    /* exact bytes + NUL terminator present (safe to treat as C string). */
    TEST_ASSERT_EQUAL_STRING("topsecret", out);
    TEST_ASSERT_EQUAL_UINT8('\0', out[len]);

    nssf_secret_ref_scrub_free(out, len); /* scrub+free owned buffer. */
    unsetenv("NSSF_TEST_SECRET_HAPPY");
}

/* ===================== env verbatim — no trim (#2) ======================== */

static void test_env_verbatim_no_trim(void)
{
    /* env arm 은 어떤 trimming 도 하지 않는다 — 후행 공백 + 후행 개행 모두 보존. */
    const char *raw = "  spaced-secret  \n";
    TEST_ASSERT_EQUAL_INT(0, setenv("NSSF_TEST_SECRET_VERBATIM", raw, 1));

    char *out = NULL;
    size_t len = 0;
    nssf_secret_ref_status_t st =
        nssf_secret_ref_resolve("env:NSSF_TEST_SECRET_VERBATIM", &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(strlen(raw), (unsigned)len);
    TEST_ASSERT_EQUAL_STRING(raw, out); /* byte-for-byte, no trim. */

    nssf_secret_ref_scrub_free(out, len);
    unsetenv("NSSF_TEST_SECRET_VERBATIM");
}

/* ========================= env unset → NOT_FOUND (#3) ===================== */

static void test_env_unset_not_found(void)
{
    unsetenv("NSSF_TEST_SECRET_ABSENT"); /* 확실히 unset. */

    char *out = (char *)0x1; /* poison — resolver 가 반드시 NULL 로 덮어야 한다. */
    size_t len = 99;
    nssf_secret_ref_status_t st =
        nssf_secret_ref_resolve("env:NSSF_TEST_SECRET_ABSENT", &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_NOT_FOUND, st);
    assert_fail_fast(st, out, len);
}

/* =================== env empty operand → BAD_FORMAT (#4) ================== */

static void test_env_empty_operand_bad_format(void)
{
    char *out = (char *)0x1;
    size_t len = 99;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve("env:", &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_BAD_FORMAT, st);
    assert_fail_fast(st, out, len);
}

/* ============== env explicitly-empty value → OK, len 0 (#5) =============== */

static void test_env_empty_value_ok_len0(void)
{
    /*
     * Header 계약 (secret_ref_resolver.h L72): *out_len 은 "env var 가 명시적으로
     * empty string 으로 set 된 경우에만 0 일 수 있다" — present-but-empty 는 OK
     * 이며 len 0, NUL terminator 는 항상 존재. nf-code 구현(resolve_env)도 빈 값을
     * strlen 0 으로 보고 OK 반환. fail-fast (BAD_FORMAT) 가 아님을 잠근다.
     */
    TEST_ASSERT_EQUAL_INT(0, setenv("NSSF_TEST_SECRET_EMPTY", "", 1));

    char *out = NULL;
    size_t len = 99;
    nssf_secret_ref_status_t st =
        nssf_secret_ref_resolve("env:NSSF_TEST_SECRET_EMPTY", &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_NOT_NULL(out); /* owned buffer present even for empty value. */
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)len);
    TEST_ASSERT_EQUAL_UINT8('\0', out[0]); /* terminator always present. */

    nssf_secret_ref_scrub_free(out, len);
    unsetenv("NSSF_TEST_SECRET_EMPTY");
}

/* ============================ file happy (#6) ============================= */

static void test_file_happy_no_newline(void)
{
    TEST_ASSERT_EQUAL_INT(0, write_tmp_secret("secret123", 9));
    char ref[320];
    make_file_ref(ref, sizeof(ref));

    char *out = NULL;
    size_t len = 0;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(ref, &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_UINT(9, (unsigned)len);
    TEST_ASSERT_EQUAL_STRING("secret123", out);

    nssf_secret_ref_scrub_free(out, len);
}

/* ===================== file trailing-LF stripped (#7) ===================== */

static void test_file_trailing_lf_stripped(void)
{
    /* "echo secret > file" 형태 — 정확히 하나의 \n 제거. */
    TEST_ASSERT_EQUAL_INT(0, write_tmp_secret("secret123\n", 10));
    char ref[320];
    make_file_ref(ref, sizeof(ref));

    char *out = NULL;
    size_t len = 0;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(ref, &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_EQUAL_UINT(9, (unsigned)len); /* LF dropped. */
    TEST_ASSERT_EQUAL_STRING("secret123", out);

    nssf_secret_ref_scrub_free(out, len);
}

/* ======================== file CRLF stripped (#8) ========================= */

static void test_file_crlf_stripped(void)
{
    /* CRLF-written file — 후행 \r\n 한 쌍 제거 (\n 하나 + 선행 \r 하나). */
    TEST_ASSERT_EQUAL_INT(0, write_tmp_secret("secret123\r\n", 11));
    char ref[320];
    make_file_ref(ref, sizeof(ref));

    char *out = NULL;
    size_t len = 0;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(ref, &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_EQUAL_UINT(9, (unsigned)len); /* \r\n dropped. */
    TEST_ASSERT_EQUAL_STRING("secret123", out);

    nssf_secret_ref_scrub_free(out, len);
}

/* ================= file internal newline preserved (#9) =================== */

static void test_file_internal_newline_preserved(void)
{
    /* 후행 개행이 없으므로 strip 미발동 — 내부 \n 은 secret 의 일부로 보존. */
    TEST_ASSERT_EQUAL_INT(0, write_tmp_secret("line1\nline2", 11));
    char ref[320];
    make_file_ref(ref, sizeof(ref));

    char *out = NULL;
    size_t len = 0;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(ref, &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_EQUAL_UINT(11, (unsigned)len);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", out); /* internal \n intact. */

    nssf_secret_ref_scrub_free(out, len);
}

/* ============ file double trailing newline — only one stripped (#10) ====== */

static void test_file_double_trailing_newline_one_stripped(void)
{
    /* "secret\n\n" → 정확히 하나만 제거 → "secret\n" (len 7). */
    TEST_ASSERT_EQUAL_INT(0, write_tmp_secret("secret\n\n", 8));
    char ref[320];
    make_file_ref(ref, sizeof(ref));

    char *out = NULL;
    size_t len = 0;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(ref, &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_EQUAL_UINT(7, (unsigned)len);
    TEST_ASSERT_EQUAL_STRING("secret\n", out); /* one \n remains. */
    TEST_ASSERT_EQUAL_UINT8('\n', out[6]);

    nssf_secret_ref_scrub_free(out, len);
}

/* ===================== file missing → NOT_FOUND (#11) ===================== */

static void test_file_missing_not_found(void)
{
    char *out = (char *)0x1;
    size_t len = 99;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(
        "file:/nonexistent/path/nssf_secret_ref_xyz", &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_NOT_FOUND, st);
    assert_fail_fast(st, out, len);
}

/* ===================== file too large → TOO_LARGE (#12) =================== */

static void test_file_too_large(void)
{
    /*
     * cap+1 바이트 (= MAX_BYTES 초과)을 기록 → resolver 는 cap+1 까지만 읽고
     * total > cap 을 감지해 TOO_LARGE, full slurp 없이 fail-fast.
     */
    size_t over = (size_t)NSSF_SECRET_REF_MAX_BYTES + 1;
    char *big = malloc(over);
    TEST_ASSERT_NOT_NULL(big);
    memset(big, 'A', over);
    TEST_ASSERT_EQUAL_INT(0, write_tmp_secret(big, over));
    free(big);

    char ref[320];
    make_file_ref(ref, sizeof(ref));
    char *out = (char *)0x1;
    size_t len = 99;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(ref, &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_TOO_LARGE, st);
    assert_fail_fast(st, out, len);
}

/* control: exactly cap bytes is allowed (boundary not off-by-one). */
static void test_file_at_cap_ok(void)
{
    size_t at = (size_t)NSSF_SECRET_REF_MAX_BYTES; /* exactly the cap. */
    char *big = malloc(at);
    TEST_ASSERT_NOT_NULL(big);
    memset(big, 'B', at);
    TEST_ASSERT_EQUAL_INT(0, write_tmp_secret(big, at));
    free(big);

    char ref[320];
    make_file_ref(ref, sizeof(ref));
    char *out = NULL;
    size_t len = 0;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(ref, &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_OK, st);
    TEST_ASSERT_EQUAL_UINT((unsigned)at, (unsigned)len); /* no newline to strip. */

    nssf_secret_ref_scrub_free(out, len);
}

/* ================== bad-format matrix + fail-fast (#13,#14) =============== */

static void test_bad_format_matrix(void)
{
    /* 각 ref 가 BAD_FORMAT + fail-fast(out NULL, len 0) 를 만족해야 한다. */
    static const char *bad_refs[] = {
        "",          /* empty reference. */
        "vault:x",   /* unknown scheme. */
        "noscheme",  /* no scheme at all. */
        "file:",     /* file scheme, empty operand. */
        "env:",      /* env scheme, empty operand (also in #4, kept for matrix). */
    };
    for (size_t i = 0; i < sizeof(bad_refs) / sizeof(bad_refs[0]); i++) {
        char *out = (char *)0x1;
        size_t len = 99;
        nssf_secret_ref_status_t st =
            nssf_secret_ref_resolve(bad_refs[i], &out, &len);
        TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_BAD_FORMAT, st);
        assert_fail_fast(st, out, len);
    }

    /* NULL ref → BAD_FORMAT, fail-fast. */
    char *out = (char *)0x1;
    size_t len = 99;
    nssf_secret_ref_status_t st = nssf_secret_ref_resolve(NULL, &out, &len);
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_BAD_FORMAT, st);
    assert_fail_fast(st, out, len);
}

/* NULL out-params → BAD_FORMAT, no crash (defensive contract). */
static void test_null_out_params_bad_format(void)
{
    size_t len = 0;
    char *out = NULL;
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_BAD_FORMAT,
                          nssf_secret_ref_resolve("env:X", NULL, &len));
    TEST_ASSERT_EQUAL_INT(NSSF_SECRET_REF_BAD_FORMAT,
                          nssf_secret_ref_resolve("env:X", &out, NULL));
}

/* ===================== scrub_free NULL-safe (#15) ========================= */

static void test_scrub_free_null_safe(void)
{
    nssf_secret_ref_scrub_free(NULL, 0);  /* no crash, no-op. */
    nssf_secret_ref_scrub_free(NULL, 42); /* len ignored when NULL. */
    TEST_PASS();
}

/* ============= status_str — generic class words, no secret (#16) ========== */

static void test_status_str_generic_class_words(void)
{
    /*
     * no-log behavior 의 unit-level proxy: status_str 는 failure class label 만
     * 반환하며 secret 값을 절대 담을 수 없다 (함수가 secret 에 접근 불가). 각
     * status 가 안정적인 generic 토큰을 반환하는지 확인 — operator diagnostic 에
     * 쓰일 문자열의 계약을 잠근다. (실제 no-value-logging enforcement 는 reviewer
     * lane 의 code review 책임; 이 테스트는 behavioral coverage 만 담당.)
     */
    TEST_ASSERT_EQUAL_STRING("ok",
                             nssf_secret_ref_status_str(NSSF_SECRET_REF_OK));
    TEST_ASSERT_EQUAL_STRING(
        "bad-format", nssf_secret_ref_status_str(NSSF_SECRET_REF_BAD_FORMAT));
    TEST_ASSERT_EQUAL_STRING(
        "not-found", nssf_secret_ref_status_str(NSSF_SECRET_REF_NOT_FOUND));
    TEST_ASSERT_EQUAL_STRING(
        "io-error", nssf_secret_ref_status_str(NSSF_SECRET_REF_IO_ERROR));
    TEST_ASSERT_EQUAL_STRING(
        "too-large", nssf_secret_ref_status_str(NSSF_SECRET_REF_TOO_LARGE));
    /* out-of-range value maps to a safe generic label, not UB. */
    TEST_ASSERT_EQUAL_STRING("unknown",
                             nssf_secret_ref_status_str((nssf_secret_ref_status_t)999));

    /* 각 label 은 non-NULL, non-empty, 그리고 어떤 known secret 토큰도 미포함. */
    const nssf_secret_ref_status_t all[] = {
        NSSF_SECRET_REF_OK, NSSF_SECRET_REF_BAD_FORMAT, NSSF_SECRET_REF_NOT_FOUND,
        NSSF_SECRET_REF_IO_ERROR, NSSF_SECRET_REF_TOO_LARGE,
    };
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        const char *s = nssf_secret_ref_status_str(all[i]);
        TEST_ASSERT_NOT_NULL(s);
        TEST_ASSERT_TRUE(strlen(s) > 0);
        TEST_ASSERT_NULL(strstr(s, "topsecret"));
        TEST_ASSERT_NULL(strstr(s, "secret123"));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_env_happy);
    RUN_TEST(test_env_verbatim_no_trim);
    RUN_TEST(test_env_unset_not_found);
    RUN_TEST(test_env_empty_operand_bad_format);
    RUN_TEST(test_env_empty_value_ok_len0);
    RUN_TEST(test_file_happy_no_newline);
    RUN_TEST(test_file_trailing_lf_stripped);
    RUN_TEST(test_file_crlf_stripped);
    RUN_TEST(test_file_internal_newline_preserved);
    RUN_TEST(test_file_double_trailing_newline_one_stripped);
    RUN_TEST(test_file_missing_not_found);
    RUN_TEST(test_file_too_large);
    RUN_TEST(test_file_at_cap_ok);
    RUN_TEST(test_bad_format_matrix);
    RUN_TEST(test_null_out_params_bad_format);
    RUN_TEST(test_scrub_free_null_safe);
    RUN_TEST(test_status_str_generic_class_words);
    return UNITY_END();
}
