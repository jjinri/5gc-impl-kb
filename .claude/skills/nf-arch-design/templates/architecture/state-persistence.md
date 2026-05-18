---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} State Persistence

## Purpose

본 문서는 어떤 상태를 누가 소유하고, 어디서 읽고 쓰며, 영속이 필요한지 정의한다. 구체 backend 는 dev 단계.

## Inputs (contract)

- TODO: contract 가 persistent state 를 요구하는지 적는다.
- TODO: request/response schema 에 포함되지만 source of truth 가 external NF 인 field 를 적는다.

## Boundaries

- 본 문서는 상태 *소유·영속 요구* 만 다룬다 — backend 제품 선택은 dev `TBD`.
- 상태에 대한 동시 접근 격리는 `runtime-model.md` 가 다룬다.

## Decisions

state ownership.

| state | owner | read path | write path | persistence need |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

cache policy.

- TODO: cache 가능한 data 와 cache 하면 안 되는 data 를 분리한다.
- TODO: invalidation source 와 TTL 결정이 필요한 항목을 적는다.

## Open Questions

| choice | status | constraint |
| --- | --- | --- |
| persistence backend | TBD | contract 에서 결정하지 않는다. |
| cache backend | TBD | source-of-truth boundary 를 침범하지 않는다. |
| schema migration strategy | TBD | data model version 추적이 가능해야 한다. |

## References

- `{{contract_path}}` — contract handoff.
- `runtime-model.md`, `module-boundaries.md`.
- TODO: 인용 spec clause (data model chain target spec 등) 를 적는다.
