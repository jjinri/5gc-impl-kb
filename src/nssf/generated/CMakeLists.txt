# nssf_generated — openapi-generator(C) 산출 + cJSON 의 정적 라이브러리.
#
# 본 디렉터리의 model/ + include/ + external/ 는 openapi-generator-cli 7.10.0
# 산출 (input = specs/29.531/TS29531_Nnssf_NSSelection.yaml + 의존 TS29571_
# CommonData.yaml). 재생성 절차는 infra/nssf/codegen/regenerate.sh.
#
# 본 target 은 *generated only* — handwritten transport / handler 는 별도
# 디렉터리에서 본 target 을 link 한다. api/ (HTTP client) 는 미포함 — NSSF
# runtime 은 nghttp2 server + handwritten handler 라 generator 의 apiClient
# 를 link 안 함.

set(NSSF_GENERATED_MODEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model")
set(NSSF_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(NSSF_GENERATED_EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")

# generated model files (alphabetical, regenerate.sh 가 deterministic emit).
file(GLOB NSSF_GENERATED_MODEL_SOURCES CONFIGURE_DEPENDS
    "${NSSF_GENERATED_MODEL_DIR}/*.c")

add_library(nssf_generated STATIC
    ${NSSF_GENERATED_MODEL_SOURCES}
    ${NSSF_GENERATED_EXTERNAL_DIR}/cJSON.c)

target_include_directories(nssf_generated PUBLIC
    ${NSSF_GENERATED_MODEL_DIR}
    ${NSSF_GENERATED_INCLUDE_DIR}
    ${NSSF_GENERATED_EXTERNAL_DIR})

target_compile_features(nssf_generated PRIVATE c_std_11)

# _POSIX_C_SOURCE — generated code 의 strdup() 호출 (snssai/plmn_id/tai 등
# 의 parseFromJSON) 이 c11 strict 에서 POSIX 함수 미노출 → implicit
# declaration → int 반환 가정 → pointer sign-extension UB. PUBLIC 으로
# downstream test 에도 전파해 동일 ABI 사용. Pane 2 권고 "generated code
# 직접 수정 금지" 준수 — CMake wrapper layer 에서 정의.
target_compile_definitions(nssf_generated PUBLIC _POSIX_C_SOURCE=200809L)

# generated code 는 -Werror 적용 안 함 (generator template 의 warning 정책
# 책임 외). agent-execution-plan / engineering-design 의 nssf_warnings 정책
# 참조 — generated 는 별도 nssf_generated_warnings (또는 -w) 권고.
target_compile_options(nssf_generated PRIVATE -w)

# sanitizer 는 적용 (debug build 의 leak/UB 검출 의무).
target_link_libraries(nssf_generated PRIVATE nssf_sanitizers)
