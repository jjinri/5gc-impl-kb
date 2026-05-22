---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Team Execution Plan

`/nf-implement {{nf}}` 의 5 agent lane 책임 분리. `impl_ready_for_codegen` gate 의 `team_execution_plan_present` 는 다섯 lane H2 가 모두 존재하는지 검사한다.

## Orchestrator Lane

- 책임 — Phase 진행·gate 체크·lane 간 작업 큐 관리·blocker 라우팅.
- 쓰기 권한 — `dev/{{nf}}/_run_state.yaml` (있을 때) 와 사용자 보고.
- 금지 — 소스 코드·테스트 직접 수정.

## Code Lane

- 책임 — `codegen-work-items.yaml` 의 `owner_lane: code` 항목 구현, generated wrapper, handwritten module 작성.
- 쓰기 권한 — `src/{{nf}}/` 또는 PR 별도 구조 합의 위치.
- 금지 — test, ADR, contract 수정.

## Reviewer Lane

- 책임 — Code lane 산출의 ADR-0004 security baseline · architecture 일관성 · contract 일관성 review.
- 쓰기 권한 — review 결과를 PR comment 또는 `dev/{{nf}}/reviews/` (있을 때).
- 금지 — 직접 수정 (PR redirect 만).

## Tester Lane

- 책임 — `verification-plan.md` 의 unit/integration/contract test 작성·실행.
- 쓰기 권한 — `tests/{{nf}}/` 또는 PR 별도 구조 합의 위치.
- 금지 — production 모듈 수정.

## Verifier Lane

- 책임 — `verification-plan.md` 의 security/e2e/observability gate 실행, signal 수집, blocker 보고.
- 쓰기 권한 — verification log, signal yaml.
- 금지 — 코드·test 수정.

## Integration Order

- TODO: Phase 1 tracer-bullet → Phase 2 task graph 보정 → Phase 3 wave → Phase 4 contract/security/e2e → Phase 5 hardening 의 lane 별 시작·종료 조건을 적는다.

## References

- `{{architecture_root}}/runtime-model.md` — module/process 모델.
- `engineering/{{nf}}/engineering-design.md` — runtime/library 결정.
- `codegen-work-items.yaml`, `verification-plan.md`.
- `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §7.
