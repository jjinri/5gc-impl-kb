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
| `t-availability-put-invalid-body` | unit | availability-put-invalid-body | malformed NssaiAvailabilityInfo (예 결측 tai, snssai 형식 오류) | NSSAIAvailabilityPut 호출 | 400 ProblemDetails (`INVALID_QUERY_PARAM` 또는 422) | `api/NSSAIAvailabilityPut`, `error-propagation.md`, `request-flow.md` |
| `t-availability-patch` | integration | availability-patch-applies | 기존 record + 유효 PatchDocument | NSSAIAvailabilityPatch | 200 `AuthorizedNssaiAvailabilityInfo` + 변경 이벤트 발행 | `api/NSSAIAvailabilityPatch`, `AvailabilityEngine.md` |
| `t-availability-patch-conflict` | unit | availability-patch-conflict (etag 옵션 지원 시) | 기존 record + stale etag | NSSAIAvailabilityPatch(if-match=stale) | 412 etag mismatch | `api/NSSAIAvailabilityPatch`, `error-propagation.md` |
| `t-availability-delete` | integration | availability-delete | 기존 record | NSSAIAvailabilityDelete | 204 + 변경 이벤트 발행 | `api/NSSAIAvailabilityDelete`, `AvailabilityEngine.md` |
| `t-availability-delete-not-found` | unit | availability-delete-not-found | 부재 AMF 의 availability record | NSSAIAvailabilityDelete 호출 | 404 ProblemDetails | `api/NSSAIAvailabilityDelete`, `error-propagation.md` |
| `t-availability-options` | unit | availability-options-supported-features | feature.flags config | NSSAIAvailabilityOptions | 200 `supportedFeatures` bitmask | `api/NSSAIAvailabilityOptions`, `configuration-strategy.md` |
| `t-availability-options-unauthorized` | unit | availability-options-unauthorized | OAuth2 token 부재 또는 invalid (oauth2_inbound.enabled=true) | NSSAIAvailabilityOptions 호출 | 401 ProblemDetails | `api/NSSAIAvailabilityOptions`, `error-propagation.md`, `interface` |
| `t-subscription-create` | integration | subscription-create | 유효 `NssfEventSubscriptionCreateData` | NSSAIAvailabilityPost 호출 | 201 `NssfEventSubscriptionCreatedData` + Location header + subscription resource persisted | `api/NSSAIAvailabilityPost`, `SubscriptionStore.md` |
| `t-subscription-notify` | integration | subscription-create-and-notify | 구독 등록 + availability change 트리거 | 변경 이벤트 발생 | callback URI 로 outbound POST 발송, correlation-id 전파 | `api/NSSAIAvailabilityPost`, `SubscriptionStore.md`, `NotificationDispatcher.md`, `observability.md` |
| `t-subscription-create-invalid-callback` | unit | subscription-create-invalid-callback | malformed `callbackUri` (예 scheme 누락, host 부재) | NSSAIAvailabilityPost 호출 | 400 ProblemDetails | `api/NSSAIAvailabilityPost`, `error-propagation.md`, `SubscriptionStore.md` |
| `t-subscription-modify` | unit | subscription-modify-filter | 기존 subscription | SubModifyPatch(filter 변경) | 200 + 새 filter 기준 매칭 | `api/NSSAIAvailabilitySubModifyPatch`, `SubscriptionStore.md` |
| `t-subscription-modify-not-found` | unit | subscription-modify-not-found | 부재 subscription id | NSSAIAvailabilitySubModifyPatch 호출 | 404 ProblemDetails | `api/NSSAIAvailabilitySubModifyPatch`, `error-propagation.md` |
| `t-subscription-unsubscribe` | integration | subscription-unsubscribe | 기존 subscription + 변경 이벤트 후속 발생 | NSSAIAvailabilityUnsubscribe → 변경 이벤트 발생 | 204, 후속 outbound 미발송 | `api/NSSAIAvailabilityUnsubscribe`, `SubscriptionStore.md` |
| `t-subscription-unsubscribe-not-found` | unit | subscription-unsubscribe-not-found | 부재 subscription id | NSSAIAvailabilityUnsubscribe 호출 | 404 ProblemDetails | `api/NSSAIAvailabilityUnsubscribe`, `error-propagation.md` |
| `t-notify-retry` | integration | notification-retry-on-5xx | mock callback receiver 가 5xx 1 회 후 200 | 변경 이벤트 발생 | retry 후 성공, retry metric 증가 | `NotificationDispatcher.md`, `error-propagation.md`, `configuration-strategy.md` |
| `t-notify-dead-letter` | integration | notification-dead-letter | mock callback 가 5xx 반복 | max_attempts 초과 | dead-letter (log + metric), business 영향 없음 | `NotificationDispatcher.md`, `error-propagation.md` |
| `t-notify-oauth2-enabled` | contract | oauth2-client-credentials-enabled | `oauth2_outbound.enabled=true` + mock NRF token endpoint | 변경 이벤트 발생 | outbound POST 가 `Authorization: Bearer <token>` 부착 (token cache miss → acquire → attach) | `NotificationDispatcher.md`, `configuration-strategy.md`, `ADR-0004` |
| `t-notify-oauth2-disabled` | contract | oauth2-client-credentials-disabled | `oauth2_outbound.enabled=false` | 변경 이벤트 발생 | outbound POST 가 `Authorization` 미부착 | `NotificationDispatcher.md`, `configuration-strategy.md` |
| `t-correlation-e2e` | end-to-end | correlation-end-to-end | inbound 에 `3gpp-Sbi-Correlation-Info` header | NSSelectionGet 또는 NSSAIAvailability* 호출 + 변경 이벤트 후속 | 모든 log / metric exemplar / outbound header 에 동일 correlation-id | `observability.md`, `NotificationDispatcher.md` |
| `t-shutdown-drain` | end-to-end | graceful-shutdown-drain | in-flight request + retry queue | shutdown signal | inbound listener stop → drain → retry flush 또는 timeout | `runtime-model.md`, `NotificationDispatcher.md` |
| `t-repo-contract` | module-integration | repository interface contract | 같은 시퀀스의 CRUD operation | PostgreSQL backend (production) + in-memory mock (unit) | repository abstraction 이 PostgreSQL 동작과 일치 (read-after-write, atomic update, atomic delete, JSONB filter index) | `state-persistence.md`, `tasks.yaml#nssf-repo-*`, `engineering-design.md` |
| `t-tls-handshake-success` | security | tls-handshake-success | `tls.enabled=true`, valid server cert + CA | client 가 TLS handshake 시도 | handshake 성공, connection 성립 | `configuration-strategy.md`, `request-flow.md`, `ADR-0004` |
| `t-tls-handshake-fail` | security | tls-handshake-fail | server cert 미스매치 또는 untrusted | client 가 TLS handshake 시도 | TLS alert → connection reject, NSSF 응답 없음 | `error-propagation.md`, `ADR-0004` |
| `t-mtls-peer-verify-success` | security | mtls-peer-verify-success | `mtls.enabled=true`, valid client cert chain | client 가 mTLS handshake 시도 | peer identity verify 통과, connection 성립 | `configuration-strategy.md`, `request-flow.md`, `ADR-0004` |
| `t-mtls-peer-reject` | security | mtls-peer-reject | `mtls.enabled=true`, untrusted client cert 또는 cert 부재 | client 가 mTLS handshake 시도 | TLS alert + 403-equivalent connection reject | `error-propagation.md`, `ADR-0004` |
| `t-oauth2-inbound-valid-bearer` | security | oauth2-inbound-valid-bearer | `oauth2_inbound.enabled=true`, valid JWT (signature/expiry/audience/scope) | inbound request with Authorization header | handler 진입, scope 검증 통과 | `request-flow.md`, `ADR-0004` |
| `t-oauth2-inbound-missing-token` | security | oauth2-inbound-missing-token | `oauth2_inbound.enabled=true`, Authorization header 부재 | inbound request | 401 `MISSING_TOKEN` ProblemDetails | `error-propagation.md`, `ADR-0004` |
| `t-oauth2-inbound-invalid-token` | security | oauth2-inbound-invalid-token | `oauth2_inbound.enabled=true`, JWT signature/expiry/audience 실패 | inbound request | 401 `INVALID_TOKEN` | `error-propagation.md`, `ADR-0004` |
| `t-oauth2-inbound-insufficient-scope` | security | oauth2-inbound-insufficient-scope | `oauth2_inbound.enabled=true`, valid JWT 이나 scope 부족 | inbound request | 403 `INSUFFICIENT_SCOPE` | `error-propagation.md`, `ADR-0004` |
| `t-oauth2-outbound-token-acquire` | security | oauth2-outbound-token-acquire | `oauth2_outbound.enabled=true`, mock NRF token endpoint | 변경 이벤트 + outbound dispatch | token cache miss → token endpoint 호출 → cache → outbound POST attach | `NotificationDispatcher.md`, `ADR-0004` |
| `t-oauth2-outbound-token-endpoint-fail` | security | oauth2-outbound-token-endpoint-fail | mock NRF token endpoint 5xx 또는 client_secret invalid | 변경 이벤트 + outbound dispatch | 5xx → retry queue, client_secret invalid → dead-letter + alert | `error-propagation.md`, `NotificationDispatcher.md` |
| `t-outbound-tls-handshake-fail` | security | outbound-tls-handshake-fail | callback receiver TLS fail (expired cert 등) | 변경 이벤트 + outbound dispatch | outbound TLS handshake 실패 → retry queue 진입 | `error-propagation.md`, `NotificationDispatcher.md` |

## Coverage Rules

- 8 operation 각각 *success path* 최소 1 개 + *주요 error* 1 개 이상.
- ProblemDetails shape 검증을 mapper unit test 와 contract test 양쪽에서.
- outbound notification 의 *correlation 전파* 는 e2e test 에서 *반드시* 검증.
- repository 의 *interface 정합성* 은 PostgreSQL backend 기준 contract test 로 검증. test seam 의 in-memory mock 은 abstraction 검증.
- ADR-0004 baseline 4 capability (TLS / mTLS / inbound OAuth2 / outbound OAuth2) 각각 *success* + *primary failure* 1 case 이상 — `t-tls-*`, `t-mtls-*`, `t-oauth2-inbound-*`, `t-oauth2-outbound-*` security 그룹이 cover.
- subscription lifetime 만료 정책은 본 inventory 의 *unit* test 에 명시되지 않음 — 본 매트릭스 외 추가 시 별도 사이클.

## Open Questions

- `t-availability-patch-conflict` 의 412 etag 정책 채택 여부 — `error-propagation.md` open question 과 연동.
- `notification-dead-letter` 후속 처리 (subscription 자동 deactivate) test 시나리오 — `NotificationDispatcher.md` open question 결정 시 추가.
- *PII 마스킹* test — UE id 가 어떤 operation 의 payload 에 도달하는지 확정 후 inventory 추가.

## References

- `design/nssf/architecture/test-strategy.md` — 시나리오 카탈로그 (security 시나리오 10 추가됨) 의 출처.
- `design/nssf/module-decomposition/` 4 module.
- `handoff/nssf/contract.yaml` — schema 진실 출처.
- `docs/adr/ADR-0004-project-security-baseline.md` — security capability 의무 source.
- `engineering/nssf/engineering-design.md` — 선택된 TLS/OAuth2/persistence lib.
- `dev/nssf/tasks.yaml` — task 와의 매핑.
- `dev/nssf/traceability.md` — module ↔ test 매핑.
