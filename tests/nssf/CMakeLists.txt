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
