---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/_handoff.yaml
generated_date: 2026-05-14
---

# NSSF Traceability

## Source map

| source | role |
| --- | --- |
| `handoff/nssf/_handoff.yaml` | machine-readable contract handoff |
| `design/nssf/contract/interface.md` | SBI transport, auth, base URL, content type constraints |
| `design/nssf/contract/api/NSSelectionGet.md` | API method, path, request, response constraints |
| `design/nssf/contract/data-model/SliceInfoForRegistration.md` | registration request data model source |
| `design/nssf/contract/data-model/AuthorizedNetworkSliceInfo.md` | success response data model source |
| `design/nssf/contract/error-handling.md` | ProblemDetails status and cause mapping source |
| `design/nssf/architecture/` | architecture design input |

## Task traceability

| task | architecture source | contract source | tests |
| --- | --- | --- | --- |
| `nssf-contract-types` | `module-boundaries.md` | API and data model contract docs | success, invalid structured query |
| `nssf-request-validator` | `request-flow.md`, `test-strategy.md` | `api/NSSelectionGet.md`, `error-handling.md` | missing query, invalid structured query |
| `nssf-nsselection-handler` | `runtime-model.md`, `request-flow.md` | `interface.md`, `api/NSSelectionGet.md` | success registration, observability success |
| `nssf-selection-core` | `module-boundaries.md`, `state-persistence.md` | data model docs, error matrix | unauthorized NSSAI, NSSAI not available |
| `nssf-external-nf-gateway` | `runtime-model.md`, `state-persistence.md` | cross-NF boundary in handoff | downstream timeout |
| `nssf-error-mapping` | `error-propagation.md` | `error-handling.md` | 400, 403, 404, 500 tests |
| `nssf-observability` | `observability.md` | interface and error causes | observability success/failure |
| `nssf-contract-tests` | `test-strategy.md` | all contract docs | all unit and boundary tests |
| `nssf-integration-scenarios` | `request-flow.md`, `test-strategy.md` | API and error contract docs | integration scenarios |

## Test traceability

| test | requirement source | task |
| --- | --- | --- |
| `nssf-test-success-registration` | `api/NSSelectionGet.md`, `request-flow.md` | `nssf-nsselection-handler`, `nssf-selection-core` |
| `nssf-test-missing-query` | `error-handling.md`, `error-propagation.md` | `nssf-request-validator`, `nssf-error-mapping` |
| `nssf-test-invalid-structured-query` | `api/NSSelectionGet.md`, `test-strategy.md` | `nssf-contract-types`, `nssf-request-validator` |
| `nssf-test-unauthorized-nssai` | `error-handling.md`, `module-boundaries.md` | `nssf-selection-core`, `nssf-error-mapping` |
| `nssf-test-nssai-not-available` | `error-handling.md`, `state-persistence.md` | `nssf-selection-core`, `nssf-error-mapping` |
| `nssf-test-downstream-timeout` | `runtime-model.md`, `error-propagation.md` | `nssf-external-nf-gateway`, `nssf-error-mapping` |
| `nssf-test-observability-success` | `observability.md` | `nssf-observability`, `nssf-integration-scenarios` |
| `nssf-test-observability-failure` | `observability.md`, `error-propagation.md` | `nssf-observability`, `nssf-error-mapping` |

## Open trace gaps

- Concrete language/runtime, HTTP framework, cache backend, telemetry library, and test framework remain TBD.
- Full production NRF/UDM integration remains deferred beyond this MVP plan.
- PDU session path and NSSAIAvailability service are not covered by this implementation planning pass.
