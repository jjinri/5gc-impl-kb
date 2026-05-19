---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Request Flow

## Purpose

inbound 8 operation 의 처리 시퀀스와 outbound notification 의 발송 시퀀스를 정의한다. 본 시퀀스가 dev 단계 handler 구조의 기준선이다.

## Inputs (contract)

- 8 API topics — method · path · request body · response 정의.
- `interface` topic — security scheme, server URL, 공통 SBI header (fresh-full 주의: 현 generated `interface` output 은 materializer `auto_interface` first-YAML-only 구현이라 첫 primary YAML 기반 partial — `overview.md` `## Inputs` 참조).
- `error-handling` topic — status code 목록 (operation×cause 매트릭스는 spec/manual augmentation — `error-propagation.md` `## Inputs` 참조).

## Boundaries

다루는 시퀀스 — 9 종 (inbound 8 + outbound 1).

다루지 않는 것 — transport layer 의 HTTP/2 frame 처리, TLS handshake, OAuth2 token 검증의 내부 (라이브러리 책임).

## Decisions

### inbound 시퀀스 (공통 단계)

모든 inbound operation 은 다음 단계를 거친다.

1. **transport** — HTTP/2 frame 수신, TLS 종료, OAuth2 access token 검증, 3gpp-Sbi-Target-apiRoot / 3gpp-Sbi-Routing-Binding 등 헤더 파싱.
2. **request validation** — query param 형식, body schema (`SliceInfoForRegistration`·`NssaiAvailabilityInfo`·`PatchDocument`·`NssfEventSubscriptionCreateData` 등) 검증. 실패 → 400 `INVALID_QUERY_PARAM` 또는 422.
3. **module dispatch** — operation 종류에 따라 SelectionEngine · AvailabilityEngine · SubscriptionStore 중 한 모듈로 라우팅.
4. **business logic** — 해당 모듈이 응답 또는 상태 변경 수행. AvailabilityEngine 이 상태 변경 시 *변경 이벤트* 발행.
5. **response build** — 정상 응답 build (`AuthorizedNetworkSliceInfo`·`AuthorizedNssaiAvailabilityInfo`·201/204 등) 또는 ProblemDetails 매핑.

### operation 별 시퀀스 핵심

| operation | dispatch 모듈 | 핵심 단계 |
|---|---|---|
| `NSSelectionGet` | SelectionEngine | subscribed NSSAI 조회 (필요 시 UDM) → allowed NSSAI 계산 → target AMF set 결정 (NRF 조회 가능) → 응답 |
| `NSSAIAvailabilityPut` | AvailabilityEngine | TAI 별 availability 저장·덮어쓰기. 변경 이벤트 발행. |
| `NSSAIAvailabilityPatch` | AvailabilityEngine | JSON Patch 적용. 변경 이벤트 발행. |
| `NSSAIAvailabilityDelete` | AvailabilityEngine | AMF 의 availability 항목 제거. 변경 이벤트 발행. |
| `NSSAIAvailabilityOptions` | AvailabilityEngine | supportedFeatures 보고. 상태 변경 없음. |
| `NSSAIAvailabilityPost` (Subscribe) | SubscriptionStore | subscription resource 생성, id 발급, callback URI 보관 |
| `NSSAIAvailabilitySubModifyPatch` | SubscriptionStore | subscription 의 filter / lifetime 갱신 |
| `NSSAIAvailabilityUnsubscribe` | SubscriptionStore | subscription resource 제거 |

### outbound notification 시퀀스

availability 변경 이벤트 트리거.

1. **event source** — AvailabilityEngine 이 Put/Patch/Delete 후 *변경 이벤트* 발행.
2. **subscription resolve** — SubscriptionStore 에 lookup, 영향 받는 subscription 목록 산출.
3. **payload build** — `NssfEventSubscriptionCreatedData` 또는 spec 정의 notification payload.
4. **correlation** — 각 outbound 호출에 *correlation-id* (3gpp-Sbi-Correlation-Info 또는 자체 필드) 부착.
5. **outbound dispatch** — NotificationDispatcher 가 callback URI 로 POST 발송. OAuth2 client credentials 가 *config 로 enable* 인 경우 NRF 토큰 부착.
6. **retry / failure** — failure 시 exponential backoff retry (정책은 `## Open Questions`). 최종 실패는 metric / log 만, business 영향 없음.

## Open Questions

- NSSelectionGet 의 NRF discovery / UDM 조회를 *blocking* 으로 architecture 단계에 명시할지 *async hook* 으로 dev 단계에 위임할지.
- *변경 이벤트* 발행이 AvailabilityEngine 의 *commit 시점* 인지 *commit 직후 별도 step* 인지 — eventual consistency vs immediate.
- retry 정책 default (예 3 회 · 1s/4s/16s) 를 본 architecture 가 권고할지 dev 가 결정할지.

## References

- [[module-boundaries]] — dispatch 모듈 정의.
- [[runtime-model]] — async vs sync 결정.
- [[error-propagation]] — operation × response code 매트릭스.
- [[observability]] — correlation-id 전파.
- `handoff/nssf/contract.yaml` — operation method / path / schema 진실 출처.
