---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Runtime Model

## Spec-derived constraints

- TODO: contract 의 idempotency, authentication, SBI header, content-type 제약을 적는다.
- TODO: external NF interaction 이 있는 경우 timeout, retry, and failure isolation 요구를 적는다.

## Runtime boundaries

| boundary | responsibility | concurrency concern | failure mode |
| --- | --- | --- | --- |
| inbound SBI | TODO | TODO | TODO |
| contract validation | TODO | TODO | TODO |
| core selection logic | TODO | TODO | TODO |
| external NF client | TODO | TODO | TODO |

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| process/thread model | TBD | request isolation 과 cancellation 을 보존해야 한다. |
| in-memory cache policy | TBD | stale data 허용 여부를 contract 근거로 표시해야 한다. |
| backpressure mechanism | TBD | 5xx error mapping 과 observability 를 보존해야 한다. |
