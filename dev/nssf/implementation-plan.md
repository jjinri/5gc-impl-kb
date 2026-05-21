---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Implementation Plan

## Scope

본 implementation plan 은 full NSSF (Nnssf_NSSelection 1 op + Nnssf_NSSAIAvailability 7 op = 8 operation) 와 outbound notification client (NSSF → AMF callback) 를 *구현 작업 단위* 로 변환한다.

본 plan 은 `docs/adr/ADR-0004-project-security-baseline.md` (TLS/mTLS/OAuth2 capability 의무 source) + `engineering/nssf/engineering-design.md` (선택된 lib/구조) 두 source 를 따른다.

포함 범위.

- inbound SBI handler (8 operation), request validation, ProblemDetails 매핑, observability.
- **inbound security** — TLS / mTLS / OAuth2 bearer validation production-capable code path (ADR-0004 의무 1~3). config 가 enable/disable. dev disable 가능, production-capable path 항상 존재.
- 4 모듈 (`SelectionEngine` · `AvailabilityEngine` · `SubscriptionStore` · `NotificationDispatcher`) 의 핵심 비즈니스 로직.
- outbound HTTP/2 client (NotificationDispatcher, nghttp2) — correlation-id 전파 필수.
- **outbound security** — outbound TLS / mTLS / OAuth2 client credentials (token acquire/attach) production-capable code path (ADR-0004 의무 1·2·4). config 가 enable/disable.
- repository interface (subscription · availability) — 추상화 + PostgreSQL/libpq backend (engineering-design 결정). test seam 의 in-memory mock 은 unit/module-integration 한정.
- retry queue (`nssf_notification_retry_queue`, PostgreSQL row-lock dequeue, at-least-once), dead-letter, metric / trace 통합.
- contract-derived unit / integration / scenario test seam (TLS / mTLS / OAuth2 시나리오 포함).

제외 범위.

- engineering-design 의 frozen 결정 재논의 (language / http stack / TLS lib / OAuth2 lib / persistence backend / codegen tool / json runtime / deployment). 본 plan 은 이 결정을 *적용* 만 함.
- 33.501 본문 인용 — ADR-0004 (project security baseline) 으로 흡수, lifecycle extraction dependency 아님.
- 33.310 / 33.210 certificate / TLS / cipher profile 세부 — operator-provided compliant cert/config + library compliance assumption (ADR-0004 의무 7).
- TLS / X.509 / JWT / JWS primitive 직접 구현 (ADR-0004 의무 6 — maintained third-party library 만).
- 38.413 — AMF reallocation via RAN (NGAP REROUTE NAS REQUEST) 미구현 결정 유지. 운영 결정 변경 시 별도 사이클.
- contract markdown body (현재 handoff yaml 까지만 — `design/nssf/contract/**/*.md` 의 AUTO/USER 본문 작성은 별도 nf-build 사이클).

## Phases

| Phase | 목표 | 핵심 모듈 | 핵심 task prefix |
|---|---|---|---|
| P1 | SBI inbound transport (HTTP/2 + TLS + mTLS + inbound OAuth2 production-capable) + 공통 utility 골격 | (공통) | `nssf-transport-*`, `nssf-request-validator`, `nssf-problem-details-mapper`, `nssf-observability` |
| P2 | SelectionEngine — NSSelectionGet end-to-end | `SelectionEngine` | `nssf-selection-*` |
| P3 | AvailabilityEngine — NSSAIAvailability 동기 CRUD + 변경 이벤트 발행 | `AvailabilityEngine` | `nssf-availability-*` |
| P4 | SubscriptionStore — subscription lifecycle persistence | `SubscriptionStore` | `nssf-subscription-*` |
| P5 | NotificationDispatcher — outbound POST + correlation + retry + outbound TLS/mTLS/OAuth2 token attach | `NotificationDispatcher` | `nssf-notify-*` |
| P6 | repository (subscription · availability) — PostgreSQL backend | (cross-module) | `nssf-repo-*` |
| P7 | end-to-end scenario + load + soak test | (전체) | `nssf-scenario-*` |

Phase 간 의존성.

- P1 → 모든 phase 의 전제.
- P2 ↑ P1.
- P3 ↑ P1 + P6 (repository).
- P4 ↑ P1 + P6.
- P5 ↑ P3 (이벤트 source) + P4 (subscription lookup) + P1.
- P6 ↑ P1.
- P7 ↑ P2 ~ P5.

병렬 가능 — P2 와 (P3 + P6) 는 별도 모듈이라 동시 진행 가능. P5 는 P3·P4 commit 후 시작.

## Test Plan

본 plan 단계는 *test seam* 만 정의. 구체 test 케이스는 `test-matrix.md` 가 진실 출처.

| 종류 | 범위 | seam |
|---|---|---|
| unit | 각 모듈의 순수 함수 (예 SelectionEngine 의 `allowed = requested ∩ subscribed ∩ policy ∩ availability`) | mock 의존, in-memory mock 한정 |
| module-integration | 모듈 + 그 모듈의 collaborator (repository, event bus) | in-memory mock backend (production PostgreSQL 은 e2e 단계) |
| contract | 8 operation 의 inbound 응답 schema · status / error 매트릭스 | spec-driven schema validator |
| outbound contract | NotificationDispatcher 의 outbound POST | mock AMF callback receiver |
| security | TLS handshake / mTLS peer verify / OAuth2 inbound bearer / OAuth2 outbound token attach | mock cert + mock JWKS + mock token endpoint |
| end-to-end scenario | `test-strategy.md` 의 시나리오 카탈로그 + security 시나리오 | 외부 NF mock + PostgreSQL backend |

본 phase 시퀀스 와 매핑.

- P1 — observability + ProblemDetails 의 unit test.
- P2 — SelectionEngine unit + NSSelectionGet contract test.
- P3 — AvailabilityEngine unit + Put/Patch/Delete contract test + 변경 이벤트 발행 단위 test.
- P4 — SubscriptionStore unit + Subscribe/SubModifyPatch/Unsubscribe contract test.
- P5 — NotificationDispatcher unit + outbound contract test + correlation 전파 e2e test.
- P1 — security 시나리오 (TLS handshake, mTLS peer verify, OAuth2 inbound bearer validation) contract test.
- P5 — security 시나리오 (outbound TLS/mTLS, OAuth2 outbound token acquire/attach) contract test.
- P6 — repository interface contract test (PostgreSQL backend 단일). test seam 의 in-memory mock 은 abstraction 검증, production 동작은 PostgreSQL 기준.
- P7 — `subscription-create-and-notify`, `notification-retry-on-5xx`, `notification-dead-letter`, `correlation-end-to-end`, `graceful-shutdown-drain`, security 종합 (`tls-handshake`, `mtls-peer-reject`, `oauth2-inbound-*`, `oauth2-outbound-*`) 등 시나리오.

## Open Risks

- **correlation-id 부재 시 정책** — observability.md open question. 무조건 생성으로 default 가정. 운영팀이 거부할 가능성 있음.
- **callback 4xx 재발 시 자동 deactivate** — error-propagation.md open question. P5 task 가 *log 만, 자동 비활성화 안 함* 으로 default 가정.
- **AMF reallocation via RAN 미구현** — 38.413 운영 결정 보류. 후속 사이클에서 hooking 자리 마련 필요 시 architecture 갱신부터.
- **outbound OAuth2 token cache miss + endpoint 실패의 분류** — request-flow.md / NotificationDispatcher.md open question 과 연동. retry queue 진입 vs 즉시 dead-letter. tasks.yaml `nssf-notify-dispatcher` acceptance 가 default 분류 (token endpoint 5xx → retry, client_secret invalid → dead-letter).
- ~~outbound HTTP/2 라이브러리 호환성~~ — closed. engineering-design 결정 = nghttp2 server+client.
- ~~subscription persistence backend default~~ — closed. engineering-design 결정 = PostgreSQL/libpq 단일.
- ~~schema 생성 전략~~ — closed. engineering-design 결정 = openapi-generator(C) + cJSON pinned/vendored.
- ~~SBI TLS / OAuth2 외부화 vs 내부 결정~~ — closed. ADR-0004 baseline = 내부 production-capable code path 의무.

## References

- `docs/adr/ADR-0004-project-security-baseline.md` — TLS / mTLS / OAuth2 production-capable code path 의무 source.
- `engineering/nssf/engineering-design.md` — eng_frozen 결정 (language=C, http=nghttp2, persistence=PostgreSQL/libpq, codegen=openapi-generator+cJSON, TLS/OAuth2 lib).
- `design/nssf/architecture/` — overview, module-boundaries, request-flow, runtime-model, state-persistence, configuration-strategy, error-propagation, observability, test-strategy.
- `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md`.
- `design/nssf/module-decomposition/` — SelectionEngine, AvailabilityEngine, SubscriptionStore, NotificationDispatcher.
- `handoff/nssf/contract.yaml` — 8 API topic + 핵심 data-model.
- `dev/nssf/tasks.yaml`, `dev/nssf/test-matrix.md`, `dev/nssf/traceability.md`.
- `docs/plans/2026-05-14-nssf-arch-dev-full-scope-rework-plan.md`.
