---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Runtime Model

## Purpose

NSSF 가 어떤 runtime 책임을 가지는지 정의한다 — request-response (stateless) 와 long-lived state (subscription · notification queue) 의 분리, concurrency 가정, lifecycle 단계.

## Inputs (contract)

- 8 API topics.
- `interface` topic — server URL · HTTP/2 stream multiplexing 전제.

## Boundaries

본 문서가 정의하는 것 — runtime boundary (process · 스레드 · async / sync) 의 *추상 모델*.

본 문서가 정의 안 하는 것 — 구현 언어의 동시성 primitive (goroutine · async/await · thread pool), 실제 컨테이너 토폴로지.

## Decisions

### runtime 책임 종류

| 종류 | 책임 | 모듈 |
|---|---|---|
| **inbound stateless** | request 당 단명 — request validation, slice selection, availability CRUD lookup, subscription CRUD | SelectionEngine, AvailabilityEngine (lookup/CRUD), SubscriptionStore (CRUD) |
| **inbound stateful** | 단일 request 안에서 *commit* 단위로 상태 변경 + 변경 이벤트 발행 | AvailabilityEngine (Put/Patch/Delete), SubscriptionStore (Subscribe/Unsubscribe) |
| **long-lived state** | request 수명 외 — subscription resource · retry queue · in-flight notification | SubscriptionStore, NotificationDispatcher |
| **outbound async** | 변경 이벤트 → outbound POST 발송. inbound request 응답과 분리된 lifecycle | NotificationDispatcher |

### concurrency 가정

- HTTP/2 multiplexing — 동일 connection 위 다중 stream 동시 처리. NSSF 모든 inbound handler 는 *동시 호출* 가정.
- AvailabilityEngine 의 *상태 변경* 은 *TAI 단위* atomic 가정. 같은 TAI 의 두 동시 Put 은 serialize 필요 (lock 또는 optimistic concurrency).
- SubscriptionStore 의 *resource* 는 *subscription id* 단위 atomic.
- NotificationDispatcher 의 outbound 호출은 동시 가능. 같은 subscription 대상 outbound 의 *순서 보장* 은 spec 강제 아님 — 본 architecture 도 *no ordering guarantee* 선택.

### lifecycle 단계

| stage | 책임 |
|---|---|
| start-up | configuration 로드, OAuth2 client 초기화 (옵션), SubscriptionStore backend 연결, NotificationDispatcher worker pool 기동 |
| running | inbound handler + outbound dispatcher 동시 동작 |
| graceful shutdown | inbound listener 종료 → in-flight request drain → NotificationDispatcher worker 가 retry queue flush 또는 timeout 후 종료 |
| crash recovery | SubscriptionStore backend 가 persistent 면 restart 후 복원. retry queue 의 미발송 notification 은 backend 정책에 따라 (전송 보장 vs 손실 허용) — `## Open Questions` |

### sync vs async 분리

- inbound entry → business logic 은 *sync* (request 안 완결).
- AvailabilityEngine 의 *변경 이벤트* 발행 → NotificationDispatcher 호출은 *async* (응답 차단 안 함).
- NotificationDispatcher 의 outbound POST 는 *async* + retry.

## Open Questions

- NotificationDispatcher 의 worker pool 크기 default 권고 여부.
- 변경 이벤트 → outbound POST 의 *전송 보장 수준* (at-most-once / at-least-once / exactly-once) — backend 선택에 의존.
- HTTP/2 connection pool 의 *per-AMF* vs *shared* 정책.

## References

- [[module-boundaries]] — 모듈 정의.
- [[request-flow]] — sync · async 시점.
- [[state-persistence]] — SubscriptionStore · retry queue persistence.
- [[observability]] — runtime metric (active connections, in-flight requests, queue depth).
