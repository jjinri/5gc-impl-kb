---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Architecture Overview

## Purpose

본 문서는 full NSSF contract 를 architecture 책임으로 변환한다 — *어떤 모듈* 이 *어떤 책임* 을 *어떤 경계* 로 가지는지 정의한다. 구현 언어·런타임·DBMS·배포·라이브러리 선택은 dev 단계의 책임이다.

## Inputs (contract)

- `handoff/nssf/contract.yaml` (handoff-v2, 25 topics).
- API topics 8 — `api/NSSelectionGet`, `api/NSSAIAvailability{Put,Patch,Delete,Post,Unsubscribe,SubModifyPatch,Options}`.
- 핵심 data-model topics — `data-model/{SliceInfoForRegistration, AuthorizedNetworkSliceInfo, NssaiAvailabilityInfo, AuthorizedNssaiAvailabilityInfo, NssfEventSubscriptionCreateData, NssfEventSubscriptionCreatedData, Snssai, Tai, PlmnId, NFType, PatchDocument, ...}`.
- 공통 topics — `interface`, `error-handling`.
- 주의 (2026-05-18 fresh-full contract 기준). `interface` topic 의 의도는 NF service-level 공통 표면 전체 대표지만, 현 generated `interface` output 은 materializer (`auto_interface`) 의 first-YAML-only 구현 때문에 `_manifest.yaml` `primary_files.yamls` 중 첫 번째 (`TS29531_Nnssf_NSSAIAvailability.yaml`) 기반 partial output 이며 `Nnssf_NSSelection` service-level metadata 가 누락된다. 본 architecture 의 full scope (두 service · 8 op) 근거는 `primary_files.yamls` 전체와 그로부터 산출된 8 api topic 이며 현 generated `interface` output 단독이 아니다.

## Boundaries

본 architecture 가 다루는 것.

- 두 SBI service (Nnssf_NSSelection · Nnssf_NSSAIAvailability) 의 8 operation 모두.
- inbound request 처리 (request validation, slice selection / availability CRUD, subscription lifecycle).
- outbound notification client (NSSF → AMF callback, correlation, retry).
- ProblemDetails 매핑, 공통 SBI header 처리.
- module 분해 (SelectionEngine, AvailabilityEngine, SubscriptionStore, NotificationDispatcher).

본 architecture 가 다루지 않는 것.

- 구현 lib 후보 비교 / 선택 (engineering-design 단계 — `engineering/nssf/engineering-design.md` 의 `sbi_server_stack` / `sbi_client_stack` / `tls_security` / `oauth2_token_validation` / `persistence` slot).
- 33.501 spec 본문 깊이 — `docs/adr/ADR-0004-project-security-baseline.md` (project security baseline) 으로 흡수, lifecycle extraction dependency 아님. 본 architecture 는 baseline 의무 (TLS/mTLS/OAuth2 production-capable path) 를 *반영* 하되 spec 본문 인용은 않는다.
- 33.310 / 33.210 certificate / cipher profile 세부 — operator-provided compliant cert/config + library compliance 외부 책임 (ADR-0004 의무 7).
- 38.413 spec 깊이 — AMF reallocation via RAN 운영 결정 보류, `_manifest.yaml` `manual_overrides.exclude` 참조.
- 배포 토폴로지 추가 (예 service mesh 와 병행) — engineering-design `deployment_topology` slot.

## Decisions

| decision | 내용 |
|---|---|
| Scope | NSSF 의 두 service 전체. 단일 op 가정은 폐기. |
| Modules | SelectionEngine, AvailabilityEngine, SubscriptionStore, NotificationDispatcher 4 모듈. |
| Outbound client | NotificationDispatcher 가 outbound HTTP/2 client 보유 (nghttp2). outbound OAuth2 client credentials 는 *config 로 enable/disable* (default off in dev, on in production; ADR-0004 의무 4). production-capable code path 항상 존재. correlation-id 추적은 필수. |
| Inbound security | inbound TLS (`tls.enabled`), mTLS peer verify (`mtls.enabled`), OAuth2 bearer validation (`oauth2_inbound.enabled`) 가 *NF 바이너리 내부* 에 항상 production-capable code path 로 존재 (ADR-0004 의무 1~3). config 가 enable/disable 만 결정. |
| Persistence | SubscriptionStore + AvailabilityEngine + retry queue = PostgreSQL/libpq 단일 backend (engineering-design 결정). `state-persistence.md` schema 참조. test seam 의 in-memory mock 은 unit/module-integration 한정. |

## Open Questions

- AMF reallocation via RAN 지원 — 38.413 §8.6.5 미구현 결정이 확정되기 전 architecture 가 hooking 자리를 둘지.

## References

- [[module-boundaries]] — 4 module 책임·seam.
- [[request-flow]] — 8 operation + Notify outbound 시퀀스 (TLS/mTLS/OAuth2 단계).
- [[runtime-model]] — request-response + notification dispatcher long-lived state + TLS context / token validator lifecycle.
- [[state-persistence]] — PostgreSQL/libpq 단일 backend + retry queue schema.
- [[configuration-strategy]] — config keys (TLS/mTLS/OAuth2 inbound·outbound 포함).
- [[error-propagation]] — 8 operation ProblemDetails + security 실패 case.
- [[observability]] — log·metric·trace + correlation-id 전파.
- [[test-strategy]] — architecture seam test + security 시나리오.
- `decisions/ADR-0001-architecture-baseline.md`.
- `docs/adr/ADR-0004-project-security-baseline.md` — security capability 의무 source.
- `engineering/nssf/engineering-design.md` — lib 후보 비교 / 선택.
- `_manifest.yaml` `manual_overrides.exclude` — 33.501 (ADR-0004 흡수) · 38.413 (운영 보류) 사유.
