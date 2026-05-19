---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Error Propagation

## Purpose

NSSF 의 8 operation × 응답 코드 × `ProblemDetails` 매핑 매트릭스를 정의한다. inbound 에러의 mapping 위치와 outbound notification 실패의 처리 정책도 함께 명시한다.

## Inputs (contract)

- `error-handling` topic — status code 목록 (operation×cause 매핑 부재 — 하단 fresh-full 주의 참조).
- 8 API topics — operation 별 응답 코드 enumeration.
- spec `application/problem+json` shape (RFC 7807, 3GPP 29.500 §5.2.7).
- 주의 (2026-05-18 fresh-full contract 기준). 현 generated `error-handling` topic 은 status code 나열만 보유한다 (meaning 전부 `-`, operation×cause 매핑 부재). 아래 cause matrix (`INVALID_QUERY_PARAM` · `UNAUTHORIZED_NSSAI` · `NSSAI_NOT_AVAILABLE` 등) 는 contract-derived 가 아니라 3GPP TS 29.531 spec-valid manual augmentation 이며 current generated contract 에 trace 되지 않는다. 추출 심화는 별도 tooling follow-up.

## Boundaries

본 문서가 다루는 것 — error → `ProblemDetails` shape 매핑, 책임 모듈, retry 가능 vs 불가 분류.

본 문서가 다루지 않는 것 — 라이브러리 별 exception → ProblemDetails 자동 변환 코드 (dev 책임).

## Decisions

### 공통 매핑 원칙

- 모든 inbound error 응답은 `application/problem+json` content-type.
- `title`, `status`, `cause` (3GPP 확장), `detail` 필드 포함.
- 5xx 응답은 `cause: SYSTEM_FAILURE` default.
- 4xx 응답은 operation 별 cause enumeration 따름.

### 매트릭스 (요약)

| operation | 200/201/204 | 400 | 401/403 | 404 | 409 | 5xx | retry 가능 (client 측) |
|---|---|---|---|---|---|---|---|
| `NSSelectionGet` | 200 `AuthorizedNetworkSliceInfo` | `INVALID_QUERY_PARAM`, `UNAUTHORIZED_NSSAI`, `NSSAI_NOT_AVAILABLE` | 401 unauthorized, 403 forbidden (OAuth2 정책) | — | — | `SYSTEM_FAILURE` | 5xx 만 client retry. 4xx 는 입력 수정 필요. |
| `NSSAIAvailabilityPut` | 200 `AuthorizedNssaiAvailabilityInfo` 또는 204 | invalid body | 401/403 | — | conflict 가능 | 5xx | 5xx + 일부 conflict (idempotency key 정의 시) |
| `NSSAIAvailabilityPatch` | 200 `AuthorizedNssaiAvailabilityInfo` | invalid patch | 401/403 | resource not found | 412 etag mismatch | 5xx | 412 는 client 가 재조회 후 재시도 |
| `NSSAIAvailabilityDelete` | 204 | — | 401/403 | not found | — | 5xx | — |
| `NSSAIAvailabilityOptions` | 200 | — | 401/403 | — | — | 5xx | — |
| `NSSAIAvailabilityPost` | 201 `NssfEventSubscriptionCreatedData` | invalid body | 401/403 | — | — | 5xx | 5xx |
| `NSSAIAvailabilitySubModifyPatch` | 200 또는 204 | invalid patch | 401/403 | subscription not found | — | 5xx | 5xx |
| `NSSAIAvailabilityUnsubscribe` | 204 | — | 401/403 | not found | — | 5xx | — |

전체 응답 코드 표는 `handoff/nssf/contract.yaml` 의 `error-handling` topic 이 진실 출처.

### 책임 모듈

| error 위치 | 매핑 책임 |
|---|---|
| transport 단계 (TLS, OAuth2 실패) | transport layer / 공통 utility |
| request validation 실패 (schema, query) | 공통 `RequestValidator` utility |
| business logic 실패 (NSSAI 인증·존재) | 해당 모듈 (SelectionEngine / AvailabilityEngine / SubscriptionStore) 이 의도된 cause 반환 → 공통 `ProblemDetailsMapper` 가 응답으로 |
| 내부 unexpected (panic / null ref) | 공통 fallback → 500 `SYSTEM_FAILURE` |

### outbound notification 실패

- HTTP status 2xx — 성공.
- HTTP status 4xx (`callback` 측 invalid) — *retry 안 함*. metric/log 만, subscription 자동 비활성화 정책 보류 (`## Open Questions`).
- HTTP status 5xx, timeout, connection error — *retry queue* 에 추가. `configuration-strategy.md` 의 retry 정책 따름.
- max_attempts 초과 — dead-letter (log + metric). 정책 보류.

## Open Questions

- callback 4xx 가 반복되면 subscription 자동 deactivate 할지 — spec 강제 없음. 운영 결정.
- ProblemDetails 의 `detail` 필드에 *내부 hint* (모듈명 등) 노출 정책 — 보안 관점 검토 필요.
- 412 etag 정책을 NSSF 가 *지원* 할지 — spec 옵션. dev 결정.

## References

- [[request-flow]] — error 발생 시점.
- [[observability]] — error 분류별 log/metric.
- [[module-boundaries]] — 모듈 책임.
- `handoff/nssf/contract.yaml` `error-handling` topic — 진실 출처.
