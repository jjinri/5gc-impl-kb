# Readiness Config Compiler 전환 Plan — 2026-05-26

Status: proposed (PR-1 in progress)
Current objective: `/nf-readiness` 와 선행 lifecycle skill 을 one-shot 문서 생성기에서 deterministic compiler + drift checker 로 전환한다. 사람이 spec 을 준비한 뒤 선행 ratify 해야 하는 구현·운영·정책 결정은 generated markdown 이 아니라 machine-readable config/policy 에서 확정하고, AI agent 는 그 입력으로 `engineering-design.md`, architecture alignment, `dev/<nf>/` readiness pack 을 재생성한다.

Companion: [`2026-05-26-artifact-audit.md`](./2026-05-26-artifact-audit.md) — PR-1 산출 *artifact necessity/reactivity audit* 표. 본 plan 의 Phase 0 결과를 굳히는 문서이며, PR-2/3 입력 목록의 source. 본 plan 과 함께 read.

## 1. Problem statement

현재 lifecycle 은 다음 drift 를 만든다.

- `dev/nssf/` readiness pack 이 한 번 생성된 고정 문서처럼 보인다.
- `/nf-readiness` 또는 하위 skill 의 반복 실행 가치가 약해진다.
- `engineering/nssf/engineering-design.md` 같은 중간 산출물이 사람 ratify contract 로 굳어져, generated artifact 와 source-of-truth 경계가 흐려진다.
- ADR prose 를 agent 가 읽고 추론해야 하는 구조는 결정론적 재생성·검증에 취약하다.
- clean checkout + specs 준비 상태에서 현재 수준의 readiness pack 을 자동 복원할 수 없다.

따라서 readiness pack 과 engineering-design 은 tracked 될 수는 있어도 **source** 가 아니라 **tracked generated output + drift target** 이어야 한다.

### 1.1 Pane 2 실증 확인 (2026-05-26)

Pane 2 second-opinion 이 현 lifecycle/skill 코드 인용으로 다음을 확정.

- `/nf-readiness` skill 은 *순차 orchestrator + read-only status aggregator* — deterministic compiler/checker 아님.
- contract layer (`materialize-contract.py`) 만 *부분적 generated artifact* 가치 보유 (AUTO/USER marker, USER 보존, AUTO 재생성). 다른 단계 generator 의 *reference pattern*.
- `engineering-design.md`, `design/<nf>/architecture/**`, `dev/<nf>/**` 는 사실상 *tracked fixed contract* 로 굳어짐.

따라서 본 plan 은 *어떤 산출이 fixed-contract→generated 로 demote 되는지* 를 audit 으로 확정한 후에야 policy YAML / generator 작업이 의미를 가진다. 그 audit 이 **Phase 0** 이며 산출은 `2026-05-26-artifact-audit.md`.

## 2. Target principle

```text
ADR is rationale.
Policy/config is law.
Schema/validator enforces law.
Generated docs explain law.
```

사람 입력은 다음 두 종류로 제한한다.

1. 원본 spec 준비: `specs/<spec>/*.docx|*.yaml`.
2. 선행 config ratify: NF 구현 정책, 운영 입력, security/profile 선택, phase/gap/test policy.

AI/lifecycle tool 은 위 입력을 읽어 다음을 재생성한다.

- `engineering/<nf>/engineering-design.md`
- `design/<nf>/architecture/**` 중 config/policy alignment 가 필요한 산출
- `dev/<nf>/` implementation plan + readiness pack
- status/drift report

## 3. Proposed source-of-truth layout

초기 목표는 최소 파일 수로 시작하고, 중복 source 를 만들지 않는 것이다. *current* 열은 본 plan 시작 시점 상태, *target* 열은 Phase D 완료 후 상태. transition path 는 audit doc §4 (promotion plan) 참조.

| Layer | File | Role | Current | Target |
|---|---|---|---|---|
| Project rationale | `docs/adr/ADR-*.md` | 왜 이 정책을 채택했는지, rejected alternatives, migration note. normative block 은 policy YAML 로 promote 후 인용 축약 | tracked (normative + rationale mixed) | tracked (rationale only) |
| Project policy | `design/policies/*.yaml` | agent/validator 가 읽는 project-wide law (lifecycle / security-baseline / source-precedence / spec-dependencies / gaps) | absent | tracked durable-source |
| Schema/profile | `design/schemas/*.yaml` | config shape, gate/profile validation. `engineering-core-slots.yaml` 이 readiness-config-v1 의 *부분 schema* | tracked (engineering-core-slots only) | tracked durable-source (+ policies-v1, nf-readiness-config-v1) |
| NF routing | `design/nf-registry.yaml` | NF key, primary spec, durable spec include/exclude override | tracked durable-source | unchanged |
| NF implementation decisions | `design/<nf>/readiness-config.yaml` | 사람 ratify 대상 구현·운영·phase·gap·test policy + per-NF 연기 레지스터 | absent | tracked durable-source |
| Rendered discovery | `design/<nf>/_manifest.yaml`, `_contract_seed.yaml` | registry/spec scan 결과 | gitignored generated-cache | unchanged |
| Generated architecture | `design/<nf>/architecture/**`, `module-decomposition/**` | spec/contract/readiness-config 에서 deterministic render. 일부 prose 의 language-neutral 부분 split 여부 audit 미해결 항목 | tracked (fixed-contract) | tracked-generated-drift-target |
| Generated engineering | `engineering/<nf>/engineering-design.md` | 사람이 읽는 render; `## Decisions` YAML 은 readiness-config 로 promote, markdown 은 audit view | tracked (fixed-contract) | tracked-generated-drift-target |
| Generated readiness pack | `dev/<nf>/**` (`.md` + 일부 `.yaml`) | implementation KB render; drift target. `design-adequacy-checklist`, `implementation-readiness-review` 는 generated-report | tracked (fixed-contract) | tracked-generated-drift-target (+ generated-report) |
| Status cache | `_*_status.yaml` | validator output | gitignored generated-cache | unchanged |
| Contract materialization | `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml` | spec+seed → contract render (AUTO/USER marker) | gitignored generated-cache | unchanged (generator reference pattern) |

분류 세부와 카테고리 정의는 [`2026-05-26-artifact-audit.md`](./2026-05-26-artifact-audit.md) 참조.

## 4. Config/policy candidates

### 4.1 Project policies

Add small machine-readable policy files before rewriting generated docs.

```text
design/policies/lifecycle.yaml
design/policies/security-baseline.yaml
design/policies/source-precedence.yaml
design/policies/spec-dependencies.yaml
design/policies/gaps.yaml
```

Initial content should be extracted from existing ADRs without changing behavior.

| Current ADR content | New machine-readable target |
|---|---|
| lifecycle stage names / gate vocabulary | `lifecycle.yaml` |
| source precedence | `source-precedence.yaml` |
| ADR-0004 TLS/mTLS/OAuth2 baseline | `security-baseline.yaml` |
| 33.501/33.310/33.210 absorbed/excluded policy | `spec-dependencies.yaml` |
| gap category enum | `gaps.yaml` |

ADR remains as rationale/history and links to the policy file.

### 4.2 NF readiness config

Add `design/<nf>/readiness-config.yaml` as the primary human-ratified config surface.

Starter shape:

```yaml
schema_version: nf-readiness-config-v1
nf: nssf
ratified_by: jjinri
ratified_date: 2026-05-26

specs:
  primary: "29.531"
  openapi:
    - specs/29.531/TS29531_Nnssf_NSSelection.yaml
    - specs/29.531/TS29531_Nnssf_NSSAIAvailability.yaml
  absorbed_security_specs:
    - "33.501"
    - "33.310"
    - "33.210"

implementation:
  language: c
  runtime: single_native_binary
  event_loop: epoll
  http2_server: nghttp2
  http2_client: nghttp2
  tls_provider: openssl
  oauth2_library: libjwt
  persistence: postgresql
  db_driver: libpq
  schema_codegen: openapi-generator-c
  json_library: cjson

security:
  baseline_profile: project-default
  dev_disable_allowed: true
  production_capable_required: true

operator_inputs:
  required:
    - tls_cert
    - postgres_dsn
    - jwks_endpoint

phase_policy:
  tracer_bullet_operation: NSSelectionGet
  phases:
    phase1:
      - WI-codegen-bootstrap
      - WI-schema-bootstrap
      - WI-tls-bootstrap
      - WI-problem-details-wrapper
      - WI-nftype-wrapper
      - WI-availability-repo
      - WI-selection-engine
      - WI-nsselection-handler

gaps:
  categories:
    - blocker
    - deferred
    - operator-provided
    - library-assumed
    - test-gap
    - assumption
```

This config is the place where the human decides what is currently embedded in `engineering-design.md` prose.

## 5. Lifecycle behavior after transition

Target command semantics:

```text
/nf-readiness nssf
  1. resolve design/nf-registry.yaml
  2. validate design/nssf/readiness-config.yaml against schemas/policies
  3. regenerate local discovery/cache: manifest, seed, contract, handoff, status
  4. render/check engineering-design.md from config + policy + architecture inputs
  5. render/check dev/nssf readiness pack from config + contract + architecture + engineering render
  6. run status validators
  7. report drift + readiness_pack_ready PASS/FAIL
```

Required CLI/tool split:

```text
design/scripts/nf-readiness-pack-generate.py <nf> --check
design/scripts/nf-readiness-pack-generate.py <nf> --write
```

- `--check`: render to temp, compare with tracked generated output, fail on drift.
- `--write`: update generated output deterministically.
- No timestamp noise. If dates are needed, use config ratified date or normalized date.

## 6. ADR simplification rule

Do not delete historical ADRs in this phase. Instead:

1. Extract normative rules to YAML policy/schema.
2. Replace long normative prose with a short pointer: “Machine-readable source: ...”.
3. Keep only:
   - decision summary,
   - rationale,
   - rejected alternatives,
   - migration notes / historical context,
   - references.
4. Move PR-specific progress, Pane 2 transcript summaries, and NSSF-only temporary state to retros/plans if still needed.

ADR text must not be the primary input for agent inference after this transition.

## 7. Execution phases

### Phase 0 — artifact necessity / reactivity audit (PR-1)

- 모든 tracked lifecycle artifact 에 5 질문 (downstream 소비? / 재생성 가능? / config-reactive? / 파일 vs config ratify? / 현재 작성자) 적용.
- 4 카테고리 (`durable-source` / `tracked-generated-drift-target` / `generated-report` / `remove-or-merge`) + `split-required` 분류.
- 산출: [`2026-05-26-artifact-audit.md`](./2026-05-26-artifact-audit.md).

Exit condition: 28 NSSF tracked + 6 project tracked artifact (= 34 파일) 분류 완료, `split-required` 7 파일의 block 별 promote target 명시. `generated-cache` 9 종 (gitignored) 은 현 정책 유지.

### Phase A — decision → config key inventory (PR-2 입력)

Phase 0 에서 `split-required` 또는 `durable-source` 로 분류된 파일의 *내부 block* 을 PR-2/3 source 로 매핑.

- A1: ADR normative block → `design/policies/*.yaml` key 매핑.
- A2: `engineering/nssf/engineering-design.md` `## Decisions` row → `design/nssf/readiness-config.yaml` key 매핑.
- A3: `dev/nssf/` 의 결정성 prose → `readiness-config` key vs `policies/*.yaml` default vs contract/architecture-derived fact vs generated prose 로 attribution.

Exit condition: inventory table identifies a source for every current generated decision (Phase A2 산출 — `readiness-config.yaml` skeleton 의 seed 가 됨).

### Phase B — introduce config and policies without behavior change (PR-2, PR-3)

- PR-2: Add `design/policies/*.yaml` for lifecycle/security/source-precedence/spec-dependencies/gaps. Schema 신설 (`design/schemas/policies-v1.yaml`). validator wiring 없음 (advisory only).
- PR-3: Add `design/nssf/readiness-config.yaml` seeded from Phase A2 inventory + `design/schemas/nf-readiness-config-v1.yaml` (engineering-core-slots 흡수). NSSF 한 NF 만.
- Phase B 의 schema 검사는 **WARN-only / advisory** — blocking gate 영향 0. blocking 승격은 Phase C generator + golden file 정합 확인 후.
- Do not rewrite generated docs yet except references if necessary.

Exit condition: existing validators still PASS, new policy/config 가 schema 검증 통과, 기존 산출 변경 없음.

### Phase B.5 — NSSF-only end-to-end dry run (선택)

- PR-3 후, PR-4 generator 본 작업 전, NSSF 한 NF 만 readiness-config → traceability.md (가장 deterministic) 단일 파일 render dry run.
- 정합 확인 후 Phase C 본 작업.

### Phase C — generator/checker implementation (PR-4+)

- Implement `nf-readiness-pack-generate.py` with `--check` and `--write`.
- Reference pattern: `design/scripts/materialize-contract.py` AUTO/USER marker — *부분 generated* 모범 사례. 확장 적용.
- First target: `dev/<nf>/traceability.md` (가장 deterministic, contract+arch+dev cross-ref 만).
- Expand until all `engineering/<nf>/engineering-design.md`, `design/<nf>/architecture/**`, `design/<nf>/module-decomposition/**`, `dev/<nf>/` readiness pack 이 deterministic render.
- `_engineering_status.yaml` validator 입력을 markdown YAML 파싱 → `readiness-config.yaml` 파싱으로 전환.
- Phase B 의 advisory schema 검사를 *blocking* 으로 승격.

Exit condition: `--check` passes on current tracked artifacts, golden file 정합 확인.

### Phase D — ADR slimming and lifecycle skill rewrite

- Update ADRs to point to machine-readable policy/config as normative source.
- Update `/nf-readiness` skill docs to call generator/checker rather than authoring fixed prose.
- Update docs/kb deterministic readiness note to reference the new source layout.

Exit condition: clean checkout + specs + config can regenerate artifacts without semantic drift.

### Phase E — clean rebuild proof

**안전망**: 본 단계는 *backup 브랜치 또는 worktree* 에서만 실행. main 또는 진행 PR 브랜치에서 직접 실행 금지.

```bash
git switch -c proof/readiness-rebuild-$(date +%Y%m%d)
rm -rf engineering/nssf dev/nssf design/nssf/architecture design/nssf/module-decomposition
/nf-readiness nssf --write
/nf-readiness nssf --check
git diff main -- engineering/ dev/ design/nssf/architecture/ design/nssf/module-decomposition/
```

Expected result: generated files match the tracked intended output, validators PASS, `readiness_pack_ready` PASS/FAIL is deterministic, diff = 0 (또는 의도된 normalized 차이만).

## 8. Open decisions

### 8.1 닫힌 결정 (Pane 1+2 합의)

1. Generated `engineering/<nf>/engineering-design.md` remain tracked. — Yes, generated drift target.
2. Generated `dev/<nf>/` readiness pack remain tracked. — Yes, generated drift target (일부는 generated-report).
3. `/nf-readiness` default — `--check`; explicit `--write` for updates.
4. Combined vs split `readiness-config.yaml` — combined initially; split when schema unwieldy.
5. ADR slim + generator 동일 PR — No. PR-1 (audit) → PR-2 (policies) → PR-3 (readiness-config) → PR-4 (generator) → PR-D (ADR slim).
6. PR-1 = policy YAML 추출 vs artifact audit — audit. Pane 2 sharpened 권고 채택 (audit-first).
7. engineering-core-slots ↔ readiness-config schema 관계 — engineering-core-slots 가 readiness-config-v1 의 *부분 schema* 로 흡수.
8. per-NF 연기 레지스터 위치 — `readiness-config.yaml` 의 `deferred_decisions` block 으로 promote (출처: `design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices`).
9. row-level `ratified_by`/`date` 입자 — readiness-config 내부 nested row 별 유지 (engineering-design 의 현 패턴 보존).

### 8.2 잔존 OD (audit 이 닫지 못함, PR-3/4/D 단계 결정)

- OD-D: WI/task ID 안정성 규칙 — PR-4 단계.
- OD-F: `--check` CI 통합 시점 — PR-4 단계.
- OD-G: NF-agnostic schema 여부 — PR-3 단계.
- OD-H: validator script 의 ADR dead reference 정리 — PR-D 단계.
- OD-I: 기존 PR F1 generator 와의 정합 — PR-4 진입 전 확인.

추가 audit 결정 (audit doc §5 의 row 별 ratify) 는 audit doc 의 acceptance 일부.

## 9. Acceptance criteria

- `design/nssf/readiness-config.yaml` exists and captures current human-ratified implementation choices.
- Project-wide rules currently embedded in ADRs have machine-readable policy equivalents.
- `engineering-design.md` and `dev/nssf/` readiness pack are documented as generated artifacts, not source-of-truth.
- `/nf-readiness nssf --check` can detect drift between config/policies and tracked generated artifacts.
- `/nf-readiness nssf --write` can regenerate the tracked generated artifacts deterministically.
- ADRs no longer need to be parsed as prose by implementation agents.

## 10. Non-goals

- Do not start actual NSSF code implementation in this plan.
- Do not remove historical ADRs.
- Do not rewrite all lifecycle tools in one unverified pass.
- Do not add new dependencies unless explicitly required.
- Do not make generated artifacts untracked until a separate policy decision says so.
