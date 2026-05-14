# NSSF arch/dev full scope 재작업 — 2026-05-14

Status: closed (2026-05-14)
Current objective: (closed) NSSF MVP (NSSelectionGet 단일 op) 가정으로 작성된 architecture / module-decomposition / dev 산출을 full NSSF (Nnssf_NSSelection + Nnssf_NSSAIAvailability 두 service · 8 operation) 기준으로 재작업한다.

Phase 0 (plan, commit `21efd49`) + Phase 1 (PR #16 merge `1ea2239`) + Phase 3 (PR #17 merge `17d3496`) + Phase 4 (본 close 마킹 + retro `docs/retros/2026-05-14-nssf-arch-dev-full-scope-summary.md`) 완료. 영구화 후보 (표준 섹션 templates 등) 는 retro 의 "다음 사이클" 섹션 참고.

## 컨텍스트

직전 사이클 retro `docs/retros/2026-05-14-nssf-full-scope-seed-gen-fix-summary.md` 결정.

- NSSF scope = full (NSSelection 1 op + NSSAIAvailability 7 op = 8 op).
- `manual_overrides.exclude` 의 33.501·38.413 reason 의 *MVP* 문구 정정 완료 (local manifest).
- `nf-seed-gen.py` 의 schema_refs HTTP code 노이즈 fix 완료 (PR #15, merge `d986697`).
- 본 plan 은 *남은 활성 산출* (architecture 9 · module-decomposition · dev/nssf 4) 의 full scope 재작업을 책임진다.

## 명시적 제외

- contract 단계 (`design/nssf/contract/`, `handoff/nssf/contract.yaml`) 자체 재생성 — `/nf-build nssf` 사이클의 책임. 본 plan 은 *그 결과* 를 입력으로 사용한다.
- `/nf-arch-status`·`/nf-impl-status` 같은 arch/dev 단계 validator skill 신설 — 별도 사이클. ADR-0001 follow-up 으로 미룬다.
- frontmatter `budget_lines`·`scope` 도입 — 본 사이클은 *구조 표준화* 만, 수치 metadata 는 별도 ADR 가 정의한 뒤 도입.
- 33.501·38.413 spec 자체 cp — 운영 결정 보류 상태. 본 사이클은 exclude 가 유지된 채로 진행.

## 표준 섹션 — 본 사이클 강제

본 글 (AWS Korea, Context Engineering from Deep Insight) 의 L2 *표준화된 응답 형식* 정신 채택. 단 *수치 budget* 은 보류하고 *섹션 일관성* 만 강제.

### architecture/<topic>.md

| 섹션 | 책임 |
|---|---|
| `## Purpose` | 본 문서가 답하는 질문 1 ~ 3 줄 |
| `## Inputs (contract)` | 의존하는 `design/<nf>/contract/**` · `handoff/<nf>/contract.yaml` 경로 + 의존 topic id 명시 |
| `## Boundaries` | 본 문서가 다루는 / 다루지 않는 책임 경계 |
| `## Decisions` | 본 문서가 확정한 설계 결정 (목록 또는 표) |
| `## Open Questions` | 다음 사이클·dev 단계로 미루는 미해결 항목 |
| `## References` | spec 인용·다른 architecture 문서 wikilink |

### module-decomposition/<module>.md

| 섹션 | 책임 |
|---|---|
| `## Responsibility` | 본 모듈의 단일 책임 |
| `## Inputs` | 본 모듈로 들어오는 데이터·호출 |
| `## Outputs` | 본 모듈이 만드는 데이터·호출 |
| `## State` | 본 모듈이 보유·소비하는 상태 |
| `## Decisions` | 본 모듈의 설계 결정 |
| `## References` | spec / contract topic / 다른 module wikilink |

### dev/nssf/

| 파일 | 표준 섹션 |
|---|---|
| `implementation-plan.md` | `## Scope`, `## Phases`, `## Test Plan`, `## Open Risks` |
| `tasks.yaml` | yaml schema 그대로 (현재 `.claude/skills/nf-impl-plan/templates/dev/tasks.yaml` 와 동일) |
| `traceability.md` | `## Contract → Module`, `## Module → Test`, `## Open Gaps` 의 표 3 개 |
| `test-matrix.md` | `## Test Inventory` (id, kind, scenario, given, when, then, refs) 1 표 |

## Phase 분해

### Phase 0 — 본 plan 작성 + push (현 단계)

- 산출 — 본 문서.
- 커밋 단위 — plan 단독 (prose-only, direct push to origin/main).
- 다음 — Phase 1 architecture 재작업 시작.

### Phase 1 — architecture 9 파일 재작업

대상 — `design/nssf/architecture/{overview, module-boundaries, request-flow, runtime-model, state-persistence, configuration-strategy, error-propagation, observability, test-strategy}.md` + `decisions/ADR-0001-architecture-baseline.md`.

변경 요점.

- 표준 섹션 (위) 으로 본문 재구성.
- *NSSelectionGet 한정* 문구 일괄 제거.
- 8 operation 전체를 다루도록 본문 확장.
  - request-flow.md — NSSelectionGet 시나리오 + NSSAIAvailabilityPut 동기 시나리오 + NSSAIAvailabilityPost 구독 + NSSAIAvailabilityNotify (POST 콜백) 시나리오까지 포함.
  - runtime-model.md — request-response 단순 모델 + notification dispatcher (long-lived subscription state 유지) 추가.
  - state-persistence.md — *MVP 자체 write path 없음* 결론에서 *NSSAIAvailability subscription 은 persistent* 로 갱신. subscription store 책임 정의.
  - module-boundaries.md — SelectionEngine 단독 → SelectionEngine + AvailabilityEngine + NotificationDispatcher + SubscriptionStore.
  - error-propagation.md — 8 operation 전체의 ProblemDetails 매핑.
  - configuration-strategy.md / observability.md / test-strategy.md — 8 operation 으로 metric · log · test 범위 확장.

PR 단위 — Phase 1 단일 PR. branch `push/nssf-arch-full-scope-20260514` (or 사이클 시점 timestamp).

검증 — 본 사이클은 arch/dev validator 부재라 *사람 리뷰* + `/nf-status nssf` 가 contract 단계만 검증 (변경 없음 OK).

### Phase 2 — module-decomposition 재작업

대상 — `design/nssf/module-decomposition/SelectionEngine.md` + 신규 `AvailabilityEngine.md`, `NotificationDispatcher.md`, `SubscriptionStore.md`.

변경 요점.

- SelectionEngine — *NSSelectionGet 1 API 한정* 문구 제거. NSSelectionGet 처리 모듈로 책임 명시 (다른 op 와의 경계).
- AvailabilityEngine — 7 NSSAIAvailability operation 의 핵심 로직. TA 별 S-NSSAI availability 저장·조회.
- NotificationDispatcher — Notify callback 발송, retry, correlation id 추적.
- SubscriptionStore — Subscribe / Unsubscribe / SubModify 의 subscription lifecycle persistence.

PR 단위 — Phase 1 과 같은 PR 또는 분리. 의존성 (Phase 1 의 module-boundaries 가 이 모듈명을 언급) 때문에 *같은 PR* 권장.

### Phase 3 — dev/nssf 재작업

대상 — `dev/nssf/{implementation-plan, traceability, test-matrix}.md` + `tasks.yaml`.

변경 요점.

- implementation-plan.md — full scope 의 phase 분해 (예 P1 NSSelectionGet, P2 NSSAIAvailability sync ops, P3 notification dispatcher, P4 subscription persistence).
- tasks.yaml — 8 operation 각각 1 task + module-decomposition 4 모듈 각각 1 task + 통합 시나리오 1 task. 8 ~ 13 task 예상.
- traceability.md — contract topic ↔ module ↔ test 3 단 표.
- test-matrix.md — 8 operation × scenario × kind (unit / integration) 의 표. notification·subscription scenario 포함.

PR 단위 — Phase 1 (+2) 머지 후 Phase 3 단일 PR. branch `push/nssf-dev-full-scope-20260514`.

### Phase 4 — retro

대상 — `docs/retros/<YYYY-MM-DD>-nssf-arch-dev-full-scope-summary.md`.

- 본 plan 의 Phase 0 ~ 3 진행 요약.
- 본 plan Status: in_progress → closed 마킹.
- direct push main (retro-only).

## 결정 사항

| 항목 | 결정 |
|---|---|
| L1 격리 | 본 repo `/nf-*` skill 단계 분리가 이미 적용. 재확인. |
| L2 표준화 — 수치 budget | **보류**. metadata 추가 안 함. |
| L2 표준화 — 섹션 구조 | **본 사이클에 강제**. 위 표 적용. |
| L3 파일 통신 | 본 repo seed → manifest → contract → handoff yaml 체인 그대로. arch/dev 는 그 *읽기 측*. |
| L4 Validator | **별도 사이클 보류**. ADR-0001 follow-up 으로. |
| PR 분기 | architecture (+ module-decomposition) Phase 1 PR, dev Phase 3 PR, retro direct push. |

## 리스크와 완화

- **리스크.** module-decomposition 4 모듈 정의가 다음 architecture 본문과 cross-ref 가 많아 wikilink 깨질 위험.
  - 완화. Phase 1 에서 architecture 본문 갱신 시 *향후 module 명* 까지 결정해서 본문에 wikilink 박고, Phase 2 가 그 wikilink target 을 채운다. Phase 2 가 늦으면 wikilink resolve 검사 (`nf-status` 의 `wikilinks_resolve` check) 가 contract 단계 한정이라 fail 보고는 안 되지만 사람이 사전 확인.
- **리스크.** 8 operation 의 schema chain 이 contract 단계에서 아직 *부분만* canonical. `/nf-build` 재생성을 plan 진행 중에 또 돌릴지 결정 필요.
  - 완화. 본 plan 은 contract 가 *현재 상태 그대로* 라 가정. contract 가 바뀌면 arch/dev 도 그 변화에 맞춰 별도 사이클.
- **리스크.** 33.501·38.413 운영 결정 보류 — full scope 인데 두 spec 부재. 보안·NGAP 관련 결정이 *Open Questions* 로 남는다.
  - 완화. 각 arch 문서의 `## Open Questions` 섹션에 명시. 결정되면 후속 사이클에 보완.

## 성공 기준

- 13 파일 (arch 10 + module-decomposition 4 + dev 4 = sub-PR 별로 누적) 모두 표준 섹션 적용.
- *MVP* / *NSSelectionGet 한정* 문구 활성 산출에서 0 건 (`grep -rn "MVP\|NSSelectionGet 한정" design/nssf/ dev/nssf/` = 0).
- 두 PR (Phase 1 / Phase 3) 머지 후 retro 작성.

## Context Engineering 매핑 — 본 plan 의 책임

본 글 4 계층 vs 본 plan.

| 계층 | 본 plan 적용 |
|---|---|
| L1 격리 | Phase 1 (arch) / Phase 3 (dev) PR 분리 — 머지 충돌 격리 |
| L2 표준화 | 섹션 구조 강제 (수치 budget 보류) |
| L3 파일 통신 | contract → arch → dev 의 *읽기* 방향만, 본 plan 은 그 체인 *수정 안 함* |
| L4 검증 | 본 사이클 *사람 리뷰* + grep 기반 성공 기준. validator skill 신설은 별도 사이클 |
