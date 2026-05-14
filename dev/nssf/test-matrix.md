---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Test Matrix

## Purpose

본 문서는 NSSF 의 8 inbound operation + outbound notification + e2e 시나리오를 검증하는 test inventory 다. `design/nssf/architecture/test-strategy.md` 의 시나리오 카탈로그가 입력. 본 inventory 의 각 항목은 `dev/nssf/tasks.yaml` 의 task 와 `dev/nssf/traceability.md` 의 매핑으로 추적된다.

## Test Inventory

| id | kind | scenario | given | when | then | refs |
|---|---|---|---|---|---|---|
| `t-selection-success` | integration | success-registration-selection | subscribed NSSAI + PLMN policy + availability map 모두 존재 | NSSelectionGet 호출 | 200 `AuthorizedNetworkSliceInfo`, allowed NSSAI 계산 + target AMF set 결정 | `api/NSSelectionGet`, `SelectionEngine.md`, `request-flow.md` |
| `t-selection-invalid-query` | unit | invalid-structured-query | malformed structured query JSON | NSSelectionGet 호출 | 400 `INVALID_QUERY_PARAM`, ProblemDetails | `api/NSSelectionGet`, `error-propagation.md` |
| `t-selection-unauthorized` | unit | unauthorized-nssai | requested S-NSSAI ∉ subscribed | NSSelectionGet 호출 | 400 `UNAUTHORIZED_NSSAI` | `SelectionEngine.md`, `error-propagation.md` |
| `t-selection-not-available` | unit | nssai-not-available | requested S-NSSAI 의 availability[tai] 부재 | NSSelectionGet 호출 | 400 `NSSAI_NOT_AVAILABLE` | `SelectionEngine.md`, `AvailabilityEngine.md` |
| `t-availability-put` | integration | availability-put-creates-record | 빈 availability map | NSSAIAvailabilityPut(tai, NssaiAvailabilityInfo) | 200 `AuthorizedNssaiAvailabilityInfo` + 변경 이벤트 발행 | `api/NSSAIAvailabilityPut`, `AvailabilityEngine.md`, `request-flow.md` |
| `t-availability-patch-conflict` | unit | availability-patch-conflict (etag 옵션 지원 시) | 기존 record + stale etag | NSSAIAvailabilityPatch(if-match=stale) | 412 etag mismatch | `api/NSSAIAvailabilityPatch`, `error-propagation.md` |
| `t-availability-delete` | integration | availability-delete | 기존 record | NSSAIAvailabilityDelete | 204 + 변경 이벤트 발행 | `api/NSSAIAvailabilityDelete`, `AvailabilityEngine.md` |
| `t-availability-options` | unit | availability-options-supported-features | feature.flags config | NSSAIAvailabilityOptions | 200 `supportedFeatures` bitmask | `api/NSSAIAvailabilityOptions`, `configuration-strategy.md` |
| `t-subscription-notify` | integration | subscription-create-and-notify | 구독 등록 + availability change 트리거 | 변경 이벤트 발생 | callback URI 로 outbound POST 발송, correlation-id 전파 | `SubscriptionStore.md`, `NotificationDispatcher.md`, `observability.md` |
| `t-subscription-modify` | unit | subscription-modify-filter | 기존 subscription | SubModifyPatch(filter 변경) | 200 + 새 filter 기준 매칭 | `api/NSSAIAvailabilitySubModifyPatch`, `SubscriptionStore.md` |
| `t-subscription-unsubscribe` | integration | subscription-unsubscribe | 기존 subscription + 변경 이벤트 후속 발생 | NSSAIAvailabilityUnsubscribe → 변경 이벤트 발생 | 204, 후속 outbound 미발송 | `api/NSSAIAvailabilityUnsubscribe`, `SubscriptionStore.md` |
| `t-notify-retry` | integration | notification-retry-on-5xx | mock callback receiver 가 5xx 1 회 후 200 | 변경 이벤트 발생 | retry 후 성공, retry metric 증가 | `NotificationDispatcher.md`, `error-propagation.md`, `configuration-strategy.md` |
| `t-notify-dead-letter` | integration | notification-dead-letter | mock callback 가 5xx 반복 | max_attempts 초과 | dead-letter (log + metric), business 영향 없음 | `NotificationDispatcher.md`, `error-propagation.md` |
| `t-notify-oauth2-enabled` | contract | oauth2-client-credentials-enabled | `oauth2.enabled=true` + mock NRF token endpoint | 변경 이벤트 발생 | outbound POST 가 `Authorization: Bearer <token>` 부착 | `NotificationDispatcher.md`, `configuration-strategy.md` |
| `t-notify-oauth2-disabled` | contract | oauth2-client-credentials-disabled | `oauth2.enabled=false` | 변경 이벤트 발생 | outbound POST 가 `Authorization` 미부착 | `NotificationDispatcher.md`, `configuration-strategy.md` |
| `t-correlation-e2e` | end-to-end | correlation-end-to-end | inbound 에 `3gpp-Sbi-Correlation-Info` header | NSSelectionGet 또는 NSSAIAvailability* 호출 + 변경 이벤트 후속 | 모든 log / metric exemplar / outbound header 에 동일 correlation-id | `observability.md`, `NotificationDispatcher.md` |
| `t-shutdown-drain` | end-to-end | graceful-shutdown-drain | in-flight request + retry queue | shutdown signal | inbound listener stop → drain → retry flush 또는 timeout | `runtime-model.md`, `NotificationDispatcher.md` |
| `t-repo-golden` | module-integration | repository golden interface | 같은 시퀀스의 CRUD operation | in-memory ↔ persistent backend 둘 다 | 동일 결과 (read-after-write, atomic update, atomic delete) | `state-persistence.md`, `tasks.yaml#nssf-repo-*` |

## Coverage Rules

- 8 operation 각각 *success path* 최소 1 개 + *주요 error* 1 개 이상.
- ProblemDetails shape 검증을 mapper unit test 와 contract test 양쪽에서.
- outbound notification 의 *correlation 전파* 는 e2e test 에서 *반드시* 검증.
- repository 의 *backend 무관성* 은 golden interface test 로 검증.
- subscription lifetime 만료 정책은 본 inventory 의 *unit* test 에 명시되지 않음 — 본 매트릭스 외 추가 시 별도 사이클.

## Open Questions

- `t-availability-patch-conflict` 의 412 etag 정책 채택 여부 — `error-propagation.md` open question 과 연동.
- `notification-dead-letter` 후속 처리 (subscription 자동 deactivate) test 시나리오 — `NotificationDispatcher.md` open question 결정 시 추가.
- *PII 마스킹* test — UE id 가 어떤 operation 의 payload 에 도달하는지 확정 후 inventory 추가.

## References

- `design/nssf/architecture/test-strategy.md` — 시나리오 카탈로그 (15 항목) 의 출처.
- `design/nssf/module-decomposition/` 4 module.
- `handoff/nssf/contract.yaml` — schema 진실 출처.
- `dev/nssf/tasks.yaml` — task 와의 매핑.
- `dev/nssf/traceability.md` — module ↔ test 매핑.
