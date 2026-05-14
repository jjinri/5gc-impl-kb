---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Error Propagation

## Spec-derived constraints

- TODO: contract error matrix 의 HTTP status, ProblemDetails.cause, recovery guidance 를 적는다.
- TODO: validation error 와 downstream error 의 mapping 차이를 적는다.

## Error map

| source | condition | response | retryable | observability |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

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
