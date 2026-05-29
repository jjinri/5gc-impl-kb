/*
 * test_schema_bootstrap.c — WI-schema-bootstrap acceptance.
 *
 * libpq fixture. POSTGRES_HOST / POSTGRES_DB / POSTGRES_USER env 가 주입된
 * 환경에서만 schema.sql 적용 + 4 table + 3 index + nssf_schema_version=1
 * 검증. env 미주입 시 TEST_PASS + 콘솔 SKIP 메시지 (CI 호환).
 *
 * P3 backlog — CI 에 PostgreSQL service container 추가 후 본 test 의
 * SKIP path 가 활성으로 전환된다.
 *
 * 본 test 는 idempotency 도 검증 — schema.sql 을 2회 적용 후 row 중복
 * 없음을 nssf_schema_version 으로 확인.
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static const char *env_host = NULL;
static const char *env_port = NULL;
static const char *env_db   = NULL;
static const char *env_user = NULL;
static const char *env_pwd  = NULL;
static const char *schema_path = NULL;

static int env_ready(void)
{
    env_host = getenv("POSTGRES_HOST");
    env_port = getenv("POSTGRES_PORT");
    env_db   = getenv("POSTGRES_DB");
    env_user = getenv("POSTGRES_USER");
    env_pwd  = getenv("POSTGRES_PASSWORD");
    schema_path = getenv("NSSF_SCHEMA_SQL_PATH");
    return env_host != NULL && env_db != NULL && env_user != NULL &&
           schema_path != NULL;
}

static PGconn *open_conn(void)
{
    char conninfo[1024];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%s dbname=%s user=%s password=%s sslmode=prefer",
             env_host,
             env_port != NULL ? env_port : "5432",
             env_db,
             env_user,
             env_pwd != NULL ? env_pwd : "");
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "[test_schema_bootstrap] connect failed: %s\n",
                PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }
    return conn;
}

static int apply_schema(void)
{
    /*
     * psql -f schema.sql via system(). libpq 가 multi-statement file 의 \i 류
     * meta-command 를 직접 처리 못해 psql CLI 위임.
     */
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "PGPASSWORD='%s' psql -h '%s' -p '%s' -U '%s' -d '%s' "
             "-v ON_ERROR_STOP=1 -f '%s' >/dev/null",
             env_pwd != NULL ? env_pwd : "",
             env_host,
             env_port != NULL ? env_port : "5432",
             env_user,
             env_db,
             schema_path);
    return system(cmd);
}

static int int_from_query(PGconn *conn, const char *sql)
{
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) < 1) {
        PQclear(res);
        return -1;
    }
    int value = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return value;
}

static void test_env_or_skip(void)
{
    if (!env_ready()) {
        fprintf(stderr,
                "[SKIP] test_schema_bootstrap — POSTGRES_* / "
                "NSSF_SCHEMA_SQL_PATH env 미주입. CI 에 PostgreSQL service "
                "container 추가 후 활성 (P3 backlog).\n");
        TEST_PASS_MESSAGE("env not set — skipping integration test");
    }
}

static void test_apply_and_verify(void)
{
    if (!env_ready()) {
        TEST_PASS_MESSAGE("env not set — skip");
        return;
    }

    int rc = apply_schema();
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "schema.sql 1차 적용 실패");

    PGconn *conn = open_conn();
    TEST_ASSERT_NOT_NULL_MESSAGE(conn, "PG 연결 실패");

    /* version row = 1. */
    int version = int_from_query(
        conn,
        "SELECT version FROM nssf_schema_version ORDER BY version DESC LIMIT 1");
    TEST_ASSERT_EQUAL_INT(1, version);

    /* 3 application table 존재. */
    int availability_present = int_from_query(
        conn,
        "SELECT CASE WHEN to_regclass('public.availability') IS NULL THEN 0 ELSE 1 END");
    int subscription_present = int_from_query(
        conn,
        "SELECT CASE WHEN to_regclass('public.subscription') IS NULL THEN 0 ELSE 1 END");
    int retry_present = int_from_query(
        conn,
        "SELECT CASE WHEN to_regclass('public.retry_queue') IS NULL THEN 0 ELSE 1 END");
    TEST_ASSERT_EQUAL_INT(1, availability_present);
    TEST_ASSERT_EQUAL_INT(1, subscription_present);
    TEST_ASSERT_EQUAL_INT(1, retry_present);

    /* 3 핵심 index 존재. */
    int idx_count = int_from_query(
        conn,
        "SELECT COUNT(*) FROM pg_indexes "
        " WHERE tablename IN ('availability','subscription','retry_queue') "
        "   AND indexname IN ("
        "       'availability_plmn_tai_idx',"
        "       'subscription_expiry_idx',"
        "       'retry_queue_next_attempt_idx')");
    TEST_ASSERT_EQUAL_INT(3, idx_count);

    PQfinish(conn);
}

static void test_boolean_columns_not_null(void)
{
    if (!env_ready()) {
        TEST_PASS_MESSAGE("env not set — skip");
        return;
    }

    PGconn *conn = open_conn();
    TEST_ASSERT_NOT_NULL(conn);

    /*
     * boolean column nullability — manifest §2 와 schema.sql 일치 검증.
     * is_nullable = 'NO' 이면 NOT NULL.
     */
    int reallocation_not_null = int_from_query(
        conn,
        "SELECT CASE WHEN is_nullable = 'NO' THEN 1 ELSE 0 END "
        "  FROM information_schema.columns "
        " WHERE table_name = 'availability' "
        "   AND column_name = 'reallocation_indication'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, reallocation_not_null,
        "availability.reallocation_indication 가 NOT NULL 아님");

    int tombstone_not_null = int_from_query(
        conn,
        "SELECT CASE WHEN is_nullable = 'NO' THEN 1 ELSE 0 END "
        "  FROM information_schema.columns "
        " WHERE table_name = 'subscription' "
        "   AND column_name = 'tombstone'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, tombstone_not_null,
        "subscription.tombstone 가 NOT NULL 아님");

    PQfinish(conn);
}

static void test_idempotent_reapply(void)
{
    if (!env_ready()) {
        TEST_PASS_MESSAGE("env not set — skip");
        return;
    }

    int rc = apply_schema();
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "schema.sql 2차 재적용 실패 (idempotency)");

    PGconn *conn = open_conn();
    TEST_ASSERT_NOT_NULL(conn);

    /* version row 가 여전히 1 개. */
    int version_row_count = int_from_query(
        conn, "SELECT COUNT(*) FROM nssf_schema_version WHERE version = 1");
    TEST_ASSERT_EQUAL_INT(1, version_row_count);

    PQfinish(conn);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_env_or_skip);
    RUN_TEST(test_apply_and_verify);
    RUN_TEST(test_boolean_columns_not_null);
    RUN_TEST(test_idempotent_reapply);
    return UNITY_END();
}
