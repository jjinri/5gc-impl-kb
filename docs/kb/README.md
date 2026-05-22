# KB guide

이 문서는 구현 agent 와 사람 reviewer 가 5gc-impl-kb 를 어떻게 읽어야 하는지 설명한다. `docs/plans/**` 와 `docs/retros/**` 는 history 이며 현재 KB 독해 순서에 포함하지 않는다.

## 1. KB 목적

5gc-impl-kb 의 KB 는 두 독자를 동시에 지원한다.

1. **AI implementation agent** — 어떤 코드를 어떤 순서로 만들고, 어떤 test/verification 으로 통과시킬지 결정한다.
2. **Human reviewer** — 지금까지 생성된 contract/design/engineering/readiness 산출이 구현 착수에 충분한지 검토한다.

## 2. Clean rebuild 에 필요한 필수 파일

fresh clone 에서 새 NF KB 를 만들 때 사람이 준비해야 하는 필수 파일은 원본 spec 이다.

| 필수성 | 위치 | 설명 |
|---|---|---|
| 필수 | `specs/<primary-spec>/*.yaml` | OpenAPI 기반 API/data-model 추출 입력 |
| 필수 | `specs/<primary-spec>/*.docx` 또는 normative text | spec section/source trace 입력 |
| 필요 시 | `specs/<supporting-spec>/*` | OpenAPI `$ref`, common data, cross-NF trace 에 필요할 때 |
| 자동/추적 | `design/nf-registry.yaml` | `/nf-readiness` 가 NF→primary spec 을 resolve 하는 catalog. bootstrap 이 generated 영역 갱신, 사람은 manual_overrides 만 편집 |

그 외 contract/architecture/dev/engineering 산출은 lifecycle skill 이 생성하거나 검증한다.

## 3. Skill 생성 산출물 분류

| 산출 | 생성 주체 | git 정책 | 읽는 사람/agent |
|---|---|---|---|
| `design/<nf>/_manifest.yaml` | `/nf-spec-discover` | 비추적 cache | readiness wrapper, contract build |
| `design/<nf>/_contract_seed.yaml` | `/nf-spec-discover` | 비추적 cache | contract build |
| `design/<nf>/contract/**` | `/nf-contract-build` | 비추적 cache | contract check, architecture generation |
| `handoff/<nf>/contract.yaml` | `/nf-contract-build` | 비추적 cache | arch/impl planning, status checks |
| `design/<nf>/architecture/**` | `/nf-arch-design` | 추적 KB | implementation agent, human reviewer |
| `design/<nf>/module-decomposition/**` | `/nf-arch-design` | 추적 KB | implementation agent |
| `engineering/<nf>/engineering-design.md` | `/nf-eng-design` + 사람 ratify | 추적 KB | implementation agent, reviewer |
| `dev/<nf>/implementation-plan.md` | `/nf-impl-plan` | 추적 KB | implementation agent |
| `dev/<nf>/tasks.yaml` | `/nf-impl-plan` | 추적 KB | implementation agent |
| `dev/<nf>/test-matrix.md` | `/nf-impl-plan` | 추적 KB | tester/reviewer |
| `dev/<nf>/traceability.md` | `/nf-impl-plan` | 추적 KB | reviewer |
| `dev/<nf>/api-implementation-matrix.md` | `/nf-impl-plan` | 추적 KB | implementation agent |
| `dev/<nf>/data-model-implementation-map.md` | `/nf-impl-plan` | 추적 KB | implementation agent |
| `dev/<nf>/codegen-work-items.yaml` | `/nf-impl-plan` | 추적 KB | orchestrator/code lane |
| `dev/<nf>/team-execution-plan.md` | `/nf-impl-plan` | 추적 KB | orchestrator/team lanes |
| `dev/<nf>/verification-plan.md` | `/nf-impl-plan` | 추적 KB | tester/verifier |
| `dev/<nf>/implementation-readiness-review.md` | `/nf-impl-plan` | 추적 KB | human reviewer |
| `dev/<nf>/design-adequacy-checklist.md` | `/nf-impl-plan` | 추적 KB | human reviewer |
| `dev/<nf>/spec-to-design-coverage.md` | `/nf-impl-plan` | 추적 KB | human reviewer, verifier |
| `dev/<nf>/open-gaps-and-assumptions.md` | `/nf-impl-plan` | 추적 KB | all lanes |
| `_*_status.yaml` | status scripts | 비추적 cache | gate evidence only |

## 4. 구현 agent 독해 순서

1. `dev/<nf>/codegen-work-items.yaml` — phase/work item queue.
2. `dev/<nf>/api-implementation-matrix.md` — operation 단위 handler, model, security, persistence, tests.
3. `dev/<nf>/data-model-implementation-map.md` — generated/wrapper/handwritten boundary.
4. `engineering/<nf>/engineering-design.md` — chosen libraries, DB, runtime, config boundary.
5. `design/<nf>/architecture/**` and `module-decomposition/**` — flow/state/error/module details.
6. `dev/<nf>/verification-plan.md` — gate evidence.
7. `dev/<nf>/open-gaps-and-assumptions.md` — non-blocker assumptions and operator inputs.

Stop condition: any item marked `category: blocker`, any `spec reread needed? = yes`, or missing work item input means `/nf-readiness` must be reopened before implementation.

## 5. 사람 reviewer 독해 순서

1. `dev/<nf>/implementation-readiness-review.md` — GO summary, risk, conditions.
2. `dev/<nf>/design-adequacy-checklist.md` — reviewer checklist.
3. `dev/<nf>/spec-to-design-coverage.md` — spec→design→task trace and no-spec-reread audit.
4. `dev/<nf>/open-gaps-and-assumptions.md` — blocker 0 and non-blocker handling.
5. `engineering/<nf>/engineering-design.md` — library/DB/security/runtime decisions.
6. `docs/adr/ADR-0004-project-security-baseline.md` — project-wide security obligations.

## 6. Public vs internal skills

| 구분 | skill | 사람이 직접 호출? |
|---|---|---|
| Public | `/nf-readiness <nf>` | 예 |
| Public | `/nf-implement <nf>` | 예, `readiness_pack_ready PASS` 후 |
| Internal | `/nf-spec-discover`, `/nf-contract-build`, `/nf-contract-check`, `/nf-arch-design`, `/nf-arch-status`, `/nf-impl-plan`, `/nf-impl-status`, `/nf-eng-design`, `/nf-eng-status` | 보통 아니오. 새 계약/override/debug 상황에서만 |

## 7. NSSF 기준

`dev/nssf/` 는 현재 NSSF implementation KB 다. `/nf-implement nssf` 는 이 pack 을 읽어 Phase 1 NSSelectionGet tracer-bullet 부터 시작한다.

## 8. Follow-up design notes

- [Deterministic readiness regeneration goal](./deterministic-readiness-regeneration.md) — `/nf-readiness` 를 clean rebuild 가능한 compiler workflow 로 발전시키기 위한 후속 논의 메모.
