# Lifecycle Artifact Guide

본 문서는 NF lifecycle 의 *각 단계에서 무엇이 만들어지고 누가 읽는지* 한눈에 보는 catalog 다.

- **Stage-by-stage** — skill 별 input·output·gate·reader 매트릭스.
- **File index** — 파일명으로 단계 역추적.
- **Reader perspective** — agent / human / orchestrator 별 읽기 순서.

관련 문서 — git 추적 정책은 [`artifact-management.md`](./artifact-management.md), KB 독해 가이드는 [`kb/README.md`](./kb/README.md), lifecycle 어휘는 [`adr/ADR-0001-nf-lifecycle-and-vocabulary.md`](./adr/ADR-0001-nf-lifecycle-and-vocabulary.md) 가 진실 출처.

---

## 1. Single-glance summary

| # | Stage | Skill / Script | Output class | Gate produced | Primary readers |
|---|---|---|---|---|---|
| 0 | Registry resolve | `/nf-readiness` Phase 0 (`nf-readiness-resolve.py`) | route resolution | confidence policy | wrapper |
| 1 | Spec discovery | `/nf-spec-discover` | local cache (yaml) | manifest ready | wrapper, contract build |
| 2 | Contract build | `/nf-contract-build` | local cache (md + yaml) | extraction basic | downstream skills |
| 3 | Contract check | `/nf-contract-check` | validation state (yaml) | `handoff_ready`, `contract_implementable` | wrapper |
| 4 | Architecture design | `/nf-arch-design` | reviewed KB (md) | — | agent, human |
| 5 | Architecture check | `/nf-arch-status` | validation state (yaml) | `arch_consistent` | wrapper |
| 6 | Implementation plan | `/nf-impl-plan` | reviewed KB (md + yaml) | — | agent, human |
| 7 | Implementation check | `/nf-impl-status` | validation state (yaml) | `impl_ready_for_codegen` | wrapper |
| 8 | Engineering design | `/nf-eng-design` | reviewed KB (md, human ratify) | — | agent, human |
| 9 | Engineering check | `/nf-eng-status` | validation state (yaml) | `eng_frozen` | wrapper |
| 10 | Readiness aggregate | `nf-readiness-status.py` | validation state (yaml) | `readiness_pack_ready` | wrapper, human |
| 11 | Implementation | `/nf-implement <nf>` (post-readiness) | source/test/CI | `tracer_bullet_passed` → `full_nf_done` | runtime |

**Output class** — 본 repo 의 [artifact-management.md §1](./artifact-management.md) 분류.
- *Local cache* — 비추적, 도구 재생성.
- *Reviewed KB* — 추적, PR review.
- *Validation state* — 비추적 gate evidence.

---

## 2. Per-stage detail

### Stage 0 — Registry resolve

```
/nf-readiness <nf>   # wrapper 의 Phase 0 자동 실행
```

NF 이름을 primary spec 으로 해석. registry 가 stale 이면 bootstrap 자동 실행.

| Input | Output | 추적 |
|---|---|---|
| `design/nf-registry.yaml` (catalog) | resolved effective spec (stdout YAML) | — |
| `specs/<spec>/` 디렉터리 존재 여부 | bootstrap 갱신 시 `design/nf-registry.yaml.generated.nfs.<nf>` | 추적 |

- **Confidence policy** — `generated.high` 또는 `manual_override.high|medium` 이면 proceed. 그 외 blocker.
- **사람 편집 영역** — `design/nf-registry.yaml.manual_overrides.nfs.<nf>` (bootstrap 가 덮어쓰지 않음).

### Stage 1 — Spec discovery

```
/nf-spec-discover <nf> --primary <spec> [--reset]
```

NF 매니페스트 + handoff seed 자동 생성.

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `design/<nf>/_manifest.yaml` | yaml | 비추적 | spec dependency 그래프 + ready_for_build status |
| `design/<nf>/_contract_seed.yaml` | yaml | 비추적 | categories / topics / tasks 정의 (contract build 의 단일 입력) |

- **Trigger 조건** — fresh NF, 새 ref spec cp 후 갱신, `--reset` 후 재시작.
- **--reset 효과** — `design/<nf>/contract/` + `_contract_status.yaml` + `handoff/<nf>/contract.yaml` 만 `_archive/<ts>/` 로 mv. manifest/seed/architecture/dev 는 보존.

### Stage 2 — Contract build

```
/nf-contract-build <nf> [--<category>] [--topic <id>]
```

OpenAPI yaml + docx 를 토픽 단위 markdown + machine-readable yaml 로 추출.

내부적으로 `materialize-contract.py` → `build-handoff.py` → `validate-extraction.py --level basic` 호출.

| Output | 형식 | 추적 | 목적 | Reader |
|---|---|---|---|---|
| `design/<nf>/contract/interface.md` | md + AUTO/USER markers | 비추적 | auth / transport block (OpenAPI metadata) + 사람 implementation notes | agent, human |
| `design/<nf>/contract/error-handling.md` | md + markers | 비추적 | operations × causes 매트릭스 | agent, human |
| `design/<nf>/contract/api/<Op>.md` | md per operation | 비추적 | method / path / params / request·response schema / security / source-line trace | agent, human |
| `design/<nf>/contract/data-model/<Schema>.md` | md per schema | 비추적 | field table / $ref chain trace / C type hint | agent, human |
| `design/<nf>/contract/data-model/<Schema>.json` | json per schema | 비추적 | resolved schema + complexity flags (codegen friendly) | codegen agent |
| `handoff/<nf>/contract.yaml` | yaml (handoff-v2) | 비추적 | self-contained NF contract — 다음 단계의 단일 진실 출처 | downstream skills, codegen |

- **AUTO marker** — 도구가 매 빌드 *덮어쓰기*. 사람 편집 무의미 (다음 빌드에서 사라짐).
- **USER marker** — 도구가 *위치만 보존*, 내용 보존. 사람 prose 영역.
- **Implementability mandate** — 본 stage 의 산출은 codegen agent 가 원본 spec 을 *의미 재발견* 용도로 다시 읽지 않아도 충분해야 한다. drift / source trace 용 read 만 허용.

### Stage 3 — Contract check

```
/nf-contract-check <nf>   # = nf-contract-check.py
```

Contract md + handoff yaml 의 정합성 + 구현 가능성 검사.

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `design/<nf>/_contract_status.yaml` | yaml | 비추적 | gate 결과 + 각 check 의 `current` / `to_pass` |

**Gates**:
- `draft` — frontmatter valid (minimal viability).
- `review_ready` — `draft` + sections complete + manifest ready.
- **`handoff_ready`** *(blocking)* — `review_ready` + data-model chain complete + api coverage + service flow coverage + handoff yaml valid + self-contained + ... .
- **`contract_implementable`** *(blocking)* — `handoff_ready` + schema implementable + api operation 6 키 mandate + problem details matrix complete + no spec re-read required + … .
- `canonical` — Tier 3 + Tier 4 (sub-agent judge) 통과.

### Stage 4 — Architecture design

```
/nf-arch-design <nf>
```

검증된 contract 를 9-area architecture + module decomposition md 로 변환.

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `design/<nf>/architecture/overview.md` | md (canonical sections) | **추적 KB** | NF 전체 모델 (Purpose / Inputs / Boundaries / Decisions / Open Questions / References) |
| `design/<nf>/architecture/module-boundaries.md` | md | 추적 | 모듈 경계 정의 |
| `design/<nf>/architecture/request-flow.md` | md | 추적 | inbound / outbound 호출 흐름 |
| `design/<nf>/architecture/runtime-model.md` | md | 추적 | thread / task model |
| `design/<nf>/architecture/state-persistence.md` | md | 추적 | 영속 / non-영속 상태 |
| `design/<nf>/architecture/configuration-strategy.md` | md | 추적 | config 분류 + 운영자 입력 |
| `design/<nf>/architecture/error-propagation.md` | md | 추적 | error class · 전파 경로 |
| `design/<nf>/architecture/observability.md` | md | 추적 | metric / log / trace |
| `design/<nf>/architecture/test-strategy.md` | md | 추적 | test 계층 전략 |
| `design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md` | md per-NF ADR | 추적 | NF-specific decisions + `## Open choices` (eng_frozen 의 per-NF 연기 레지스터) |
| `design/<nf>/module-decomposition/<Module>.md` | md per module | 추적 | 모듈별 responsibility / interface |

- **Canonical section 계약** — 모든 architecture md 는 `## Purpose` · `## Inputs (contract)` · `## Boundaries` · `## Decisions` · `## Open Questions` · `## References` 6 섹션 정확히 보유. `/nf-arch-status` 가 검사.

### Stage 5 — Architecture check

```
/nf-arch-status <nf>
```

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `design/<nf>/_arch_status.yaml` | yaml | 비추적 | gate `arch_consistent` 결과 |

**Gate `arch_consistent`** *(blocking)* — files present + canonical sections exact + module decomposition present + frontmatter valid + module section exact.

### Stage 6 — Implementation plan

```
/nf-impl-plan <nf>
```

Architecture + handoff yaml → 9-file readiness pack (`dev/<nf>/`).

| File | 형식 | 추적 | 분류 | 주 reader |
|---|---|---|---|---|
| `dev/<nf>/implementation-plan.md` | md | 추적 KB | overview | agent |
| `dev/<nf>/tasks.yaml` | yaml | 추적 KB | task graph (`impl-plan-v1`) | agent |
| `dev/<nf>/test-matrix.md` | md | 추적 KB | test inventory | tester, reviewer |
| `dev/<nf>/traceability.md` | md | 추적 KB | spec → contract → module → test 추적 | reviewer |
| `dev/<nf>/api-implementation-matrix.md` | md | 추적 KB | **Agent Execution Pack** — operation 단위 handler / model / security / persistence / test mapping | agent |
| `dev/<nf>/data-model-implementation-map.md` | md | 추적 KB | **Agent Execution Pack** — generated / wrapper / handwritten / deferred 분류 | agent |
| `dev/<nf>/codegen-work-items.yaml` | yaml | 추적 KB | **Agent Execution Pack** — work queue, expected files, tests, verification commands | orchestrator, code lane |
| `dev/<nf>/team-execution-plan.md` | md | 추적 KB | **Agent Execution Pack** — orchestrator / code / reviewer / tester / verifier lane 계약 | orchestrator, all lanes |
| `dev/<nf>/verification-plan.md` | md | 추적 KB | **Agent Execution Pack** — unit / integration / contract / security / e2e / observability gate (PR-17b 의 AUTO non-empty enforce 대상) | tester, verifier |
| `dev/<nf>/implementation-readiness-review.md` | md | 추적 KB | **Human Review Pack** — executive GO / NO-GO summary | human reviewer |
| `dev/<nf>/design-adequacy-checklist.md` | md | 추적 KB | **Human Review Pack** — reviewer checklist | human reviewer |
| `dev/<nf>/spec-to-design-coverage.md` | md | 추적 KB | **Human Review Pack** — spec re-read 금지 audit | human reviewer, verifier |
| `dev/<nf>/open-gaps-and-assumptions.md` | md | 추적 KB | **Human Review Pack** — blocker / deferred / operator / library / test / assumption 분류 | all lanes |

### Stage 7 — Implementation check

```
/nf-impl-status <nf>
```

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `dev/<nf>/_impl_status.yaml` | yaml | 비추적 | gate 결과 |

**Gates**:
- **`impl_consistent`** *(blocking)* — files present + sections exact + tasks schema valid + frontmatter valid + traceability xref.
- **`impl_ready_for_codegen`** *(blocking)* — `impl_consistent` + api matrix covers all operations + data-model map covers all contract models + all work items have files/tests/verification + verification-plan AUTO 6 sub-section 모두 non-empty + security-gate-matrix row 수 정합 + no spec re-read + … (14 checks).

### Stage 8 — Engineering design

```
/nf-eng-design <nf>   # AI 가 초안 자동 생성, 사람 ratify 필수
```

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `engineering/<nf>/engineering-design.md` | md (사람 소유, frozen 결정 ratify) | **추적 KB** | library / DB / runtime / tool / operator-policy 결정 — `design/schemas/engineering-core-slots.yaml` v2 의 13 core slot 충족 |

- **자동 생성과 ratify 분리** — AI 가 architecture + contract 근거로 *초안* 작성, 사람이 `ratified_by` + `date` 필드 채워 freeze.
- **Inventory** = `engineering-core-slots.yaml` 의 core slot ∪ per-NF 연기 레지스터 (`design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices`).

### Stage 9 — Engineering check

```
/nf-eng-status <nf>
```

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `engineering/<nf>/_engineering_status.yaml` | yaml | 비추적 | `gates.eng_frozen` + 비차단 `advisory.impl_plan_alignment` |

**Gate `eng_frozen`** *(blocking)* — typed schema 검사 (slot 별 dependency_shape / required_shape / common_row_fields). LLM judge 없음, deterministic.

### Stage 10 — Readiness aggregate

```
nf-readiness-status.py <nf>
```

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `dev/<nf>/_readiness_status.yaml` | yaml | 비추적 | aggregate gate 결과 |

**Gate `readiness_pack_ready`** *(blocking aggregate)* —

```text
handoff_ready ∧ contract_implementable ∧ arch_consistent
              ∧ impl_ready_for_codegen ∧ eng_frozen
```

`/nf-implement <nf>` 의 *유일한 GO 신호*.

### Stage 11 — Implementation (post-readiness)

```
/nf-implement <nf>
```

`readiness_pack_ready PASS` 확인 후 Phase 1 tracer-bullet → Phase 5 hardening.

| Output | 형식 | 추적 | 목적 |
|---|---|---|---|
| `src/<nf>/*` | source code | 추적 | NF 구현 본체 |
| `src/<nf>/generated/*` | generated stubs | 추적 | OpenAPI generator 산출 |
| `tests/<nf>/*` | test code | 추적 | unit / integration / contract / e2e / observability |
| `.github/workflows/*` | CI updates | 추적 | NF-specific build/test |
| `dev/<nf>/_implementation_run_state.yaml` | yaml | 비추적 | resume/checkpoint state |

---

## 3. File index — alphabetical

| File path | Stage | Type | Tracked | Primary reader |
|---|---|---|---|---|
| `design/<nf>/_arch_status.yaml` | 5 | validation | — | wrapper |
| `design/<nf>/_contract_seed.yaml` | 1 | local cache | — | contract build |
| `design/<nf>/_contract_status.yaml` | 3 | validation | — | wrapper |
| `design/<nf>/_manifest.yaml` | 1 | local cache | — | wrapper, contract build |
| `design/<nf>/architecture/configuration-strategy.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md` | 4 | reviewed KB | ✓ | agent, human, eng-design |
| `design/<nf>/architecture/error-propagation.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/architecture/module-boundaries.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/architecture/observability.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/architecture/overview.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/architecture/request-flow.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/architecture/runtime-model.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/architecture/state-persistence.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/architecture/test-strategy.md` | 4 | reviewed KB | ✓ | agent, human |
| `design/<nf>/contract/api/<Op>.md` | 2 | local cache | — | agent, human |
| `design/<nf>/contract/data-model/<Schema>.json` | 2 | local cache | — | codegen |
| `design/<nf>/contract/data-model/<Schema>.md` | 2 | local cache | — | agent, human |
| `design/<nf>/contract/error-handling.md` | 2 | local cache | — | agent, human |
| `design/<nf>/contract/interface.md` | 2 | local cache | — | agent, human |
| `design/<nf>/module-decomposition/<Module>.md` | 4 | reviewed KB | ✓ | agent |
| `design/nf-registry.yaml` | 0 | registry | ✓ | wrapper, bootstrap |
| `dev/<nf>/_impl_status.yaml` | 7 | validation | — | wrapper |
| `dev/<nf>/_implementation_run_state.yaml` | 11 | runtime state | — | `/nf-implement` |
| `dev/<nf>/_readiness_status.yaml` | 10 | validation | — | wrapper, human |
| `dev/<nf>/api-implementation-matrix.md` | 6 | reviewed KB | ✓ | agent (Agent Execution Pack) |
| `dev/<nf>/codegen-work-items.yaml` | 6 | reviewed KB | ✓ | orchestrator (Agent Execution Pack) |
| `dev/<nf>/data-model-implementation-map.md` | 6 | reviewed KB | ✓ | agent (Agent Execution Pack) |
| `dev/<nf>/design-adequacy-checklist.md` | 6 | reviewed KB | ✓ | human reviewer (Human Review Pack) |
| `dev/<nf>/implementation-plan.md` | 6 | reviewed KB | ✓ | agent |
| `dev/<nf>/implementation-readiness-review.md` | 6 | reviewed KB | ✓ | human reviewer (Human Review Pack) |
| `dev/<nf>/open-gaps-and-assumptions.md` | 6 | reviewed KB | ✓ | all lanes (Human Review Pack) |
| `dev/<nf>/spec-to-design-coverage.md` | 6 | reviewed KB | ✓ | reviewer, verifier (Human Review Pack) |
| `dev/<nf>/tasks.yaml` | 6 | reviewed KB | ✓ | agent |
| `dev/<nf>/team-execution-plan.md` | 6 | reviewed KB | ✓ | orchestrator (Agent Execution Pack) |
| `dev/<nf>/test-matrix.md` | 6 | reviewed KB | ✓ | tester, reviewer |
| `dev/<nf>/traceability.md` | 6 | reviewed KB | ✓ | reviewer |
| `dev/<nf>/verification-plan.md` | 6 | reviewed KB | ✓ | tester, verifier (Agent Execution Pack) |
| `engineering/<nf>/_engineering_status.yaml` | 9 | validation | — | wrapper |
| `engineering/<nf>/engineering-design.md` | 8 | reviewed KB | ✓ | agent, human (사람 ratify 필수) |
| `handoff/<nf>/contract.yaml` | 2 | local cache | — | downstream skills, codegen |
| `specs/<spec>/*.docx` | input | source input | ✓ | spec discovery, contract build |
| `specs/<spec>/*.yaml` | input | source input | ✓ | spec discovery, contract build |

---

## 4. Reader perspective

### 4.1 AI implementation agent (`/nf-implement` Phase 1~5)

```text
1. dev/<nf>/codegen-work-items.yaml        # phase / wave / work item queue
2. dev/<nf>/api-implementation-matrix.md   # operation → handler / model / security / test
3. dev/<nf>/data-model-implementation-map.md  # generated / wrapper / handwritten 분류
4. engineering/<nf>/engineering-design.md  # library / DB / runtime / operator boundary
5. design/<nf>/architecture/**             # flow / state / error / observability
6. design/<nf>/module-decomposition/**     # 모듈별 responsibility
7. dev/<nf>/verification-plan.md           # gate 별 evidence 요구사항
8. handoff/<nf>/contract.yaml              # canonical API/data-model schema (drift trace 용)
9. dev/<nf>/open-gaps-and-assumptions.md   # blocker 0 확인, non-blocker 조건
```

**금지**.
- 원본 OpenAPI/spec 을 *의미 재발견* 용도로 다시 읽기 (drift / source trace / generator input 만 허용).
- readiness pack 의 `category: blocker` gap 무시.
- TLS / X.509 / JWT primitive 직접 구현 (ADR-0004 mandate M6).
- `_*_status.yaml` 을 source of truth 로 편집.

**Stop 조건**.
- `category: blocker` 발견.
- "spec reread needed = yes" 표시.
- work item 의 input 누락.

### 4.2 Human reviewer (PR review / pre-implementation audit)

```text
1. dev/<nf>/implementation-readiness-review.md  # executive GO / NO-GO + risk
2. dev/<nf>/design-adequacy-checklist.md        # 설계 적정성 체크리스트
3. dev/<nf>/spec-to-design-coverage.md          # spec 재독 금지 audit
4. dev/<nf>/open-gaps-and-assumptions.md        # blocker / non-blocker 분류
5. engineering/<nf>/engineering-design.md       # library/DB/security 결정
6. design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md  # Open choices ratify
7. design/<nf>/architecture/**                  # 9-area 의사결정 검토
8. docs/adr/ADR-0004-project-security-baseline.md  # security 의무 검증
```

**Gate evidence 확인** — `_*_status.yaml` 은 *참고용*. 본 review 의 GO/NO-GO 판단은 사람 책임.

### 4.3 Lifecycle orchestrator (`/nf-readiness` wrapper)

읽기만 함, *모든 KB 산출을 수정 안 함*.

```text
1. design/nf-registry.yaml                 # Phase 0 — NF route
2. design/<nf>/_manifest.yaml              # Step 1 후 ready_for_build 확인
3. design/<nf>/_contract_status.yaml       # Step 3 gate
4. design/<nf>/_arch_status.yaml           # Step 5 gate
5. dev/<nf>/_impl_status.yaml              # Step 7 gate
6. engineering/<nf>/_engineering_status.yaml  # Step 9 gate
7. dev/<nf>/_readiness_status.yaml         # Step 10 aggregate gate
```

각 step 의 `blocked_by[]` 첫 check 의 `to_pass` 가 사용자에게 보고되는 *다음 행동*.

---

## 5. Tracking policy quick reference

본 가이드의 *추적/비추적* 분류 요약. 상세 정책은 [`artifact-management.md`](./artifact-management.md) 참조.

| 추적 (git tracked) | 비추적 (local regen) |
|---|---|
| `specs/<spec>/*` — 원본 입력 | `design/<nf>/_manifest.yaml` |
| `design/nf-registry.yaml` | `design/<nf>/_contract_seed.yaml` |
| `design/<nf>/architecture/**` | `design/<nf>/_contract_status.yaml` |
| `design/<nf>/module-decomposition/**` | `design/<nf>/_arch_status.yaml` |
| `engineering/<nf>/engineering-design.md` | `design/<nf>/contract/**` |
| `dev/<nf>/<readiness pack md/yaml>` | `handoff/<nf>/contract.yaml` |
| | `dev/<nf>/_impl_status.yaml` |
| | `dev/<nf>/_readiness_status.yaml` |
| | `dev/<nf>/_implementation_run_state.yaml` |
| | `engineering/<nf>/_engineering_status.yaml` |

---

## 6. Failure → next action

각 stage 의 status yaml 은 *FAIL 시 정확히 다음 행동* 을 `to_pass` 필드로 제시한다. 일반 패턴.

| Stage / gate FAIL | 일반 to_pass |
|---|---|
| Stage 1 manifest_ready=false | `specs/<spec>/` 에 누락 ref spec cp 후 `/nf-spec-discover` 재호출 |
| Stage 3 `handoff_ready` blocked | `(참조 규격 미등록)` leaf 해소 (manual_overrides 추가 또는 spec cp), `/nf-contract-build` 재실행 |
| Stage 5 `arch_consistent` blocked | canonical section 누락·module decomposition missing 보강 후 `/nf-arch-status` 재실행 |
| Stage 7 `impl_ready_for_codegen` blocked | readiness pack 의 누락 work item / verification-plan AUTO non-empty 보강 |
| Stage 9 `eng_frozen` blocked | engineering-design.md 의 미결정 slot ratify (사람 `ratified_by` / `date` 추가) |
| Stage 10 `readiness_pack_ready` blocked | 위 sub-gate 중 FAIL 항목의 to_pass 따라가기 |
