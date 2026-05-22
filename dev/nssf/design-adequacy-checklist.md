---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-22
---

# NSSF Design Adequacy Checklist

사람이 audit 가능한 *질문 목록*. 자동 승인이 아니라 audit surface — `/nf-impl-status` 는 본 문서의 *체크 상태 품질* 을 자동 평가하지 않는다.

## Checklist

각 항목 status — `ok` (검토 후 문제 없음) · `concern` (재검토 필요) · `pending` (아직 검토 안 됨).

| id | question | status | note |
|---|---|---|---|
| C-01 | contract 의 모든 API operation 이 `api-implementation-matrix.md` 에 매핑됐는가. | ok | handoff/nssf/contract.yaml 의 api/* 8 topic 모두 matrix row 와 1:1 일치 — NSSelectionGet + NSSAIAvailability 7. |
| C-02 | 모든 data-model schema 가 `data-model-implementation-map.md` 에 분류됐는가. | ok | contract data-model topic 33 = map row 33. generated 14 + wrapper 19 (NFType 1 + 17 status codes + default 1) + handwritten 0 + deferred 0. |
| C-03 | ADR-0004 security baseline 7 항목이 architecture·engineering-design 에 모두 반영됐는가. | ok | engineering-design `tls_security` / `oauth2_token_validation` / `sbi_client_stack` slot freeze 가 7 항목 모두 cover. `implementation-readiness-review.md` ## Decision Audit 의 7 항목 점검 표 참조. |
| C-04 | `codegen-work-items.yaml` 의 각 item 이 expected_files·tests·verification_commands 를 가지는가. | ok | 17 work item 모두 비어있지 않은 3 키 보유. `impl_ready_for_codegen` 의 all_work_items_* check 가 PASS 가능. |
| C-05 | `team-execution-plan.md` 의 lane 분리가 architecture/engineering-design 와 일치하는가. | ok | 5 lane (orchestrator/code/reviewer/tester/verifier) + Integration Order 표가 codegen-work-items 의 depends_on 그래프와 1:1 일치. |
| C-06 | `verification-plan.md` 의 6 gate 가 모두 ADR-0004 와 architecture observability 를 반영하는가. | ok | Security gate 가 ADR-0004 7 항목 1:1 매핑, Observability gate 가 architecture observability.md 의 metric/log/trace seam 그대로 인용. |
| C-07 | `open-gaps-and-assumptions.md` 의 `blocker` count 가 0 인가. | ok | 14 gap 중 blocker 0. category — deferred 2 / operator-provided 3 / library-assumed 3 / test-gap 1 / assumption 5. |
| C-08 | autonomous codegen agent 가 원본 OpenAPI YAML 을 *다시 읽지 않고도* 구현 판단 가능한가. | ok | `spec-to-design-coverage.md` 의 `spec reread needed?` 컬럼에 yes 행 0. WI-codegen-bootstrap 의 inputs 에 OpenAPI YAML 이 *generator input only* 로 표기 (의미 보강 용도 아님). |
| C-09 | repository abstraction (`AvailabilityRepository` / `SubscriptionRepository`) 가 PostgreSQL 단일 backend + in-memory mock 만 가지는가. | ok | architecture state-persistence.md 의 repository 결정과 codegen-work-items 의 WI-availability-repo / WI-subscription-store 가 1:1 일치. mock 은 unit/module-integration 한정. |
| C-10 | `PatchDocument` 의 content-type / payload semantics 가 29.531 spec 과 일치하는가. | ok | 29.531 spec 검증 — content-type `application/json-patch+json`, schema `type: array items: TS29571 PatchItem` (RFC 6902). 본 readiness pack 의 api matrix / data-model map / codegen-work-items 가 모두 RFC 6902 으로 통일. G-04 (confirmed) assumption 으로 보존, RFC 7396 merge-patch payload 진입 시 415 reject. |
| C-11 | `NotificationDispatcher` 의 retry 정책 (G-08) 이 Phase 1~3 운영 부하 spike 시 충분한가. | concern | Phase 1~3 은 동기 dispatch + 1회 재시도. spike 시 retry_queue 적체 가능성. Phase 4 hardening 으로 backoff/dead-letter 결정. 본 사이클 진입에는 충분, 운영 도입 전 재검토 필요. |
| C-12 | operator-provided 입력 3종 (G-01 TLS cert, G-02 PostgreSQL DSN, G-06 JWKS endpoint) 의 운영팀 확보 일정. | pending | implementation-readiness-review C1 조건. Phase 1 deploy 전 확보 확정 필요 — 운영팀 보고 후 ok 로 갱신. |

## Outstanding

- C-11 (concern) — NotificationDispatcher retry tuning 의 Phase 4 hardening 시점 명시. owner = code + verifier lane. 기한 = Phase 4 진입 전.
- C-12 (pending) — operator-provided 3종 확보 확정. owner = 운영팀. 기한 = Phase 1 deploy 직전. 본 readiness pack 진입 (F2 PR) 에는 영향 없음 — readiness pack 은 운영 입력 *gap* 으로 분류하면 충분.

## References

- `api-implementation-matrix.md`, `data-model-implementation-map.md`, `codegen-work-items.yaml`, `team-execution-plan.md`, `verification-plan.md`, `open-gaps-and-assumptions.md`.
- `docs/adr/ADR-0004-project-security-baseline.md`.
- `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §4.4.
- `implementation-readiness-review.md` — C1/C2 조건의 source.
