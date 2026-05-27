#!/usr/bin/env bash
# Regenerate src/nssf/generated/ from specs/29.531/ + specs/29.571/ + specs/29.510/.
#
# Usage:
#   bash infra/nssf/codegen/regenerate.sh              # regenerate in-place
#   bash infra/nssf/codegen/regenerate.sh --check      # regenerate to tmp + diff
#
# 의존:
#   - JRE (default-jre)
#   - openapi-generator-cli 7.10.0 jar (자동 download to /tmp 또는 OPENAPI_GEN_JAR env)
#   - 3 spec yaml (specs/29.531/TS29531_Nnssf_NSSelection.yaml + 의존)
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

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
SPECS_DIR="${REPO_ROOT}/specs"
TARGET_DIR="${REPO_ROOT}/src/nssf/generated"
GENERATOR_VERSION="7.10.0"
GENERATOR_JAR_URL="https://repo1.maven.org/maven2/org/openapitools/openapi-generator-cli/${GENERATOR_VERSION}/openapi-generator-cli-${GENERATOR_VERSION}.jar"
GENERATOR_JAR_PATH="${OPENAPI_GEN_JAR:-/tmp/openapi-generator-cli-${GENERATOR_VERSION}.jar}"

MODE="${1:---write}"

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

echo "[regenerate] running openapi-generator on NSSelection..." >&2
(
    cd "${BUNDLE_DIR}"
    java -jar "${GENERATOR_JAR_PATH}" generate \
        -i TS29531_Nnssf_NSSelection.yaml \
        -g c \
        -o "${OUT_DIR}/nsselection" \
        --additional-properties=packageName=nssf \
        --skip-validate-spec \
        > /dev/null 2>&1
)

# Stage산출 to tmp target.
STAGE_DIR="$(mktemp -d -t nssf-codegen-stage-XXXXXX)"
mkdir -p "${STAGE_DIR}/model" "${STAGE_DIR}/include" "${STAGE_DIR}/external"

cp "${OUT_DIR}/nsselection/model/"*.h "${OUT_DIR}/nsselection/model/"*.c "${STAGE_DIR}/model/"
cp "${OUT_DIR}/nsselection/include/list.h" \
   "${OUT_DIR}/nsselection/include/binary.h" \
   "${OUT_DIR}/nsselection/include/keyValuePair.h" \
   "${STAGE_DIR}/include/"
cp "${OUT_DIR}/nsselection/external/cJSON.h" \
   "${OUT_DIR}/nsselection/external/cJSON.c" \
   "${OUT_DIR}/nsselection/external/cJSON.licence" \
   "${STAGE_DIR}/external/"

if [ "${MODE}" = "--check" ]; then
    # Diff against committed.
    DIFF_OUTPUT=$(diff -r "${TARGET_DIR}/model" "${STAGE_DIR}/model" || true)
    DIFF_OUTPUT+=$'\n'$(diff -r "${TARGET_DIR}/include" "${STAGE_DIR}/include" || true)
    DIFF_OUTPUT+=$'\n'$(diff -r "${TARGET_DIR}/external" "${STAGE_DIR}/external" || true)

    # Filter empty.
    DIFF_FILTERED=$(printf "%s\n" "${DIFF_OUTPUT}" | grep -v '^$' || true)

    if [ -n "${DIFF_FILTERED}" ]; then
        echo "[regenerate] FAIL drift detected:" >&2
        printf "%s\n" "${DIFF_FILTERED}" >&2
        rm -rf "${OUT_DIR}" "${STAGE_DIR}"
        exit 1
    fi
    echo "[regenerate] PASS — no drift (src/nssf/generated ↔ regenerated)" >&2
    rm -rf "${OUT_DIR}" "${STAGE_DIR}"
    exit 0
fi

# --write mode: replace committed.
rm -rf "${TARGET_DIR}/model" "${TARGET_DIR}/include" "${TARGET_DIR}/external"
mv "${STAGE_DIR}/model" "${TARGET_DIR}/model"
mv "${STAGE_DIR}/include" "${TARGET_DIR}/include"
mv "${STAGE_DIR}/external" "${TARGET_DIR}/external"
rm -rf "${OUT_DIR}" "${STAGE_DIR}"
echo "[regenerate] wrote ${TARGET_DIR}/{model,include,external}/" >&2
