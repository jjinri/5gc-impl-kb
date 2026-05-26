---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: '2026-05-26'
generator: design/scripts/nf-readiness-pack-generate.py
source_readiness_config: design/nssf/readiness-config.yaml
generated_sections:
  - phase-integration-map
user_sections:
  - intro-note
  - orchestrator-lane-body
  - code-lane-body
  - reviewer-lane-body
  - tester-lane-body
  - verifier-lane-body
  - integration-order-body
  - references-body
---

# NSSF Team Execution Plan

<!-- USER:intro-note:start -->
`/nf-implement nssf` 의 5 agent lane 책임 분리. `impl_ready_for_codegen` gate 의 `team_execution_plan_present` 는 다섯 lane H2 가 모두 존재하는지 검사한다.
<!-- USER:intro-note:end -->

## Orchestrator Lane

<!-- USER:orchestrator-lane-body:start -->
- 책임 — Phase 진행·gate 체크·lane 간 작업 큐 관리·blocker 라우팅.
- 쓰기 권한 — `dev/nssf/_run_state.yaml` (있을 때) 와 사용자 보고.
- 금지 — 소스 코드·테스트 직접 수정.
<!-- USER:orchestrator-lane-body:end -->

## Code Lane

<!-- USER:code-lane-body:start -->
- 책임 — `codegen-work-items.yaml` 의 `owner_lane: code` 항목 구현, generated wrapper, handwritten module 작성.
- 쓰기 권한 — `src/nssf/` 또는 PR 별도 구조 합의 위치.
- 금지 — test, ADR, contract 수정.
<!-- USER:code-lane-body:end -->

## Reviewer Lane

<!-- USER:reviewer-lane-body:start -->
- 책임 — Code lane 산출의 ADR-0004 security baseline · architecture 일관성 · contract 일관성 review.
- 쓰기 권한 — review 결과를 PR comment 또는 `dev/nssf/reviews/` (있을 때).
- 금지 — 직접 수정 (PR redirect 만).
<!-- USER:reviewer-lane-body:end -->

## Tester Lane

<!-- USER:tester-lane-body:start -->
- 책임 — `verification-plan.md` 의 unit/integration/contract test 작성·실행.
- 쓰기 권한 — `tests/nssf/` 또는 PR 별도 구조 합의 위치.
- 금지 — production 모듈 수정.
<!-- USER:tester-lane-body:end -->

## Verifier Lane

<!-- USER:verifier-lane-body:start -->
- 책임 — `verification-plan.md` 의 security/e2e/observability gate 실행, signal 수집, blocker 보고.
- 쓰기 권한 — verification log, signal yaml.
- 금지 — 코드·test 수정.
<!-- USER:verifier-lane-body:end -->

## Integration Order

<!-- USER:integration-order-body:start -->
Phase 별 lane 시작·종료 조건. `codegen-work-items.yaml` 의 `depends_on` 그래프와 1:1 일치한다.

| Phase | wave | work items | 시작 조건 | 종료 조건 (다음 Phase 진입 게이트) |
|---|---|---|---|---|
| Phase 1 | wave A (tracer-bullet) | `WI-codegen-bootstrap` → `WI-schema-bootstrap` / `WI-tls-bootstrap` / `WI-problem-details-wrapper` / `WI-nftype-wrapper` → `WI-availability-repo` → `WI-selection-engine` → `WI-nsselection-handler` | F2 readiness pack ready (현 PR). | `WI-nsselection-handler` 의 integration + contract test PASS, NSSelectionGet golden path green. |
| Phase 2 | wave A (NSSAIAvailability handlers) | `WI-availability-engine` → `WI-availability-handlers` (Put/Patch/Delete/Options 4 handler — Options 는 same work item 내부 independent sub-step) + `WI-notification-dispatcher` | Phase 1 종료. | Put/Patch/Delete/Options integration test PASS, retry_queue row enqueue 검증. |
| Phase 3 | wave A (subscription) | `WI-subscription-store` → `WI-subscription-handlers` (Post/Unsubscribe/SubModifyPatch 3 handler) | Phase 2 wave A 종료 (NotificationDispatcher 필요). | Subscription 3 op integration test PASS, initial snapshot dispatch 검증. |
| Phase 4 | wave A (verify) | `WI-contract-tests` / `WI-security-tests` / `WI-e2e-tests` | Phase 3 종료. | 8 op × 18 cause matrix green, ADR-0004 7 항목 evidence emit, e2e 시나리오 green. |
| Phase 5 | wave A (hardening) | `WI-observability-finalize` | Phase 4 종료. | metric/log/trace seam production-capable signal emit, label cardinality / log redaction / trace sampler freeze. |

lane 별 진행 규칙.

- Orchestrator — Phase 진입 게이트 통과 여부를 lane 별 보고로 확정. 게이트 미통과 시 blocker 라우팅 (open-gaps-and-assumptions 의 gap id 와 연결).
- Code — Phase 1~3 wave 진행, Phase 4 contract test 까지 진행. Phase 5 의 observability 모듈도 code lane.
- Reviewer — Phase 1 의 NSSelectionGet PR 부터 모든 PR 에서 ADR-0004 / architecture / contract 정합성 review. Phase 가 진전될 때마다 누적 영역 점검.
- Tester — Phase 1 unit/integration test 부터 시작, Phase 4 contract test 까지 작성. e2e 는 verifier lane 과 공동 책임.
- Verifier — Phase 4 의 security/e2e 부터 시작, Phase 5 observability gate 까지 책임. signal yaml 누적.
<!-- USER:integration-order-body:end -->

<!-- AUTO:phase-integration-map:start -->
### Phase Integration Map

`design/nssf/readiness-config.yaml` `phase_policy` derive — tracer-bullet operation = `NSSelectionGet`, 총 5 phase. config 변경 시 본 map 이 같이 갱신된다.

| phase | description | work_items |
|---|---|---|
| `phase1` | tracer-bullet, PR G — NSSelectionGet single-op end-to-end | `WI-codegen-bootstrap`, `WI-schema-bootstrap`, `WI-tls-bootstrap`, `WI-problem-details-wrapper`, `WI-nftype-wrapper`, `WI-availability-repo`, `WI-selection-engine`, `WI-nsselection-handler` |
| `phase2` | NSSAIAvailability writes, PR G 후속 | `WI-availability-engine`, `WI-availability-handlers`, `WI-notification-dispatcher` |
| `phase3` | subscription, PR G 후속 | `WI-subscription-store`, `WI-subscription-handlers` |
| `phase4` | contract / security / e2e tests, PR G 후속 | `WI-contract-tests`, `WI-security-tests`, `WI-e2e-tests` |
| `phase5` | hardening, PR G 후속 | `WI-observability-finalize` |
<!-- AUTO:phase-integration-map:end -->

## References

<!-- USER:references-body:start -->
- `design/nssf/architecture/runtime-model.md` — module/process 모델.
- `engineering/nssf/engineering-design.md` — runtime/library 결정.
- `codegen-work-items.yaml`, `verification-plan.md`.
- `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §7.
<!-- USER:references-body:end -->
