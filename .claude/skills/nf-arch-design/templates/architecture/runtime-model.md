---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Runtime Model

## Purpose

본 문서는 동시성·격리·실패 격리 등 runtime boundary 제약을 정의한다. 구체 process/thread 구현은 dev 단계.

## Inputs (contract)

- TODO: contract 의 idempotency, authentication, SBI header, content-type 제약을 적는다.
- TODO: external NF interaction 이 있는 경우 timeout, retry, failure isolation 요구를 적는다.

## Boundaries

- 본 문서는 runtime *경계 제약* 만 다룬다 — 구체 동시성 라이브러리·런타임은 dev `TBD`.
- 상태 소유·영속은 `state-persistence.md` 가 다룬다.
- error 분류는 `error-propagation.md` 가 다룬다.

## Decisions

| boundary | responsibility | concurrency concern | failure mode |
| --- | --- | --- | --- |
| inbound SBI | TODO | TODO | TODO |
| contract validation | TODO | TODO | TODO |
| core logic | TODO | TODO | TODO |
| external NF client | TODO | TODO | TODO |

## Open Questions

| choice | status | constraint |
| --- | --- | --- |
| process/thread model | TBD | request isolation 과 cancellation 을 보존해야 한다. |
| in-memory cache policy | TBD | stale data 허용 여부를 contract 근거로 표시해야 한다. |
| backpressure mechanism | TBD | 5xx error mapping 과 observability 를 보존해야 한다. |

## References

- `{{contract_path}}` — contract handoff.
- `state-persistence.md`, `error-propagation.md`, `request-flow.md`.
- TODO: 인용 spec clause (SBI 공통 29.500 등) 를 적는다.
