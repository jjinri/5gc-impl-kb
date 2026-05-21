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
- `interface` topic — server URL · HTTP/2 stream multiplexing 전제 (fresh-full 주의: 현 generated `interface` output 은 materializer `auto_interface` first-YAML-only 구현이라 첫 primary YAML 기반 partial — `overview.md` `## Inputs` 참조).

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
| start-up | configuration 로드 → **TLS context init** (`tls.enabled=true` 시 cert/key/CA load via library context, library default cipher/version) → **mTLS peer config** (`mtls.enabled=true` 시 client cert config, peer verify mode) → **inbound OAuth2 token validator init** (`oauth2_inbound.enabled=true` 시 JWKS fetch from `issuer.jwks_uri`, validator + key cache 초기화) → **outbound OAuth2 token cache init** (`oauth2_outbound.enabled=true` 시 cache 초기화, lazy token acquire) → SubscriptionStore (PostgreSQL/libpq) 연결 → NotificationDispatcher worker pool 기동. 본 단계의 cert/JWKS load 실패는 fail-fast (해당 capability enable 시). |
| running | inbound handler + outbound dispatcher 동시 동작. TLS context · token validator/cache 는 thread-safe shared resource (HTTP/2 multiplexing 가정). JWKS background refresh (default TTL 5 min) + outbound token TTL 기반 refresh. |
| graceful shutdown | inbound listener 종료 → in-flight request drain → outbound NotificationDispatcher worker 가 retry queue flush 또는 timeout → TLS context · token cache · DB pool close 후 종료 |
| crash recovery | SubscriptionStore (PostgreSQL) restart 후 복원. retry queue (`nssf_notification_retry_queue`) row-lock dequeue 로 미발송 notification 자연스 재개. TLS / token validator 는 stateless restart. |

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
- [[request-flow]] — sync · async 시점, TLS / mTLS / OAuth2 단계 매핑.
- [[state-persistence]] — SubscriptionStore · retry queue persistence (PostgreSQL).
- [[observability]] — runtime metric (active connections, in-flight requests, queue depth).
- [[configuration-strategy]] — `tls.*` / `mtls.*` / `oauth2_inbound.*` / `oauth2_outbound.*` config.
- `docs/adr/ADR-0004-project-security-baseline.md` — security capability 의무 source.
