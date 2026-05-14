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
- OAuth2 token cache (옵션, `configuration-strategy.md` 의 `oauth2.enabled=true` 일 때).
- retry queue (`state-persistence.md` 의 backend 후보).

## Outputs

- outbound HTTP/2 POST — callback URI 로, payload = `NssfEventSubscriptionCreatedData` 또는 spec 정의 notification body.
- log/metric/trace — 발송 결과 (success / retry / dead-letter).
- retry queue mutation — 실패 시 backoff 후 재시도 일정 등록.

## State

- *outbound HTTP/2 client connection pool* — per-callback-host pool (또는 shared, `runtime-model.md` 의 open question).
- *retry queue* — 미발송 + 실패 + 재시도 대기. backend 선택 (`state-persistence.md`).
- *OAuth2 token cache* — 활성화 시. expiry 기반 갱신. in-memory.
- *correlation context* — 호출 lifetime 동안만, 영속 아님.

## Decisions

| 결정 | 내용 |
|---|---|
| 단일 책임 | outbound 만. subscription 의 CRUD 와 분리. |
| correlation-id 전파 | **필수**. inbound trigger 의 correlation-id → outbound `3gpp-Sbi-Correlation-Info` header + W3C `traceparent` baggage + log/metric exemplar. 부재 시 NSSF 자체 생성. |
| OAuth2 client credentials | **옵션** — config `oauth2.enabled` true 시 NRF 또는 별도 authz server 에서 token 받아 outbound `Authorization: Bearer ...` 부착. false 시 token 미부착 (TLS / network 정책으로 보안 외부화). |
| HTTP client | dev 단계 라이브러리 선택. 본 architecture 는 *interface* 만 — connection reuse, HTTP/2 multiplexing 지원 가정. |
| retry 정책 | exponential backoff. default 권고 `max_attempts=3, initial=1s, max=16s` (configurable). |
| 실패 분류 | 4xx → no retry, log + metric. 5xx / timeout / connection error → retry. max_attempts 초과 → dead-letter. |
| ordering 보장 | *없음*. 같은 subscription 의 두 outbound 가 어느 순서로 도착할지 spec 강제 아님. callback receiver (AMF) 가 *idempotent* 가정. |
| transport 책임 | TLS, HTTP/2 frame 자체는 transport layer / 라이브러리. 본 모듈은 HTTP semantic layer 만. |

## Open Questions

- retry queue 가 SubscriptionStore 와 같은 backend 공유 vs 분리 (`state-persistence.md` open question).
- dead-letter 의 후처리 — log 만 vs alerting vs subscription 자동 비활성화.
- OAuth2 token 갱신 실패 정책 — outbound 보류 vs 토큰 없이 시도.
- callback 가 4xx (특히 401 - subscription 만료) 일 때 본 NSSF 가 *자동 deactivate* 할지 — `error-propagation.md` open question 과 연동.
- worker pool size default 권고.

## References

- [[../architecture/module-boundaries]] — 4 모듈 책임.
- [[../architecture/request-flow]] — outbound notification 시퀀스.
- [[../architecture/runtime-model]] — outbound async + concurrency.
- [[../architecture/state-persistence]] — retry queue · OAuth2 token cache.
- [[../architecture/configuration-strategy]] — `oauth2.enabled`, `notify.retry.*`, `notify.worker_pool_size`.
- [[../architecture/error-propagation]] — outbound 실패 분류 정책.
- [[../architecture/observability]] — correlation-id 전파.
- [[SubscriptionStore]] — 영향 받는 subscription source.
- [[AvailabilityEngine]] — 변경 이벤트 source.
