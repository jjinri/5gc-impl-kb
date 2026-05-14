---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Configuration Strategy

## Purpose

NSSF 가 *외부 입력* 으로 받아야 하는 config 항목과 그 경계를 정의한다. 본 architecture 가 명시한 config key 가 dev 단계 config schema 의 기준선이다.

## Inputs (contract)

- `interface` topic — server URL, security scheme, supportedFeatures.
- 8 API topics — operation 별 input 의 validation 규칙.

## Boundaries

본 문서가 다루는 것 — config 항목 *분류 + key 이름 + 기본값 권고*.

본 문서가 다루지 않는 것 — config 파일 포맷 (yaml / env / TOML), 동적 reload 메커니즘, secret management (KMS / vault).

## Decisions

### config 분류

| 범주 | key (예) | default 권고 | 비고 |
|---|---|---|---|
| **transport** | `server.listen_addr`, `server.http2_max_streams` | listen `:8080` (dev), TLS *외부 라이브러리/sidecar* | 본 NSSF 는 transport 라이브러리 의존. |
| **TLS** | `tls.enabled`, `tls.cert_path`, `tls.key_path`, `tls.ca_bundle` | `tls.enabled=false` (dev), production 은 외부 정책 | SBA mTLS 는 운영/배포 결정. 33.501 보류와 연동. |
| **OAuth2 client (outbound)** | `oauth2.enabled`, `oauth2.token_url`, `oauth2.client_id`, `oauth2.client_secret_ref`, `oauth2.scope` | `oauth2.enabled=false` (옵션) | NotificationDispatcher 의 outbound POST 에 token 부착. enable 시 NRF token endpoint 호출. |
| **NRF discovery** | `nrf.base_url`, `nrf.discovery_cache_ttl` | TTL 60s | NSSelectionGet 의 target AMF set 조회. |
| **persistence backend** | `store.kind` (in-memory / file / kv / rdbms), `store.dsn` | `store.kind=in-memory` (dev) | `state-persistence.md` 의 backend 후보와 연동. |
| **notification** | `notify.retry.max_attempts`, `notify.retry.backoff_initial_ms`, `notify.retry.backoff_max_ms`, `notify.worker_pool_size` | `max_attempts=3`, `initial=1000`, `max=16000` | 권고일 뿐, 운영 환경 별 조정 가능. |
| **supportedFeatures** | `feature.flags.<name>` | 8 op 별 feature flag map | `interface.supportedFeatures` bitmask 와 매핑. |
| **observability** | `log.level`, `metrics.exporter`, `trace.sample_rate` | `log.level=info`, sample `0.1` | observability.md 의 sink 와 연동. |

### config 변경 정책

- *동적 reload* 권고 — supportedFeatures · retry 정책 · log.level.
- *재시작 필요* — listen addr · TLS cert · persistence backend kind / dsn.
- *런타임 변경 금지* — server.listen_addr, OAuth2 client_id (별도 인스턴스 의도가 명확하지 않으면).

### OAuth2 옵션 의도

- 본 NSSF 는 *outbound POST* 에 OAuth2 client credentials 부착을 *옵션* 으로 제공.
- enable=true 시 NotificationDispatcher 가 NRF (또는 별도 authz server) 에 token 요청 → 응답 outbound 호출에 부착.
- enable=false 시 outbound POST 는 본문만 (TLS / network 정책으로 보안 외부화).
- 본 결정의 사유 — 33.501 운영 결정 보류 상태에서 보안 정책을 *batteries-included* 로 강제하지 않고 운영 결정을 따른다.

## Open Questions

- TLS default 가 `enabled=false` 인 dev 권고가 architecture 문서에 적합한지 — production 권고도 같이 둘지.
- supportedFeatures flag map 의 *기본값* — 8 op 의 spec default 와 본 NSSF 의 enabled feature 차이.
- secret management — `oauth2.client_secret_ref` 가 어떤 ref 형식인지 (env / vault path) 는 본 architecture 가 형식을 강제하지 않는다 — dev 결정.

## References

- [[overview]] — full scope 정의.
- [[state-persistence]] — `store.kind` 선택지.
- [[runtime-model]] — config reload vs 재시작.
- [[observability]] — log / metrics / trace 설정.
- `handoff/nssf/contract.yaml` — `interface` topic 의 server URL · security.
