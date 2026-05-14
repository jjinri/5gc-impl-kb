---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Error Propagation

## Spec-derived constraints

- Error response content type 은 `application/problem+json` 이다.
- ProblemDetails `cause` 는 TS 29.500 generic causes 와 TS 29.531 NSSF-specific causes 를 허용한다.
- 가능한 invalid query 는 `invalidParams` 에 최대한 함께 담아 client round trip 을 줄인다.

## Error map

| source | condition | response | retryable | observability |
| --- | --- | --- | --- | --- |
| Request Validator | required query 누락 또는 형식 오류 | 400 `INVALID_QUERY_PARAM` | no | validation failure counter |
| Selection Core | requested NSSAI 가 PLMN policy 밖 | 403 `UNAUTHORIZED_NSSAI` | no | policy rejection counter |
| Selection Core | matching NSI instance 부재 | 404 `NSSAI_NOT_AVAILABLE` | maybe fallback | availability miss counter |
| External NF Gateway | UDM/NRF timeout 또는 invalid response | 500 `SYSTEM_FAILURE` | yes with backoff | downstream failure counter and trace span |
| Internal invariant | 예상하지 못한 core exception | 500 `SYSTEM_FAILURE` | yes with backoff | sanitized error log |

## Propagation rules

- Contract validation error 는 client-correctable error 로 분류한다.
- Contract policy rejection 은 authorization or availability error 로 분류한다.
- Internal invariant violation 은 sanitized ProblemDetails 로 변환한다.
- External NF failure 는 timeout, unavailable, invalid response 를 구분한다.

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| exception/error type model | TBD | ProblemDetails mapping 이 deterministic 해야 한다. |
| retry policy implementation | TBD | non-retryable client error 를 재시도하지 않는다. |
| redaction policy | TBD | sensitive identifier 를 log 에 그대로 남기지 않는다. |
