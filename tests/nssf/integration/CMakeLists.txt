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

# Phase 1 wave 1 WI-nsselection-handler — NSSelectionGet inbound integration
# tests (happy + errors). Both drive nssf_nsselection_get_handle IN-PROCESS — no
# socket, no DB: JWKS seeded offline from an in-test RSA key (libjwt signs the
# bearer), availability from nssf_availability_repo_new_inmemory, body-sourced
# subscribed NSSAI + zero-init seams. So the golden path + 401/403/404/414/500
# run UNCONDITIONALLY in CI (real coverage). The PG-fixture / live nghttp2 probe
# variants are SKIP PASS when probe env (NSSF_DB_CONNINFO / NSSF_BIND_* /
# NSSF_TLS_* / NSSF_OAUTH2_JWKS_URL) is absent — the readiness-check CI has
# neither a PG service nor a live server. ctest -R nssf_nsselection_get_happy /
# nssf_nsselection_get_errors 로 매칭 (gate NAME pattern nssf_nsselection_.*_happy
# / nssf_nsselection_.*_errors).
add_executable(test_nsselection_get_happy
    test_nsselection_get_happy.c)

target_link_libraries(test_nsselection_get_happy
    PRIVATE
        unity
        nssf_handlers
        nssf_engine
        nssf_runtime
        nssf_generated
        PkgConfig::libjwt
        OpenSSL::SSL
        OpenSSL::Crypto
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_nsselection_get_happy PRIVATE c_std_11)

add_test(NAME nssf_nsselection_get_happy
         COMMAND test_nsselection_get_happy)

add_executable(test_nsselection_get_errors
    test_nsselection_get_errors.c)

target_link_libraries(test_nsselection_get_errors
    PRIVATE
        unity
        nssf_handlers
        nssf_engine
        nssf_runtime
        nssf_generated
        PkgConfig::libjwt
        OpenSSL::SSL
        OpenSSL::Crypto
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_nsselection_get_errors PRIVATE c_std_11)

add_test(NAME nssf_nsselection_get_errors
         COMMAND test_nsselection_get_errors)

# Phase 2 WI-availability-handlers — NSSAIAvailability Put/Patch/Delete/Options
# inbound integration tests. Each drives its handler IN-PROCESS — no socket, no
# DB: JWKS seeded offline from an in-test RSA key (libjwt signs the bearer with
# scope nnssf-nssaiavailability), the AvailabilityEngine over
# nssf_availability_repo_new_inmemory + zero-init seams (NULL publish — the
# phase2 deferred enqueue/cascade no-op). So the gate + status coverage (200 +
# 401/403/404/411/413/415/400 + 204 + Allow) runs UNCONDITIONALLY in CI (real
# coverage). The PG-fixture row probe + live `Allow:` HTTP probe variants are
# SKIP PASS when probe env (NSSF_DB_CONNINFO / NSSF_BIND_* / NSSF_TLS_* /
# NSSF_OAUTH2_JWKS_URL) is absent — the readiness-check CI has neither a PG
# service nor a live server (route registration deferred to a server-wiring
# slice). ctest -R nssf_nssaiavailability_<verb>_integration 로 매칭 (gate NAME
# pattern nssf_nssaiavailability_put / _patch / _delete / _options _integration).
#
# Must register BEFORE the find_package(PostgreSQL)/return() guard below so they
# build + run even where libpq is absent.
foreach(_verb put patch delete options)
    add_executable(test_nssaiavailability_${_verb}_integration
        test_nssaiavailability_${_verb}_integration.c)

    target_link_libraries(test_nssaiavailability_${_verb}_integration
        PRIVATE
            unity
            nssf_handlers
            nssf_engine
            nssf_runtime
            nssf_generated
            PkgConfig::libjwt
            OpenSSL::SSL
            OpenSSL::Crypto
            nssf_sanitizers
            nssf_warnings)

    target_compile_features(test_nssaiavailability_${_verb}_integration
        PRIVATE c_std_11)

    add_test(NAME nssf_nssaiavailability_${_verb}_integration
             COMMAND test_nssaiavailability_${_verb}_integration)
endforeach()

# Phase 2 WI-outbound-oauth2 — NRF token-acquire integration test (ADR-0004 M4).
#
# End-to-end client_credentials acquire/attach against a MOCK NRF token endpoint,
# wired through the test-only loopback ctor (nssf_oauth2_outbound_create_insecure,
# http://127.0.0.1 — mirrors test_jwks_fetch.c's insecure ctor) + the mock token-
# transport seam. No socket, no live NRF, no DB — runs UNCONDITIONALLY in CI. The
# live real-NRF acquire (NSSF_NRF_TOKEN_URL) is SKIP-PASS when that env is absent.
# curl_slist + curl/curl.h come transitively from nssf_runtime (PUBLIC libcurl).
#
# Must register BEFORE the find_package(PostgreSQL)/return() guard below so it
# builds + runs where libpq is absent. ctest -R nssf_oauth2_outbound 매칭
# (nssf_oauth2_outbound_token_acquire).
add_executable(test_nrf_token_acquire
    test_nrf_token_acquire.c)

target_link_libraries(test_nrf_token_acquire
    PRIVATE
        unity
        nssf_runtime
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_nrf_token_acquire PRIVATE c_std_11)

add_test(NAME nssf_oauth2_outbound_token_acquire
         COMMAND test_nrf_token_acquire)

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
