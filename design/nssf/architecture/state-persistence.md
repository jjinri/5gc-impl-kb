---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF State Persistence

## Purpose

NSSF 가 보유하는 *상태* 의 분류, 영속성 요구, backend 후보를 정의한다. 본 architecture 단계는 *선택지 표* 와 *결정 보류 항목* 을 명시하고, 최종 backend 선정은 dev 단계의 책임이다.

## Inputs (contract)

- `api/NSSAIAvailabilityPut · Patch · Delete · Options` — availability data 의 CRUD shape.
- `api/NSSAIAvailabilityPost · SubModifyPatch · Unsubscribe` — subscription resource lifecycle.
- `data-model/NssaiAvailabilityInfo`, `AuthorizedNssaiAvailabilityInfo`, `NssfEventSubscriptionCreateData`, `NssfEventSubscriptionCreatedData`.

## Boundaries

본 문서가 다루는 것 — 4 모듈의 상태 분류 + persistence 요구.

본 문서가 다루지 않는 것 — 구체적 DB schema · 마이그레이션 · 백업 정책 (dev / ops).

## Decisions

### 상태 분류

| 상태 | 모듈 | 영속성 요구 | crash 후 손실 영향 |
|---|---|---|---|
| TAI × S-NSSAI availability map | AvailabilityEngine | **persistent** | AMF 가 다시 Put 으로 복구 가능. 단 복구 전 NSSelectionGet 결과 부정확. 권장 *persistent*. |
| subscription resource (id, callbackUri, filter, lifetime) | SubscriptionStore | **persistent** | subscription 손실 시 AMF 가 재구독 필요. 권장 *persistent*. |
| in-flight outbound notification (pending POST) | NotificationDispatcher | optional | 손실 허용 가능 — at-most-once 정책일 때. at-least-once 면 persistent retry queue. |
| OAuth2 token cache | (공통 transport) | in-memory | 만료되면 NRF 재요청. persistent 불필요. |
| HTTP/2 connection pool | (공통 transport) | in-memory | runtime 자원. |

### backend 후보 (SubscriptionStore + availability map)

| backend | 장점 | 단점 |
|---|---|---|
| in-memory | 가장 단순, 빠름 | restart 시 손실, 단일 인스턴스 |
| local file (예 JSON / SQLite) | persistent, 의존 적음 | scale-out 어려움 |
| external KV (예 Redis / etcd) | scale-out 가능, atomic op 풍부 | 외부 의존 |
| external RDBMS | transaction · query 풍부 | 무거움 |

### NSSF architecture 단계 권고 — 본 architecture *의도*

- subscription · availability 양쪽 모두 *persistent* 필요.
- backend 선택 자체는 *운영 단계 결정* — dev 가 추상 인터페이스 (예 `SubscriptionRepository`, `AvailabilityRepository`) 정의 후 적어도 in-memory + 하나의 persistent (file / KV) 구현 제공.

## Open Questions

- 본 architecture 가 in-memory default 를 *권고* 할지 — 단일 인스턴스 / dev 환경 빠른 시작에 유용하나 production 적합성 검증 부담.
- retry queue 와 subscription store 가 *같은 backend* 공유 vs 분리 — 같은 backend 시 transaction 으로 subscription 변경과 retry enqueue 묶을 수 있음.
- *변경 이벤트* (AvailabilityEngine → NotificationDispatcher) 의 영속성 — at-least-once 보장 시 이벤트 자체 persistent log 필요.
- 38.413 운영 결정 시 AMF reallocation 관련 상태 (예 redirect target AMF set cache) 추가 가능성.

## References

- [[module-boundaries]] — 상태 소유 모듈.
- [[runtime-model]] — crash recovery 단계.
- [[request-flow]] — 변경 이벤트 발행 시점.
- `handoff/nssf/contract.yaml` — subscription · availability schema.
