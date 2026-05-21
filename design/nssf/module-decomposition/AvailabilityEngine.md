---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# AvailabilityEngine

## Responsibility

NSSF inbound `Nnssf_NSSAIAvailability` service 의 *동기 CRUD* 처리 — TA 별 S-NSSAI availability map 의 store / lookup / patch / delete. AMF 가 보낸 availability 변화는 본 모듈이 commit 하고, *변경 이벤트* 를 발행해 SubscriptionStore + NotificationDispatcher 가 후속 처리한다. 본 모듈은 *동기 CRUD* 만 — subscription lifecycle 과 outbound notification 책임 없음.

## Inputs

- inbound — `api/NSSAIAvailabilityPut`, `api/NSSAIAvailabilityPatch`, `api/NSSAIAvailabilityDelete`, `api/NSSAIAvailabilityOptions`.
- contract data-model — `NssaiAvailabilityInfo`, `AuthorizedNssaiAvailabilityInfo`, `PatchDocument`, `Tai`, `Snssai`, `SupportedFeatures`.
- persistence — availability repository interface (`state-persistence.md` 의 `nssf_availability` PostgreSQL table).

## Outputs

- 정상 — `AuthorizedNssaiAvailabilityInfo` (Put 200) / 204 (Patch · Delete) / 200 (Options · supportedFeatures).
- error — 의도된 cause → ProblemDetailsMapper.
- **변경 이벤트** — `(tai, change_type, before, after)` payload 의 추상 이벤트. SubscriptionStore + NotificationDispatcher 가 소비.
- 정의된 collaborator — availability repository 의 동기 commit.

## State

- *AMF 가 본 NSSF 에 등록한* TAI × S-NSSAI availability map 의 *논리적 소유자*.
- 실제 backend 는 repository interface 뒤로 격리. production backend = PostgreSQL/libpq (engineering-design 결정). test seam 의 in-memory mock 은 unit/module-integration 한정.
- 모듈 자체는 *runtime cache* 보유 안 함 (cache 가 필요하면 repository 가 내부적으로).

## Decisions

| 결정 | 내용 |
|---|---|
| 단일 책임 | 동기 CRUD 만. subscription · notification · outbound 책임 분리. |
| commit + 이벤트 ordering | commit 성공 후 *동기* 변경 이벤트 발행. fail 시 이벤트 미발행. |
| atomic 단위 | TAI 단위 atomic. 같은 TAI 동시 Put 은 backend 정책 (lock 또는 optimistic concurrency) 따라 serialize. |
| Options 응답 | spec 의 `supportedFeatures` bitmask 반환. config 의 `feature.flags` 와 매핑. |
| Patch 적용 | `PatchDocument` (JSON Patch / Merge Patch) 적용 후 *결과* 가 spec invariant 위반 시 422. |

## Open Questions

- 변경 이벤트 발행이 *in-process call* vs *bus* — `module-boundaries.md` 의 open question 과 동일.
- 412 etag 정책 지원 — Patch 시 client 의 if-match 확인 강제 여부.
- availability map 의 lifetime — AMF 가 deregister 안 하면 stale 채로 남는 정책.

## References

- [[../architecture/module-boundaries]] — 4 모듈 책임.
- [[../architecture/request-flow]] — Put/Patch/Delete 시퀀스 + 변경 이벤트.
- [[../architecture/state-persistence]] — availability repository PostgreSQL table.
- `handoff/nssf/contract.yaml` `api/NSSAIAvailability{Put,Patch,Delete,Options}` topics.
- [[SubscriptionStore]] — 변경 이벤트 consumer.
- [[NotificationDispatcher]] — outbound 발송.
