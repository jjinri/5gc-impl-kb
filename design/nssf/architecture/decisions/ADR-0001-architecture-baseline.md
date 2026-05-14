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
- outbound notification client — NotificationDispatcher 모듈에 *outbound HTTP/2 client* 책임 부속. OAuth2 client credentials 부착은 *config 옵션 (enable/disable)*. correlation-id 전파는 *필수* (inbound `3gpp-Sbi-Correlation-Info` → outbound 동일 header + log/trace 전파).
- subscription persistence — SubscriptionStore 가 책임. backend 후보 (in-memory / file / external KV / RDBMS) 는 `state-persistence.md` 의 `## Open Questions` 에 보류, dev 단계가 *repository interface* 추상화 후 선정.
- 33.501·38.413 spec 은 `_manifest.yaml` `manual_overrides.exclude` 로 보류. SBA TLS / AMF reallocation 관련 결정은 본 architecture 가 *strict default* 강요하지 않고 운영 결정을 따른다.

## Consequences

- `/nf-impl-plan nssf` 는 본 architecture 문서 set 을 review 후 입력으로 사용한다. 본 사이클 Phase 3 의 책임.
- contract 변경 시 영향 받는 architecture 문서를 먼저 갱신 후 implementation planning 을 진행한다.
- 본 architecture 의 `## Open Questions` 항목이 미해결 상태로 implementation planning 으로 흘러가면 dev task 의 *test plan* 이 해당 결정을 명시적으로 다루도록 강제된다 (test-strategy.md 의 *시나리오 카탈로그* 가 그 hook).
- `/nf-arch-status` · `/nf-impl-status` 같은 단계별 validator skill 은 본 사이클 범위 외, 별도 사이클 (ADR-0001 follow-up) 로 미룬다.

## Open choices

| choice | status | note |
|---|---|---|
| 구현 언어 / 런타임 | TBD | dev 단계 결정 |
| persistence backend (subscription, availability) | TBD | `state-persistence.md` 후보 표 참조 |
| HTTP/2 / TLS / OAuth2 client library | TBD | `configuration-strategy.md` 의 config 항목 기준 |
| 배포 토폴로지 (service mesh / NF 내장 TLS) | TBD | 운영 결정 |
| AMF reallocation via RAN 지원 (NGAP REROUTE NAS REQUEST) | 보류 | 운영 결정. 미지원이 default. 지원으로 전환 시 38.413 cp + manual_overrides.exclude 제거 + architecture hooking |
| SBA security profile 깊이 (TLS version, cipher) | 보류 | 33.501 cp 운영 결정. 본 architecture 는 *config 외부화* 권고 |
| Subscription store backend default 권고 | 보류 | `state-persistence.md` `## Open Questions` |

## References

- `_manifest.yaml` `manual_overrides.exclude` — 33.501·38.413 보류 사유.
- `handoff/nssf/contract.yaml` — 8 API topic · 핵심 data-model.
- `docs/plans/2026-05-14-nssf-arch-dev-full-scope-rework-plan.md` — 본 사이클 plan.
- `docs/retros/2026-05-14-nssf-full-scope-seed-gen-fix-summary.md` — 본 baseline 직전 사이클 결정.
