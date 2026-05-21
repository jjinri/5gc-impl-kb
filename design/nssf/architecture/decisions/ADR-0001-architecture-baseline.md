# ADR-0001 — NSSF architecture baseline

Date: 2026-05-14
Status: Accepted (full scope)
Source contract: `handoff/nssf/contract.yaml`

## Context

NSSF contract 가 `handoff_ready` 까지 도달 (PR #14 fresh tracking 정책 + PR #15 seed-gen schema_refs fix 머지 후) 했고, full NSSF 범위 (Nnssf_NSSelection 1 op + Nnssf_NSSAIAvailability 7 op = 8 op) 의 architecture 가 본격 시작된다. 2026-05-12 ~ 2026-05-14 사이 작성된 *단일 op (NSSelectionGet)* 가정의 architecture 산출은 폐기되고 본 architecture set 이 baseline 으로 교체된다.

본 architecture 는 spec 이 강제하는 책임 (8 op, ProblemDetails, OAuth2 사용 사실, SBI 헤더 전제) 을 유지하고, 구현 기술 선택 (언어, 런타임, DBMS, HTTP 라이브러리, 배포 토폴로지) 은 dev 단계 결정으로 남긴다.

## Decision

`design/nssf/architecture/` 의 문서 set (9 문서 + 본 ADR) 을 contract extraction 과 implementation planning 사이의 경계로 사용한다.

- 모듈 분해 — `SelectionEngine`, `AvailabilityEngine`, `SubscriptionStore`, `NotificationDispatcher` 4 모듈. `RequestValidator` / `ProblemDetailsMapper` 는 공통 utility, 모듈 미격상.
- outbound notification client — NotificationDispatcher 모듈에 *outbound HTTP/2 client* 책임 부속. outbound TLS / mTLS / OAuth2 client credentials production-capable code path 는 *항상 존재* (ADR-0004 의무 1·2·4), config (`tls.enabled` / `mtls.enabled` / `oauth2_outbound.enabled`) 가 enable/disable 만 결정. correlation-id 전파는 *필수* (inbound `3gpp-Sbi-Correlation-Info` → outbound 동일 header + log/trace 전파).
- subscription persistence — SubscriptionStore 가 책임. backend = PostgreSQL/libpq 단일 (engineering-design `persistence` slot 결정). `state-persistence.md` 의 `nssf_subscriptions` table schema 참조. repository interface 추상화 + test seam 의 in-memory mock 은 unit/module-integration 한정.
- 33.501 spec 은 *project security baseline ADR-0004* 으로 흡수 (lifecycle dependency 아님). SBA TLS / mTLS / inbound OAuth2 / outbound OAuth2 *production-capable code path 의무* 는 본 architecture 가 명시 반영 (request-flow / runtime-model / configuration-strategy / error-propagation / test-strategy 참조). 38.413 spec 은 `_manifest.yaml` `manual_overrides.exclude` 로 운영 결정 보류 — AMF reallocation via RAN 미구현이 default.

## Consequences

- `/nf-impl-plan nssf` 는 본 architecture 문서 set 을 review 후 입력으로 사용한다. 본 사이클 Phase 3 의 책임.
- contract 변경 시 영향 받는 architecture 문서를 먼저 갱신 후 implementation planning 을 진행한다.
- 본 architecture 의 `## Open Questions` 항목이 미해결 상태로 implementation planning 으로 흘러가면 dev task 의 *test plan* 이 해당 결정을 명시적으로 다루도록 강제된다 (test-strategy.md 의 *시나리오 카탈로그* 가 그 hook).
- `/nf-arch-status` · `/nf-impl-status` 같은 단계별 validator skill 은 본 사이클 범위 외, 별도 사이클 (ADR-0001 follow-up) 로 미룬다.

## Open choices

| choice | status | note |
|---|---|---|
| 구현 언어 / 런타임 | engineering 단계에서 닫힘 | `engineering/nssf/engineering-design.md` 의 `language` / `runtime` slot. NSSF = C / native binary. |
| persistence backend (subscription, availability) | engineering 단계에서 닫힘 | engineering-design 결정 = PostgreSQL/libpq 단일. `state-persistence.md` schema 참조. |
| HTTP/2 server / client library | engineering 단계에서 닫힘 | engineering-design `sbi_server_stack` / `sbi_client_stack` slot. server = nghttp2; client 는 ADR-0004 baseline 만족하는 TLS-capable HTTP/2 후보 비교 후 결정. |
| TLS library | engineering 단계에서 닫힘 | ADR-0004 의무 6 (third-party library 의무). engineering-design `tls_security` slot — OpenSSL/BoringSSL/mbedTLS 후보 비교 후 결정. |
| OAuth2 / JWT library | engineering 단계에서 닫힘 | ADR-0004 의무 6. engineering-design `oauth2_token_validation` slot (inbound) + outbound client 슬롯 — libjwt / library-native 후보 비교 후 결정. |
| 배포 토폴로지 (service mesh / NF 내장 TLS) | ADR-0004 baseline 으로 닫힘 | NF 내부 TLS stack 의무 (ADR-0004 의무 1). mesh sidecar 의존 단일값 close 금지. engineering-design `deployment_topology` slot 이 *추가* 토폴로지 (예 mesh sidecar 와 병행) 만 결정. |
| TLS version / cipher 세부 | 외부 처리 | ADR-0004 의무 7. NF 가 직접 결정하지 않음 — operator-provided config + library default. `configuration-strategy.md` `tls.cipher_suites` 옵션. |
| AMF reallocation via RAN 지원 (NGAP REROUTE NAS REQUEST) | 보류 | 운영 결정. 미지원이 default. 지원으로 전환 시 38.413 cp + manual_overrides.exclude 제거 + architecture hooking |

## References

- `_manifest.yaml` `manual_overrides.exclude` — 33.501 (ADR-0004 흡수) · 38.413 (운영 보류) 사유.
- `handoff/nssf/contract.yaml` — 8 API topic · 핵심 data-model.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS / mTLS / OAuth2 capability 의무 source. 본 architecture 의 security flow 가 baseline 을 반영.
- `engineering/nssf/engineering-design.md` — Open choices 의 engineering-에서-닫힘 항목 결정 (재-ratify 후 ADR-0004 baseline 만족).
- `docs/plans/2026-05-14-nssf-arch-dev-full-scope-rework-plan.md` — 본 사이클 plan.
- `docs/retros/2026-05-14-nssf-full-scope-seed-gen-fix-summary.md` — 본 baseline 직전 사이클 결정.
