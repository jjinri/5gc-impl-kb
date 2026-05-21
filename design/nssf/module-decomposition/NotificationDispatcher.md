---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NotificationDispatcher

## Responsibility

NSSF 의 *유일한 outbound 책임* — availability 변경 이벤트와 SubscriptionStore 의 영향 받는 subscription 목록을 결합해 callback URI 로 outbound POST 발송한다. correlation-id 전파, retry, dead-letter 정책을 가진다. 본 모듈은 *outbound* 만 — inbound 처리 책임 없음, subscription 의 CRUD 책임 없음.

## Inputs

- AvailabilityEngine 의 *변경 이벤트* → SubscriptionStore 의 *영향 받는 subscription list* (id, callbackUri, filter).
- 본 NSSF 의 *correlation-id source* — `observability.md` 의 correlation context (inbound `3gpp-Sbi-Correlation-Info` 또는 신규 생성).
- outbound OAuth2 token cache (`configuration-strategy.md` 의 `oauth2_outbound.enabled=true` 일 때) — production-capable code path 는 항상 존재 (ADR-0004 의무 4).
- retry queue (`state-persistence.md` 의 `nssf_notification_retry_queue` PostgreSQL table).

## Outputs

- outbound HTTP/2 POST — callback URI 로, payload = `NssfEventSubscriptionCreatedData` 또는 spec 정의 notification body.
- log/metric/trace — 발송 결과 (success / retry / dead-letter).
- retry queue mutation — 실패 시 backoff 후 재시도 일정 등록.

## State

- *outbound HTTP/2 client connection pool* — per-callback-host pool (또는 shared, `runtime-model.md` 의 open question). nghttp2 client (engineering-design 결정).
- *retry queue* — `nssf_notification_retry_queue` (PostgreSQL). row-lock dequeue (FOR UPDATE SKIP LOCKED). subscription 변경 + enqueue 1 트랜잭션.
- *outbound OAuth2 token cache* — `oauth2_outbound.enabled=true` 시 in-memory cache. expiry 기반 refresh. miss 시 token endpoint 호출.
- *TLS / mTLS context* — `tls.enabled` / `mtls.enabled` 시 cert/key/CA load (start-up). outbound handshake 는 library 책임.
- *correlation context* — 호출 lifetime 동안만, 영속 아님.

## Decisions

| 결정 | 내용 |
|---|---|
| 단일 책임 | outbound 만. subscription 의 CRUD 와 분리. |
| correlation-id 전파 | **필수**. inbound trigger 의 correlation-id → outbound `3gpp-Sbi-Correlation-Info` header + W3C `traceparent` baggage + log/metric exemplar. 부재 시 NSSF 자체 생성. |
| outbound OAuth2 client credentials | ADR-0004 의무 4 — *production-capable code path 항상 존재*. config `oauth2_outbound.enabled` 가 enable/disable 만 결정. true 시 token endpoint (NRF 또는 별도 authz server) 호출 → cache → outbound `Authorization: Bearer ...` 부착. false 시 token 미부착 (dev/local profile). |
| outbound TLS / mTLS | ADR-0004 의무 1·2 — production-capable code path 항상 존재. `tls.enabled` / `mtls.enabled` config 가 enable/disable. NF 는 cert/CA load + library 호출 책임, handshake primitive 는 library. |
| HTTP client lib | engineering-design 결정 (TLS-capable HTTP/2 후보 비교: libcurl / nghttp2 direct / nghttp2+libuv). 본 모듈은 *interface* 만 — connection reuse, HTTP/2 multiplexing, TLS context 지원 가정. |
| retry 정책 | exponential backoff. engineering-design 결정 — `max_attempts=10, initial=1s, cap=60s` (configurable via `notify.retry.*`). |
| 실패 분류 | 4xx → no retry, log + metric. 5xx / timeout / connection error / outbound TLS handshake fail / OAuth2 token endpoint 5xx → retry. OAuth2 client_secret invalid → 즉시 dead-letter + alert. max_attempts 초과 → dead-letter. |
| ordering 보장 | *없음*. 같은 subscription 의 두 outbound 가 어느 순서로 도착할지 spec 강제 아님. callback receiver (AMF) 가 *idempotent* 가정. |

## Open Questions

- dead-letter 의 후처리 — log 만 vs alerting vs subscription 자동 비활성화.
- callback 가 4xx (특히 401 - subscription 만료) 일 때 본 NSSF 가 *자동 deactivate* 할지 — `error-propagation.md` open question 과 연동.
- outbound OAuth2 token cache miss + token endpoint 실패의 분류 (retry queue vs 즉시 dead-letter) — `request-flow.md` open question 과 연동.
- worker pool size default 권고.

## References

- [[../architecture/module-boundaries]] — 4 모듈 책임.
- [[../architecture/request-flow]] — outbound notification 시퀀스 (token acquire/attach 단계 포함).
- [[../architecture/runtime-model]] — outbound async + concurrency + TLS/token cache lifecycle.
- [[../architecture/state-persistence]] — `nssf_notification_retry_queue` PostgreSQL schema + OAuth2 token cache.
- [[../architecture/configuration-strategy]] — `tls.*`, `mtls.*`, `oauth2_outbound.*`, `notify.retry.*`, `notify.worker_pool_size`.
- [[../architecture/error-propagation]] — outbound 실패 분류 + security 실패 case.
- [[../architecture/observability]] — correlation-id 전파.
- `docs/adr/ADR-0004-project-security-baseline.md` — security capability 의무 source.
- [[SubscriptionStore]] — 영향 받는 subscription source.
- [[AvailabilityEngine]] — 변경 이벤트 source.
