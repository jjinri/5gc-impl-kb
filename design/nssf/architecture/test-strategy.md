---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Test Strategy

## Purpose

NSSF 의 architecture-level test seam 을 정의한다 — 어떤 경계가 unit test, integration test, contract test 로 검증되어야 하는지. 본 strategy 가 dev 단계 test pyramid 의 기준선이다.

## Inputs (contract)

- 8 API topics — input · output schema.
- `error-handling` topic — status code 목록 (operation×error 매트릭스는 spec/manual augmentation — `error-propagation.md` `## Inputs` 참조).
- 본 architecture 의 4 모듈 + outbound dispatcher.

## Boundaries

본 문서가 다루는 것 — *어떤 경계에서* *무엇을* 검증할지.

본 문서가 다루지 않는 것 — test framework 선택 (pytest / go test 등), CI 통합, coverage 목표 수치.

## Decisions

### test pyramid

| 종류 | 범위 | 책임 | 도구 예 |
|---|---|---|---|
| **unit** | 단일 모듈의 내부 함수 / 클래스 | 각 모듈 (SelectionEngine 등) 의 비즈니스 로직 | mock 의존 |
| **module-integration** | 모듈 + 그 모듈 영역의 collaborator (예 AvailabilityEngine + repository) | 변경 이벤트 발행, persistence 와의 connector | in-memory backend |
| **contract** | 8 operation 의 inbound 응답 schema · status / error 매트릭스 | 전체 entry handler. mock 외부 NF | spec-driven (schema validator) |
| **outbound contract** | NotificationDispatcher 가 spec 의 callback payload 를 정확히 발송 | mock AMF callback receiver | record-replay |
| **end-to-end / scenario** | 두 service 의 핵심 시나리오 | 외부 NF mock (NRF / UDM / AMF callback receiver) | 운영 시뮬레이션 |

### 핵심 시나리오 카탈로그

| 시나리오 | 검증 모듈 | 검증 항목 |
|---|---|---|
| `success-registration-selection` | SelectionEngine | NSSelectionGet 정상 응답, allowed NSSAI 계산, target AMF set 결정 |
| `invalid-structured-query` | SelectionEngine | 400 `INVALID_QUERY_PARAM` + ProblemDetails shape |
| `unauthorized-nssai` | SelectionEngine | 400 `UNAUTHORIZED_NSSAI` |
| `nssai-not-available` | SelectionEngine | 400 `NSSAI_NOT_AVAILABLE` (availability map 부재 시) |
| `availability-put-creates-record` | AvailabilityEngine | NSSAIAvailabilityPut 후 SubscriptionStore 에 변경 이벤트 발행 |
| `availability-patch-conflict` | AvailabilityEngine | 412 etag mismatch (옵션 지원 시) |
| `subscription-create-and-notify` | SubscriptionStore + NotificationDispatcher | Subscribe → availability 변경 → outbound POST 발송 + correlation-id 전파 |
| `subscription-modify-filter` | SubscriptionStore | filter 변경 후 notification 대상 변경 |
| `subscription-unsubscribe` | SubscriptionStore | 삭제 후 notification 미발송 |
| `notification-retry-on-5xx` | NotificationDispatcher | callback 5xx → retry → 성공 |
| `notification-dead-letter` | NotificationDispatcher | max_attempts 초과 → dead-letter + metric |
| `oauth2-client-credentials-enabled` | NotificationDispatcher + transport | token acquire (NRF token endpoint) → cache → 부착 outbound POST |
| `oauth2-client-credentials-disabled` | NotificationDispatcher + transport | token 없이 outbound |
| `correlation-end-to-end` | (전체) | inbound `3gpp-Sbi-Correlation-Info` → 모든 log / outbound 전파 |
| `graceful-shutdown-drain` | runtime | in-flight request + retry queue drain |
| `tls-handshake-success` | transport | inbound TLS handshake 정상, cert/CA load 후 connection 성립 |
| `tls-handshake-fail` | transport | server cert 미스매치 / library default cipher mismatch → TLS alert reject |
| `mtls-peer-verify-success` | transport | client cert chain + peer identity verify 통과 |
| `mtls-peer-reject` | transport | client cert 미제공 또는 untrusted CA → connection reject |
| `oauth2-inbound-valid-bearer` | transport | valid JWT → handler 진입, scope 검증 통과 |
| `oauth2-inbound-missing-token` | transport | bearer 부재 → 401 `MISSING_TOKEN` ProblemDetails |
| `oauth2-inbound-invalid-token` | transport | signature/expiry/audience 실패 → 401 `INVALID_TOKEN` |
| `oauth2-inbound-insufficient-scope` | transport | scope 부족 → 403 `INSUFFICIENT_SCOPE` |
| `oauth2-outbound-token-acquire` | NotificationDispatcher | token cache miss → NRF token endpoint 호출 → cache → outbound POST 부착 |
| `oauth2-outbound-token-endpoint-fail` | NotificationDispatcher | token endpoint 5xx → retry queue, client_secret invalid → dead-letter |
| `outbound-tls-handshake-fail` | NotificationDispatcher | callback receiver TLS 실패 → retry queue 진입 |

### test seam 강제

- 외부 NF 의존 (`NRF`, `UDM`, `AMF callback`) 은 *interface* 로 추상화. contract test 에서 mock 으로 대체.
- 시간 의존 (retry backoff, lifetime expiry) 은 *clock interface* 로 추상화.
- persistence backend 는 *repository interface* 로 추상화.

## Open Questions

- coverage 목표 — 본 architecture 는 *seam* 만, 수치 목표는 dev 결정.
- spec conformance test — 3GPP 가 별도 conformance suite 제공 시 통합 여부.
- callback 의 negative test (TLS 실패, DNS 실패 등) 의 시나리오 깊이.

## References

- [[module-boundaries]] — 모듈 별 단일 책임.
- [[request-flow]] — 시나리오 시퀀스 (TLS / mTLS / OAuth2 단계 포함).
- [[error-propagation]] — error case 매트릭스 (security 실패 case).
- [[observability]] — test 시 검증할 signal.
- [[configuration-strategy]] — security config keys.
- `handoff/nssf/contract.yaml` — schema · 응답 코드 진실 출처.
- `docs/adr/ADR-0004-project-security-baseline.md` — security capability 의무 source.
