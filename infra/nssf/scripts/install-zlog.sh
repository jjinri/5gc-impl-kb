#!/usr/bin/env bash
# Source-build + install zlog (HardySimpson/zlog).
#
# 이유 — zlog 가 Ubuntu 22.04 universe (apt) 에 부재. NSSF logging slot 의
# ratified dependency (engineering/nssf/dependency-decisions.yaml 의 logging
# = zlog). readiness-check.yml 가 이전엔 best-effort apt + soft-fail 로 둠
# ("source build PR pending"). 본 script 가 그 deferred provisioning 의
# canonical 구현 — CI step + 로컬 dev 공용 단일 출처.
#
# License — zlog 는 Apache-2.0 (>= 1.2.17; homepage 옛 표기만 LGPL, GitHub
# LICENSE/release/src header 는 Apache-2.0, 1.2.18 포함 — 2026-06-01 정정).
# permissive (static/dynamic link 무방). 본 build 는 shared lib (dynamic).
#
# Usage:
#   sudo bash infra/nssf/scripts/install-zlog.sh           # install to /usr (CI + system)
#   ZLOG_PREFIX=$HOME/.local bash infra/nssf/scripts/install-zlog.sh   # user-local
#   ZLOG_VERSION=1.2.18 sudo bash infra/nssf/scripts/install-zlog.sh   # pin override
#
# Idempotent — 이미 >= min_version 설치돼 있으면 no-op.

set -euo pipefail

ZLOG_VERSION="${ZLOG_VERSION:-1.2.18}"
ZLOG_MIN_VERSION="1.2.16"          # cmake-dependencies.yaml pkg_config zlog min_version
PREFIX="${ZLOG_PREFIX:-/usr}"
SRC_URL="https://github.com/HardySimpson/zlog/archive/refs/tags/${ZLOG_VERSION}.tar.gz"

# Pinned source-archive SHA256 (provenance — tag pin). Only the default pinned
# version carries a known-good checksum; an overridden ZLOG_VERSION skips the
# gate with a warning (operator must pin its checksum to re-enforce).
ZLOG_SHA256_1_2_18="3977dc8ea0069139816ec4025b320d9a7fc2035398775ea91429e83cb0d1ce4e"

WORK="$(mktemp -d -t zlog-build-XXXXXX)"
trap 'rm -rf "${WORK}"' EXIT

# Idempotent guard.
if pkg-config --exists zlog 2>/dev/null \
   && pkg-config --atleast-version="${ZLOG_MIN_VERSION}" zlog 2>/dev/null; then
    echo "[install-zlog] already present — zlog $(pkg-config --modversion zlog) (>= ${ZLOG_MIN_VERSION})"
    exit 0
fi

echo "[install-zlog] fetching zlog ${ZLOG_VERSION} ..."
curl -fsSL -o "${WORK}/zlog.tar.gz" "${SRC_URL}"

# Provenance — verify pinned SHA256 for the default pinned version.
EXPECT_SHA=""
case "${ZLOG_VERSION}" in
    1.2.18) EXPECT_SHA="${ZLOG_SHA256_1_2_18}" ;;
esac
if [ -n "${EXPECT_SHA}" ]; then
    echo "${EXPECT_SHA}  ${WORK}/zlog.tar.gz" | sha256sum -c - \
        || { echo "[install-zlog] FAIL — SHA256 mismatch for zlog ${ZLOG_VERSION} (provenance gate)" >&2; exit 1; }
    echo "[install-zlog] SHA256 verified — zlog ${ZLOG_VERSION}"
else
    echo "[install-zlog] WARN — no pinned SHA256 for zlog ${ZLOG_VERSION}; skipping provenance gate" >&2
fi

tar -xzf "${WORK}/zlog.tar.gz" -C "${WORK}"
cd "${WORK}/zlog-${ZLOG_VERSION}"

echo "[install-zlog] building (make) ..."
make -j"$(nproc)"

echo "[install-zlog] installing to PREFIX=${PREFIX} ..."
make PREFIX="${PREFIX}" install

# zlog upstream ships no pkg-config (.pc) file. cmake-dependencies.yaml expects
# PkgConfig::zlog, so generate a minimal zlog.pc.
PC_DIR="${PREFIX}/lib/pkgconfig"
mkdir -p "${PC_DIR}"
cat > "${PC_DIR}/zlog.pc" <<PC
prefix=${PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: zlog
Description: a reliable, high-performance, thread safe, flexible C logging library
Version: ${ZLOG_VERSION}
Libs: -L\${libdir} -lzlog
Libs.private: -lpthread
Cflags: -I\${includedir}
PC

# Refresh linker cache when installing to a system prefix.
if [ "${PREFIX}" = "/usr" ] || [ "${PREFIX}" = "/usr/local" ]; then
    ldconfig || true
fi

INSTALLED="$(PKG_CONFIG_PATH="${PC_DIR}:${PKG_CONFIG_PATH:-}" pkg-config --modversion zlog 2>/dev/null || echo "${ZLOG_VERSION}")"
echo "[install-zlog] done — zlog ${INSTALLED} installed at ${PREFIX} (pc: ${PC_DIR}/zlog.pc)"
