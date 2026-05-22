---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} API Implementation Matrix

## Operation Matrix

`handoff/{{nf}}/contract.yaml` 의 모든 API operation 1:1 매핑. 누락된 row 가 있으면 `impl_ready_for_codegen` gate 의 `api_matrix_covers_all_operations` 가 FAIL.

| operationId | method | path | handler | data-model | security | persistence | unit test | integration test |
|---|---|---|---|---|---|---|---|---|
| TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |

## Coverage

- 본 matrix 가 `contract.api.operations` 의 모든 entry 를 cover 하는지 확인 — count 와 operationId 집합 일치.
- 각 operation 의 `security` 열은 `engineering/<nf>/engineering-design.md` 와 ADR-0004 security baseline 을 반영한다.
- `persistence` 열은 architecture `state-persistence.md` 의 backend 결정과 일치.

## Open Questions

- TODO: spec 에서 implementation 판단이 더 필요한 항목을 `open-gaps-and-assumptions.md` 의 gap id 와 연결.

## References

- `{{contract_path}}` — operation 진실 출처.
- `design/{{nf}}/contract/api/` — operation 별 topic 문서.
- `{{architecture_root}}/request-flow.md`, `{{architecture_root}}/error-propagation.md`.
- `engineering/{{nf}}/engineering-design.md` — security/persistence freeze.
