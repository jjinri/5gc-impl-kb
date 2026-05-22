---
name: nf-impl-status
description: 본 5gc-impl-kb 의 implementation-planning 산출 (dev/<nf>/) 이 canonical 섹션 계약·tasks schema·자기 일관성을 만족하는지 검사해 `_impl_status.yaml` 산출하는 워크플로우. 사용자가 "/nf-impl-status nssf", "NSSF dev 검증", "implementation plan 일관성 검사", "impl status", "tasks traceable 확인" 등을 말하거나 NF 이름 + dev 검사를 지정하면 이 skill 을 사용한다. 동작 — `design/scripts/nf-impl-status.py <nf>` 호출 → `dev/<nf>/_impl_status.yaml` 갱신 + 콘솔에 acceptance gate (`impl_consistent`) 보고. contract 단계는 `/nf-status`(=`/nf-contract-check`), architecture 단계는 `/nf-arch-status` 의 책임이며 본 skill 은 implementation-planning 단계 *측정* 에 집중한다.
argument-hint: "<nf> [--no-write]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-impl-status.py *) Bash(cat *) Bash(ls *)
---

# nf-impl-status — implementation-planning 자기 일관성 검사

implementation-planning 단계 status 검사다. ADR-0001 L54 가 예고한 "separate future skill" — contract 단계 `/nf-status`, architecture 단계 `/nf-arch-status` 와 책임이 분리된다.

## 입력
- `<nf>` — NF 이름.
- `--no-write` — `_impl_status.yaml` 저장 없이 stdout 만. 일회성 점검에 유용.

## 본 skill 의 책임 범위

본 skill 은 *측정* 만 한다. 어떤 산출도 *수정* 하지 않는다 — `_impl_status.yaml` 은 기계가 매번 새로 쓰는 보고서이며 사용자가 직접 편집하지 않는다.

| 시나리오 | 사용 skill |
| --- | --- |
| contract 완성도 검사 | `/nf-status` (= `/nf-contract-check`) |
| architecture 자기 일관성 검사 | `/nf-arch-status` |
| implementation-planning 자기 일관성 검사 | `/nf-impl-status` (본 skill) |
| implementation-planning 산출 생성·갱신 | `/nf-impl-plan` |

## 동작 원칙 (이유 포함)

- **CLAUDE.md 정책 + ADR-0001 lifecycle vocabulary 가 우선.**
- **read-only.** dev 산출을 수정하지 않는다. 생성·갱신은 `/nf-impl-plan` 의 책임.
- **discover 기반, 하드코딩 금지.** task/test id 를 박지 않고 `tasks.yaml`·`traceability.md` 에서 읽어 *관계만* 검사. 1 NF 표본 과적합 차단.
- **semantic judge 제외.** test coverage 품질·구현 기술 선택은 평가하지 않는다. 구조·schema·참조 존재까지만.
- **gate 는 binary check 의 AND, WARN 은 advisory.** `impl_consistent` = Tier 1 4개 AND. `traceability_xref` 는 Tier 2 advisory WARN — gate 비포함 (Phase 1). 깊은 id-relation 무결성은 Phase 2 별도 사이클.
- **모든 check 는 criterion + to_pass 의무.**
- **canonical 섹션은 SKILL.md mandate 와 동일 source.** script 의 `DEV_MD_CANON` 상수는 `nf-impl-plan/SKILL.md` mandate (PR #20 영구화) 와 1:1. 함께 갱신.

## 검사 항목

### Phase 1 — `impl_consistent` (legacy planning 4 파일)

| id | tier | PASS 정의 |
| --- | --- | --- |
| `impl_files_present` | 1 | `implementation-plan.md, tasks.yaml, test-matrix.md, traceability.md` 존재 |
| `impl_sections_exact` | 1 | implementation-plan 5 / test-matrix 5 / traceability 4 canonical 섹션 순서 일치 |
| `tasks_schema_valid` | 1 | `tasks.yaml` `schema_version: impl-plan-v1`, 각 task `id` + `trace_to` 비어있지 않음 |
| `impl_frontmatter_valid` | 1 | 3 `.md` frontmatter valid + 필수 키 (`nf, stage, status, source_architecture, source_contract, generated_date`) |
| `traceability_xref` | 2 | advisory WARN (gate 비포함) — 각 task id 가 `traceability.md` 에 등장 |

gate — `impl_consistent` = 위 Tier 1 4개 AND.

### `impl_ready_for_codegen` (PR C, 2026-05-21 plan §4.5)

Readiness Pack 9 파일이 autonomous codegen agent 가 원본 OpenAPI YAML 재독해 없이 구현 가능한 수준인지 검사.

| id | tier | PASS 정의 |
| --- | --- | --- |
| `readiness_pack_files_present` | 1 | Agent Execution Pack 5 + Human Review Pack 4 = 9 파일 모두 존재 |
| `api_matrix_covers_all_operations` | 1 | `api-implementation-matrix.md` `## Operation Matrix` 표가 `handoff/<nf>/contract.yaml` 의 `topics.api/<Op>` 키 전체 cover |
| `data_model_map_covers_all_contract_models` | 1 | `data-model-implementation-map.md` `## Schema Classification` 표가 `topics.data-model/<S>` 키 전체 cover |
| `all_work_items_have_expected_files` | 1 | `codegen-work-items.yaml` (`schema_version: codegen-work-items-v1`) `items[].expected_files` 모두 non-empty + TODO 아님 |
| `all_work_items_have_tests` | 1 | `items[].tests` 모두 non-empty + TODO 아님 |
| `all_work_items_have_verification_commands` | 1 | `items[].verification_commands` 모두 non-empty + TODO 아님 |
| `team_execution_plan_present` | 1 | `team-execution-plan.md` 5 lane (Orchestrator/Code/Reviewer/Tester/Verifier) + Integration Order + References 순서 일치 |
| `human_review_pack_traceable` | 1 | Human Review Pack 4 파일 존재 + frontmatter 필수 키 + 본문이 TODO/header 만은 아님 |
| `gaps_classified` | 1 | `open-gaps-and-assumptions.md` `## Gaps` 표 모든 행이 category 컬럼 ∈ {blocker, deferred, operator-provided, library-assumed, test-gap, assumption} |
| `blocker_gaps_zero` | 1 | 위 표의 `blocker` category 행 0개 |
| `no_spec_reread_required` | 1 | aggregate sentinel — `blocker_gaps_zero` PASS + `spec-to-design-coverage.md` `## Coverage Trace` 에 최소 1 non-TODO 행 + open-gaps/design-adequacy 본문에 "spec 재독해" 단서 부재 |

gate — `impl_ready_for_codegen` = 위 11개 AND.

본 gate 는 `impl_consistent` 와 *독립* 으로 평가된다. PR D 의 `readiness_pack_ready` aggregate gate (engineering 단계) 의 한 구성요소다.

## Workflow

### 1. 입력 검증
- `<nf>` 가 비어있으면 사용자에게 묻고 정지.
- `dev/<nf>/` 디렉터리 존재 확인. 없으면 "/nf-impl-plan <nf> 먼저" 안내 후 정지.

### 2. 실행
```bash
.venv/bin/python3 design/scripts/nf-impl-status.py <nf>
```
- `--no-write` 면 `_impl_status.yaml` 미저장, stdout 만.
- script 가 gate FAIL 이면 exit 1.

### 3. 보고
- `impl_consistent` gate PASS/FAIL.
- `impl_ready_for_codegen` gate PASS/FAIL.
- FAIL check 와 그 `to_pass` 액션.
- WARN (advisory) 는 비차단으로 별도 표시.
- 다음 추천 — 두 gate 모두 PASS 면 `/nf-eng-design <nf>` (이미 freeze 됐다면 PR D 의 `readiness_pack_ready` aggregate 검사 단계), 아니면 `/nf-impl-plan <nf>` 로 to_pass 반영.

## 자주 틀리는 지점
- `_impl_status.yaml` 을 손으로 편집 — 금지. 기계 재생성물. to_pass 를 따라 *산출* 을 고치고 재실행.
- 사용자 추가 섹션이 `impl_sections_exact` FAIL — *의도* (PR #20 구조 영구화). 진짜 우회는 `manual_overrides.pass_anyway` 에 사유 + 사용자 명시 결정.

## 참고 — 본 skill 안에 다시 적지 말 것
- lifecycle vocabulary: `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`.
- canonical 섹션 정의: `nf-impl-plan/SKILL.md` mandate.
- architecture 검사: 자매 skill `/nf-arch-status`.
