---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Spec → Design Coverage

3GPP spec / OpenAPI topic 별 *contract → architecture → module → dev task → test → engineering decision* trace. autonomous codegen agent 가 원본 spec 을 다시 읽지 않고 구현 판단할 수 있는지 사람이 audit 가능한 view 를 제공.

## Coverage Trace

| spec/topic | contract artifact | architecture file | module | dev task id | test ref | eng decision |
|---|---|---|---|---|---|---|
| TODO | TODO | TODO | TODO | TODO | TODO | TODO |

## Gaps

- TODO: trace 가 끊긴 항목을 `open-gaps-and-assumptions.md` 의 gap id 와 함께 적는다.
- TODO: spec 재독해가 필요한 항목은 `blocker` gap 으로 표면화.

## References

- `{{contract_path}}` — handoff-v2 진실 출처.
- `design/{{nf}}/contract/` — topic 별 contract.
- `specs/<spec>/` — 원본 3GPP (autonomous agent 가 다시 읽지 않는 것이 목표).
- `engineering/{{nf}}/engineering-design.md`.
