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

다루지 않는 것 — TLS handshake 의 *cryptographic primitive 내부* (library 책임 — ADR-0004 의무 6) · OAuth2 token signature verification 의 *primitive 내부* (library 책임). NF 는 cert/key/CA load, validator/cache 초기화, peer/scope/audience 검사 *호출* 까지 책임.

## Decisions

### inbound 시퀀스 (공통 단계)

모든 inbound operation 은 다음 단계를 거친다. [ADR-0004](../../../docs/adr/ADR-0004-project-security-baseline.md) baseline 의 production-capable security path 가 *항상 존재* 하며, 1~2-pre 단계의 enable/disable 만 config 가 결정한다.

1. **transport — HTTP/2 frame 수신**. nghttp2 server (engineering-design 결정) 가 connection / stream 처리. TLS 적용 시 handshake 는 본 단계 이전에 library 가 수행 (NF 는 cert/key/CA load 책임 — `runtime-model.md` start-up 참조).
1-a. **mTLS peer verify** (`mtls.enabled=true` 시). server 측에서 client cert chain + peer identity (SAN / 33.310 NF cert identity) verification. 실패 → connection level reject, 응답 없음 (TLS alert).
1-b. **inbound OAuth2 bearer validation** (`oauth2_inbound.enabled=true` 시). `Authorization: Bearer <jwt>` header 파싱 → third-party JWT library 로 signature / expiry / audience / scope 검증. 실패 → 401 `INVALID_TOKEN` 또는 403 `INSUFFICIENT_SCOPE` (`error-propagation.md` 매트릭스 참조).
1-c. **SBI header parse** — 3gpp-Sbi-Target-apiRoot / 3gpp-Sbi-Routing-Binding / 3gpp-Sbi-Correlation-Info 파싱.
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
5. **outbound OAuth2 token acquire / attach** (`oauth2_outbound.enabled=true` 시). NotificationDispatcher 가 token cache lookup → miss 시 NRF (또는 별도 authz server) token endpoint 호출 (client_credentials grant) → cache store → outbound POST 의 `Authorization: Bearer <token>` header 부착. token 획득 실패는 outbound 호출 실패로 분류.
6. **outbound dispatch** — NotificationDispatcher 가 callback URI 로 POST 발송. TLS / mTLS 적용 (`tls.enabled` / `mtls.enabled`) — outbound TLS handshake 는 library, NF 는 cert/CA load 책임.
7. **retry / failure** — failure 시 exponential backoff retry (engineering-design 결정 — max_attempts=10, initial=1s, cap=60s). retry queue persist = `nssf_notification_retry_queue` (PostgreSQL row-lock dequeue FOR UPDATE SKIP LOCKED). 최종 실패는 dead-letter (log + metric), business 영향 없음.

## Open Questions

- NSSelectionGet 의 NRF discovery / UDM 조회를 *blocking* 으로 architecture 단계에 명시할지 *async hook* 으로 dev 단계에 위임할지.
- *변경 이벤트* 발행이 AvailabilityEngine 의 *commit 시점* 인지 *commit 직후 별도 step* 인지 — eventual consistency vs immediate.
- outbound OAuth2 token cache miss + token endpoint 실패의 분류 — retry queue 진입 vs 즉시 dead-letter.

## References

- [[module-boundaries]] — dispatch 모듈 정의.
- [[runtime-model]] — async vs sync 결정, TLS context / token validator lifecycle.
- [[error-propagation]] — operation × response code 매트릭스 (TLS / mTLS / OAuth2 실패 case 포함).
- [[observability]] — correlation-id 전파.
- [[configuration-strategy]] — `tls.*` / `mtls.*` / `oauth2_inbound.*` / `oauth2_outbound.*` config.
- `handoff/nssf/contract.yaml` — operation method / path / schema 진실 출처.
- `docs/adr/ADR-0004-project-security-baseline.md` — security capability 의무 source.
