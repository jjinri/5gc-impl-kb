---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/_handoff.yaml
generated_date: 2026-05-14
---

# NSSF Implementation Plan

## Purpose

이 문서는 NSSF architecture design 을 실제 구현 작업 단위로 변환한다.
소스 코드, dependency file, build system 은 이 단계에서 만들지 않는다.

## Inputs

- Architecture root: `design/nssf/architecture`
- Contract handoff: `handoff/nssf/_handoff.yaml`
- Contract artifacts: `design/nssf/contract/`

## Scope

- 이번 implementation plan 은 `NSSelectionGet` registration path MVP 를 다룬다.
- 포함 범위는 inbound SBI handler, request validation, Selection Core, external NF boundary abstraction, response/error mapping, observability, test seams 이다.
- 제외 범위는 PDU session path, NSSAIAvailability service, 실제 NRF/UDM production client, deployment topology, 구체 runtime/framework 선택이다.

## Workstreams

| stream | purpose | primary tasks |
| --- | --- | --- |
| Contract boundary | API schema, request parsing, response shape 를 고정 | `nssf-contract-types`, `nssf-request-validator`, `nssf-nsselection-handler` |
| Domain logic | requested/subscribed/policy NSSAI decision 을 구현 가능한 core 로 분리 | `nssf-selection-core` |
| Integration boundary | UDM/NRF lookup 을 mock 가능한 port 로 정의 | `nssf-external-nf-gateway` |
| Failure and telemetry | ProblemDetails mapping, logs, metrics, traces 를 정의 | `nssf-error-mapping`, `nssf-observability` |
| Verification | contract-derived test matrix 를 구현 준비 상태로 닫음 | `nssf-contract-tests`, `nssf-integration-scenarios` |

## Technology decisions

| decision | status | owner | blocking tasks |
| --- | --- | --- | --- |
| language/runtime | TBD | dev | concrete package layout, build system |
| HTTP/SBI framework | TBD | dev | handler implementation details |
| persistence/cache backend | TBD | dev | subscription and NSI cache implementation |
| telemetry stack | TBD | dev | metrics/tracing exporters |
| schema generation strategy | TBD | dev | generated vs hand-written data model types |

## Delivery order

1. Establish contract type boundary and request validation skeleton.
2. Implement Selection Core as transport-independent domain logic.
3. Add response builder and deterministic ProblemDetails mapping.
4. Add external NF gateway interfaces with mock implementations.
5. Add observability events, metrics, and trace spans.
6. Close contract-derived unit, integration, and boundary tests.

## Exit criteria

- Every task in `tasks.yaml` is done or explicitly deferred.
- Every acceptance test in `test-matrix.md` has an implementation owner.
- Every task and test links back to contract or architecture evidence in `traceability.md`.
- Unresolved technology choices are explicitly marked before implementation begins.
