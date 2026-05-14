---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Observability

## Spec-derived constraints

- TODO: contract 의 request identifiers, SBI headers, error causes, external NF calls 를 observability signal 로 연결한다.
- TODO: 개인정보 또는 가입자 식별자 redaction 요구를 적는다.

## Logs

| event | level | fields | redaction |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |

## Metrics

| metric | type | labels | purpose |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |

## Traces

| span | parent | key attributes |
| --- | --- | --- |
| TODO | TODO | TODO |

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| telemetry library | TBD | logs, metrics, traces can correlate request path. |
| sampling policy | TBD | error and slow path evidence is retained. |
| dashboard system | TBD | contract-level SLO fields are exportable. |
