---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: '2026-05-26'
generator: design/scripts/nf-readiness-pack-generate.py
source_readiness_config: design/nssf/readiness-config.yaml
generated_sections:
  - unit-tests-table
  - integration-tests-table
  - security-gate-matrix
user_sections:
  - intro-note
  - unit-body
  - integration-body
  - contract-body
  - security-body
  - end-to-end-body
  - observability-body
  - references-body
---

# NSSF Verification Plan

<!-- USER:intro-note:start -->
`/nf-implement nssf` 의 verification gate 6종. 각 gate 는 unit/integration/contract/security/e2e/observability 한 종류씩 책임. `codegen-work-items.yaml` 의 work item 별 `tests` / `verification_commands` 가 본 plan 의 gate 를 가리킨다.
<!-- USER:intro-note:end -->

## Unit

<!-- USER:unit-body:start -->
- 범위 — 개별 module 함수·class·utility (`SelectionEngine`, `AvailabilityEngine`, `SubscriptionStore`, `NotificationDispatcher`, ProblemDetails/NFType wrapper, repository mock, TLS context, OAuth2 scope check).
- pass 조건 — line coverage ≥ 80% (NSSelectionGet path 는 ≥ 90%), business-rule branch coverage 100%, 모든 unit test green.
- tooling — `engineering/nssf/engineering-design.md` `test_framework` slot 에 따라 CTest + unity (또는 동등). cJSON serializer round-trip 은 generated struct 단위 test (`WI-codegen-bootstrap`).
- 핵심 gate command — `ctest --test-dir build -R nssf_.*_unit` PASS.
- evidence — `build/Testing/Temporary/LastTest.log` 의 unit 섹션, coverage report (`gcov` / `llvm-cov`).
<!-- USER:unit-body:end -->

<!-- AUTO:unit-tests-table:start -->
### Unit Tests Inventory

`dev/nssf/test-matrix.md` `## Test Inventory` 의 kind=unit row derive — 총 12개. test-matrix 변경 시 본 sub-section 이 같이 갱신된다.

| id | scenario | refs |
|---|---|---|
| `t-selection-invalid-query` | invalid-structured-query | `api/NSSelectionGet`, `error-propagation.md` |
| `t-selection-unauthorized` | unauthorized-nssai | `SelectionEngine.md`, `error-propagation.md` |
| `t-selection-not-available` | nssai-not-available | `SelectionEngine.md`, `AvailabilityEngine.md` |
| `t-availability-put-invalid-body` | availability-put-invalid-body | `api/NSSAIAvailabilityPut`, `error-propagation.md`, `request-flow.md` |
| `t-availability-patch-conflict` | availability-patch-conflict (etag 옵션 지원 시) | `api/NSSAIAvailabilityPatch`, `error-propagation.md` |
| `t-availability-delete-not-found` | availability-delete-not-found | `api/NSSAIAvailabilityDelete`, `error-propagation.md` |
| `t-availability-options` | availability-options-supported-features | `api/NSSAIAvailabilityOptions`, `configuration-strategy.md` |
| `t-availability-options-unauthorized` | availability-options-unauthorized | `api/NSSAIAvailabilityOptions`, `error-propagation.md`, `interface` |
| `t-subscription-create-invalid-callback` | subscription-create-invalid-callback | `api/NSSAIAvailabilityPost`, `error-propagation.md`, `SubscriptionStore.md` |
| `t-subscription-modify` | subscription-modify-filter | `api/NSSAIAvailabilitySubModifyPatch`, `SubscriptionStore.md` |
| `t-subscription-modify-not-found` | subscription-modify-not-found | `api/NSSAIAvailabilitySubModifyPatch`, `error-propagation.md` |
| `t-subscription-unsubscribe-not-found` | subscription-unsubscribe-not-found | `api/NSSAIAvailabilityUnsubscribe`, `error-propagation.md` |
<!-- AUTO:unit-tests-table:end -->

## Integration

<!-- USER:integration-body:start -->
- 범위 — module 간 SBI/service 인터페이스 (handler → engine → repository), PostgreSQL backend, configuration loading, retry_queue dequeue.
- pass 조건 — PostgreSQL fixture 가 schema.sql 적용 후 round-trip 검증 (find/upsert/patch/delete), nghttp2 server 가 NSSelectionGet 응답 schema 일치, NotificationDispatcher 가 mocked peer 로 outbound dispatch.
- tooling — CTest integration label, docker-compose 또는 testcontainers 로 PostgreSQL fixture.
- 핵심 gate command — `ctest --test-dir build -L integration -R nssf_` PASS.
- evidence — fixture PostgreSQL log, integration test stdout, retry_queue row count assertion.
<!-- USER:integration-body:end -->

<!-- AUTO:integration-tests-table:start -->
### Integration Tests Inventory

`dev/nssf/test-matrix.md` `## Test Inventory` 의 kind=integration row derive — 총 9개. test-matrix 변경 시 본 sub-section 이 같이 갱신된다.

| id | scenario | refs |
|---|---|---|
| `t-selection-success` | success-registration-selection | `api/NSSelectionGet`, `SelectionEngine.md`, `request-flow.md` |
| `t-availability-put` | availability-put-creates-record | `api/NSSAIAvailabilityPut`, `AvailabilityEngine.md`, `request-flow.md` |
| `t-availability-patch` | availability-patch-applies | `api/NSSAIAvailabilityPatch`, `AvailabilityEngine.md` |
| `t-availability-delete` | availability-delete | `api/NSSAIAvailabilityDelete`, `AvailabilityEngine.md` |
| `t-subscription-create` | subscription-create | `api/NSSAIAvailabilityPost`, `SubscriptionStore.md` |
| `t-subscription-notify` | subscription-create-and-notify | `api/NSSAIAvailabilityPost`, `SubscriptionStore.md`, `NotificationDispatcher.md`, `observability.md` |
| `t-subscription-unsubscribe` | subscription-unsubscribe | `api/NSSAIAvailabilityUnsubscribe`, `SubscriptionStore.md` |
| `t-notify-retry` | notification-retry-on-5xx | `NotificationDispatcher.md`, `error-propagation.md`, `configuration-strategy.md` |
| `t-notify-dead-letter` | notification-dead-letter | `NotificationDispatcher.md`, `error-propagation.md` |
<!-- AUTO:integration-tests-table:end -->

## Contract

<!-- USER:contract-body:start -->
- 범위 — `handoff/nssf/contract.yaml` 정합성, problem-details matrix (8 op × 18 cause), error response schema, response schema byte-level alignment.
- pass 조건 — 모든 API operation × cause matrix 1회 이상 실행, response 가 generated struct serializer 산출과 byte-level 일치.
- tooling — `WI-contract-tests` 의 `test_handoff_contract_alignment.c` + `test_error_matrix.c`. 추가 contract test runner (예 schemathesis) 도입 시 engineering-design 에 freeze.
- 핵심 gate command — `ctest --test-dir build -R nssf_contract_` PASS.
- evidence — operation × cause matrix 적합 출력 (모든 cell green), schema diff report 0건.
<!-- USER:contract-body:end -->

## Security

<!-- USER:security-body:start -->
- 범위 — ADR-0004 security baseline 7 항목.
  - (1) TLS 의무 (mode=enabled, app_library, openssl backend)
  - (2) inbound OAuth2 토큰 검증 (jwks, libjwt)
  - (3) outbound OAuth2 client credentials (sbi_client_stack)
  - (4) dev disable 한정 (dev profile 만 mTLS off 허용)
  - (5) library mandate (TLS/OAuth2 라이브러리 sub-300 LOC handcraft 금지)
  - (6) operator-provided cert/JWKS/scope 의존 (G-01, G-06 gap)
  - (7) production-capable evidence (mocked peer / mocked JWKS server fixture 로 fail-closed 검증)
- pass 조건 — 각 baseline 항목별 verification command 가 evidence emit, fail-closed 시나리오 (scope mismatch, 만료 cert, broken JWKS) 가 401/403 으로 reject.
- tooling — `WI-security-tests` 의 4 test file + OpenSSL probe + mocked JWKS server fixture.
- 핵심 gate command — `ctest --test-dir build -R nssf_security_` PASS.
- evidence — 7 항목 별 `baseline_*.log` 의 PASS marker, fail-closed 시나리오의 401/403 status code.
<!-- USER:security-body:end -->

<!-- AUTO:security-gate-matrix:start -->
### Security Gate Matrix

`design/policies/security-baseline.yaml` `baseline_mandates` (ADR-0004) derive — verification 적용 매핑. policy 변경 시 본 matrix 갱신. 각 mandate 행의 `verification.kind` / `verification.test_id_prefix` 가 source.

| mandate | name | verification kind | test id prefix |
|---|---|---|---|
| `M1` | `internal_https_tls_code_path` | security | t-tls-* |
| `M2` | `mtls_code_path` | security | t-mtls-* |
| `M3` | `inbound_oauth2_bearer_validation_code_path` | security | t-oauth2-inbound-* |
| `M4` | `outbound_oauth2_token_attach_code_path` | security | t-oauth2-outbound-* |
| `M5` | `dev_disable_with_production_path_present` | security | (test-matrix kind=security 종합) |
| `M6` | `third_party_library_mandate` | code review | — |
| `M7` | `profile_spec_depth_externalized` | operator/library 외부 처리 | — |
<!-- AUTO:security-gate-matrix:end -->

## End-to-End

<!-- USER:end-to-end-body:start -->
- 범위 — service flow 시나리오 (`design/nssf/architecture/request-flow.md` 의 핵심 시나리오), multi-NF interaction (mocked AMF peer).
  - UE 등록 → NSSelectionGet → AuthorizedNetworkSliceInfo 응답.
  - AMF NSSAIAvailabilityPut → NotificationDispatcher → mocked AMF callback.
  - subscription Post → initial snapshot dispatch → subsequent change notify.
- pass 조건 — 각 시나리오 happy path + 핵심 edge (cert reject, availability cascade delete) green.
- tooling — `WI-e2e-tests` 의 e2e test file + mocked AMF fixture.
- 핵심 gate command — `ctest --test-dir build -R nssf_e2e_` PASS.
- evidence — 시나리오 별 e2e log + retry_queue dispatch evidence.
<!-- USER:end-to-end-body:end -->

## Observability

<!-- USER:observability-body:start -->
- 범위 — `design/nssf/architecture/observability.md` 의 metric/log/trace seam.
  - metric — `nssf_nsselection_requests_total` / `nssf_availability_writes_total` / `nssf_subscription_active_count` / `nssf_notification_retry_queue_depth`.
  - log — structured JSON (level, op_id, nf_instance_id, sub_id, cause).
  - trace — OpenTelemetry compatible span (`SelectionEngine.select`, `AvailabilityRepository.*`, `NotificationDispatcher.dispatch`).
- pass 조건 — 각 seam 이 production-capable signal emit (label cardinality bounded, log redaction 적용, trace sampler config-driven), dashboard query 검증.
- tooling — `WI-observability-finalize` 의 metrics/log/trace 모듈, observability seam unit test.
- 핵심 gate command — `ctest --test-dir build -R nssf_observability_` PASS.
- evidence — metric emit dump, log redaction 검증 출력, trace span graph 캡처.
<!-- USER:observability-body:end -->

## References

<!-- USER:references-body:start -->
- `design/nssf/architecture/test-strategy.md`, `design/nssf/architecture/observability.md`.
- `engineering/nssf/engineering-design.md` — test/verification tooling freeze.
- `docs/adr/ADR-0004-project-security-baseline.md` — 7 항목 baseline.
- `codegen-work-items.yaml` — work item 의 `tests` / `verification_commands` 가 본 plan 의 gate 를 가리킨다.
- `open-gaps-and-assumptions.md` — G-13 (peer NF interop test-gap), G-14 (dashboard tuning deferred).
<!-- USER:references-body:end -->
