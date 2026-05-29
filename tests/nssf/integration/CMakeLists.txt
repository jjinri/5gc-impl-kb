# Phase 1 wave 1 WI-schema-bootstrap integration test.
#
# libpq + psql 의존. PostgreSQL service container 미배치 환경 (현 CI) 에서는
# env 미주입 → 본 test 가 SKIP PASS 로 회피. CI 에 PostgreSQL service 추가
# 후 활성으로 전환된다 (P3 backlog).
#
# libpq 부재 시 — build skip. 별도 fail 신호 없이 wave 1 build 통과.

find_package(PostgreSQL QUIET)

if(NOT PostgreSQL_FOUND)
    message(STATUS "[nssf] libpq not found — skipping integration tests "
                   "(install libpq-dev to enable)")
    return()
endif()

add_executable(test_schema_bootstrap
    test_schema_bootstrap.c)

target_link_libraries(test_schema_bootstrap
    PRIVATE
        unity
        PostgreSQL::PostgreSQL
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_schema_bootstrap PRIVATE c_std_11)

add_test(NAME nssf_schema_bootstrap
         COMMAND test_schema_bootstrap)

# schema.sql 경로를 test 가 NSSF_SCHEMA_SQL_PATH env 로 받는다 — ctest 가 env
# 주입할 수 있도록 안내 메시지 (POSTGRES_* env 는 별도 ctest 호출 측 책임).
set_tests_properties(nssf_schema_bootstrap PROPERTIES
    ENVIRONMENT
        "NSSF_SCHEMA_SQL_PATH=${CMAKE_SOURCE_DIR}/infra/nssf/schema.sql")
