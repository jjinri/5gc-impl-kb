#!/usr/bin/env bash
# Regenerate src/nssf/generated/ from specs/29.531/ + specs/29.571/ + specs/29.510/.
#
# Usage:
#   bash infra/nssf/codegen/regenerate.sh              # regenerate in-place (--write)
#   bash infra/nssf/codegen/regenerate.sh --check      # regenerate to tmp + diff
#
# 의존:
#   - JRE (java 17 검증됨)
#   - openapi-generator-cli 7.10.0 jar (자동 download to /tmp 또는 OPENAPI_GEN_JAR env)
#   - NSSF 2 spec yaml (NSSelection + NSSAIAvailability) + 의존 (TS29571 / TS29510)
#
# 정책:
#   - 본 script 는 *deterministic regen* — generator version + spec + flag
#     pin 변경 시만 산출 변경. 그 외 drift 는 fail.
#   - 산출은 src/nssf/generated/{model,include,external}/ 의 *subset*.
#     generator emits 추가 디렉터리 (api, src, docs, unit-test, Packing.cmake,
#     README, libcurl.licence, uncrustify-rules.cfg) 는 *commit 대상 외* —
#     NSSF runtime 이 generator's HTTP client (api/, src/) 를 link 안 함.
#   - include/ 는 model/ 가 reference 하는 utility header (list / binary /
#     keyValuePair) 만 commit. apiClient.h 는 미포함.
#
# Multi-spec emit (PR-codegen-nssaiavailability-extension):
#   - openapi-generator-cli 는 단일 -i input 만 받으므로 NSSelection 과
#     NSSAIAvailability 를 *각각 generate 후 union merge* 한다.
#   - WHY NSSelection-is-canonical-for-shared: 두 spec 모두 TS29571 의
#     Snssai / PlmnId / Tai / TaiRange / NsagInfo 등 공통 schema 를 transitive
#     $ref 한다. generator 의 C target 은 root input 으로부터 도달하는 $ref
#     graph 의 *깊이* 에 따라 같은 공통 schema 를 다르게 emit 한다 — NSSelection
#     run 은 nested model 을 정상 struct (plmn_id_t* / tai_t*) 로 resolve 하지만
#     NSSAIAvailability run 은 일부 deep transitive 를 generic object_t 로
#     collapse 한다. 그러나 NSSAIAvailability run 의 *고유* 파일들은 같은
#     공통 schema 를 struct API (tai_parseFromJSON / plmn_id_free / struct
#     plmn_id_t*) 로 호출한다 — 즉 collapse 된 헤더는 같은 run 안에서조차
#     실제로 소비되지 않는 generator 내부 부산물이다. 따라서 공통 파일은
#     NSSelection (canonical) 버전을 채택해야 union 이 type-coherent 하다
#     (50 model .c 전체 compile-clean 확인됨). 이는 의미 재해석이 아니라
#     generator 산출 deterministic merge rule 이다.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
SPECS_DIR="${REPO_ROOT}/specs"
TARGET_DIR="${REPO_ROOT}/src/nssf/generated"
GENERATOR_VERSION="7.10.0"
GENERATOR_JAR_URL="https://repo1.maven.org/maven2/org/openapitools/openapi-generator-cli/${GENERATOR_VERSION}/openapi-generator-cli-${GENERATOR_VERSION}.jar"
GENERATOR_JAR_PATH="${OPENAPI_GEN_JAR:-/tmp/openapi-generator-cli-${GENERATOR_VERSION}.jar}"

MODE="${1:---write}"

# include/ 안 *handwritten supplement* — generator 가 emit 하지 않는 파일.
# WHY: list.c (list.h runtime, generator 가 supportingFiles 미활성으로 .c
# 누락) + codegen_shim.c (name-mangling 보충) 는 PR #98 의 commit. drift
# check 는 generator 가 *실제 emit 하는* utility header 만 비교해야 한다 —
# 그렇지 않으면 이 supplement 파일들이 영구 false drift 로 잡힌다.
GEN_INCLUDE_FILES=(list.h binary.h keyValuePair.h)
# external/ — cJSON 은 두 run 에서 동일 copy. licence 포함.
GEN_EXTERNAL_FILES=(cJSON.h cJSON.c cJSON.licence)

# Ensure generator jar present.
if [ ! -f "${GENERATOR_JAR_PATH}" ]; then
    echo "[regenerate] downloading openapi-generator-cli ${GENERATOR_VERSION}..." >&2
    curl -sL -o "${GENERATOR_JAR_PATH}" "${GENERATOR_JAR_URL}"
fi

# Bundle specs into flat tmp dir (generator 의 RELATIVE ref 해석 위해).
BUNDLE_DIR="$(mktemp -d -t nssf-spec-bundle-XXXXXX)"
trap 'rm -rf "${BUNDLE_DIR}"' EXIT

cp "${SPECS_DIR}/29.531/"*.yaml "${BUNDLE_DIR}/"
cp "${SPECS_DIR}/29.571/"*.yaml "${BUNDLE_DIR}/"
cp "${SPECS_DIR}/29.510/"*.yaml "${BUNDLE_DIR}/"

OUT_DIR="$(mktemp -d -t nssf-codegen-out-XXXXXX)"

gen_one() {
    # $1 = input yaml basename, $2 = output subdir name.
    (
        cd "${BUNDLE_DIR}"
        java -jar "${GENERATOR_JAR_PATH}" generate \
            -i "$1" \
            -g c \
            -o "${OUT_DIR}/$2" \
            --additional-properties=packageName=nssf \
            --skip-validate-spec \
            > /dev/null 2>&1
    )
}

echo "[regenerate] running openapi-generator on NSSelection..." >&2
gen_one TS29531_Nnssf_NSSelection.yaml nsselection

echo "[regenerate] running openapi-generator on NSSAIAvailability..." >&2
gen_one TS29531_Nnssf_NSSAIAvailability.yaml nssaiavail

NS_DIR="${OUT_DIR}/nsselection"
AV_DIR="${OUT_DIR}/nssaiavail"

# ─── Stage union into tmp target ──────────────────────────────────────
STAGE_DIR="$(mktemp -d -t nssf-codegen-stage-XXXXXX)"
mkdir -p "${STAGE_DIR}/model" "${STAGE_DIR}/include" "${STAGE_DIR}/external"

# model/ union — NSSelection (canonical) 전체 먼저, 그다음 NSSAIAvailability
# *고유* 파일만 추가. 공통 파일은 canonical 채택 (위 정책 WHY 참조).
cp "${NS_DIR}/model/"*.h "${NS_DIR}/model/"*.c "${STAGE_DIR}/model/"
for hf in "${AV_DIR}/model/"*.h; do
    base="$(basename "${hf}" .h)"
    if [ ! -f "${NS_DIR}/model/${base}.h" ]; then
        cp "${AV_DIR}/model/${base}.h" "${AV_DIR}/model/${base}.c" "${STAGE_DIR}/model/"
    fi
done

# include/ + external/ — 두 run 이 동일 emit 이어야 한다. 다르면 generator
# version/spec drift 의 *진짜 신호* — FAIL LOUD.
for f in "${GEN_INCLUDE_FILES[@]}"; do
    if ! cmp -s "${NS_DIR}/include/${f}" "${AV_DIR}/include/${f}"; then
        echo "[regenerate] FAIL include/${f} differs between NSSelection and NSSAIAvailability runs — real drift signal, not auto-resolving." >&2
        rm -rf "${OUT_DIR}" "${STAGE_DIR}"
        exit 1
    fi
    cp "${NS_DIR}/include/${f}" "${STAGE_DIR}/include/"
done
for f in "${GEN_EXTERNAL_FILES[@]}"; do
    if ! cmp -s "${NS_DIR}/external/${f}" "${AV_DIR}/external/${f}"; then
        echo "[regenerate] FAIL external/${f} differs between NSSelection and NSSAIAvailability runs — real drift signal, not auto-resolving." >&2
        rm -rf "${OUT_DIR}" "${STAGE_DIR}"
        exit 1
    fi
    cp "${NS_DIR}/external/${f}" "${STAGE_DIR}/external/"
done

if [ "${MODE}" = "--check" ]; then
    # model/ 는 순수 generator 산출이므로 full diff -r (committed ↔ staged).
    DIFF_OUTPUT=$(diff -r "${TARGET_DIR}/model" "${STAGE_DIR}/model" || true)

    # include/ + external/ 는 committed 가 *superset* (handwritten supplement
    # list.c / codegen_shim.c 포함). generator 가 실제 emit 하는 파일만 비교 —
    # superset 의 extra 파일은 drift 아님.
    for f in "${GEN_INCLUDE_FILES[@]}"; do
        DIFF_OUTPUT+=$'\n'$(diff "${TARGET_DIR}/include/${f}" "${STAGE_DIR}/include/${f}" 2>&1 || true)
    done
    for f in "${GEN_EXTERNAL_FILES[@]}"; do
        DIFF_OUTPUT+=$'\n'$(diff "${TARGET_DIR}/external/${f}" "${STAGE_DIR}/external/${f}" 2>&1 || true)
    done

    # Filter empty.
    DIFF_FILTERED=$(printf "%s\n" "${DIFF_OUTPUT}" | grep -v '^$' || true)

    if [ -n "${DIFF_FILTERED}" ]; then
        echo "[regenerate] FAIL drift detected:" >&2
        printf "%s\n" "${DIFF_FILTERED}" >&2
        rm -rf "${OUT_DIR}" "${STAGE_DIR}"
        exit 1
    fi
    echo "[regenerate] PASS — no drift (src/nssf/generated ↔ regenerated, multi-spec union)" >&2
    rm -rf "${OUT_DIR}" "${STAGE_DIR}"
    exit 0
fi

# --write mode: replace committed *generator subset* in place.
# WHY not rm-rf include/: include/ 는 handwritten supplement (list.c /
# codegen_shim.c) 를 보유한다 — 통째 삭제하면 그 파일들이 사라진다. generator
# emit 파일만 덮어쓴다. model/ 는 순수 generator 산출이라 통째 교체.
rm -rf "${TARGET_DIR}/model"
mv "${STAGE_DIR}/model" "${TARGET_DIR}/model"
for f in "${GEN_INCLUDE_FILES[@]}"; do
    cp "${STAGE_DIR}/include/${f}" "${TARGET_DIR}/include/${f}"
done
for f in "${GEN_EXTERNAL_FILES[@]}"; do
    cp "${STAGE_DIR}/external/${f}" "${TARGET_DIR}/external/${f}"
done
rm -rf "${OUT_DIR}" "${STAGE_DIR}"
echo "[regenerate] wrote ${TARGET_DIR}/{model,include,external}/ (multi-spec union — NSSelection + NSSAIAvailability)" >&2
