---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/_handoff.yaml
generated_date: 2026-05-14
---

# NSSF Test Matrix

## Purpose

이 문서는 implementation tasks 가 만족해야 하는 contract-derived test surface 를 정의한다.
테스트 framework 와 CI 위치는 dev 단계에서 결정한다.

## Test cases

| id | type | scenario | input | expected | trace |
| --- | --- | --- | --- | --- | --- |
| `nssf-test-success-registration` | integration | valid registration NSSelectionGet | required query plus valid `SliceInfoForRegistration` | 200 `AuthorizedNetworkSliceInfo` | `design/nssf/contract/api/NSSelectionGet.md`, `design/nssf/architecture/request-flow.md` |
| `nssf-test-missing-query` | unit/integration | required query missing | missing `nf-id` or `tai` | 400 `INVALID_QUERY_PARAM` with `invalidParams` | `design/nssf/contract/error-handling.md`, `design/nssf/architecture/error-propagation.md` |
| `nssf-test-invalid-structured-query` | unit | invalid structured JSON query | malformed registration slice info query value | 400 `INVALID_QUERY_PARAM` | `design/nssf/contract/api/NSSelectionGet.md`, `design/nssf/architecture/test-strategy.md` |
| `nssf-test-unauthorized-nssai` | unit | requested NSSAI outside policy | requested set not allowed by policy view | 403 `UNAUTHORIZED_NSSAI` | `design/nssf/contract/error-handling.md`, `design/nssf/architecture/module-boundaries.md` |
| `nssf-test-nssai-not-available` | unit | allowed slice has no NSI | valid request with no matching NSI instance | 404 `NSSAI_NOT_AVAILABLE` | `design/nssf/contract/error-handling.md`, `design/nssf/architecture/state-persistence.md` |
| `nssf-test-downstream-timeout` | boundary/integration | external NF lookup timeout | mocked UDM or NRF timeout | 500 `SYSTEM_FAILURE` or documented fallback | `design/nssf/architecture/runtime-model.md`, `design/nssf/architecture/error-propagation.md` |
| `nssf-test-observability-success` | integration | successful request emits telemetry | valid success scenario | request metric, trace spans, redacted logs exist | `design/nssf/architecture/observability.md` |
| `nssf-test-observability-failure` | integration | failure request emits telemetry | validation or downstream failure | cause-labelled metric and sanitized log exist | `design/nssf/architecture/observability.md` |

## Coverage rules

- API success path 를 최소 1개 이상 포함한다.
- Error matrix 의 400, 403, 404, 500 paths 를 포함한다.
- Data model validation 과 ProblemDetails mapping 을 분리해 검증한다.
- External NF boundary failure 를 mock 또는 contract test 로 검증한다.
- Observability 와 redaction evidence 를 success/failure 양쪽에서 확인한다.
