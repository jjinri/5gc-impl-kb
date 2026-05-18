---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Test Matrix

## Purpose

본 문서는 implementation task 가 만족해야 하는 contract-derived test surface 를 정의한다 — 진실 출처.

## Test Inventory

| id | kind | scenario | given | when | then | refs |
| --- | --- | --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO | TODO | TODO |

## Coverage Rules

- API success path 를 operation 당 최소 1 개 포함한다.
- Error matrix 의 각 major status code 를 포함한다.
- Data model validation 과 ProblemDetails mapping 을 분리해 검증한다.
- External NF boundary failure 를 mock 또는 contract test 로 검증한다.
- TODO: 본 NF 고유 coverage rule 을 추가하고, rule 별 대응 Test Inventory row 수를 자기 검증한다.

## Open Questions

- TODO: test 전략의 미해결 항목 (load/soak 포함 여부, mock vs 실제 NF 등) 을 적는다.

## References

- `{{contract_path}}` — API matrix, error matrix, data-model.
- `{{architecture_root}}/test-strategy.md` — test seam 정의.
- `traceability.md` — test ↔ module ↔ contract 매핑.
