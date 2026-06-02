# Phase 1 wave 1 WI-nsselection-handler — NSSelectionGet 200 response-schema
# contract test.
#
# Drives nssf_nsselection_get_handle IN-PROCESS to produce the real serialized
# 200 body, then asserts conformance to the AuthorizedNetworkSliceInfo response
# schema the contract names for NSSelectionGet (handoff/nssf/contract.yaml →
# data-model/AuthorizedNetworkSliceInfo.json). Runs UNCONDITIONALLY (no env, no
# DB, no socket): JWKS seeded offline from an in-test RSA key (libjwt signs the
# bearer), availability from nssf_availability_repo_new_inmemory. ctest -R
# nssf_nsselection_response_schema 로 매칭 (gate NAME).
#
# imported target (OpenSSL / libjwt) 은 이 directory scope 에서 별도로 노출해야
# 한다 — runtime 의 find_package/pkg_check 는 그 scope 한정. 동일 패키지라 1회
# 재선언 (버전 제약은 runtime/integration 과 일치).

find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libjwt REQUIRED IMPORTED_TARGET libjwt>=3.3.3)

add_executable(test_nsselection_response_schema
    test_nsselection_response_schema.c)

target_link_libraries(test_nsselection_response_schema
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

target_compile_features(test_nsselection_response_schema PRIVATE c_std_11)

add_test(NAME nssf_nsselection_response_schema
         COMMAND test_nsselection_response_schema)
