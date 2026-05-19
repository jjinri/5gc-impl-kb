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

- 구현 언어·런타임·DBMS·HTTP 라이브러리 선택 (dev 단계).
- 33.501·38.413 spec 깊이 (운영 결정 보류, `_manifest.yaml` `manual_overrides.exclude` 참고).
- 배포 토폴로지·service mesh / sidecar 선택.

## Decisions

| decision | 내용 |
|---|---|
| Scope | NSSF 의 두 service 전체. 단일 op 가정은 폐기. |
| Modules | SelectionEngine, AvailabilityEngine, SubscriptionStore, NotificationDispatcher 4 모듈. |
| Outbound client | NotificationDispatcher 가 outbound HTTP/2 client 보유. OAuth2 client credentials 는 *config 옵션* (enable / disable). correlation-id 추적은 필수. |
| Persistence | SubscriptionStore 가 subscription lifecycle persistence 책임. backend 후보 (in-memory / file / external KV) 는 `state-persistence.md` 의 `## Open Questions` 에 정리. |

## Open Questions

- SBI security profile (TLS version·cipher·mutual auth) 의 깊이 — 33.501 cp 결정 보류 상태에서 어떤 default 를 architecture 가 *권고* 할지.
- AMF reallocation via RAN 지원 — 38.413 §8.6.5 미구현 결정이 확정되기 전 architecture 가 hooking 자리를 둘지.
- SubscriptionStore backend 의 default — in-memory 가 본 architecture 의 default 권고가 될지, 결정 자체를 dev 단계로 미룰지.

## References

- [[module-boundaries]] — 4 module 책임·seam.
- [[request-flow]] — 8 operation + Notify outbound 시퀀스.
- [[runtime-model]] — request-response + notification dispatcher long-lived state.
- [[state-persistence]] — subscription persistence.
- [[configuration-strategy]] — config keys + OAuth2 옵션.
- [[error-propagation]] — 8 operation ProblemDetails 매핑.
- [[observability]] — log·metric·trace + correlation-id 전파.
- [[test-strategy]] — architecture seam test.
- `decisions/ADR-0001-architecture-baseline.md`.
- `_manifest.yaml` `manual_overrides.exclude` — 33.501·38.413 보류 사유.
