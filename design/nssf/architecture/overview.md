---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
contract_status: design/nssf/_contract_status.yaml
generated_date: 2026-05-14
---

# NSSF Architecture Overview

## Purpose

이 문서는 handoff-ready NSSF contract 를 상세 아키텍처 설계로 변환한다.
구현 언어, 런타임, DBMS, 배포 방식, 라이브러리 선택은 dev 단계 결정으로 남긴다.

## Source contract

- Contract handoff: `handoff/nssf/contract.yaml`
- Contract status: `design/nssf/_contract_status.yaml`
- Contract artifacts: `design/nssf/contract/`

## Spec-derived constraints

- NSSF MVP 범위는 `NSSelectionGet` 하나의 GET operation 이다.
- API path 는 `/network-slice-information` 이며 base URL 은 `{apiRoot}/nnssf-nsselection/<apiVersion>` 이다.
- 인증 scope 는 `nnssf-nsselection` 이고 OAuth 2.0 client credentials 흐름을 따른다.
- 정상 응답은 `AuthorizedNetworkSliceInfo` 이고 주요 요청 입력은 `SliceInfoForRegistration` 이다.
- 오류 응답은 `application/problem+json` 과 ProblemDetails shape 를 보존해야 한다.
- 주요 error cause 는 `INVALID_QUERY_PARAM`, `UNAUTHORIZED_NSSAI`, `NSSAI_NOT_AVAILABLE`, `SYSTEM_FAILURE` 이다.
- SBI transport 는 HTTP/2 over TLS 와 `3gpp-Sbi-*` header 전달을 전제로 한다.

## Architecture scope

이번 architecture pass 는 registration path 의 NSSelectionGet 처리, request validation, slice selection core, ProblemDetails mapping, external NF boundary 를 다룬다.
PDU session path, NSSAIAvailability service, 실제 NRF/UDM client 구현, deployment topology 는 후속 단계로 남긴다.

## Implementation choices

| decision | status | owner | note |
| --- | --- | --- | --- |
| language/runtime | TBD | dev | contract 에서 결정하지 않는다. |
| persistence backend | TBD | dev | NSSF 자체 영속 상태가 필요한지 dev 단계에서 확정한다. |
| HTTP/SBI framework | TBD | dev | HTTP/2, TLS, JSON, ProblemDetails shape 를 보존해야 한다. |
| deployment topology | TBD | dev | service mesh 또는 NF 내장 TLS 모두 가능하다. |

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
