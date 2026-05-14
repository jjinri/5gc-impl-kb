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

포함 범위.

- inbound SBI handler (8 operation), request validation, ProblemDetails 매핑, observability.
- 4 모듈 (`SelectionEngine` · `AvailabilityEngine` · `SubscriptionStore` · `NotificationDispatcher`) 의 핵심 비즈니스 로직.
- outbound HTTP/2 client (NotificationDispatcher) — correlation-id 전파 필수, OAuth2 client credentials config 옵션.
- repository interface (subscription · availability) — 추상화 + 적어도 in-memory 구현, persistent backend 1 종 (file 또는 KV).
- retry queue, dead-letter, metric / trace 통합.
- contract-derived unit / integration / scenario test seam.

제외 범위.

- 구현 언어 · 런타임 · HTTP 라이브러리 · TLS 라이브러리 선택 (각 task 의 `owner: dev` 가 결정).
- 배포 토폴로지 (service mesh / sidecar / NF 내장 TLS).
- 33.501 깊이 — SBA TLS profile / OAuth2 token introspection 의 *내부 구현*. config 외부화로 위임.
- 38.413 — AMF reallocation via RAN (NGAP REROUTE NAS REQUEST) 미구현 결정 유지. 운영 결정 변경 시 별도 사이클.
- contract markdown body (현재 handoff yaml 까지만 — `design/nssf/contract/**/*.md` 의 AUTO/USER 본문 작성은 별도 nf-build 사이클).

## Phases

| Phase | 목표 | 핵심 모듈 | 핵심 task prefix |
|---|---|---|---|
| P1 | SBI inbound transport + 공통 utility 골격 | (공통) | `nssf-transport-*`, `nssf-request-validator`, `nssf-problem-details-mapper`, `nssf-observability` |
| P2 | SelectionEngine — NSSelectionGet end-to-end | `SelectionEngine` | `nssf-selection-*` |
| P3 | AvailabilityEngine — NSSAIAvailability 동기 CRUD + 변경 이벤트 발행 | `AvailabilityEngine` | `nssf-availability-*` |
| P4 | SubscriptionStore — subscription lifecycle persistence | `SubscriptionStore` | `nssf-subscription-*` |
| P5 | NotificationDispatcher — outbound POST + correlation + retry | `NotificationDispatcher` | `nssf-notify-*` |
| P6 | repository (subscription · availability) — in-memory + persistent | (cross-module) | `nssf-repo-*` |
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
| unit | 각 모듈의 순수 함수 (예 SelectionEngine 의 `allowed = requested ∩ subscribed ∩ policy ∩ availability`) | mock 의존, in-memory |
| module-integration | 모듈 + 그 모듈의 collaborator (repository, event bus) | in-memory backend |
| contract | 8 operation 의 inbound 응답 schema · status / error 매트릭스 | spec-driven schema validator |
| outbound contract | NotificationDispatcher 의 outbound POST | mock AMF callback receiver |
| end-to-end scenario | `test-strategy.md` 의 시나리오 카탈로그 (15 개) | 외부 NF mock + persistent backend |

본 phase 시퀀스 와 매핑.

- P1 — observability + ProblemDetails 의 unit test.
- P2 — SelectionEngine unit + NSSelectionGet contract test.
- P3 — AvailabilityEngine unit + Put/Patch/Delete contract test + 변경 이벤트 발행 단위 test.
- P4 — SubscriptionStore unit + Subscribe/SubModifyPatch/Unsubscribe contract test.
- P5 — NotificationDispatcher unit + outbound contract test + correlation 전파 e2e test.
- P6 — repository in-memory ↔ persistent 동등성 (golden interface test).
- P7 — `subscription-create-and-notify`, `notification-retry-on-5xx`, `notification-dead-letter`, `correlation-end-to-end`, `graceful-shutdown-drain` 등 시나리오.

## Open Risks

- **outbound HTTP/2 라이브러리 호환성** — 본 plan 은 라이브러리 강제 안 함. 선택한 라이브러리가 HTTP/2 prior knowledge / ALPN / connection coalescing 모두 지원하는지 P5 task 의 acceptance 에 포함.
- **subscription persistence backend default** — `state-persistence.md` open question. P6 가 in-memory + persistent 1 종 (file 권고) 까지만 구현, production backend 결정은 운영 사이클.
- **correlation-id 부재 시 정책** — observability.md open question. 무조건 생성으로 default 가정. 운영팀이 거부할 가능성 있음.
- **callback 4xx 재발 시 자동 deactivate** — error-propagation.md open question. P5 task 가 *log 만, 자동 비활성화 안 함* 으로 default 가정.
- **AMF reallocation via RAN 미구현** — 38.413 운영 결정 보류. 후속 사이클에서 hooking 자리 마련 필요 시 architecture 갱신부터.
- **schema 생성 전략** — `interface` / data-model schema 를 코드 생성기로 처리할지 hand-written 으로 처리할지 본 plan 미결정 (P1 의 `nssf-contract-types` task 가 결정).

## References

- `design/nssf/architecture/` — overview, module-boundaries, request-flow, runtime-model, state-persistence, configuration-strategy, error-propagation, observability, test-strategy.
- `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md`.
- `design/nssf/module-decomposition/` — SelectionEngine, AvailabilityEngine, SubscriptionStore, NotificationDispatcher.
- `handoff/nssf/contract.yaml` — 8 API topic + 핵심 data-model.
- `dev/nssf/tasks.yaml`, `dev/nssf/test-matrix.md`, `dev/nssf/traceability.md`.
- `docs/plans/2026-05-14-nssf-arch-dev-full-scope-rework-plan.md`.
