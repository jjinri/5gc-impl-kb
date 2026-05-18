# 표준 섹션 templates 영구화 — 2026-05-18

Status: closed (2026-05-18)
Current objective: (closed) NSSF arch/dev full scope 사이클 (PR #16/#17, retro `docs/retros/2026-05-14-nssf-arch-dev-full-scope-summary.md`) 에서 검증된 *표준 섹션 구조* 를 skill template + SKILL.md 에 영구화한다. 다음 NF 사이클이 같은 구조를 자동 상속하게 만든다.

P0~P5 완료 — 단일 PR #20 머지 (`f3dbdda`). 2 라운드 second-opinion 리뷰 (1 차 request changes: presence-only validation → exact-match 반영 commit `4780600`, 2 차 approve). 사이클 요약은 `docs/retros/2026-05-18-templates-canonical-summary.md`. 다음 사이클 후보는 해당 retro "다음 사이클 후보" 참고.

retro "영구화 후보" 표의 첫 3 항목 (architecture 6 종 · module-decomposition 7 종 · dev 표준 섹션) 을 *단일 PR 사이클* 로 처리한다. retro "다음 사이클 후보" 의 사용자 결정 — (4) 진행, (2) 다른 NF 사이클 미진행.

## 컨텍스트

- canonical source of truth = PR #16/#17 로 머지된 NSSF 산출의 실제 섹션 구조 (`design/nssf/architecture/*.md`, `design/nssf/module-decomposition/*.md`, `dev/nssf/*.md`), retro 가 ratify.
- 직전 plan `docs/plans/2026-05-14-nssf-arch-dev-full-scope-rework-plan.md` 의 표준 섹션 표는 *baseline 이전 사양* 이라 일부 섹션이 빠져 있다 (module-decomposition 의 `## Open Questions`, dev test-matrix 의 `## Purpose`/`## References`). 실제 머지 산출이 더 풍부하며 그것이 canonical.
- 작업 중 발견 — `.claude/skills/nf-arch-design/SKILL.md` 의 model 이 canonical baseline 과 *불일치*. 본 plan 의 명시 스코프에 포함 (아래 §스코프 추가).

## Canonical 섹션 (영구화 대상)

머지된 NSSF 산출에서 `grep '^## '` 로 추출한 실제 집합.

### architecture/<topic>.md — 6 종 (10 파일 전부 동일)

`## Purpose` · `## Inputs (contract)` · `## Boundaries` · `## Decisions` · `## Open Questions` · `## References`

### module-decomposition/<Module>.md — 7 종

`## Responsibility` · `## Inputs` · `## Outputs` · `## State` · `## Decisions` · `## Open Questions` · `## References`

(`## Open Questions` 가 baseline 에서 추가됨 — 2026-05-14 plan 표의 6 종이 아니라 7 종이 canonical.)

### dev/<nf>/

| 파일 | canonical 섹션 |
|---|---|
| `implementation-plan.md` | `## Scope` · `## Phases` · `## Test Plan` · `## Open Risks` · `## References` |
| `test-matrix.md` | `## Purpose` · `## Test Inventory` · `## Coverage Rules` · `## Open Questions` · `## References` |
| `traceability.md` | `## Contract → Module` · `## Module → Test` · `## Open Gaps` · `## References` |
| `tasks.yaml` | yaml schema 그대로 (현 template 과 동일, 변경 없음) |

frontmatter — arch/module-decomp = `nf` · `stage: architecture-design` · `status: draft` · `source_contract` · `generated_date`. dev = `nf` · `stage: implementation-planning` · `status: draft` · `source_architecture` · `source_contract` · `generated_date`. baseline 산출에 `contract_status` 없음 — template 의 `{{status_path}}` 제거.

## 스코프 추가 — SKILL.md stale model 정합화

현 `.claude/skills/nf-arch-design/SKILL.md` 가 canonical baseline 과 불일치하는 지점.

- L22~23 — `design/<nf>/module-decomposition/` 를 "legacy ... pending a separate compatibility-aware migration" 으로 취급. 그러나 PR #16 이 이를 *first-class 산출* 로 생성했고 retro 가 표준 섹션 영구화 대상으로 지정. → legacy 서술 제거, 표준 산출로 승격.
- Output files block (L38~51) — `module-decomposition/` 누락. → 추가.
- L60 — "Separate **Spec-derived constraints** from **Implementation choices** in every document". canonical 산출에 그 헤딩 없음 (Boundaries + Decisions 로 대체됨). → canonical 섹션 기준 문구로 교체.
- L54 — `{{status_path}}` placeholder 참조. → 제거.
- validation snippet — module-decomposition 디렉터리 존재만 가볍게 확인 (모듈 수는 NF 별이라 enumerate 안 함).

`nf-impl-plan/SKILL.md` 는 stale model 없음 — L62 의 template 참조에 *필수 섹션 enumerate* 만 추가.

## 명시적 제외

- `/nf-arch-status`·`/nf-impl-status` validator skill 신설 — retro 후보 (3). 별도 사이클. 본 plan 은 template + SKILL.md 텍스트만.
- frontmatter 수치 budget (`budget_lines` 등) — 2026-05-14 결정대로 보류.
- 기존 `design/nssf/**` · `dev/nssf/**` 산출 재생성 — 이미 canonical. 본 plan 은 *역방향* (산출 → template) 만.
- 다른 NF 사이클 (NRF·AMF) — 사용자 결정으로 미진행.

## Phase 분해 (단일 PR)

- P0 — 본 plan 작성 (현 단계).
- P1 — architecture template 10 파일 표준 6 종 재작성 + frontmatter 정합 (task #1).
- P2 — module-decomposition template 신설 7 종 (task #2).
- P3 — dev template 3 파일 canonical 재작성 (task #3).
- P4 — `nf-arch-design/SKILL.md` stale model 정합 + 섹션 강제 (task #4), `nf-impl-plan/SKILL.md` 섹션 강제 (task #5).
- P5 — smoke (각 재작성 template 의 `## ` 집합 == canonical 집합 grep 검증) + 로컬 main 머지 → push 브랜치 + PR (task #6).

PR 단위 — 전체 단일 PR (`push/templates-canonical-20260518`). retro "단일 PR 사이클" 결정 준수.

## 성공 기준

- 10 arch template + 1 module-decomp template + 3 dev template 의 `## ` 헤딩 집합이 canonical NSSF 산출과 정확히 일치.
- 두 SKILL.md 가 canonical 섹션을 *필수* 로 명시, nf-arch-design SKILL.md 의 legacy/stale 서술 0 건 (`grep -n 'legacy.*module-decomposition\|Spec-derived constraints' .claude/skills/nf-arch-design/SKILL.md` = 0).
- PR 머지 후 retro 작성 (별도, 본 plan close 마킹 포함).

## 리스크와 완화

- **리스크.** template 의 per-doc TODO 안내를 표준 섹션으로 옮기며 doc 별 고유 가이드가 소실될 수 있음.
  - 완화. 각 arch template 의 기존 doc-specific 안내 (예 request-flow 의 sequence, observability 의 metric) 를 표준 섹션 *안* 의 TODO bullet 로 보존.
- **리스크.** canonical = "실제 머지 산출" 인데 NSSF 1 NF 표본이라 일반화 과적합 가능.
  - 완화. retro 가 이미 ratify. 6/7/5 섹션은 NF-중립 (Purpose/Decisions/Open Questions/References 등). NF 고유 내용은 TODO 로만 표현, 강제 대상 아님.
