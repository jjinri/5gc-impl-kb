---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Request Flow

## Spec-derived constraints

- TODO: contract API matrix 의 method, path, auth scope, request schema, response schema 를 적는다.
- TODO: error-handling contract 의 status code 와 cause mapping 을 적는다.

## Main sequence

```mermaid
sequenceDiagram
    participant Client
    participant Handler
    participant Validator
    participant Core
    participant ExternalNF
    participant ResponseBuilder

    Client->>Handler: TODO request
    Handler->>Validator: parse and validate contract fields
    Validator->>Core: validated command
    Core->>ExternalNF: optional contract-derived lookup
    ExternalNF-->>Core: lookup result or failure
    Core->>ResponseBuilder: domain result
    ResponseBuilder-->>Client: contract response
```

## Failure sequence

- TODO: invalid query 또는 schema violation 이 어떤 module 에서 ProblemDetails 로 변환되는지 적는다.
- TODO: external NF failure 가 retry, fallback, or terminal error 중 무엇으로 분류되는지 적는다.

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| sync vs async handler model | TBD | externally visible API semantics 를 바꾸지 않는다. |
| timeout budget split | TBD | contract 의 timeout/retry guidance 와 충돌하지 않는다. |
| serialization library | TBD | OpenAPI schema 와 ProblemDetails shape 를 보존한다. |
