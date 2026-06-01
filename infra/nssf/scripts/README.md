# infra/nssf/scripts

NSSF build/runtime dependency provisioning — apt 가 만족 못 하는 dependency 의
canonical source-build. CI (`.github/workflows/readiness-check.yml`) 와 로컬
dev 가 *동일 script* 를 호출한다 (단일 출처).

## install-zlog.sh

zlog (HardySimpson/zlog) source build + install. Ubuntu 22.04 universe 에 zlog
가 부재하여 apt 로 설치 불가 — ratified logging dependency
(`engineering/nssf/dependency-decisions.yaml` logging slot = zlog).

```bash
# 시스템 설치 (CI 와 동일, /usr) — root 필요
sudo bash infra/nssf/scripts/install-zlog.sh

# 사용자 로컬 설치 (root 없이)
ZLOG_PREFIX="$HOME/.local" bash infra/nssf/scripts/install-zlog.sh
# 이후 빌드 전: export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH"

# 버전 override
ZLOG_VERSION=1.2.18 sudo bash infra/nssf/scripts/install-zlog.sh
```

- pinned `1.2.18` (cmake-dependencies.yaml `pkg_config.zlog.min_version: 1.2.16`).
- zlog 는 pkg-config (`.pc`) 를 배포하지 않으므로 script 가 `zlog.pc` 를 생성
  (`PkgConfig::zlog` lookup 충족).
- idempotent — 이미 `>= 1.2.16` 설치 시 no-op.
- provenance — 1.2.18 source archive SHA256
  (`3977dc8ea0069139816ec4025b320d9a7fc2035398775ea91429e83cb0d1ce4e`) 을 script
  가 `sha256sum -c` 로 검증. version override 시 미pin 이면 WARN + skip.
- License — zlog 는 Apache-2.0 (>= 1.2.17; homepage 옛 표기만 LGPL, GitHub
  LICENSE/release/src header 는 Apache-2.0 — 2026-06-01 정정). permissive
  (`dependency-decisions.yaml` license_summary.permissive; static/dynamic 무방,
  본 build 는 dynamic).

## 미해결 — libjwt

`cmake-dependencies.yaml` 는 libjwt `>= 1.16` 을 요구하나 Ubuntu 22.04 apt
candidate 는 `1.10.2` (구버전 + 다른 API). upstream (`benmcollins/libjwt`)
현행 release 는 `v2.x`/`v3.x` 로 API rework 가 커서 어느 line 을 채택할지는
engineering 결정 (ratified `dependency-decisions.yaml` jwt slot 갱신 동반 가능).
본 scripts dir 의 후속 `install-libjwt.sh` 는 그 결정 확정 후 추가한다.
