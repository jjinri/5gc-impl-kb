#!/usr/bin/env bash
# Source-build + install libjwt (benmcollins/libjwt) v3.x.
#
# 이유 — Ubuntu 22.04 apt libjwt-dev candidate 는 1.10.2 로 (a) CVE-2026-33996
# affected 이고 (b) v3 JWKS API (jwks_load / jwks_load_fromurl / jwks_find_bykid
# + jwt_checker_verify) 부재. NSSF jwt slot 의 ratified dependency 는 v3.3.3
# (engineering/nssf/dependency-decisions.yaml jwt slot, re-ratify 2026-06-01
# operator + Pane 2). apt 사용 금지 — source-build only.
#
# License — libjwt v3.3.3 는 MPL-2.0 (Pane 2 검증). weak copyleft —
# NSSF 바이너리는 dynamic link (shared lib) 으로 사용.
#
# Build backend — OpenSSL (crypto) + jansson (JSON) + libcurl (JWKS-from-URL).
#   WITH_GNUTLS=OFF / WITH_MBEDTLS=OFF -> OpenSSL. WITH_JSON_C=OFF -> jansson.
#   WITH_LIBCURL=ON -> jwks_load_fromurl. WITH_TESTS=OFF.
#
# Build-time deps (apt): cmake, pkg-config, build-essential, libjansson-dev,
#   libssl-dev, libcurl4-openssl-dev.
#
# Usage:
#   sudo bash infra/nssf/scripts/install-libjwt.sh                 # install to /usr (CI + system)
#   LIBJWT_PREFIX=$HOME/.local bash infra/nssf/scripts/install-libjwt.sh   # user-local
#
# Idempotent — 이미 == LIBJWT_VERSION 설치돼 있으면 no-op.

set -euo pipefail

LIBJWT_VERSION="${LIBJWT_VERSION:-3.3.3}"
PREFIX="${LIBJWT_PREFIX:-/usr}"
ASSET="libjwt-${LIBJWT_VERSION}.tar.xz"
SRC_URL="https://github.com/benmcollins/libjwt/releases/download/v${LIBJWT_VERSION}/${ASSET}"

# Pinned release-asset SHA256 (provenance). Overridden version skips with WARN.
LIBJWT_SHA256_3_3_3="88d56f428d186cf1af180f52b841ea348c6b4f1d1f0fbd3e75df8f1bd076df64"

WORK="$(mktemp -d -t libjwt-build-XXXXXX)"
trap 'rm -rf "${WORK}"' EXIT

# Idempotent guard — exact pinned version (v3 ABI/API is pin-sensitive).
if pkg-config --exists libjwt 2>/dev/null \
   && [ "$(pkg-config --modversion libjwt 2>/dev/null)" = "${LIBJWT_VERSION}" ]; then
    echo "[install-libjwt] already present — libjwt ${LIBJWT_VERSION}"
    exit 0
fi

# Build-time dep sanity (clear message rather than a deep cmake error).
for dep in jansson openssl libcurl; do
    if ! pkg-config --exists "${dep}" 2>/dev/null; then
        echo "[install-libjwt] FAIL — missing build dep '${dep}'. Install: libjansson-dev libssl-dev libcurl4-openssl-dev pkg-config cmake" >&2
        exit 1
    fi
done

echo "[install-libjwt] fetching libjwt ${LIBJWT_VERSION} (${ASSET}) ..."
curl -fsSL -o "${WORK}/${ASSET}" "${SRC_URL}"

# Provenance — verify pinned SHA256 for the default pinned version.
EXPECT_SHA=""
case "${LIBJWT_VERSION}" in
    3.3.3) EXPECT_SHA="${LIBJWT_SHA256_3_3_3}" ;;
esac
if [ -n "${EXPECT_SHA}" ]; then
    echo "${EXPECT_SHA}  ${WORK}/${ASSET}" | sha256sum -c - \
        || { echo "[install-libjwt] FAIL — SHA256 mismatch for libjwt ${LIBJWT_VERSION} (provenance gate)" >&2; exit 1; }
    echo "[install-libjwt] SHA256 verified — libjwt ${LIBJWT_VERSION}"
else
    echo "[install-libjwt] WARN — no pinned SHA256 for libjwt ${LIBJWT_VERSION}; skipping provenance gate" >&2
fi

tar -xf "${WORK}/${ASSET}" -C "${WORK}"
SRC_DIR="${WORK}/libjwt-${LIBJWT_VERSION}"

echo "[install-libjwt] configuring (cmake — OpenSSL + jansson + libcurl) ..."
cmake -S "${SRC_DIR}" -B "${SRC_DIR}/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DWITH_GNUTLS=OFF \
    -DWITH_MBEDTLS=OFF \
    -DWITH_JSON_C=OFF \
    -DWITH_LIBCURL=ON \
    -DWITH_TESTS=OFF

echo "[install-libjwt] building ..."
cmake --build "${SRC_DIR}/build" -j"$(nproc)"

echo "[install-libjwt] installing to PREFIX=${PREFIX} ..."
cmake --install "${SRC_DIR}/build"

# libjwt ships its own libjwt.pc (cmake install) — no .pc generation needed.
if [ "${PREFIX}" = "/usr" ] || [ "${PREFIX}" = "/usr/local" ]; then
    ldconfig || true
fi

INSTALLED="$(PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig:${PREFIX}/lib64/pkgconfig:${PKG_CONFIG_PATH:-}" pkg-config --modversion libjwt 2>/dev/null || echo "${LIBJWT_VERSION}")"
echo "[install-libjwt] done — libjwt ${INSTALLED} installed at ${PREFIX}"
