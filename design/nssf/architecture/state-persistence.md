---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/_handoff.yaml
generated_date: 2026-05-14
---

# NSSF State Persistence

## Spec-derived constraints

- `SliceInfoForRegistration` 에는 requested NSSAI, subscribed NSSAI, mappingOfNssai 같은 decision input 이 포함된다.
- Subscribed NSSAI 의 source of truth 는 UDM subscription data 로 취급한다.
- NSI instance availability 의 source of truth 는 NRF or deployment inventory boundary 로 취급한다.
- NSSF MVP 자체가 반드시 영속 저장해야 하는 write path 는 contract 에서 확인되지 않는다.

## State ownership

| state | owner | read path | write path | persistence need |
| --- | --- | --- | --- | --- |
| subscribed NSSAI | UDM | External NF Gateway | outside NSSF | cache only if policy allows |
| requested NSSAI | AMF/client request | Request Validator | outside NSSF | no |
| PLMN slice policy | operator policy source | Selection Core | outside MVP | TBD |
| NSI availability | NRF or inventory | External NF Gateway | outside NSSF | cache only if policy allows |
| request trace context | NSSF runtime | inbound SBI | log/trace sink | observability retention only |

## Cache policy

- Subscribed NSSAI cache 는 expiry 와 invalidation source 가 결정되기 전까지 optional 이다.
- NSI availability cache 는 stale result 가 404 오판으로 이어질 수 있으므로 TTL 결정이 필요하다.
- OAuth token cache 는 token expiry 를 반드시 따라야 한다.

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| persistence backend | TBD | contract 에서 결정하지 않는다. |
| cache backend | TBD | source-of-truth boundary 를 침범하지 않는다. |
| schema migration strategy | TBD | data model version 추적이 가능해야 한다. |
