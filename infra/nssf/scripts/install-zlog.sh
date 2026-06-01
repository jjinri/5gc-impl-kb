#!/usr/bin/env bash
# Source-build + install zlog (HardySimpson/zlog).
#
# 이유 — zlog 가 Ubuntu 22.04 universe (apt) 에 부재. NSSF logging slot 의
# ratified dependency (engineering/nssf/dependency-decisions.yaml 의 logging
# = zlog). readiness-check.yml 가 이전엔 best-effort apt + soft-fail 로 둠
# ("source build PR pending"). 본 script 가 그 deferred provisioning 의
# canonical 구현 — CI step + 로컬 dev 공용 단일 출처.
#
# License — zlog 는 LGPL-2.1. NSSF 바이너리는 *dynamic link* (shared lib) 로
# 사용 (dependency-decisions.yaml license_summary.weak_copyleft). static link
# 금지.
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
