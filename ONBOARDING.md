
# 5gc-impl-kb — NF 개발 온보딩 (specs → 자율 구현)

**프로젝트 최종 목표는 AI agent 가 3GPP spec 을 입력으로 완성된 NSSF 구현 코드를 자율 생성·빌드하는 것이다.** 사람의 public workflow 는 *3 행위* 로 축소된다 — (1) `specs/` 에 3GPP 원본 투입, (2) `/nf-readiness <nf>` 요청, (3) `/nf-implement <nf>` 요청. 세부 lifecycle skill (`/nf-spec-discover`, `/nf-contract-build`, `/nf-arch-design`, `/nf-impl-plan`, `/nf-eng-design` 등) 은 `/nf-readiness` 내부 subroutine 으로 재배치되며 사람의 주 작업 surface 가 아니다.

> **2026-05-21 workflow upgrade pending.** 본 문서의 lifecycle 흐름은 in-progress upgrade 사이클의 *현재 상태* 를 반영한다. 최종 public workflow (`/nf-readiness` + `/nf-implement`) 는 `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` 의 PR A~G 사이클로 단계적으로 도입된다. 본 PR (PR A) 는 정책/문서 layer 만 — 신규 wrapper skill 자체는 PR E 에서 신설.

> 용어·단계 이름은 `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md` 를 따른다. 정책은 `CLAUDE.md`, Git 원칙은 `AGENTS.md`.
> 파일 관리 기준은 `docs/artifact-management.md` 를 따른다 — 원본, 로컬 재생성 산출물, git 추적 lifecycle 산출물, 작업 계획/회고를 구분한다.

---

## 0. 큰 그림

### Target public workflow (upgrade 완료 후)

```
specs/ (사람: 3GPP 원본 투입)
  └─ /nf-readiness <nf>  ─→  implementation readiness pack 자동 생성
        └─ 내부 pipeline (spec discovery → contract build → contract implementability check
            → arch design/status → impl-plan + readiness pack → impl-status
            → eng-design/status → readiness_pack_ready)
              └─ ★ readiness_pack_ready PASS ★
                    └─ /nf-implement <nf>  ─→  장기 autonomous 구현
                          └─ Phase 1 tracer-bullet proof
                          └─ Phase 2 feedback integration
                          └─ Phase 3 full feature waves
                          └─ Phase 4 contract/security/e2e verification
                          └─ Phase 5 hardening/review/merge
                                └─ ★ full_nf_done ★
```

**최종 GO gate = `readiness_pack_ready`** (이전 `eng_frozen` 단독 GO 폐기). `eng_frozen` 은 readiness 의 한 구성요소 (tech decision freeze) 일 뿐, 단독으로 autonomous implementation GO 가 아니다.

### Current internal pipeline (PR A 시점)

```
specs/ (사람: 3GPP 원본 투입)
  └─ /nf-spec-discover ─→ _manifest.yaml + _contract_seed.yaml
        └─ /nf-contract-build ─→ design/<nf>/contract/ + handoff/<nf>/contract.yaml
              └─ /nf-contract-check ──[gate: handoff_ready · contract_implementable (PR B)]
                    └─ /nf-arch-design ─→ design/<nf>/architecture/ + module-decomposition/
                          └─ /nf-arch-status ──[gate: arch_consistent]
                                └─ /nf-impl-plan ─→ dev/<nf>/ (impl-plan + tasks + test-matrix + traceability + readiness pack PR C)
                                      └─ /nf-impl-status ──[gate: impl_consistent · impl_ready_for_codegen (PR C)]
                                            └─ /nf-eng-design ─→ engineering/<nf>/engineering-design.md (사람 ratify)
                                                  └─ /nf-eng-status ──[gate: eng_frozen]
                                                        └─ readiness_pack_ready (PR D aggregate)
```

본 lifecycle 단계들은 *PR E 에서 `/nf-readiness` 내부 subroutine 으로 재배치* 된다. 그때 사람은 더이상 단계별 skill 을 직접 호출하지 않는다 (third-party library 교체 / DBMS 교체 / security policy 변경 / spec 추가·제외 등 "새 계약" 시에만 예외적으로 직접 호출).

핵심 원칙.

- **lifecycle skill 은 다음 skill 을 자동 호출하지 않는다.** 각 단계는 자기 script/check 만 실행하고 다음 단계는 *추천* 으로 보고. 사람이 단계 진행을 결정한다. (PR E `/nf-readiness` wrapper 신설 후엔 wrapper 가 단계 호출 책임.)
- **산출물은 로컬 재생성물** (`_manifest.yaml`, `_contract_seed.yaml`, `contract/`, `handoff/`, `_*_status.yaml`) — git 비추적. 도구가 진실 출처. fresh checkout 후 skill 재실행으로 복원.
- **architecture/dev/engineering 산출은 git 추적** — PR 사이클로 머지.
- **status skill 은 측정만 (read-only).** 산출 수정은 생성 skill 의 책임.
- **codegen agent 가 원본 OpenAPI YAML 을 *semantic rediscovery* 용도로 다시 읽으면 실패다.** 허용 = drift / regen / source trace 확인. 비허용 = 구현 판단 / data model 의미 재추출 / API operation matrix 재구성. 이 기준은 PR B (`contract_implementable`) + PR C (`impl_ready_for_codegen`) validator 가 강제한다.

---

## 1. 단계별 절차

각 단계 = 트리거 / 사람 / AI agent / skill / script / 입력 / 산출 / gate.

### 단계 A — spec 파일 준비

| 항목 | 내용 |
|---|---|
| 사람 | 3GPP 원본 (`.docx`/`.yaml`/`.pdf`) 을 `specs/<spec-number>/` 에 투입 (예 `specs/29.531/29531-j60.docx` + `TS29531_Nnssf_*.yaml`). 정식 점 표기 폴더, 다운로드 원본 파일명 유지. |
| AI agent | 없음 (사람 전용 — 원본 확보·라이선스·버전 선택은 사람 판단). |
| 입력 | 3GPP 포털 다운로드. |
| 산출 | `specs/<spec>/` 의 원본 파일 (git 추적, `_extracted/` 등 캐시는 비추적). |

### 단계 B — spec discovery + seed 생성

| 항목 | 내용 |
|---|---|
| 트리거 | "NSSF 시작", "/nf-spec-discover nssf --primary 29.531" |
| skill | `/nf-spec-discover` (호환 alias `/nf-init`) |
| script | `design/scripts/nf-manifest.py` (의존 spec 자동 검출 → `_manifest.yaml`), `design/scripts/nf-seed-gen.py` (ready 면 `_contract_seed.yaml` auto-gen) |
| AI agent | skill 실행, manifest refresh, ready 판정, seed 생성, 부족 spec 을 priority 순 보고. |
| 사람 | (1) 부족 spec 을 `specs/` 에 추가 후 재호출 (반복 가능). (2) `_manifest.yaml` `manual_overrides.exclude` 결정 — 33.501 / 33.310 / 33.210 등 *project-wide security/profile spec* 은 `docs/adr/ADR-0004-project-security-baseline.md` 으로 흡수 (lifecycle extraction dependency 아님). 그 외 cross-NF/operational spec (예 38.413 NGAP) 지원 여부는 *정책 결정*, agent 독단 금지. (3) stale seed 의심·도구 fix 후 `nf-seed-gen.py --force` 판단. |
| 산출 | `design/<nf>/_manifest.yaml`, `design/<nf>/_contract_seed.yaml` (둘 다 재생성물). |
| gate | manifest `status.ready_for_build == true`. |

### 단계 C — contract 생성

| 항목 | 내용 |
|---|---|
| 트리거 | "/nf-contract-build nssf", "NSSF contract 생성" |
| skill | `/nf-contract-build` (호환 alias `/nf-build`) |
| script | `resolve-yaml-refs.py --emit-json` (data-model JSON), `build-handoff.py` (→ `handoff/<nf>/contract.yaml` handoff-v2), `validate-extraction.py` (basic 13 룰) |
| AI agent | seed 의 categories/topics 에 따라 토픽 파일 생성·갱신, AUTO/USER marker 로 사람 산문 보존, handoff yaml 빌드 + 검증. |
| 사람 | USER marker 영역 산문 작성·검토. 자동 커밋 안 함 — 사람이 커밋 결정. |
| 산출 | `design/<nf>/contract/<category>/<topic>.md(.json)`, `handoff/<nf>/contract.yaml` (재생성물). |

### 단계 D — contract 검증 (게이트)

| 항목 | 내용 |
|---|---|
| 트리거 | "/nf-contract-check nssf", "handoff_ready 확인" |
| skill | `/nf-contract-check` (호환 alias `/nf-status`) |
| script | `design/scripts/nf-status.py` → `_contract_status.yaml` |
| AI agent | Tier 1~4 check 실행, acceptance gate 보고. Tier 4 subjective 는 sub-agent judge. |
| 사람 | FAIL 시 `to_pass` 를 보고 contract 보강 사이클 반복 결정. `manual_overrides.pass_anyway` 는 사람 명시 결정 (사유 필수). |
| gate | **`handoff_ready`** — PASS 면 architecture 설계 시작 가능 (= 상세 아키텍처 입력으로 충분). |

### 단계 E — architecture 설계

| 항목 | 내용 |
|---|---|
| 트리거 | "/nf-arch-design nssf" (handoff_ready PASS 후) |
| skill | `/nf-arch-design` |
| 입력 | `handoff/<nf>/contract.yaml`, `design/<nf>/contract/**`, `_manifest.yaml` (메타만) |
| AI agent | canonical 6 섹션 (Purpose / Inputs (contract) / Boundaries / Decisions / Open Questions / References) 으로 arch 10 문서 + ADR-0001 + module-decomposition (canonical 7 섹션) 생성. contract-derived 사실만, 구현 선택은 `TBD`. |
| 사람 | arch 문서 검토, 운영 결정 보류 항목 (보안 profile·cross-NF) 의 Open Questions 확정 여부. PR 리뷰·머지 결정. |
| 산출 | `design/<nf>/architecture/*.md` + `decisions/ADR-0001-architecture-baseline.md` + `design/<nf>/module-decomposition/<Module>.md` (git 추적). |

### 단계 F — architecture 자기 일관성 검증 (게이트)

| 항목 | 내용 |
|---|---|
| 트리거 | "/nf-arch-status nssf" |
| skill | `/nf-arch-status` (read-only 측정) |
| script | `design/scripts/nf-arch-status.py` → `design/<nf>/_arch_status.yaml` |
| AI agent | canonical 섹션 exact-match, 필수 파일, frontmatter, module-decomposition 존재·섹션, module-boundaries xref (advisory WARN) 검사. |
| 사람 | FAIL 시 `/nf-arch-design` 으로 `to_pass` 반영. 진짜 우회는 `_arch_status.yaml` `manual_overrides.pass_anyway` 에 `{check, reason, confirmed_by}` 명시. |
| gate | **`arch_consistent`** — Tier 1 binary AND. |

### 단계 G — implementation-planning (dev harness 설계)

| 항목 | 내용 |
|---|---|
| 트리거 | "/nf-impl-plan nssf" (arch 검토 후) |
| skill | `/nf-impl-plan` |
| 입력 | `design/<nf>/architecture/**`, `module-decomposition/**`, `handoff/<nf>/contract.yaml` |
| AI agent | architecture 를 *구현 작업 단위* 로 변환 — implementation-plan (Scope/Phases/Test Plan/Open Risks/References), tasks.yaml (`impl-plan-v1`, 각 task `trace_to`), test-matrix (Test Inventory/Coverage Rules), traceability (Contract→Module / Module→Test). 소스 코드·build system 은 만들지 않음. |
| 사람 | 기술 결정 (`TBD` owner: dev) — 구현 언어·런타임·HTTP/SBI framework·persistence backend·telemetry stack·배포 토폴로지. PR 리뷰·머지. |
| 산출 | `dev/<nf>/{implementation-plan.md, tasks.yaml, test-matrix.md, traceability.md}` (git 추적). |

### 단계 H — impl-planning 자기 일관성 검증 (게이트, 하네스 설계 완료)

| 항목 | 내용 |
|---|---|
| 트리거 | "/nf-impl-status nssf" |
| skill | `/nf-impl-status` (read-only 측정) |
| script | `design/scripts/nf-impl-status.py` → `dev/<nf>/_impl_status.yaml` |
| AI agent | 필수 파일, canonical 섹션 exact-match, tasks.yaml schema + `trace_to`, frontmatter, traceability xref (advisory WARN) 검사. |
| 사람 | FAIL 시 `/nf-impl-plan` 으로 반영. `manual_overrides.pass_anyway` 동일 규칙. |
| gate | **`impl_consistent`** — Tier 1 binary AND. → PASS 시 impl-planning 검증 완료. 다음 = 단계 I. |

### 단계 I — Engineering Design Freeze (사람 소유 결정 freeze)

| 항목 | 내용 |
|---|---|
| 트리거 | "/nf-eng-design nssf" (impl-status 후) |
| skill | `/nf-eng-design` |
| 입력 | `design/<nf>/architecture/**` (특히 `decisions/ADR-0001-architecture-baseline.md` `## Open choices` = 연기 레지스터, **F-hard**), `design/schemas/engineering-core-slots.yaml` (13 slot v2 profile), `handoff/<nf>/contract.yaml` + contract data model/API/lifecycle, `dev/<nf>/` (soft 참조). |
| AI agent | F/G+contract 근거로 13 core slot + per-NF 연기 레지스터 결정을 *초안 자동생성* (persistence backend 후보·DB schema 초안 tables/columns/PK/indexes/constraints/migration_policy 포함). canonical 5 섹션 + `## Decisions` ```yaml 블록. |
| 사람 | 초안 검토 후 **각 결정·`explicitly_out_of_scope` ratify** (`ratified_by`/`date`). engineering decision 은 spec-derived 아님 — ratify 전 frozen 아님. PR 리뷰·머지. |
| 산출 | `engineering/<nf>/engineering-design.md` (git 추적, 사람 소유). |

### 단계 J — Engineering Design Freeze 검증 (게이트, 하네스 설계 완료)

| 항목 | 내용 |
|---|---|
| 트리거 | "/nf-eng-status nssf" |
| skill | `/nf-eng-status` (read-only 측정) |
| script | `design/scripts/nf-eng-status.py` → `engineering/<nf>/_engineering_status.yaml` |
| AI agent | profile ∪ 연기 레지스터 inventory 커버, slot typed shape (conditional 포함), 공통 필드·status·ratify·미결정표현 검사 (결정론 blocking). `advisory.impl_plan_alignment` 는 비차단. |
| 사람 | FAIL 시 `/nf-eng-design` 으로 반영·ratify. `manual_overrides.pass_anyway` 동일 규칙. |
| gate | **`eng_frozen`** — Tier 1 binary AND, 결정론. → PASS 시 **하네스 설계 완료 = 자율 코드 생성 GO**. 이후가 *AI agents 주도 자율 코드 생성* (프로젝트 최종 목표, 본 문서 절차 범위 밖 — 입력은 `eng_frozen` PASS). 상세 ADR-0002. |

---

## 2. 자율 코드 생성 *전* — 스킬 / 워크플로우 / 산출물 요약

| 단계 | skill (canonical / alias) | 주 script | 핵심 산출물 | 추적 | gate |
|---|---|---|---|---|---|
| A spec 준비 | (사람 전용) | — | `specs/<spec>/*` | git | — |
| B discovery | `/nf-spec-discover` (`/nf-init`) | nf-manifest.py, nf-seed-gen.py | `_manifest.yaml`, `_contract_seed.yaml` | 비추적 | manifest ready |
| C contract 생성 | `/nf-contract-build` (`/nf-build`) | resolve-yaml-refs.py, build-handoff.py, validate-extraction.py | `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml` | 비추적 | — |
| D contract 검증 | `/nf-contract-check` (`/nf-status`) | nf-status.py | `_contract_status.yaml` | 비추적 | **handoff_ready** |
| E arch 설계 | `/nf-arch-design` | (skill 내 생성) | `architecture/**`, `module-decomposition/**` | git | — |
| F arch 검증 | `/nf-arch-status` | nf-arch-status.py | `_arch_status.yaml` | 비추적 | **arch_consistent** |
| G impl-plan | `/nf-impl-plan` | (skill 내 생성) | `dev/<nf>/{plan,tasks,test-matrix,traceability}` | git | — |
| H impl 검증 | `/nf-impl-status` | nf-impl-status.py | `_impl_status.yaml` | 비추적 | **impl_consistent** |
| I eng-design | `/nf-eng-design` | (skill 내 생성) | `engineering/<nf>/engineering-design.md` | git | — |
| J eng 검증 | `/nf-eng-status` | nf-eng-status.py | `_engineering_status.yaml` | 비추적 | **eng_frozen** |

### 사람 vs AI agent 책임 경계

**사람만 하는 일.**
- 3GPP spec 원본 확보·`specs/` 투입 (단계 A).
- 정책·범위 결정 — `manual_overrides.exclude` (33.501/33.310/33.210 = ADR-0004 흡수, 38.413 AMF reallocation 운영 보류), scope (full vs 축소).
- 구현 기술 선택 — engineering-design.md 의 사람 ratify (언어·런타임·HTTP/TLS 라이브러리·persistence·telemetry·배포). ADR-0004 security baseline (TLS/mTLS/OAuth2 production-capable code path) 의무.
- PR 리뷰·머지 결정, `manual_overrides.pass_anyway` 명시 우회 결정.
- 단계 진행 결정 (skill 은 다음 단계 자동 호출 안 함).

**AI agent 가 하는 일.**
- 모든 skill 실행, 산출물 생성·검증·재생성.
- 의존 spec 자동 검출, contract 추출, canonical 섹션 산출, 자기 일관성 측정, `to_pass` 액션 제시.
- second-opinion 리뷰 사이클 (`/pane-send` → `/pane-compare`), plan/retro 작성.

### 거버넌스

- 비-trivial 작업 = plan (`docs/plans/`) → Phase 별 PR → retro (`docs/retros/`) → plan close. retro-only/plan-close 단독은 direct push 허용 (AGENTS.md §git).
- 스킬·스크립트·계약 변경은 PR + second-opinion 리뷰 후 머지. canonical 섹션은 `/nf-arch-design`·`/nf-impl-plan` SKILL.md mandate 와 status validator 상수가 동일 source (drift 주의).

---

## 3. 현재 상태 (NSSF, 2026-05-18)

- 단계 A~H 의 도구·스킬·게이트 모두 구축 완료. NSSF 는 contract → architecture → impl-planning baseline 확정 (PR #16/#17/#20), 4 단계 status validator 신설 (PR #21).
- NSSF dev harness 설계 = `dev/nssf/` + `impl_consistent` gate PASS 상태. **다음 = AI agents 주도 구현 단계** (본 문서 범위 밖) — 착수 전 짧은 implementation kickoff plan 으로 stack/runtime `TBD` 를 사람이 확정 권고.
