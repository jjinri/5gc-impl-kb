# Session 2026-05-27 (Evening) Handoff — Phase 1 wave 1 진입 + WI-codegen-bootstrap 완성

> **목적.** 2026-05-27 저녁 세션 (PR #87~#92, 6 PR merged) 의 종결 상태와 다음 세션의 *즉시 재개* 절차를 self-contained 로 보존. 본 doc + git log + 기존 docs/handover.md + memory 만으로 다음 Claude 가 재개 가능해야 함.

---

## TL;DR

- **Status** — `phase1_wave1_codegen_bootstrap_merged`. NSSF 의 32 generated schema + nssf_generated static lib + verify oracle + drift gate active.
- **Main HEAD** — `6b12c05` (PR #92 머지 후).
- **본 세션 PR 머지** — `#87` ~ `#92` (6 PR). Phase 1 wave 0 tracer-bullet → execution-control pack → autonomous-impl-prep pack → drift validator → CI deps prep → WI-codegen-bootstrap.
- **다음 PR 후보** — pr-slicing-plan.yaml 의 Phase 1 wave 1 sequence 의 next.

---

## 1. 새 세션 첫 행동 체크리스트

```bash
# (1) repo sync
git clone https://github.com/jjinri/5gc-impl-kb.git   # 또는 cd 후 git pull --ff-only
cd 5gc-impl-kb
git log --oneline -3   # 6b12c05 head 확인

# (2) Python venv (PC 별 .venv 는 git 비추적)
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt

# (3) auto-memory 재생성 — 기존 handover.md + 본 doc.
DIR=~/.claude/projects/-home-$USER-AI-5gc-impl-kb/memory
mkdir -p "$DIR"
# docs/handover.md §1.1 ~ §1.6 markdown 블록 cp.

# (4) Phase 1 wave 1 prerequisite — cmake + apt deps (PR #91 의 readiness-check.yml step 과 동일).
sudo apt-get update
sudo apt-get install -y cmake build-essential pkg-config \
    libnghttp2-dev libcurl4-openssl-dev libuv1-dev libssl-dev \
    libjwt-dev libpq-dev valgrind clang-tidy default-jre
sudo apt-get install -y libzlog-dev || true   # universe 부재 가능, source build PR 별도

# (5) 전체 lifecycle 재생성 (gitignored status yaml).
.venv/bin/python3 design/scripts/nf-manifest.py nssf --primary 29.531 --write
.venv/bin/python3 design/scripts/nf-seed-gen.py nssf
.venv/bin/python3 design/scripts/materialize-contract.py nssf --force
.venv/bin/python3 design/scripts/build-handoff.py nssf
.venv/bin/python3 design/scripts/nf-contract-check.py nssf
.venv/bin/python3 design/scripts/nf-arch-status.py nssf
.venv/bin/python3 design/scripts/nf-impl-status.py nssf
.venv/bin/python3 design/scripts/nf-eng-status.py nssf
.venv/bin/python3 design/scripts/nf-readiness-status.py nssf
# 기대 — readiness_pack_ready PASS

# (6) pytest 회귀.
.venv/bin/python3 -m pytest tests/ -q   # 기대 98 passed

# (7) CMake build + ctest (PR #87 wave 0 + PR #92 wave 1 codegen).
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure   # 기대 2/2 PASS

# (8) 본 세션 added validators.
.venv/bin/python3 design/scripts/validate-artifact-catalog.py
.venv/bin/python3 design/scripts/validate-execution-control-drift.py nssf
.venv/bin/python3 design/scripts/verify-generated-models.py nssf

# (9) (옵션) codegen drift check (JRE + 30MB jar download 필요).
bash infra/nssf/codegen/regenerate.sh --check
```

위 9 step 후 다음 PR 진입 가능.

---

## 2. 본 세션 (2026-05-27 저녁) 누적 변경

| # | 분류 | 제목 | LOC |
|---|---|---|---|
| #87 | feat | Phase 1 tracer-bullet — NSSelectionGet 501 stub + CMake skeleton | +4620 (Unity 4353 vendored) |
| #88 | plan | execution-control pack — agent + verification + slicing yaml | +1033 |
| #89 | plan | autonomous-impl-prep pack — 13 산출 + library stack ratify | +1621 |
| #90 | ci | drift validator — 5 cross-source id 일관성 가드 | +511 |
| #91 | ci | Phase 1 wave 1 dependency prep — apt install + sanity probe | +70 |
| #92 | feat | WI-codegen-bootstrap — openapi-generator(C) + cJSON + 32 schema | +15482 (generator output 대부분) |

### 결과 요약

- **Phase 1 wave 0 (tracer-bullet)** — handler stub + CMakeLists + Unity + ctest. ASan/UBSan 활성.
- **Execution-control pack** — `dev/nssf/agent-execution-plan.yaml`, `verification-matrix.yaml`, `pr-slicing-plan.yaml`. 5 lane + 7 phase + 28 check id + 20 PR slice 카탈로그.
- **Autonomous-impl-prep pack** — 13 추가 산출. library stack ratified (libcurl + libuv + uthash + zlog + valgrind 채택). config schema + operator-inputs + error-cause-catalog + failure-recovery + migrations manifest + GENERATION_MANIFEST + openapi-generator config + drift-allowlist + test fixture manifest + golden json + cmake-dependencies + dependency-decisions yaml.
- **Drift validator** — 5 section cross-source 일관성 검사 + CI gate. 첫 검출 1건 (nf_eng_status 누락) → 동일 PR 에서 fix.
- **CI deps prep** — JRE + libnghttp2 + libcurl + libuv + libssl + libjwt + libpq + valgrind + clang-tidy apt install + pkg-config sanity probe step.
- **WI-codegen-bootstrap** — 32 schema (NSSelection emit + TS29571 transitive) + cJSON vendored + nssf_generated static lib + verify-generated-models.py oracle + regenerate.sh (deterministic regen + --check). 3-way classification (expected / wrapper_expected / deferred / unexpected).

---

## 3. NSSF 구현 진행 상태 — Phase 1 wave 1 다음 단계

### 3.1 현재 위치 (pr-slicing-plan.yaml 기준)

| PR slice | 상태 | 비고 |
|---|---|---|
| `PR-phase1-wave0` | ✓ merged (#87) | tracer-bullet toolchain |
| `PR-execution-control-pack` | ✓ merged (#88) | agent/verification/slicing yaml |
| `PR-library-stack-decision-pass` | ✓ merged (#89 흡수) | autonomous-impl-prep + library ratify |
| `PR-drift-validator` | ✓ merged (#90) | execution-control drift |
| `PR-ci-dependency-prep` | ✓ merged (#91) | apt install + sanity |
| `PR-phase1-wave1-codegen-bootstrap` | ✓ merged (#92) | openapi-generator + 32 schema |
| `PR-codegen-nssaiavailability-extension` | not_started | NSSAIAvailability spec codegen (multi-spec strategy 2단계) |
| `PR-phase1-wave1-nftype-wrapper` | not_started | NFType anyOf passthrough wrapper (G-09) |
| `PR-phase1-wave1-problem-details-wrapper` | not_started | RFC 7807 + 18 cause factory + header rule |
| `PR-phase1-wave1-schema-bootstrap` | not_started | PostgreSQL schema.sql + 3 table + retry_queue |
| `PR-phase1-wave1-tls-bootstrap` | not_started | TLS context + OAuth2 JWKS + scope check |
| `PR-phase1-wave1-availability-repo` | not_started | libpq backend + in-memory mock |
| `PR-phase1-wave1-selection-engine` | not_started | Registration / PDUSession / UEConfigUpdate engine |
| `PR-phase1-wave1-nsselection-handler` | not_started | NSSelectionGet route + golden path. Boundary stub 교체. |

### 3.2 권장 다음 PR 순서

`pr-slicing-plan.yaml` 의 `depends_on` 그래프 따라:

1. **`PR-codegen-nssaiavailability-extension`** — NSSAIAvailability spec emit + verifier deferred → expected 승격. 의존 0 (PR #92 머지 후 즉시 진입).
2. **`PR-phase1-wave1-nftype-wrapper`** — NFType wrapper (depends_on: PR #92). G-09 정책 구현.
3. **`PR-phase1-wave1-problem-details-wrapper`** — 18 cause factory (depends_on: PR #92). `error-cause-catalog.yaml` 의 entry 와 1:1.
4. **`PR-phase1-wave1-schema-bootstrap`** — PostgreSQL schema.sql (depends_on: PR #92). M001-bootstrap migration.
5. **`PR-phase1-wave1-tls-bootstrap`** — TLS / OAuth2 (depends_on: PR #92). libzlog source build 가 필요할 수 있음.

2~5 는 *parallel* 가능 (의존 모두 PR #92 만). 1 은 codegen pipeline 확장.

### 3.3 사전 결정 필요 사항 (다음 PR 진입 전 확정)

| # | 결정 | 추천 default | 근거 |
|---|---|---|---|
| 1 | `libzlog-dev` Ubuntu 22.04 universe 부재 → source build 여부 | Phase 1 wave 1 첫 tls-bootstrap PR 진입 시 source build PR 분리 | PR #91 의 best-effort step 결과 확인 후 |
| 2 | NSSAIAvailability codegen 의 emit count | regenerate.sh 확장 후 verify-generated-models.py 의 expected_present 결과로 확정 | NSSelection 7 emitted, 합쳐서 14 contract data-model 도달 목표 |
| 3 | openapi-generator-cli version pin 7.10.0 → upgrade 시점 | spec update 시 동시에 검토 | 현재 7.10.0 stable |
| 4 | cJSON version unification (vendored 1.7.18 vs generator's external) | 향후 cJSON wrapper PR 에서 통일 | 현재는 generator's external 사용 |
| 5 | 4 arch module dir (selection_engine/availability_engine/subscription_store/notification_dispatcher) 생성 시점 | PR-phase1-wave1-selection-engine 진입 시 일괄 생성 | engineering-design L58-62 layout 정합 |

---

## 4. 결정·인계된 정책

### 4.1 Library stack ratified (2026-05-27 user instruction)

`engineering/nssf/dependency-decisions.yaml` 의 16 slot 모두 ratified.

| Slot | Decision |
|---|---|
| language | C11 |
| build_system | CMake 3.22+ |
| test_framework | Unity 2.6.0 (vendored) |
| http_server | nghttp2 |
| http_client | **libcurl** (채택) |
| event_loop | **libuv** (채택) |
| json | cJSON 1.7.18 (vendored) |
| tls | OpenSSL |
| jwt | libjwt |
| logging | **zlog** (채택) |
| inmemory_cache | **uthash** hot cache (채택) |
| database | PostgreSQL / libpq |
| schema_codegen | openapi-generator 7.10.0 |
| lint_tool | clang-tidy |
| memory_check | **valgrind** (추가) |
| static_analysis | clang-analyzer |

### 4.2 Generated code 보정 정책 (Pane 2 권고)

generated code 직접 수정 금지. 보정은 외부 layer 에서.

순서:
1. bundle/preprocess (regenerate.sh 의 spec flat dir)
2. generator config mappings (additional-properties, --skip-validate-spec)
3. template override (필요 시)
4. wrapper/adapter (NFType passthrough, ProblemDetails factory, RFC 6902 validator)
5. verifier gate (verify-generated-models.py)

### 4.3 Multi-spec strategy

openapi-generator-cli 단일 -i input 한계. NSSelection + NSSAIAvailability 는 *각각 generate + 통합*. 본 세션 PR #92 = NSSelection only. NSSAIAvailability 는 별도 PR (multi_spec_strategy.next_pr).

### 4.4 Drift gate / verifier classification

**execution-control drift** (PR #90) — 5 source yaml 의 id reference 일관성. CI fail 시 fresh-clone agent 가 잘못된 source 읽는 위험 차단.

**verify-generated-models** (PR #92) — 3-way classification:
- expected_present (PASS)
- expected_missing (FAIL)
- wrapper_expected_missing (PASS classified — wrapper PR 에서 보정)
- deferred_missing (PASS — multi-spec)
- unexpected_transitive (informational — TS29571/TS29510 transitive)

**codegen drift** (PR #92) — `regenerate.sh --check` 가 generator output ↔ committed src/nssf/generated/ byte-level diff. 본 세션은 CI 미포함 (JRE + 30MB jar download 비용).

### 4.5 Pane 간 작업 분리

본 세션 (Pane 1) = NSSF 구현 / CI / planning 산출.
Pane 2 = docs / README / CONTEXT / ONBOARDING / lifecycle-artifacts.md / kb README WIP.

Pane 1 은 main 의 unstaged docs 변경 *건드리지 않음*. Pane 2 가 별도 PR 처리 예정.

### 4.6 Worktree 격리 패턴

Pane 2 가 main 에 docs WIP 보유 시 본 pane (Pane 1) 작업은 `git worktree add ../<dir-suffix> -b <branch> origin/main` 으로 clean main 기준 격리.

본 세션 worktree 사용 PR:
- PR #91 (`/home/jjinri/AI/5gc-impl-kb-ci-deps`)
- PR #92 (`/home/jjinri/AI/5gc-impl-kb-codegen-bootstrap`)

머지 후 `git worktree remove --force` + `git branch -D` 정리.

---

## 5. 남은 follow-ups

### 5.1 Phase 1 wave 1 잔여 (pr-slicing-plan.yaml 의 not_started)

- NSSAIAvailability codegen extension (multi-spec 2단계)
- NFType wrapper (anyOf passthrough)
- ProblemDetails wrapper (RFC 7807 factory)
- PatchDocument wrapper (RFC 6902 validator)
- schema-bootstrap (PostgreSQL)
- tls-bootstrap (OpenSSL + libjwt + libuv)
- availability-repo (libpq + in-memory mock)
- selection-engine (Registration / PDUSession / UEConfigUpdate)
- nsselection-handler (route + golden path + boundary stub 교체)

### 5.2 Wave 1 종료 → Wave 2~5 (Phase 2~5)

- Phase 2 — NSSAIAvailability writes (Put/Patch/Delete/Options + notification dispatcher)
- Phase 3 — subscription store + handlers
- Phase 4 — contract / security / e2e tests
- Phase 5 — observability finalize (metric / log / trace seam)

### 5.3 Framework 검증 (선택)

- 두 번째 NF `/nf-readiness <nf>` 사이클 — NRF / AMF / UDM 중 1개. framework NF-invariant 검증.
- agent-execution-plan ↔ team-execution-plan drift validator 추가 (Pane 2 권고).

### 5.4 Engineering / Operations

- `libzlog-dev` source build PR (Ubuntu 22.04 universe 부재 시).
- openapi-generator-cli sha256 pin (현재 placeholder).
- `infra/nssf/codegen/openapi-generator-cli.config.yaml` 의 version pin 7.10.0 검토.
- CI 에 `regenerate.sh --check` step 추가 검토 (JRE download 비용 vs drift 보장).

---

## 6. 본 세션이 보존하지 않는 것

- 이번 conversation 의 in-flight 추론 — 본 doc + git log + ADR/policy yaml + memory 로 충분히 재구성 가능.
- Pane 2 docs WIP 상세 — Pane 2 가 별도 handoff/PR 로 처리.
- 다른 PC 의 .venv / pip cache / build/ — PC-local, gitignored. 재생성 필요.
- Pane 2 와의 송수신 메시지 상세 — 본 세션 PR description / commit message 에 핵심 결정만 보존.

---

## 7. 자동 검증 (새 세션 첫 진입 시)

```bash
# 의존 install 후 (위 §1 step 4).
.venv/bin/python3 -m pytest tests/ -q
# 기대 — 98 passed.

.venv/bin/python3 design/scripts/nf-readiness-status.py nssf | tail -3
# 기대 — gates.readiness_pack_ready PASS.

.venv/bin/python3 design/scripts/validate-artifact-catalog.py
.venv/bin/python3 design/scripts/validate-execution-control-drift.py nssf
.venv/bin/python3 design/scripts/verify-generated-models.py nssf
# 기대 — 셋 다 PASS.

cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
# 기대 — 2/2 PASS (nsselection_handler_smoke + generated_smoke).

bash infra/nssf/codegen/regenerate.sh --check
# 기대 — PASS, no drift (JRE 필요).
```

위 5 검증 모두 PASS 이면 새 세션 환경 정합 + 본 세션 회귀 0 확정.

---

## 8. PR 머지 시퀀스 (2026-05-27 저녁)

```
6b12c05 feat(nssf): WI-codegen-bootstrap — openapi-generator(C) + cJSON + 32 schema (#92)
90350e6 ci(deps): Phase 1 wave 1 dependency prep — apt install + sanity probe (#91)
8693e37 ci(execution-control): drift validator — 5 cross-source id 일관성 가드 (#90)
d6d1733 plan(nssf): autonomous-impl-prep pack — 13 산출 + library stack ratify (#89)
041128c plan(nssf): execution-control pack — agent + verification + slicing (#88)
52ff1f6 feat(nssf): Phase 1 tracer-bullet — NSSelectionGet 501 stub + CMake skeleton (#87)
```

---

**End of evening handoff.** 다음 세션은 본 doc 의 §1 → §3 → §5 순으로 읽으면 즉시 진입 가능. `pr-slicing-plan.yaml` 의 not_started PR 중 다음 후보 선택 후 worktree 격리 + 진행.
