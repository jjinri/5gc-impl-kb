---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Observability

## Purpose

본 문서는 log·metric·trace signal 과 redaction 경계를 정의한다 — contract 의 식별자·에러·external call 을 관측 가능하게.

## Inputs (contract)

- TODO: contract 의 request identifier, SBI header, error cause, external NF call 을 observability signal 로 연결한다.
- TODO: 개인정보 또는 가입자 식별자 redaction 요구를 적는다.

## Boundaries

- 본 문서는 *어떤 signal* 을 *어떤 redaction* 으로 낼지만 다룬다.
- telemetry 라이브러리·dashboard 제품 선택은 dev `TBD`.

## Decisions

logs.

| event | level | fields | redaction |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |

metrics.

| metric | type | labels | purpose |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |

traces.

| span | parent | key attributes |
| --- | --- | --- |
| TODO | TODO | TODO |

## Open Questions

| choice | status | constraint |
| --- | --- | --- |
| telemetry library | TBD | logs, metrics, traces 가 request path 로 correlate 되어야 한다. |
| sampling policy | TBD | error/slow path 증거가 유지되어야 한다. |
| dashboard system | TBD | contract-level SLO field 가 export 가능해야 한다. |

## References

- `{{contract_path}}` — contract handoff.
- `error-propagation.md`, `request-flow.md`, `configuration-strategy.md`.
- TODO: 인용 spec clause 를 적는다.
