# Lifecycle Artifact Necessity / Reactivity Audit — 2026-05-26

Status: proposed (PR-1 deliverable for `2026-05-26-readiness-config-compiler-plan.md`)

Scope: 본 audit 은 `5gc-impl-kb` 의 lifecycle 산출물을 *config 변경에 반응해야 하는가 / 사람이 ratify 해야 하는가 / agent 가 소비하는가* 기준으로 분류한다. NSSF 한 NF 의 현 산출이 대상. 결과는 PR-2 (policy YAML 추출) 과 PR-3 (`readiness-config.yaml` skeleton) 의 *입력 목록* 이다.

## 1. Framework

### 1.1 다섯 질문

각 파일/산출물에 대해.

| 코드 | 질문 |
|---|---|
| Q1 | downstream agent/tool 이 이 파일을 *실제 소비* 하는가? |
| Q2 | `specs/`·`design/policies/*.yaml`·`design/<nf>/readiness-config.yaml`·`handoff/<nf>/contract.yaml` 로부터 *재생성 가능* 한가? |
| Q3 | config (readiness-config / policy / contract) 변경 시 *내용이 바뀌어야* 하는가? |
| Q4 | 사람이 *이 파일 자체* 를 ratify 해야 하는가, 아니면 *config 를 ratify* 하면 충분한가? |
| Q5 | 현재 누가 작성하는가 — `human-prose` / `script` / `hybrid` (AI draft + 사람 ratify)? |

### 1.2 분류 카테고리

| 카테고리 | 정의 | PR-2/3 처리 |
|---|---|---|
| `durable-source` | Q1=yes ∧ Q2=no ∧ Q4=파일 | source-of-truth 유지. 일부는 policy YAML / readiness-config 로 *promote*. |
| `tracked-generated-drift-target` | Q1=yes ∧ Q2=yes ∧ Q3=yes ∧ Q4=config | tracked 유지, `--check` drift target, `--write` 로 재생성. |
| `generated-report` | Q1=no(or audit only) ∧ Q2=yes | tracked 유지, audit/snapshot 가치. `--write` 로 재생성. |
| `remove-or-merge` | Q1=no ∧ audit 가치 낮음 | PR-2/3 시점에 제거 또는 다른 산출에 흡수. |
| `split-required` | 같은 파일이 여러 카테고리 행 포함 — 내부 분리 후 재분류 | PR-1.5 미니 작업 (block 단위 매핑). |

### 1.3 source-of-truth promotion 후보

`durable-source` 중 *기계 가독성* 이 필요하면 다음으로 promote.

- `design/policies/*.yaml` — project-wide normative (lifecycle / security-baseline / source-precedence / spec-dependencies / gaps).
- `design/schemas/*.yaml` — config shape / validator schema.
- `design/nf-registry.yaml` — NF 라우팅 + manual override.
- `design/<nf>/readiness-config.yaml` — NF 별 사람 ratify 구현/운영/security/phase/gap/test 결정.

## 2. Audit tables

`Q3=–` = config 변경이 *직접* 내용 변경 강제 안 함 (다른 입력에 의존).
`Q5=hybrid` = 현재 AI/script 가 draft, 사람이 row 별 ratify (engineering-design 의 ratified_by/date 패턴).

### 2.1 Project rationale + schema

| 파일 | Q1 | Q2 | Q3 | Q4 | Q5 | 카테고리 | 비고 |
|---|---|---|---|---|---|---|---|
| `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md` | yes | no | no | 파일 (rationale) | human-prose | `split-required` | normative (stage names / gate vocabulary / migration notes) → `design/policies/lifecycle.yaml` promote; 나머지 rationale + history 유지 |
| `docs/adr/ADR-0002-engineering-design-freeze.md` | yes | no | no | 파일 | human-prose | `split-required` | `eng_frozen` gate semantics + inventory contract → schema/policy 인용으로 축약; rationale 유지 |
| `docs/adr/ADR-0003-engineering-dependency-closure.md` | yes | no | no | 파일 | human-prose | `split-required` | `dependency_shape` 정의 → `engineering-core-slots.yaml` v2 와 중복. ADR 은 rationale 만 |
| `docs/adr/ADR-0004-project-security-baseline.md` | yes | no | no | 파일 | human-prose | `split-required` | 7 baseline 항목 + source precedence → `design/policies/security-baseline.yaml` promote; rationale 유지 |
| `design/nf-registry.yaml` | yes | partial (auto bootstrap) | no | 파일 (`manual_overrides`) | hybrid (bootstrap + 사람) | `durable-source` | `generated.*` block 은 script 산출, `manual_overrides.*` 만 사람 ratify. shape 안정. |
| `design/schemas/engineering-core-slots.yaml` | yes (validator) | no | no | 파일 | human-prose | `durable-source` | `readiness-config-v1` schema 의 *부분 schema*. PR-3 에서 흡수/연결. |

### 2.2 NSSF architecture (`design/nssf/architecture/`)

| 파일 | Q1 | Q2 | Q3 | Q4 | Q5 | 카테고리 | 비고 |
|---|---|---|---|---|---|---|---|
| `overview.md` | yes | partial | yes (language/runtime/persistence/HTTP stack 변경 시 본문 변경) | config | hybrid | `tracked-generated-drift-target` | language-neutral 부분은 contract 에서 render; 일부 prose 는 readiness-config 결정 인용 |
| `module-boundaries.md` | yes | partial | yes (persistence/HTTP 변경 시 모듈 경계 일부 갱신) | config | hybrid | `tracked-generated-drift-target` | 4 모듈 식별은 NSSF spec 강제 (contract-derived); HTTP/persistence 결정은 readiness-config 에서 render |
| `runtime-model.md` | yes | yes | yes (event loop / HTTP stack / TLS 변경 시) | config | hybrid | `tracked-generated-drift-target` | engineering-design `sbi_server_stack`/`runtime` slot + contract API 흐름에서 render |
| `request-flow.md` | yes | yes | yes (API 추가/삭제 시) | config | hybrid | `tracked-generated-drift-target` | contract `api/*` + security-baseline flow 에서 deterministic render |
| `state-persistence.md` | yes | yes | yes (`persistence` slot 변경 시 schema/layout 변경) | config | hybrid | `tracked-generated-drift-target` | `data-model/*` contract + readiness-config `persistence` 에서 render |
| `error-propagation.md` | yes | yes | yes (TLS/OAuth2 error 정책 변경 시) | config | hybrid | `tracked-generated-drift-target` | contract error model + security-baseline 에서 render |
| `configuration-strategy.md` | yes | yes | yes (config_management slot / operator_inputs 변경 시) | config | hybrid | `tracked-generated-drift-target` | readiness-config `operator_inputs` + `configuration_management` slot 에서 render |
| `observability.md` | yes | yes | yes (`telemetry` slot 변경 시) | config | hybrid | `tracked-generated-drift-target` | readiness-config `telemetry` slot 에서 render |
| `test-strategy.md` | yes | partial | yes (test framework / contract test 변경 시) | config | hybrid | `tracked-generated-drift-target` | readiness-config `test_build_tooling` + contract operations 에서 render |
| `decisions/ADR-0001-architecture-baseline.md` | yes | partial | yes (security/persistence/HTTP/spec include 변경 시 `## Open choices` 갱신) | config (YAML block) + 파일 (rationale) | hybrid | `split-required` | `## Open choices` YAML block (per-NF 연기 레지스터) → `readiness-config.yaml` 의 `deferred_decisions` 로 promote; rationale prose 유지 |

### 2.3 NSSF module decomposition (`design/nssf/module-decomposition/`)

| 파일 | Q1 | Q2 | Q3 | Q4 | Q5 | 카테고리 | 비고 |
|---|---|---|---|---|---|---|---|
| `AvailabilityEngine.md` | yes | yes | yes (`persistence`/HTTP 변경 시) | config | hybrid | `tracked-generated-drift-target` | module-boundaries + contract `api/NSSAIAvailability*` + data-model 에서 render |
| `SelectionEngine.md` | yes | yes | yes | config | hybrid | `tracked-generated-drift-target` | contract `api/NSSelectionGet` + data-model `SliceInfoFor*` 에서 render |
| `SubscriptionStore.md` | yes | yes | yes (`persistence` 변경 시 schema 변경) | config | hybrid | `tracked-generated-drift-target` | readiness-config `persistence` + contract subscription data 에서 render |
| `NotificationDispatcher.md` | yes | yes | yes (`sbi_client_stack`/outbound TLS 변경 시) | config | hybrid | `tracked-generated-drift-target` | readiness-config `sbi_client_stack` + contract subscription notification 에서 render |

### 2.4 NSSF engineering

| 파일 | Q1 | Q2 | Q3 | Q4 | Q5 | 카테고리 | 비고 |
|---|---|---|---|---|---|---|---|
| `engineering/nssf/engineering-design.md` | yes (eng-status validator + downstream agent) | yes (config + arch + contract) | yes (모든 slot 변경 시 행 갱신) | config | hybrid (AI draft + row 별 ratify) | `split-required` (즉시) → 이후 `tracked-generated-drift-target` | **`## Decisions` YAML block → `readiness-config.yaml` 의 *instance config* 로 promote**; markdown 본문은 audit render (eng-status validator 도 readiness-config 파싱으로 전환 필요) |

### 2.5 NSSF dev readiness pack (`dev/nssf/`)

| 파일 | Q1 | Q2 | Q3 | Q4 | Q5 | 카테고리 | 비고 |
|---|---|---|---|---|---|---|---|
| `api-implementation-matrix.md` | yes | yes | yes (API 추가/삭제 시) | config | hybrid | `tracked-generated-drift-target` | contract `api/*` 에서 render |
| `codegen-work-items.yaml` | yes (codegen agent) | yes | yes (language / HTTP / persistence / TLS / OAuth2 / phase 변경 시) | config | hybrid | `tracked-generated-drift-target` | readiness-config `implementation` + `phase_policy` 에서 render. **WI ID 안정성 규칙 필요** (OD-D) |
| `tasks.yaml` | yes (impl agent) | yes | yes | config | hybrid | `tracked-generated-drift-target` | codegen-work-items 의 task graph instance. ID 안정성 OD-D 적용 |
| `data-model-implementation-map.md` | yes | yes | yes (`persistence` / language 변경 시) | config | hybrid | `tracked-generated-drift-target` | contract `data-model/*` + persistence slot 에서 render |
| `implementation-plan.md` | yes | yes | yes (phase / arch 변경 시) | config | hybrid | `tracked-generated-drift-target` | readiness-config `phase_policy` + architecture 에서 render |
| `spec-to-design-coverage.md` | yes (coverage check) | yes | yes (spec include/exclude 변경 시) | config | hybrid | `tracked-generated-drift-target` | manifest + contract + architecture cross-ref 에서 render |
| `test-matrix.md` | yes (test agent) | yes | yes (API / test_build_tooling 변경 시) | config | hybrid | `tracked-generated-drift-target` | contract operations + readiness-config `test_build_tooling` 에서 render |
| `traceability.md` | yes | yes | yes (모든 입력 변경 시) | config | script | `tracked-generated-drift-target` | contract + arch + dev cross-ref. 가장 deterministic render 후보 — PR-4 generator 의 *첫 target* 권고 |
| `verification-plan.md` | yes | yes | yes (gate 정의 / status 변경 시) | config | hybrid | `tracked-generated-drift-target` | gates + status validator 정의 에서 render. 단 일부 prose 는 status snapshot 성격 |
| `team-execution-plan.md` | partial | yes | yes (`phase_policy` 변경 시) | config | hybrid | `tracked-generated-drift-target` | readiness-config `phase_policy` + roles 에서 render |
| `design-adequacy-checklist.md` | audit only | yes | partial | config | hybrid | `generated-report` | status snapshot. drift target 아니라 render-on-demand |
| `implementation-readiness-review.md` | audit only | yes | partial | config | hybrid | `generated-report` | "내용 품질 자동 승인하지 않음, GO/NO-GO 사람 책임" 명시 — audit view |
| `open-gaps-and-assumptions.md` | yes (impl agent + audit) | yes | yes (`gaps.yaml` policy + readiness-config `gaps` 변경 시) | config | hybrid | `split-required` | gap *분류 정책* (policy YAML) + NSSF-specific *gap 목록* (readiness-config or generated) 분리 필요 |

### 2.6 Status / cache (gitignored)

| 파일 | 카테고리 | 비고 |
|---|---|---|
| `design/nssf/_manifest.yaml` | `generated-cache` | nf-manifest.py 산출. shape 안정. |
| `design/nssf/_contract_seed.yaml` | `generated-cache` | nf-seed-gen.py 산출. |
| `design/nssf/_contract_status.yaml` | `generated-cache` | nf-status.py 산출. |
| `design/nssf/_arch_status.yaml` | `generated-cache` | nf-arch-status.py 산출. |
| `engineering/nssf/_engineering_status.yaml` | `generated-cache` | nf-eng-status.py 산출. **readiness-config 전환 시 validator 입력 변경 필요**. |
| `dev/nssf/_impl_status.yaml` | `generated-cache` | nf-impl-status.py 산출. |
| `dev/nssf/_readiness_status.yaml` | `generated-cache` | nf-readiness-status.py 산출. |
| `design/nssf/contract/**` | `generated-cache` (gitignored) | materialize-contract.py 산출. AUTO/USER marker 보존 — *부분 generated* 모범 사례. |
| `handoff/nssf/contract.yaml` | `generated-cache` (gitignored) | build-handoff.py 산출. |

## 3. Classification 요약

집계 범위: *tracked 파일만*. `generated-cache` 9 종은 gitignored 라 tracked 합계 외 별도 행. 같은 파일이 `split-required` 면 *primary 카테고리* (split 후 dominant block 의 카테고리) 가 아니라 `split-required` 로만 집계 (중복 없음).

| 카테고리 | tracked 파일 수 | 구성 |
|---|---|---|
| `durable-source` | 2 (project) | `design/nf-registry.yaml`, `design/schemas/engineering-core-slots.yaml` (+ 신설 예정 `design/policies/*.yaml`, `design/<nf>/readiness-config.yaml`) |
| `tracked-generated-drift-target` | 23 (NSSF) | arch 9 (`configuration-strategy`, `error-propagation`, `module-boundaries`, `observability`, `overview`, `request-flow`, `runtime-model`, `state-persistence`, `test-strategy` — `decisions/ADR-0001` 제외) + module 4 + dev 10 (`api-implementation-matrix`, `codegen-work-items.yaml`, `data-model-implementation-map`, `implementation-plan`, `spec-to-design-coverage`, `tasks.yaml`, `team-execution-plan`, `test-matrix`, `traceability`, `verification-plan`) |
| `generated-report` | 2 (NSSF) | `dev/nssf/design-adequacy-checklist.md`, `dev/nssf/implementation-readiness-review.md` |
| `remove-or-merge` | 0 | 본 audit 에서 *제거 권고 없음* — 모든 파일 audit 가치 ≥1 |
| `split-required` | 7 (project 4 + NSSF 3) | project: ADR-0001/0002/0003/0004; NSSF: `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md`, `engineering/nssf/engineering-design.md`, `dev/nssf/open-gaps-and-assumptions.md` |

**Tracked 합계** — project: 6 (durable-source 2 + split-required 4), NSSF: 28 (drift-target 23 + generated-report 2 + split-required 3). 합계 34.

**별도 — `generated-cache`** (gitignored, tracked 집계 외): 9 (`_manifest.yaml`, `_contract_seed.yaml`, `_contract_status.yaml`, `_arch_status.yaml`, `_engineering_status.yaml`, `_impl_status.yaml`, `_readiness_status.yaml`, `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml`).

## 4. Promotion plan (PR-2/3 입력)

`split-required` 파일의 *normative block* → 다음 source 로 promote.

| 출처 block | promote target | PR |
|---|---|---|
| ADR-0001 lifecycle stage / gate vocabulary | `design/policies/lifecycle.yaml` | PR-2 |
| ADR-0004 7 baseline 항목 + source precedence | `design/policies/security-baseline.yaml` | PR-2 |
| ADR-0004 spec dependency 정책 (33.501/33.310/33.210 absorbed) | `design/policies/spec-dependencies.yaml` | PR-2 |
| ADR-0002 `eng_frozen` gate inventory contract | `design/policies/lifecycle.yaml` (gate 절) + `engineering-core-slots.yaml` (shape) | PR-2 |
| ADR-0003 `dependency_shape` 정의 | `engineering-core-slots.yaml` v2 (이미 존재) — ADR 은 인용으로 축약 | PR-2 |
| `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices` YAML | `design/nssf/readiness-config.yaml` `deferred_decisions` block | PR-3 |
| `engineering/nssf/engineering-design.md` `## Decisions` YAML block | `design/nssf/readiness-config.yaml` (instance config — engineering-core-slots schema 준수) | PR-3 |
| `dev/nssf/open-gaps-and-assumptions.md` gap *category 정의* | `design/policies/gaps.yaml` | PR-2 |
| `dev/nssf/open-gaps-and-assumptions.md` NSSF-specific gap *목록* | `design/nssf/readiness-config.yaml` `gaps` block + generated section | PR-3 / PR-4 |

## 5. Open audit decisions (사람 ratify 필요)

본 audit 의 row 분류 중 추가 ratify 필요 항목.

1. `design/nssf/architecture/*.md` 의 일부 prose 가 *language-neutral arch* 인지 *config-render* 인지 row 별 재확인. 본 audit 은 전부 `tracked-generated-drift-target` 으로 잠정 분류했으나, generator 작성 시 *language-neutral 부분* 은 `durable-source` 후보일 수 있다.
2. `dev/nssf/team-execution-plan.md` 가 *render* 대상으로 충분한가, *사람 ratify* 가 필요한가. phase_policy 외 *팀 조직 결정* 이 들어있으면 split.
3. `design/nssf/module-decomposition/*.md` 의 *모듈 식별 자체* 는 NSSF spec 강제 (contract-derived) — 만약 모듈 식별이 *사람 결정* 이면 `durable-source` 부분 + render 부분 split 필요.
4. `_status.yaml` 9 종 중 *어느 것이 readiness-config 전환 후에도 동일 입력으로 동작* 하는지 — 특히 `_engineering_status.yaml` 는 markdown YAML 파싱에서 readiness-config 파싱으로 *전환 필요* (PR-3 의존).
5. `generated-cache` 와 `tracked-generated-drift-target` 의 *경계* — `contract/**` 와 `handoff/contract.yaml` 가 gitignored 인 현 정책 유지 vs tracked 로 승격해 drift target 으로. 현 audit 은 *gitignored 유지* 잠정 분류.

## 6. PR-1 산출 vs 다음 PR

본 PR-1 산출.

- 본 문서 (`docs/plans/2026-05-26-artifact-audit.md`).
- `docs/plans/2026-05-26-readiness-config-compiler-plan.md` 의 §1·§3·§7 patch (Phase 0 신설 + Phase A 분할 + layout 표 audit 결과 행 추가).

본 PR-1 *비포함*.

- `design/policies/*.yaml` 신설 (PR-2).
- `design/nssf/readiness-config.yaml` 신설 (PR-3).
- generator 구현 (PR-4+).
- ADR slimming (PR-D).
- 기존 tracked artifact 의 *내용 변경* (PR-4+ generator 적용 시).

## 7. 잔존 OD (audit 이 닫지 못함)

- OD-D: WI/task ID 안정성 규칙 — PR-4 단계.
- OD-F: `--check` CI 통합 시점 — PR-4 단계.
- OD-G: NF-agnostic schema 여부 — PR-3 단계.
- OD-H: validator script 의 ADR dead reference 정리 — PR-D 단계.
- OD-I: 기존 PR F1 generator 와의 정합 — PR-4 진입 전 확인.

## 8. Acceptance

- 28 NSSF tracked artifact + 6 project tracked artifact = **34 tracked 파일** 이 5 카테고리 (또는 `split-required`) 중 하나로 분류됨. 별도 `generated-cache` 9 종 (gitignored) 은 현 정책 유지.
- `split-required` 7 파일 (project 4 ADR + NSSF 3: arch-decisions ADR + engineering-design + open-gaps) 의 내부 block 별 promote target 이 §4 promotion plan 에 명시됨.
- 잔존 OD 5 개는 PR-2/3/4/D 단계로 명시 위임.
