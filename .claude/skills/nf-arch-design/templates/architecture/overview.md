---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
contract_status: {{status_path}}
generated_date: {{generated_date}}
---

# {{NF}} Architecture Overview

## Purpose

이 문서는 handoff-ready contract 를 상세 아키텍처 설계로 변환한다.
구현 언어, 런타임, DBMS, 배포 방식, 라이브러리 선택은 dev 단계 결정으로 남긴다.

## Source contract

- Contract handoff: `{{contract_path}}`
- Contract status: `{{status_path}}`
- Contract artifacts: `design/{{nf}}/contract/`

## Spec-derived constraints

- TODO: contract 의 API, data model, error matrix, cross-NF 제약을 요약한다.
- TODO: architecture 가 반드시 보존해야 하는 3GPP field, status code, cause, SBI header 제약을 적는다.

## Architecture scope

- TODO: 이번 architecture pass 가 다루는 API operation 과 제외 범위를 적는다.
- TODO: 추후 NF service 확장 시 다시 열어야 하는 경계를 적는다.

## Implementation choices

| decision | status | owner | note |
| --- | --- | --- | --- |
| language/runtime | TBD | dev | contract 에서 결정하지 않는다. |
| persistence backend | TBD | dev | contract 에서 결정하지 않는다. |
| HTTP/SBI framework | TBD | dev | contract 에서 결정하지 않는다. |
| deployment topology | TBD | dev | contract 에서 결정하지 않는다. |

## Output map

| file | purpose |
| --- | --- |
| `module-boundaries.md` | logical module responsibility and seams |
| `request-flow.md` | request processing sequence |
| `runtime-model.md` | concurrency and runtime boundary constraints |
| `state-persistence.md` | state ownership and persistence requirements |
| `configuration-strategy.md` | config keys and policy boundaries |
| `error-propagation.md` | error mapping and recovery behavior |
| `observability.md` | logs, metrics, traces, audit points |
| `test-strategy.md` | architecture-level test seams |
