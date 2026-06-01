/*
 * test_availability_repo_pg.c — WI-availability-repo libpq backend acceptance.
 *
 * AvailabilityRepository libpq/PostgreSQL backend round-trip against a real
 * Postgres. POSTGRES_HOST/DB/USER (+ optional PORT/PASSWORD) env 주입 시에만
 * 실행 — 미주입 시 SKIP PASS (CI 에 PostgreSQL service container 부재).
 * NSSF_SCHEMA_SQL_PATH 로 schema.sql 적용 (test_schema_bootstrap 미러).
 * ctest -R nssf_availability_repo_ 매칭 (nssf_availability_repo_pg).
 *
 * env 주입 시 검증 — upsert → find → patch → delete round-trip 을 real
 * Postgres 에 실행, 각 단계 assert. 삽입 row 는 delete 로 정리 (idempotent).
 */

#define _POSIX_C_SOURCE 200809L

#include "unity.h"

#include "availability_repository.h"
#include "cJSON.h"

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

/* 본 test 가 삽입하는 고정 id — cleanup 으로 idempotent 보장. */
static const char *TEST_ID = "deadbeef-1111-2222-3333-444444444444";

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

/* schema.sql 을 psql 로 적용 (test_schema_bootstrap 미러). 0 ok. */
static int apply_schema(void)
{
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

/* env 로 libpq conninfo 조립. */
static void build_conninfo(char *buf, size_t len)
{
    snprintf(buf, len,
             "host=%s port=%s dbname=%s user=%s password=%s sslmode=prefer",
             env_host,
             env_port != NULL ? env_port : "5432",
             env_db,
             env_user,
             env_pwd != NULL ? env_pwd : "");
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

static cJSON *make_snssais(int sst, const char *sd)
{
    cJSON *arr = cJSON_CreateArray();
    cJSON *s = cJSON_CreateObject();
    cJSON_AddNumberToObject(s, "sst", sst);
    if (sd != NULL) {
        cJSON_AddStringToObject(s, "sd", sd);
    }
    cJSON_AddItemToArray(arr, s);
    return arr;
}

static void test_env_or_skip(void)
{
    if (!env_ready()) {
        fprintf(stderr,
                "[SKIP] test_availability_repo_pg — POSTGRES_* / "
                "NSSF_SCHEMA_SQL_PATH not set. CI 에 PostgreSQL service "
                "container 추가 후 활성 (P3 backlog).\n");
        TEST_PASS_MESSAGE("env not set — skipping pg integration test");
    }
}

/*
 * Real Postgres round-trip — schema bootstrap 후 upsert → find → patch →
 * delete 를 순서대로 assert. 삽입 row 는 마지막 delete 로 정리 (재실행 idempotent).
 */
static void test_pg_round_trip(void)
{
    if (!env_ready()) {
        TEST_PASS_MESSAGE("env not set — skip");
        return;
    }

    int rc = apply_schema();
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "schema.sql 적용 실패");

    char conninfo[1024];
    build_conninfo(conninfo, sizeof(conninfo));

    char errbuf[256] = {0};
    nssf_availability_repo_t *repo =
        nssf_availability_repo_new_pg(conninfo, errbuf, sizeof(errbuf));
    TEST_ASSERT_NOT_NULL_MESSAGE(repo, errbuf);

    /* 이전 실행 잔여 row 정리 (idempotent 시작점). 0 or 1 모두 정상. */
    int pre = nssf_availability_repo_delete(repo, TEST_ID);
    TEST_ASSERT_TRUE_MESSAGE(pre == 0 || pre == 1,
                             "pre-clean delete 가 비정상 return");

    /* ── 1. upsert insert ─────────────────────────────────────────────── */
    nssf_availability_record_t rec;
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s", TEST_ID);
    rec.plmn_id = make_plmn("001", "01");
    rec.tai = make_tai("001", "01", "000001");
    rec.snssais = make_snssais(1, "000001");
    rec.amf_set_id = strdup("amfset-001");
    rec.reallocation_indication = false;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_availability_repo_upsert(repo, &rec),
                                  "pg upsert insert 실패");
    nssf_availability_record_clear(&rec);

    /* ── 2. find → 1 row + field 일치 ────────────────────────────────── */
    cJSON *q_plmn = make_plmn("001", "01");
    cJSON *q_tai = make_tai("001", "01", "000001");
    nssf_availability_record_list_t list;
    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list),
        "pg find 실패");
    TEST_ASSERT_EQUAL_size_t_MESSAGE(1, list.count, "pg insert 후 find count != 1");
    TEST_ASSERT_EQUAL_STRING(TEST_ID, list.records[0].id);
    TEST_ASSERT_NOT_NULL(list.records[0].amf_set_id);
    TEST_ASSERT_EQUAL_STRING("amfset-001", list.records[0].amf_set_id);
    TEST_ASSERT_FALSE(list.records[0].reallocation_indication);
    nssf_availability_record_list_free(&list);

    /* ── 3. 동일 id 재 upsert → UPDATE (중복 없음) ───────────────────── */
    memset(&rec, 0, sizeof(rec));
    snprintf(rec.id, sizeof(rec.id), "%s", TEST_ID);
    rec.plmn_id = make_plmn("001", "01");
    rec.tai = make_tai("001", "01", "000001");
    rec.snssais = make_snssais(2, "0000ab");
    rec.amf_set_id = strdup("amfset-002");
    rec.reallocation_indication = true;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_availability_repo_upsert(repo, &rec),
                                  "pg re-upsert (update) 실패");
    nssf_availability_record_clear(&rec);

    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(1, list.count, "pg re-upsert 가 row 중복");
    TEST_ASSERT_EQUAL_STRING("amfset-002", list.records[0].amf_set_id);
    TEST_ASSERT_TRUE(list.records[0].reallocation_indication);
    nssf_availability_record_list_free(&list);

    /* ── 4. patch — amf_set_id 만 변경, reallocation 보존 ────────────── */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0,
        nssf_availability_repo_patch(repo, TEST_ID, NULL, NULL, NULL,
                                     "amfset-PATCHED", false, false),
        "pg patch 가 0 (updated) 반환 실패");

    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list));
    TEST_ASSERT_EQUAL_size_t(1, list.count);
    TEST_ASSERT_EQUAL_STRING("amfset-PATCHED", list.records[0].amf_set_id);
    TEST_ASSERT_TRUE_MESSAGE(list.records[0].reallocation_indication,
                             "pg patch (set_reallocation=false) 가 flag 변경");
    nssf_availability_record_list_free(&list);

    /* 미존재 id patch → 1 (no-match). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1,
        nssf_availability_repo_patch(repo,
                                     "00000000-0000-0000-0000-0000000000ff",
                                     NULL, NULL, NULL, "x", false, false),
        "pg 미존재 id patch 가 1 반환 실패");

    /* ── 5. delete → 0 (deleted), find empty ─────────────────────────── */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, nssf_availability_repo_delete(repo, TEST_ID),
                                  "pg delete 가 0 반환 실패");
    memset(&list, 0, sizeof(list));
    TEST_ASSERT_EQUAL_INT(
        0, nssf_availability_repo_find_for_plmn_tai(repo, q_plmn, q_tai, &list));
    TEST_ASSERT_EQUAL_size_t_MESSAGE(0, list.count, "pg delete 후 find != 0");
    nssf_availability_record_list_free(&list);

    /* 미존재 id delete → 1 (no-match). */
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        1, nssf_availability_repo_delete(repo, TEST_ID),
        "pg 이미 삭제된 id delete 가 1 반환 실패");

    cJSON_Delete(q_plmn);
    cJSON_Delete(q_tai);
    nssf_availability_repo_free(repo);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_env_or_skip);
    RUN_TEST(test_pg_round_trip);
    return UNITY_END();
}
