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

- `interface` topic — server URL, security scheme, supportedFeatures (fresh-full 주의: 현 generated `interface` output 은 materializer `auto_interface` first-YAML-only 구현이라 첫 primary YAML 기반 partial — `overview.md` `## Inputs` 참조).
- 8 API topics — operation 별 input 의 validation 규칙.

## Boundaries

본 문서가 다루는 것 — config 항목 *분류 + key 이름 + 기본값 권고*.

본 문서가 다루지 않는 것 — config 파일 포맷 (yaml / env / TOML), 동적 reload 메커니즘, secret management (KMS / vault).

## Decisions

### config 분류

본 NSSF 는 [ADR-0004](../../../docs/adr/ADR-0004-project-security-baseline.md) project security baseline 의 *production-capable code path 의무* 를 따른다. TLS / mTLS / inbound OAuth2 / outbound OAuth2 capability 가 NF 바이너리 *내부* 에 항상 존재하며, 본 config 가 enable/disable 만 결정한다 (path 자체 누락 금지).

| 범주 | key (예) | default 권고 | 비고 |
|---|---|---|---|
| **transport** | `server.listen_addr`, `server.http2_max_streams` | listen `:8080` | nghttp2-based HTTP/2 server (engineering-design 결정). |
| **TLS (inbound + outbound)** | `tls.enabled`, `tls.cert_path`, `tls.key_path`, `tls.ca_bundle`, `tls.min_version`, `tls.cipher_suites` (옵션 — 명시 시 library 에 전달) | `tls.enabled=false` (dev), `tls.enabled=true` (production) | NF 내부 TLS stack 보유 (ADR-0004 의무 1). cipher/version 은 NF 가 *직접 결정하지 않음* — library default + 본 config 로 전달, compliance 는 library + operator. dev disable 가능, production code path 항상 존재. |
| **mTLS (peer auth)** | `mtls.enabled`, `mtls.client_cert_required` (server 측), `mtls.client_cert_path`, `mtls.client_key_path` (client 측), `mtls.peer_verify` | `mtls.enabled=false` (dev), `mtls.enabled=true` + `client_cert_required=true` (production) | server/client cert + CA bundle + peer verification path (ADR-0004 의무 2). cert 형식은 operator-provided compliant cert (33.310 profile 외부 책임). |
| **OAuth2 inbound (bearer validation)** | `oauth2_inbound.enabled`, `oauth2_inbound.issuer`, `oauth2_inbound.jwks_uri`, `oauth2_inbound.expected_audience`, `oauth2_inbound.required_scopes` | `oauth2_inbound.enabled=false` (dev), `true` (production) | inbound access token 검증 path (ADR-0004 의무 3). signature/expiry/audience/scope 검증. third-party JWT library (engineering decision). dev disable 가능, production code path 항상 존재. |
| **OAuth2 outbound (client credentials / token attach)** | `oauth2_outbound.enabled`, `oauth2_outbound.token_url`, `oauth2_outbound.client_id`, `oauth2_outbound.client_secret_ref`, `oauth2_outbound.scope`, `oauth2_outbound.token_cache_ttl` | `oauth2_outbound.enabled=false` (dev), `true` (production) | outbound 호출 (NRF discovery, AMF callback 등) 에 access token 부착 (ADR-0004 의무 4). NRF token endpoint 호출 + cache. |
| **NRF discovery** | `nrf.base_url`, `nrf.discovery_cache_ttl` | TTL 60s | NSSelectionGet 의 target AMF set 조회. outbound TLS / OAuth2 적용. |
| **persistence backend** | `store.dsn` | PostgreSQL/libpq (engineering-design 결정) | `state-persistence.md` schema 참조. backend 후보 제거 (단일 결정). |
| **notification** | `notify.retry.max_attempts`, `notify.retry.backoff_initial_ms`, `notify.retry.backoff_max_ms`, `notify.worker_pool_size` | `max_attempts=10`, `initial=1000`, `max=60000` (engineering-design 결정) | retry queue 는 `nssf_notification_retry_queue` table (PostgreSQL row-lock dequeue). |
| **supportedFeatures** | `feature.flags.<name>` | 8 op 별 feature flag map | `interface.supportedFeatures` bitmask 와 매핑. |
| **observability** | `log.level`, `metrics.exporter`, `trace.sample_rate` | `log.level=info`, sample `0.1` | observability.md 의 sink 와 연동. |

### config 변경 정책

- *동적 reload* 권고 — supportedFeatures · retry 정책 · log.level · `oauth2_*.token_cache_ttl`.
- *재시작 필요* — listen addr · TLS cert/key/CA · mTLS cert · `oauth2_inbound.jwks_uri` · persistence dsn.
- *런타임 변경 금지* — server.listen_addr · OAuth2 client_id (별도 인스턴스 의도가 명확하지 않으면).

### Security baseline 적용 의도

- 본 NSSF 는 ADR-0004 baseline 7 항목을 따른다 — TLS / mTLS / inbound OAuth2 / outbound OAuth2 *production-capable code path* 가 NF 바이너리 안에 항상 존재한다.
- `*.enabled=false` 는 *capability 비활성화* 이지 *path 제거* 가 아니다 — disable 상태에서도 production-ready code path 는 보존되며 단순히 config flag 가 false 일 뿐이다.
- cipher suite / TLS version / 33.310 certificate profile / 33.210 cipher profile 등 *세부 정책* 은 NF 가 직접 결정하지 않는다 — operator-provided compliant cert/config + maintained library compliance 가 보장.
- TLS/JWT primitive 직접 구현 금지 (ADR-0004 의무 6). 후보 library 는 `engineering/nssf/engineering-design.md` 의 `sbi_server_stack` / `sbi_client_stack` / 관련 slot 에서 결정.

## Open Questions

- supportedFeatures flag map 의 *기본값* — 8 op 의 spec default 와 본 NSSF 의 enabled feature 차이.
- secret management — `oauth2_outbound.client_secret_ref` / `mtls.client_key_path` 등의 ref 형식 (env / vault path / file path) — 본 architecture 가 형식을 강제하지 않는다, engineering-design 또는 operator 결정.
- `tls.cipher_suites` 명시 강제 여부 — default 는 library default, operator 가 명시할 경우 본 config 로 전달. compliance enforcement 는 외부 책임.

## References

- [[overview]] — full scope 정의.
- [[state-persistence]] — PostgreSQL schema.
- [[runtime-model]] — config reload vs 재시작, TLS context / token validator lifecycle.
- [[observability]] — log / metrics / trace 설정.
- `handoff/nssf/contract.yaml` — `interface` topic 의 server URL · security.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS / mTLS / OAuth2 capability 의무 source.
- `engineering/nssf/engineering-design.md` — TLS / OAuth2 / sbi_server_stack / sbi_client_stack lib 결정.
