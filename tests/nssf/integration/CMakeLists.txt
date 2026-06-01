# Phase 1 wave 1 WI-tls-bootstrap — inbound OAuth2 JWKS fetch integration test.
#
# In-process mock JWKS HTTP server (127.0.0.1, ephemeral port, plain http) +
# in-test RSA keypair. 외부 서비스·네트워크·fixture 파일 의존 없음 — CI 에서
# 그대로 실행된다. ctest -R nssf_oauth2_ (nssf_oauth2_jwks_fetch) 로 매칭.
#
# PostgreSQL 가 없어 아래 early return 하더라도 본 test 는 먼저 등록되어야
# 하므로 find_package(PostgreSQL) 위에 둔다.
#
# imported target (OpenSSL / Threads / libjwt / libcurl) 은 이 directory scope
# 에서 별도로 노출해야 한다 — runtime 의 find_package/pkg_check 는 그 scope
# 한정. 동일 패키지라 1회 재선언 (버전 제약은 runtime 과 일치).
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libjwt REQUIRED IMPORTED_TARGET libjwt>=3.3.3)
pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl>=7.81)

add_executable(test_jwks_fetch
    test_jwks_fetch.c)

target_link_libraries(test_jwks_fetch
    PRIVATE
        unity
        nssf_runtime
        PkgConfig::libjwt
        PkgConfig::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_jwks_fetch PRIVATE c_std_11)

add_test(NAME nssf_oauth2_jwks_fetch
         COMMAND test_jwks_fetch)

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

# Phase 1 wave 1 WI-availability-repo — AvailabilityRepository libpq backend
# round-trip integration test. schema_bootstrap 과 동일 gate (PostgreSQL_FOUND
# 일 때만 build/register) + 동일 SKIP PASS 규약 (POSTGRES_* env 미주입 시).
# ctest -R nssf_availability_repo_ 매칭 (nssf_availability_repo_pg).
add_executable(test_availability_repo_pg
    test_availability_repo_pg.c)

target_link_libraries(test_availability_repo_pg
    PRIVATE
        unity
        nssf_repository
        PostgreSQL::PostgreSQL
        nssf_generated
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_availability_repo_pg PRIVATE c_std_11)

add_test(NAME nssf_availability_repo_pg
         COMMAND test_availability_repo_pg)

set_tests_properties(nssf_availability_repo_pg PROPERTIES
    ENVIRONMENT
        "NSSF_SCHEMA_SQL_PATH=${CMAKE_SOURCE_DIR}/infra/nssf/schema.sql")
