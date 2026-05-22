# NF Readiness / Autonomous Implementation Workflow Upgrade Plan — 2026-05-21

Status: proposed
Current objective: PR #38 의 단발성 NSSF codegen tracer-bullet plan 을 폐기/대체하고, 최종 목표인 “사람 개입 없는 장기 NF 구현”을 가능하게 하는 public workflow + lifecycle skill 산출물 체계를 정립한다.

## 1. Decision

PR #38 (`codegen-nssf-tracer-bullet-plan`) 은 현재 형태로 merge 하지 않는다.

이유:
- `eng_frozen` PASS 는 third-party library / DBMS / runtime / tool decision freeze 이지, 전체 구현 착수에 충분한 “구현 재료 완성” 신호가 아니다.
- 현재 `dev/nssf/*` 산출은 큰 기능 단위 task / test inventory 수준이며, file-level work item, data-model implementation map, API implementation matrix, team execution plan, autonomous verification loop 가 부족하다.
- codegen agent 가 `TS29531_Nnssf_NSSelection.yaml` 등 원본 spec 을 다시 읽어 구현 판단을 해야 한다면, contract/dev 산출물이 implementation-ready 수준까지 spec 을 흡수하지 못했다는 증거다.
- tracer-bullet 은 최종 autonomous implementation workflow 내부 Phase 1 proof 여야지, 사람이 별도로 리뷰·merge하는 독립 lifecycle 단계가 되면 안 된다.

대체 방향:
- public human workflow 를 3개 행위로 축소한다.
  1. `specs/` 문서 준비.
  2. `/nf-readiness <nf>` 요청 — implementation readiness pack 생성/검증.
  3. `/nf-implement <nf>` 요청 — 장기 autonomous 구현 시작.
- 기존 세부 lifecycle skill 은 public 주 작업이 아니라 `/nf-readiness` 내부 subroutine 으로 재배치한다.
- third-party library 교체, DBMS 교체, security policy 변경, spec 추가/제외, public contract 변경 같은 “새 계약”이 필요한 경우에만 사람이 세부 skill/config 를 직접 사용한다.

## 2. Target public workflow

```text
Human input:
  specs/<spec>/*.docx|*.yaml 준비

/nf-readiness nssf --primary 29.531
  → specs_ready
  → contract_implementable
  → arch_consistent
  → impl_ready_for_codegen
  → eng_frozen
  → readiness_pack_ready

/nf-implement nssf
  → readiness pack 확인
  → Phase 1 tracer-bullet proof
  → Phase 2 task graph 보정
  → Phase 3 full NSSF feature waves
  → Phase 4 contract/security/e2e verification
  → Phase 5 hardening/review/merge
```

## 3. Gate semantics 정정

| Gate | 의미 | 다음 단계 |
|---|---|---|
| `specs_ready` | 필요한 원본 spec 파일이 준비됨 | contract extraction |
| `contract_implementable` | 원본 spec 재해석 없이 API/data-model 구현 판단 가능 | architecture design |
| `arch_consistent` | module/flow/state/error/test seam 정합 | implementation planning |
| `impl_ready_for_codegen` | file-level work item + test + team/readiness 산출 완성 | engineering freeze |
| `eng_frozen` | library/DB/runtime/tool/operator-policy 결정 freeze | readiness finalization |
| `readiness_pack_ready` | `/nf-implement` 시작 가능 | autonomous implementation |
| `tracer_bullet_passed` | 실제 codegen/build/test path 검증됨 | full feature waves |
| `full_nf_done` | full NF 기능 + contract/security/e2e 검증 완료 | merge/release |

중요: `eng_frozen` 단독으로 codegen GO 를 의미하지 않는다. 최종 GO 는 `readiness_pack_ready` 다.

## 3.1 Registry vs manifest 관계 (2026-05-22 amendment)

NF 별 lifecycle 입력은 두 layer 로 분리한다.

| Layer | 파일 | 성격 | git 추적 | 책임자 |
|---|---|---|---|---|
| **durable NF catalog / routing source** | `design/nf-registry.yaml` | spec 스캔으로 AI/tool 이 생성·보강, 사람은 `manual_overrides` 만 | tracked | tool 자동 + 사람 override |
| **per-NF discovery rendered output** | `design/<nf>/_manifest.yaml` | registry + spec scan 결과의 *rendered* artifact. 재생성물. | gitignored | `/nf-spec-discover` 도구 |

Canonical flow.

```
specs/<spec>/ 준비
    └─ (bootstrap) tool 이 specs/ 스캔 → design/nf-registry.yaml 자동 생성·보강
          └─ /nf-readiness <nf>
                ├─ design/nf-registry.yaml 에서 nfs[<nf>].primary_spec resolve
                ├─ resolve 실패/저신뢰 → blocker 로 보고 후 사람 manual_overrides 확정
                └─ /nf-spec-discover <nf> --primary <resolved> (internal subroutine)
                      └─ design/<nf>/_manifest.yaml (rendered output)
                            └─ design/<nf>/_contract_seed.yaml
                                  └─ contract / arch / impl / eng / readiness pipeline
```

핵심 원칙.

- `design/nf-registry.yaml` 은 *routing source* — fresh checkout 에서 `/nf-readiness <nf>` 가 동작하려면 본 파일이 있어야 한다 (git tracked).
- `_manifest.yaml` 은 *rendered output* — durable 한 정책 결정은 *registry 또는 ADR 에 저장* 하고 manifest 는 그것을 인용한 결과만 담는다.
- `--primary` 는 *override / bootstrap / 실험용* — canonical UX 가 아니다. registry 미존재 또는 신뢰도 낮을 때만 사용.

## 3.2 Durable manual decisions 위치 (2026-05-22 amendment)

33.501 / 33.310 / 33.210 같은 *durable 정책 결정* 을 `_manifest.yaml` 의 `manual_overrides.exclude` *에만* 두면 fresh checkout 후 manifest 가 새로 생성될 때 정책이 사라진다 (manifest = rendered output). durable source 는 다음 둘 중 하나여야 한다.

| 결정 종류 | durable 위치 | manifest 가 인용 |
|---|---|---|
| project-wide security/compliance baseline (33.501, 33.310, 33.210 등) | `docs/adr/ADR-0004-project-security-baseline.md` (또는 후속 ADR) | `manifest.deps.security[i].excluded` reason 이 ADR 인용 |
| NF-specific spec subset / scope policy (예 NSSF 가 38.413 보류) | `design/nf-registry.yaml` 의 `manual_overrides.nfs.<nf>.spec_policy.exclude.<spec>` | manifest 가 `source_registry: nf-registry.yaml` 명시 + `excluded: true` + reason 동일 |
| operational decision (deployment 옵션 등) | engineering-design.md 의 해당 slot | manifest 가 직접 참조 안 함 (engineering 단계 책임) |

예시 (registry).

```yaml
# design/nf-registry.yaml (excerpt)
manual_overrides:
  nfs:
    nssf:
      spec_policy:
        exclude:
          "33.501":
            reason: "absorbed_by_ADR_0004 — project security baseline 으로 통합"
            ratified_by: "jjinri"
            date: "2026-05-21"
          "38.413":
            reason: "AMF reallocation via RAN 운영 보류"
            ratified_by: "jjinri"
            date: "2026-05-19"
```

manifest 는 위 registry 결정을 *render* 한 output 으로 `deps.security` / `cross-nf` row 에 excluded=true + reason 을 채운다.

## 3.3 Lifecycle 단계 표 — 산출물·gate·병합 후보 (2026-05-22 amendment)

| Stage | 산출물 (durable) | Gate | 책임 skill (internal) | 병합/리네임 후보 |
|---|---|---|---|---|
| (A) Spec preparation | `specs/<spec>/*.docx`, `*.yaml` (사람 투입) | `specs_ready` | — | 유지 |
| (B) NF routing registry | `design/nf-registry.yaml` | (registry 자체는 gate 아님; `/nf-readiness` precondition) | `/nf-readiness` bootstrap | PR E 신설 |
| (C) Spec discovery | `_manifest.yaml`·`_contract_seed.yaml` (rendered) | manifest `ready_for_build` | `/nf-spec-discover` (internal) | 유지 |
| (D) Contract build | `design/<nf>/contract/**` + `handoff/<nf>/contract.yaml` | — | `/nf-contract-build` (internal) | 유지 |
| (E) Contract check | `_contract_status.yaml` | `handoff_ready` + `contract_implementable` | `/nf-contract-check` (internal) | 두 gate 모두 같은 status 파일 안 |
| (F) Architecture | `design/<nf>/architecture/**` + `module-decomposition/**` | `arch_consistent` | `/nf-arch-design` + `/nf-arch-status` (internal) | 유지 |
| (G) Implementation planning + readiness pack | `dev/<nf>/{impl-plan,tasks,test-matrix,traceability,api-implementation-matrix,data-model-implementation-map,codegen-work-items,team-execution-plan,verification-plan,implementation-readiness-review,design-adequacy-checklist,spec-to-design-coverage,open-gaps-and-assumptions}` | `impl_consistent` + `impl_ready_for_codegen` | `/nf-impl-plan` + `/nf-impl-status` (internal) | PR C 가 산출 9 파일 의무화 |
| (H) Engineering design freeze | `engineering/<nf>/engineering-design.md` | `eng_frozen` (tech decision freeze, *not* final codegen GO) | `/nf-eng-design` + `/nf-eng-status` (internal) | ADR-0002 wording 정정 (PR D) |
| (I) Readiness aggregate | (계산값, 별 파일 없음 또는 `_readiness_status.yaml`) | `readiness_pack_ready` = handoff_ready ∧ contract_implementable ∧ arch_consistent ∧ impl_ready_for_codegen ∧ eng_frozen | `/nf-readiness` aggregate (public) | PR D 신설 |
| (J) Autonomous implementation Phase 1 | `src/`, `generated/`, `sql/`, `tests/`, `vendored/`, CI | `tracer_bullet_passed` | `/nf-implement` Phase 1 (public) | PR E + PR G |
| (K) Full implementation waves | (위 + module 별 산출) | `full_nf_done` | `/nf-implement` Phase 2~5 | follow-up |

병합/단순화 검토.

- `/nf-arch-design` + `/nf-arch-status` — 산출/gate 가 명확히 분리되어 유지.
- `/nf-impl-plan` + `/nf-impl-status` — 같은 패턴, 유지. impl_consistent 와 impl_ready_for_codegen 두 gate 가 같은 status 파일 안.
- `/nf-eng-design` + `/nf-eng-status` — 사람 ratify 단계 분리 위해 유지.
- legacy alias (`/nf-init` / `/nf-build` / `/nf-status`) — 호환 유지하되 `/nf-readiness` 내부 호출만, public 직접 호출 비권장 (PR E 에서 SKILL.md 갱신).

## 4. Required skill upgrades

### 4.1 `/nf-contract-build` upgrade

현재 한계:
- topic/data-model 산출이 architecture 시작에는 충분할 수 있으나, 구현자가 spec 없이 코딩할 만큼 field semantics / validation / generated-vs-wrapper 판단을 담지 못한다.

추가 산출/보강:
- `design/<nf>/contract/api/<Operation>.md`
  - method/path/server base
  - query/header/body params
  - required/optional/default
  - security requirement
  - response status/body/content-type
  - error/ProblemDetails matrix
  - operation-specific validation rule
  - trace source line/reference
- `design/<nf>/contract/data-model/<Schema>.md`
  - field table: name, required, nullable, type, enum, array/map cardinality
  - external `$ref` resolution target
  - `oneOf` / `allOf` / `anyOf` / discriminator handling
  - C type mapping hint
  - serializer/deserializer hint
  - validator hint
  - generated vs hand-written wrapper recommendation
- machine JSON extension
  - codegen-friendly normalized schema
  - unresolved/ref complexity flags
  - c_type_hint / validation_hint / wrapper_required

New quality rule:
- codegen agent may use original OpenAPI YAML for regeneration/drift, but not for semantic rediscovery.

### 4.2 `/nf-contract-check` upgrade

현재 `handoff_ready` 는 “architecture 시작 가능”이다. 추가 gate 필요:

```text
contract_implementable:
  requires:
    - api_operation_complete
    - data_model_field_tables_complete
    - external_refs_resolved_or_classified
    - schema_complexity_classified
    - generated_wrapper_boundary_declared
    - problem_details_matrix_complete
    - no_spec_reread_required_for_implementation
```

검사 방식:
- 문서 내용의 “사람 승인”이 아니라 구조/coverage/source/gap classification 검사.
- source 없는 claim 은 assumption/gap 으로 강등.

### 4.3 `/nf-arch-design` upgrade

현재 architecture 문서는 module boundary 수준은 있으나 implementation readiness 에 필요한 interface contract 가 약하다.

보강:
- module interface sketch
  - input/output C-level DTO boundary
  - sync/async boundary
  - ownership/lifetime rule
  - error propagation contract
- operation-to-module execution path
- persistence touch map
- security enforcement point map
- observability event/span/metric map
- test seam map

### 4.4 `/nf-impl-plan` upgrade

`Implementation Readiness Pack` 은 별도 public lifecycle 단계가 아니라 `/nf-impl-plan` 의 필수 산출이어야 한다.

필수 산출:

```text
dev/<nf>/api-implementation-matrix.md
dev/<nf>/data-model-implementation-map.md
dev/<nf>/codegen-work-items.yaml
dev/<nf>/team-execution-plan.md
dev/<nf>/verification-plan.md
dev/<nf>/implementation-readiness-review.md
dev/<nf>/design-adequacy-checklist.md
dev/<nf>/spec-to-design-coverage.md
dev/<nf>/open-gaps-and-assumptions.md
```

Agent Execution Pack:
- `api-implementation-matrix.md`
  - 8 operation 전체 method/path/handler/data-model/security/persistence/test 매핑.
- `data-model-implementation-map.md`
  - schema 별 generated / wrapper / handwritten / deferred 분류.
- `codegen-work-items.yaml`
  - file-level 작업 단위.
  - `id`, `owner_lane`, `depends_on`, `inputs`, `expected_files`, `acceptance`, `tests`, `verification_commands` 필수.
- `team-execution-plan.md`
  - orchestrator/code/reviewer/tester/verifier lane, write scope, integration order.
- `verification-plan.md`
  - unit/integration/contract/security/e2e/observability gates.

Human Review Pack:
- `implementation-readiness-review.md`
  - 사람이 전체 설계 적정성을 빠르게 볼 수 있는 executive review.
- `design-adequacy-checklist.md`
  - 사람이 체크 가능한 질문 목록. 자동 승인이 아니라 audit surface.
- `spec-to-design-coverage.md`
  - spec/OpenAPI topic → contract → architecture → module → dev task → test → eng decision trace.
- `open-gaps-and-assumptions.md`
  - 모든 gap 을 `blocker`, `deferred`, `operator-provided`, `library-assumed`, `test-gap`, `assumption` 으로 분류.

### 4.5 `/nf-impl-status` upgrade

`nf-impl-status` 는 Human Review Pack 의 내용 타당성을 승인하지 않는다. 대신 다음을 자동 판단한다.

```text
impl_ready_for_codegen:
  requires:
    - readiness_pack_files_present
    - api_matrix_covers_all_operations
    - data_model_map_covers_all_contract_models
    - all_work_items_have_expected_files
    - all_work_items_have_tests
    - all_work_items_have_verification_commands
    - team_execution_plan_present
    - human_review_pack_traceable
    - gaps_classified
    - blocker_gaps_zero
    - no_spec_reread_required
```

검사 원칙:
- 사람이 “설계가 좋다”고 승인했는지는 gate 에 넣지 않는다.
- 사람이 볼 문서가 존재하고, claim/source/gap 구조가 검증 가능한지만 본다.
- 원본 spec 재독해가 필요한 항목은 blocker gap 으로 표면화한다.

### 4.6 `/nf-eng-design` / `/nf-eng-status` adjustment

`eng_frozen` 의미를 좁힌다.

현재 유지:
- language/runtime
- HTTP stack
- TLS/OAuth library
- persistence backend
- schema/codegen tool
- config/test/lint/deploy policy

보강:
- generated code policy
- wrapper policy
- module interface policy
- DB migration policy
- CI/runtime dependency policy
- agent source precedence
- operator/library responsibility boundary

하지만 `eng_frozen` 은 여전히 readiness 의 한 구성요소일 뿐, 단독 codegen GO 가 아니다.

## 5. New public workflow skills

### 5.1 `/nf-readiness <nf>`

Purpose:
- 사람이 “implementation readiness pack 만들어줘”라고 요청하는 단일 entrypoint.

Internal pipeline:

```text
1. spec discovery/manifest refresh
2. contract build
3. contract implementability check
4. architecture design/update
5. architecture status
6. implementation planning + readiness pack generation
7. implementation status (`impl_ready_for_codegen`)
8. engineering design/freeze update
9. engineering status
10. final readiness status (`readiness_pack_ready`)
```

Output:
- readiness pack files under `dev/<nf>/`.
- status report explaining blockers if any.
- Human Review Pack for audit, not approval gate.

Stop condition:
- `readiness_pack_ready` PASS or explicit blockers with to_pass.

### 5.2 `/nf-implement <nf>`

Purpose:
- 사람의 세부 개입 없이 장기 autonomous implementation 시작.

Precondition:
- `readiness_pack_ready` PASS.
- no blocker gaps.
- team execution plan present.

Internal phases:

```text
Phase 0. Readiness verification
Phase 1. Tracer-bullet proof
  - CMake bootstrap
  - OpenAPI generated model integration
  - transport/security/persistence skeleton
  - 1 endpoint stub
  - smoke test/CI path
Phase 2. Feedback integration
  - tracer 결과로 work item graph 보정
Phase 3. Full feature waves
  - NSSelectionGet real logic
  - NSSAIAvailability ops
  - SubscriptionStore
  - NotificationDispatcher
  - NRF/UDM/AMF seams
Phase 4. Verification waves
  - unit/integration/contract/security/e2e/observability
Phase 5. Hardening/review/merge
```

Tracer-bullet 의미:
- full implementation trigger 자체가 아니라 `/nf-implement` 내부의 first proof slice.
- readiness pack 이 실제 코드 생성/빌드/test 로 이어지는지 검증한다.

## 6. NSSF-specific immediate plan

### PR #38 handling

- Do not merge current PR #38.
- Either close it as superseded or amend it to state it is superseded by this workflow upgrade plan.
- The existing tracer-bullet content can be reused later as `/nf-implement nssf` Phase 1 draft, but only after `readiness_pack_ready` PASS.

### Next PR sequence

PR A — Workflow policy / docs
- Add this plan.
- Update ONBOARDING/README lifecycle overview:
  - public user workflow = specs 준비 → `/nf-readiness` → `/nf-implement`.
  - old per-stage skills = internal/exception tools.
  - `eng_frozen` no longer final codegen GO; `readiness_pack_ready` is final GO.

PR B — Contract implementability upgrade
- Upgrade `/nf-contract-build` instructions.
- Extend scripts if needed for normalized data-model/codegen metadata.
- Upgrade `/nf-contract-check` with `contract_implementable` gate.
- Regenerate/check NSSF contract artifacts locally.

PR C — Impl readiness pack generator
- Upgrade `/nf-impl-plan` to emit Agent Execution Pack + Human Review Pack.
- Upgrade `/nf-impl-status` with `impl_ready_for_codegen`.
- Generate NSSF readiness pack.

PR D — Engineering/status semantics update
- Adjust ADR-0002/ONBOARDING/status wording.
- `eng_frozen` = tech decision freeze, not autonomous implementation GO by itself.
- Add `readiness_pack_ready` aggregate status.

PR E — New public wrappers
- Add `/nf-readiness` skill.
- Add `/nf-implement` skill skeleton.
- `/nf-implement` initially runs Phase 0+Phase 1 only until tracer-bullet stabilizes.

PR F — NSSF readiness rerun
- Run `/nf-readiness nssf`.
- Review generated readiness pack.
- Confirm no blocker gaps.

PR G — NSSF implementation start
- Run `/nf-implement nssf`.
- Phase 1 tracer-bullet.
- Then proceed to feature waves.

## 7. Agent team model for `/nf-implement`

Minimum lanes:

| Lane | Role | Ownership |
|---|---|---|
| Orchestrator/Integrator | task graph, source precedence, conflict resolution, final merge | whole repo coordination |
| Contract/DataModel Agent | generated vs wrapper, C mapping, validators | `generated/`, model wrappers, schema tests |
| Transport/Security Agent | nghttp2, TLS, mTLS, OAuth2 inbound/outbound | `src/transport`, security config/tests |
| Persistence Agent | libpq, migration, repositories, transactions | `sql/`, `src/*repository*`, DB tests |
| Feature Agent | SelectionEngine, AvailabilityEngine, SubscriptionStore, NotificationDispatcher | module implementation |
| Test/Verifier Agent | Unity/CTest, contract/security/e2e, CI gates | `tests/`, CI, verification report |
| Reviewer | independent code/design review | no primary write scope |

Rules:
- Each lane gets explicit write scope from `team-execution-plan.md`.
- Reviewer/verifier must be separate from implementation lanes.
- Tracer-bullet should use fewer lanes; full waves can expand.
- Agents must not use original specs for semantic rediscovery unless readiness pack marks a blocker and orchestrator reopens `/nf-readiness`.

## 8. Definition of done for workflow upgrade

Workflow upgrade is complete when:

1. A fresh checkout with specs can run `/nf-readiness nssf` and produce readiness pack.
2. `nf-impl-status` can report `impl_ready_for_codegen` PASS/FAIL with actionable blockers.
3. Human Review Pack exists and has source-backed claims/gap classifications.
4. Agent Execution Pack has file-level work items, tests, verification commands, and team lane ownership.
5. `/nf-implement nssf` can start without reading original specs for implementation semantics.
6. Tracer-bullet becomes Phase 1 inside `/nf-implement`, not standalone human-managed lifecycle.
