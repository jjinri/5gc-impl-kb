add_library(unity STATIC vendor/Unity/unity.c)
target_include_directories(unity PUBLIC vendor/Unity)
target_compile_options(unity PRIVATE -w)

add_executable(test_nsselection_handler
    unit/test_nsselection_handler.c)

target_link_libraries(test_nsselection_handler
    PRIVATE
        unity
        nssf_transport
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_nsselection_handler PRIVATE c_std_11)

add_test(NAME nsselection_handler_smoke
         COMMAND test_nsselection_handler)

# Phase 1 wave 1 WI-codegen-bootstrap smoke test.
# nssf_generated 가 link 가능 + snssai/plmn_id 의 alloc/serialize/parse/free
# round-trip 증명. -w 적용 (generated header 의 doxygen-escape 등 warning
# 정책 책임 외).
add_executable(test_generated_smoke
    unit/test_generated_smoke.c)

target_link_libraries(test_generated_smoke
    PRIVATE
        unity
        nssf_generated
        nssf_sanitizers)

target_compile_features(test_generated_smoke PRIVATE c_std_11)
target_compile_options(test_generated_smoke PRIVATE -Wall -Wextra)

add_test(NAME generated_smoke
         COMMAND test_generated_smoke)

# Phase 1 wave 1 WI-problem-details-wrapper — RFC 7807 factory + mandatory
# header metadata. dev/nssf/error-cause-catalog.yaml 의 18 production cause
# 와 1:1 매핑.
add_executable(test_problem_details_factory
    unit/test_problem_details_factory.c)

target_link_libraries(test_problem_details_factory
    PRIVATE
        unity
        nssf_wrappers
        nssf_sanitizers)

target_compile_features(test_problem_details_factory PRIVATE c_std_11)
target_compile_options(test_problem_details_factory PRIVATE -Wall -Wextra)

add_test(NAME nssf_problem_details_factory
         COMMAND test_problem_details_factory)

add_executable(test_problem_details_headers
    unit/test_problem_details_headers.c)

target_link_libraries(test_problem_details_headers
    PRIVATE
        unity
        nssf_wrappers
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_problem_details_headers PRIVATE c_std_11)

add_test(NAME nssf_problem_details_headers
         COMMAND test_problem_details_headers)

# Phase 1 wave 1 WI-nftype-wrapper — NFType anyOf passthrough policy (G-09).
# design/nssf/contract/data-model/NFType.json 의 67 known enum + unknown
# passthrough wrapper.
add_executable(test_nftype_wrapper
    unit/test_nftype_wrapper.c)

target_link_libraries(test_nftype_wrapper
    PRIVATE
        unity
        nssf_wrappers
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_nftype_wrapper PRIVATE c_std_11)

add_test(NAME nssf_nftype_wrapper
         COMMAND test_nftype_wrapper)

# Phase 1 wave 1 WI-schema-bootstrap — PostgreSQL bootstrap integration test.
# libpq fixture, ctest -R nssf_schema_bootstrap 매칭. libpq 부재 또는
# POSTGRES_* env 미주입 시 SKIP PASS 로 회피.
add_subdirectory(integration)
