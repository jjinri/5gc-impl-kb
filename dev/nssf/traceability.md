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
  - references
user_sections:
  - intro-note
  - contract-module-table
  - module-test-table
  - open-gaps
---

# NSSF Traceability

<!-- USER:intro-note:start -->
> **Source-of-truth** (resolved 2026-05-21). 본 traceability 의 TLS / mTLS / inbound OAuth2 / outbound OAuth2 production-capable code path 매핑은 ADR-0004 baseline + architecture + engineering-design (TLS=OpenSSL app_library, OAuth2=libjwt, sbi_client_stack=nghttp2 outbound) 3 source 정합. PostgreSQL/libpq, nghttp2 server, openapi-generator+cJSON, retry 10/60 등은 기존 engineering-design ratify 유지. codegen 사이클 진입 가능 — `nf-eng-status` `eng_frozen` PASS (GO 신호) 후.
<!-- USER:intro-note:end -->

## Contract → Module

<!-- USER:contract-module-table:start -->
8 operation 과 핵심 data-model 이 어느 모듈로 매핑되는지.

| contract topic | 모듈 | task |
|---|---|---|
| `api/NSSelectionGet` | SelectionEngine | nssf-selection-handler, nssf-selection-core |
| `api/NSSAIAvailabilityPut` | AvailabilityEngine | nssf-availability-engine |
| `api/NSSAIAvailabilityPatch` | AvailabilityEngine | nssf-availability-engine |
| `api/NSSAIAvailabilityDelete` | AvailabilityEngine | nssf-availability-engine |
| `api/NSSAIAvailabilityOptions` | AvailabilityEngine | nssf-availability-engine |
| `api/NSSAIAvailabilityPost` (Subscribe) | SubscriptionStore | nssf-subscription-store |
| `api/NSSAIAvailabilitySubModifyPatch` | SubscriptionStore | nssf-subscription-store |
| `api/NSSAIAvailabilityUnsubscribe` | SubscriptionStore | nssf-subscription-store |
| (outbound NSSAIAvailability Notify) | NotificationDispatcher | nssf-notify-dispatcher |
| `interface` | (공통 transport) | nssf-transport-server |
| `error-handling` | (공통 utility) | nssf-problem-details-mapper |
| `data-model/SliceInfoForRegistration` · `…ForPDUSession` · `…ForUEConfigurationUpdate` | SelectionEngine | nssf-selection-core |
| `data-model/AuthorizedNetworkSliceInfo` | SelectionEngine | nssf-selection-handler |
| `data-model/NssaiAvailabilityInfo` · `AuthorizedNssaiAvailabilityInfo` | AvailabilityEngine | nssf-availability-engine |
| `data-model/NssfEventSubscriptionCreateData` · `NssfEventSubscriptionCreatedData` | SubscriptionStore | nssf-subscription-store |
| `data-model/PatchDocument` | (request validator) | nssf-request-validator |
| `data-model/Snssai` · `Tai` · `PlmnId` · `NfInstanceId` · `NFType` · `SupportedFeatures` | (공통 type) | nssf-contract-types / nssf-request-validator |
| (outbound UDM SDM / NRF NFDiscovery — gateway 추상화) | SelectionEngine | nssf-external-nf-gateway |
<!-- USER:contract-module-table:end -->

## Module → Test

<!-- USER:module-test-table:start -->
각 모듈/task 가 어느 test 시나리오로 검증되는지.

| task | test 시나리오 | test id (test-matrix.md) |
|---|---|---|
| nssf-selection-handler · nssf-selection-core | success-registration-selection | t-selection-success |
| nssf-selection-handler | invalid-structured-query | t-selection-invalid-query |
| nssf-selection-core | unauthorized-nssai | t-selection-unauthorized |
| nssf-selection-core | nssai-not-available | t-selection-not-available |
| nssf-availability-engine | availability-put-creates-record | t-availability-put |
| nssf-availability-engine + nssf-request-validator | availability-put-invalid-body | t-availability-put-invalid-body |
| nssf-availability-engine | availability-patch-applies | t-availability-patch |
| nssf-availability-engine | availability-patch-conflict (옵션) | t-availability-patch-conflict |
| nssf-availability-engine | availability-delete | t-availability-delete |
| nssf-availability-engine | availability-delete-not-found | t-availability-delete-not-found |
| nssf-availability-engine | availability-options-supported-features | t-availability-options |
| nssf-transport-server + nssf-availability-engine | availability-options-unauthorized | t-availability-options-unauthorized |
| nssf-subscription-store | subscription-create | t-subscription-create |
| nssf-subscription-store + nssf-notify-dispatcher | subscription-create-and-notify | t-subscription-notify |
| nssf-subscription-store + nssf-request-validator | subscription-create-invalid-callback | t-subscription-create-invalid-callback |
| nssf-subscription-store | subscription-modify-filter | t-subscription-modify |
| nssf-subscription-store | subscription-modify-not-found | t-subscription-modify-not-found |
| nssf-subscription-store | subscription-unsubscribe | t-subscription-unsubscribe |
| nssf-subscription-store | subscription-unsubscribe-not-found | t-subscription-unsubscribe-not-found |
| nssf-notify-dispatcher | notification-retry-on-5xx | t-notify-retry |
| nssf-notify-dispatcher | notification-dead-letter | t-notify-dead-letter |
| nssf-notify-dispatcher + nssf-transport-server | oauth2-client-credentials-enabled | t-notify-oauth2-enabled |
| nssf-notify-dispatcher + nssf-transport-server | oauth2-client-credentials-disabled | t-notify-oauth2-disabled |
| nssf-observability + (전체) | correlation-end-to-end | t-correlation-e2e |
| nssf-scenario-graceful-shutdown | graceful-shutdown-drain | t-shutdown-drain |
| nssf-repo-availability · nssf-repo-subscription | repository interface contract (PostgreSQL backend) | t-repo-contract |
| nssf-transport-server | tls-handshake-success | t-tls-handshake-success |
| nssf-transport-server | tls-handshake-fail | t-tls-handshake-fail |
| nssf-transport-server | mtls-peer-verify-success | t-mtls-peer-verify-success |
| nssf-transport-server | mtls-peer-reject | t-mtls-peer-reject |
| nssf-transport-server | oauth2-inbound-valid-bearer | t-oauth2-inbound-valid-bearer |
| nssf-transport-server | oauth2-inbound-missing-token | t-oauth2-inbound-missing-token |
| nssf-transport-server | oauth2-inbound-invalid-token | t-oauth2-inbound-invalid-token |
| nssf-transport-server | oauth2-inbound-insufficient-scope | t-oauth2-inbound-insufficient-scope |
| nssf-notify-dispatcher | oauth2-outbound-token-acquire | t-oauth2-outbound-token-acquire |
| nssf-notify-dispatcher | oauth2-outbound-token-endpoint-fail | t-oauth2-outbound-token-endpoint-fail |
| nssf-notify-dispatcher | outbound-tls-handshake-fail | t-outbound-tls-handshake-fail |
| nssf-scenario-contract-tests | 8 operation contract test (success + error matrix) | (test-matrix.md `t-*` 매트릭스 전체) |
| nssf-scenario-end-to-end | subscription-create-and-notify · notification-retry-on-5xx · notification-dead-letter · correlation-end-to-end | t-subscription-notify · t-notify-retry · t-notify-dead-letter · t-correlation-e2e |
| nssf-external-nf-gateway | (UDM SDM / NRF discovery mock port) | (test-matrix.md 의 외부 NF mock seam) |
<!-- USER:module-test-table:end -->

## Open Gaps

<!-- USER:open-gaps:start -->
- `handoff/nssf/contract.yaml` 과 `design/nssf/contract/**` 는 *workflow-generated, normally untracked* 산출이다 — `.gitignore` 로 제외되며 fresh checkout 후엔 `/nf-init` + `/nf-build` 사이클로 로컬 재생성된다 (`CLAUDE.md` Source-of-truth policy 참고). 본 traceability 의 `handoff/nssf/contract.yaml` 참조는 *현재 워크플로우 산출 경로* 를 의미.
- contract markdown body 부재 — 본 사이클은 `/nf-build` 의 *handoff yaml 까지만* 결정. `design/nssf/contract/**/*.md` 의 본문 (AUTO/USER section) 은 별도 nf-build 사이클에서 채운다. 본 traceability 는 *handoff yaml topic ID* 기준이라 동작은 함.
- `data-model/<HTTP code>` 등 18 개 노이즈 — PR #15 머지로 fix 됐고 본 seed 는 깨끗. 잔여 없음.
- 33.501 — `docs/adr/ADR-0004-project-security-baseline.md` (project security baseline) 으로 흡수, lifecycle extraction dependency 아님. `nssf-transport-server` 와 `nssf-notify-dispatcher` 가 TLS / mTLS / OAuth2 production-capable code path 의무 적용. 33.310 / 33.210 세부는 operator-provided cert/config + library compliance 외부 처리.
- 38.413 운영 결정 보류 — NGAP 절차 깊이는 운영 결정. 본 traceability 미반영.
- subscription persistence backend — engineering-design 결정 = PostgreSQL/libpq 단일. test seam = `t-repo-contract` (production PostgreSQL 기준 contract test).
<!-- USER:open-gaps:end -->

<!-- AUTO:references:start -->
## References

- `docs/adr/ADR-0004-project-security-baseline.md` — TLS / mTLS / OAuth2 production-capable code path 의무.
- `design/policies/security-baseline.yaml` — baseline 7 mandate (M1~M7) machine-readable.
- `design/policies/lifecycle.yaml` — lifecycle stage / gate vocabulary.
- `design/policies/gaps.yaml` — gap category enum (6) — open-gaps 의 category 컬럼 source.
- `design/policies/source-precedence.yaml` — ADR-0004 baseline > engineering-design > architecture > dev enforcement order.
- `design/policies/spec-dependencies.yaml` — 33.501 / 33.310 / 33.210 absorbed by ADR-0004.
- `design/schemas/engineering-core-slots.yaml` — 13 core slot typed shape (v2 + dependency_shape).
- `design/schemas/nf-readiness-config-v1.yaml` — readiness-config shape contract.
- `design/nssf/readiness-config.yaml` — NSSF 사람 ratify config — implementation·security·phase·gap·deferred 결정 source.
- `engineering/nssf/engineering-design.md` — 현재 적용 lib/구조 source (readiness-config.implementation 흡수 대상).
- `dev/nssf/implementation-plan.md`.
- `dev/nssf/tasks.yaml`.
- `dev/nssf/test-matrix.md`.
- `design/nssf/architecture/` 전체.
- `design/nssf/module-decomposition/` 전체.
- `handoff/nssf/contract.yaml` — 8 API topic + data-model.
<!-- AUTO:references:end -->
