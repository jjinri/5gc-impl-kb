---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/_handoff.yaml
generated_date: 2026-05-14
---

# NSSF Observability

## Spec-derived constraints

- `nf-id`, `nf-type`, `tai`, requested NSSAI count, response status, ProblemDetails cause 는 request diagnosis 에 필요하다.
- Subscriber identifier 또는 UE 식별자로 해석될 수 있는 값은 redaction 대상이다.
- External NF lookup 은 request trace 안에서 별도 span 으로 관찰되어야 한다.

## Logs

| event | level | fields | redaction |
| --- | --- | --- | --- |
| request accepted | info | operation, nf-type, correlation id | nf-id hash recommended |
| validation failed | warn | missing fields, invalid field names, cause | raw value omit |
| selection rejected | info | cause, requested count, policy source | S-NSSAI value policy dependent |
| downstream failure | error | dependency, timeout, cause | identifiers omit |
| internal failure | error | error class, correlation id | stack trace policy TBD |

## Metrics

| metric | type | labels | purpose |
| --- | --- | --- | --- |
| `nssf_requests_total` | counter | operation, status | traffic and error ratio |
| `nssf_request_duration_ms` | histogram | operation, status | latency budget tracking |
| `nssf_validation_failures_total` | counter | cause | client integration quality |
| `nssf_selection_rejections_total` | counter | cause | policy and availability visibility |
| `nssf_downstream_failures_total` | counter | dependency, cause | external NF health |

## Traces

| span | parent | key attributes |
| --- | --- | --- |
| inbound NSSelectionGet | root | operation, status, cause |
| validate request | inbound NSSelectionGet | invalid field count |
| select NSSAI | inbound NSSelectionGet | requested count, allowed count, rejected count |
| external NF lookup | select NSSAI | dependency, timeout, result |
| build response | inbound NSSelectionGet | response schema |

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| telemetry library | TBD | logs, metrics, traces can correlate request path. |
| sampling policy | TBD | error and slow path evidence is retained. |
| dashboard system | TBD | contract-level SLO fields are exportable. |
