# nssf_wrappers — generated 산출 보정 layer.
#
# Pane 2 권고 "generated code 직접 수정 금지" 준수 — 본 lib 가 generated/
# problem_details / nf_type 등 schema 위에 RFC 7807 factory · anyOf passthrough
# wrapper 를 제공한다.
add_library(nssf_wrappers STATIC
    nf_problem_details_wrapper.c
    nf_type_wrapper.c)

target_include_directories(nssf_wrappers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_wrappers PRIVATE c_std_11)
target_link_libraries(nssf_wrappers
    PUBLIC nssf_generated
    PRIVATE nssf_sanitizers nssf_warnings)
