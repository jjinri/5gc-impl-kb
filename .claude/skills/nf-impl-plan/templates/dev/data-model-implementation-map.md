---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Data Model Implementation Map

## Schema Classification

`handoff/{{nf}}/contract.yaml` 의 모든 data-model schema 1:1 매핑. 누락된 row 가 있으면 `impl_ready_for_codegen` gate 의 `data_model_map_covers_all_contract_models` 가 FAIL.

| schema | kind | target path | rationale | tests |
|---|---|---|---|---|
| TODO | generated\|wrapper\|handwritten\|deferred | TODO | TODO | TODO |

`kind` 값.

- `generated` — openapi-generator 등 codegen 도구가 생성. wrapper 불필요.
- `wrapper` — generated 위 사람 친화 wrapper 가 필요한 schema (`wrapper_required: true` in contract JSON).
- `handwritten` — codegen 부적합 또는 비표준 변환이 필요해 사람이 작성.
- `deferred` — 본 사이클 범위 밖. `open-gaps-and-assumptions.md` 에 `deferred` gap 으로 표시.

## Wrapper Rationale

- TODO: wrapper 가 필요한 schema 각각의 이유를 `complexity_flags` (oneOf/allOf/anyOf/discriminator/external_ref) 와 연결.
- 운영자 식별자·외부 ref·security profile 처리 등 wrapper 가 흡수할 책임을 명시.

## Open Schemas

- TODO: classification 이 확정되지 않은 schema 를 `open-gaps-and-assumptions.md` 의 gap id 와 함께 적는다.

## References

- `{{contract_path}}` — data-model 진실 출처.
- `design/{{nf}}/contract/data-model/*.json` — `complexity_flags`, `wrapper_required`, `c_type_hint`, `validation_hint`.
- `{{architecture_root}}/module-boundaries.md` — generated vs handwritten boundary.
- `engineering/{{nf}}/engineering-design.md` — schema/codegen tool freeze.
