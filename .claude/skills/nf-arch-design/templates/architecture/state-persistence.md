---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} State Persistence

## Spec-derived constraints

- TODO: contract 가 persistent state 를 요구하는지 적는다.
- TODO: request/response schema 에 포함되지만 source of truth 가 external NF 인 field 를 적는다.

## State ownership

| state | owner | read path | write path | persistence need |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

## Cache policy

- TODO: cache 가능한 data 와 cache 하면 안 되는 data 를 분리한다.
- TODO: invalidation source 와 TTL 결정이 필요한 항목을 적는다.

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| persistence backend | TBD | contract 에서 결정하지 않는다. |
| cache backend | TBD | source-of-truth boundary 를 침범하지 않는다. |
| schema migration strategy | TBD | data model version 추적이 가능해야 한다. |
