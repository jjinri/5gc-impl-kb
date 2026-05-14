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

이 문서는 implementation tasks 가 만족해야 하는 contract-derived test surface 를 정의한다.

## Test cases

| id | type | scenario | input | expected | trace |
| --- | --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO | TODO |

## Coverage rules

- API success path 를 최소 1개 이상 포함한다.
- Error matrix 의 각 major status code 를 포함한다.
- Data model validation 과 ProblemDetails mapping 을 분리해 검증한다.
- External NF boundary failure 를 mock 또는 contract test 로 검증한다.
