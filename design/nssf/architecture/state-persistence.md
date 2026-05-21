---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF State Persistence

## Purpose

NSSF 가 보유하는 *상태* 의 분류, 영속성 요구, schema 를 정의한다. backend 선정은 engineering-design 단계에서 PostgreSQL/libpq 단일 backend 로 닫혔다 (`engineering/nssf/engineering-design.md` `persistence` slot). 본 문서는 그 결정의 *schema 와 layout* 을 명시한다.

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
| outbound notification retry queue | NotificationDispatcher | **persistent** | row-lock dequeue (FOR UPDATE SKIP LOCKED). at-least-once 정책. |
| OAuth2 token cache (inbound JWKS + outbound access token) | (공통 transport / NotificationDispatcher) | in-memory (runtime cache) | 만료/JWKS refresh 후 재요청. persistent 불필요. |
| TLS context / cert (cert·key·CA) | (공통 transport) | in-memory (start-up load) | config 의 `tls.cert_path` 등에서 load. |
| HTTP/2 connection pool | (공통 transport) | in-memory | runtime 자원. |

### Backend — PostgreSQL/libpq 단일 결정

engineering-design 결정 (Pane 2 교차검토 합의): persistent 상태 3 종 (subscription · availability · retry queue) 모두 PostgreSQL via libpq. 같은 backend 로 subscription 변경 + retry enqueue 1 트랜잭션 가능.

rejected — in_memory (restart 손실), file (scale-out 불가), kv (복잡 query · transaction 약).

### Schema

| table | columns (요약) | primary key | indexes |
|---|---|---|---|
| `nssf_subscriptions` | `subscription_id`, `callback_uri`, `event`, `filter_json` JSONB, `tai_filter_json` JSONB, `amf_id`, `amf_set_id`, `expiry timestamptz`, `supported_features`, `etag_version int`, `created_at`, `updated_at` | `subscription_id` | `(expiry)`, JSONB GIN on `filter_json` (query 필요 시) |
| `nssf_availability` | `tai_mcc`, `tai_mnc`, `tai_tac`, `tai_nid`, `amf_set_id`, `availability_json` JSONB, `supported_snssai_json` JSONB, `etag_version int`, `updated_at` | `(tai_mcc, tai_mnc, tai_tac, tai_nid)` | `(amf_set_id)`, JSONB GIN on `availability_json` |
| `nssf_notification_retry_queue` | `id`, `subscription_id` FK, `callback_uri`, `payload_json` JSONB, `correlation_id`, `status`, `attempt_count int`, `next_attempt_at timestamptz`, `locked_by`, `locked_until timestamptz`, `last_error`, `created_at`, `updated_at` | `id` | `(status, next_attempt_at)` |

constraints — `FK nssf_notification_retry_queue.subscription_id → nssf_subscriptions.subscription_id ON DELETE CASCADE`, `CHECK status IN ('pending','inflight','done','failed')`, `CHECK attempt_count >= 0`, `expiry NULL = non-expiring`.

### Strategy

- subscription · availability 양쪽 모두 PostgreSQL row 로 persistent.
- core key column 화 + spec payload/filter JSONB 병행 (Pane 2 보정 — JSONB-only 금지).
- retry queue = table-as-queue (row-lock dequeue via FOR UPDATE SKIP LOCKED).
- repository interface 추상화 — `SubscriptionRepository` / `AvailabilityRepository` interface, PostgreSQL 구현 단일. test seam 의 in-memory mock 은 unit/module-integration 한정.

## Open Questions

- *변경 이벤트* (AvailabilityEngine → NotificationDispatcher) 의 영속성 — 같은 transaction 안에서 retry queue enqueue 로 처리. 별도 event log table 필요 여부는 운영 모니터링 요구에 따라 dev 결정.
- 38.413 운영 결정 시 AMF reallocation 관련 상태 (예 redirect target AMF set cache) 추가 가능성.

## References

- [[module-boundaries]] — 상태 소유 모듈.
- [[runtime-model]] — crash recovery 단계 + TLS context / token cache lifecycle.
- [[request-flow]] — 변경 이벤트 발행 시점, outbound retry 흐름.
- [[configuration-strategy]] — `store.dsn` config + TLS/OAuth2 cache config.
- `handoff/nssf/contract.yaml` — subscription · availability schema.
- `engineering/nssf/engineering-design.md` `persistence` slot — PostgreSQL/libpq 단일 결정.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS context / OAuth2 cache 가 in-memory 인 이유 (security capability).
