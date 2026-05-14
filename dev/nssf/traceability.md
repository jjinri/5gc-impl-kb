---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Traceability

## Contract → Module

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

## Module → Test

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
| nssf-repo-availability · nssf-repo-subscription | repository golden interface (in-memory ↔ persistent) | t-repo-golden |

## Open Gaps

- `handoff/nssf/contract.yaml` 과 `design/nssf/contract/**` 는 *workflow-generated, normally untracked* 산출이다 — `.gitignore` 로 제외되며 fresh checkout 후엔 `/nf-init` + `/nf-build` 사이클로 로컬 재생성된다 (`CLAUDE.md` Source-of-truth policy 참고). 본 traceability 의 `handoff/nssf/contract.yaml` 참조는 *현재 워크플로우 산출 경로* 를 의미.
- contract markdown body 부재 — 본 사이클은 `/nf-build` 의 *handoff yaml 까지만* 결정. `design/nssf/contract/**/*.md` 의 본문 (AUTO/USER section) 은 별도 nf-build 사이클에서 채운다. 본 traceability 는 *handoff yaml topic ID* 기준이라 동작은 함.
- `data-model/<HTTP code>` 등 18 개 노이즈 — PR #15 머지로 fix 됐고 본 seed 는 깨끗. 잔여 없음.
- 33.501 · 38.413 운영 결정 보류 — TLS profile / NGAP 절차 깊이가 결정되기 전에는 본 traceability 의 `(공통 transport)` 와 `nssf-transport-server` 안에 *config 외부화* 로만 표현.
- subscription persistence backend default — `state-persistence.md` open question. test golden interface 가 이 결정의 추상화 검증.

## References

- `dev/nssf/implementation-plan.md`.
- `dev/nssf/tasks.yaml`.
- `dev/nssf/test-matrix.md`.
- `design/nssf/architecture/` 전체.
- `design/nssf/module-decomposition/` 전체.
- `handoff/nssf/contract.yaml` — 8 API topic + data-model.
