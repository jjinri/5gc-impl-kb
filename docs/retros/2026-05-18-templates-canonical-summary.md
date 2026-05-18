# 2026-05-18 — 표준 섹션 templates 영구화 summary

Status: complete
Current objective: NSSF arch/dev full scope 사이클 (PR #16/#17, retro `docs/retros/2026-05-14-nssf-arch-dev-full-scope-summary.md`) 에서 검증된 표준 섹션 구조를 skill template + SKILL.md 에 영구화한 사이클을 한 문서로 남긴다.

## 한 줄 요약

`docs/plans/2026-05-18-templates-canonical-permanence-plan.md` 를 단일 PR (#20) 로 실행했다. retro 2026-05-14 "영구화 후보" 표 첫 3 항목 (architecture 6 종 · module-decomposition 7 종 · dev 표준 섹션) 을 skill 계약으로 박았고, 작업 중 발견한 `nf-arch-design/SKILL.md` 의 stale model (module-decomposition 을 legacy 로 취급) 도 같은 PR 에서 정합화했다.

## 사이클 진행

### 다음 사이클 후보 결정 (Pane 2 비교)

- Pane 1 이 retro 2026-05-14 "다음 사이클 후보" 7 건을 우선순위와 함께 정리, `/pane-send 2` → `/pane-compare 2` 로 codex (gpt-5.5) 와 비교.
- 합의 5/7, 분기 2/7 — (1) NSSF 구현 vs (2) 다른 NF 는 *목표 의존* 분기 (사용자 결정 필요), (7) `/nf-init --force` 가이드는 Pane 2 가 우선순위 상향 권고.
- 사용자 결정 — (4) templates 영구화 진행, (2) 다른 NF 사이클 미진행.

### 구현 (PR #20)

- worktree `templates-canonical-20260518` 격리.
- architecture template 10 + ADR → canonical 6 종 재작성, frontmatter 정합 (`contract_status`/`{{status_path}}` 제거).
- module-decomposition template 신설 `templates/module-decomposition/_MODULE.md` (7 종).
- dev template 3 재작성 (implementation-plan / test-matrix / traceability), tasks.yaml schema 불변.
- `nf-arch-design/SKILL.md` — module-decomposition 을 first-class 산출로 승격 (legacy 서술 제거), output map 추가, canonical 섹션 강제 명시, validation snippet 추가.
- `nf-impl-plan/SKILL.md` — dev 파일별 canonical 섹션 강제 + validation snippet.
- smoke — 전 재작성 template `## ` 집합 == canonical, 양 SKILL.md snippet 을 canonical NSSF 산출에 실행 통과.

### 리뷰 사이클 (2 라운드 second-opinion)

- 1 차 `$git-pr-review 20` (Pane 2) — Verdict **request changes**, Finding [Medium]: validation snippet 이 presence-only (`h not in body`) 인데 SKILL.md 는 "exactly canonical section set" 선언 → extra heading 통과. + non-blocking: `_MODULE.md` 예시 `SelectionEngine.md` NSSF 표본 흔적.
- 반영 (commit `4780600`) — 양 snippet 을 `got != want` 순서까지 exact-match 로, module-decomposition 7-section exact 추가, ADR 별도 set, `_MODULE.md` 예시 → `ExampleModule.md`. canonical NSSF exact-match 통과 + synthetic extra heading reject 재현.
- 2 차 `$git-pr-review 20` follow-up (Pane 2) — Verdict **approve / no blocking findings**. Medium 해소 독립 확인 (자체 `nf=nssf` 환경에서 exact-match PASS, synthetic reject).
- 머지 `f3dbdda` → 로컬 main FF 동기화 → 원격·로컬 push 브랜치 삭제 → worktree 제거.

## 잘된 점

- **canonical = "실제 머지 산출"** 원칙이 작동했다. 2026-05-14 plan 표 (baseline 이전 사양, module-decomposition 6 종) 가 아니라 실제 PR #16/#17 머지 산출 (7 종, dev 도 Purpose/References 추가) 을 진실 출처로 삼아 grep 으로 추출 → template 에 박았다. retro 가 ratify 한 형태가 그대로 계약이 됐다.
- **리뷰 사이클이 실질 결함을 잡았다.** "exactly … set" 선언 vs presence-only 검사 불일치는 사람 눈에 안 띄기 쉬운 계약-구현 괴리. Pane 2 가 synthetic extra heading 재현으로 입증, exact-match 로 닫음. 2 라운드 second-opinion 패턴 (1 차 request changes → 반영 → 2 차 approve) 이 단일 PR 안에서 완결됐다.
- **스코프 발견을 plan 에 명시 흡수.** 작업 중 SKILL.md stale model 발견 시 별도 사이클로 미루지 않고 plan §"스코프 추가" 에 기록 후 같은 PR 에서 처리 — template 만 고치고 SKILL.md 가 legacy 라 부르면 모순이 남았을 것.

## 영구화 효과

- 다음 NF 사이클 (`/nf-arch-design`, `/nf-impl-plan`) 이 canonical 섹션을 *자동 상속*. reviewer 가 섹션 구조 노이즈 대신 내용만 본다 (2026-05-14 retro 가 관찰한 효과를 도구화).
- 양 SKILL.md 의 embedded validation snippet 이 *exact-match* 라, 향후 NF 산출이 canonical 외 섹션을 추가하면 즉시 fail — templates 영구화 목적이 검사로 보장됨.

## 다음 사이클 후보

- **(3) `/nf-arch-status`·`/nf-impl-status` validator skill 신설** — retro 2026-05-14 후보 (3). 본 사이클의 exact-match snippet 이 그 skill 의 검사 핵심 후보. Pane 2 도 templates 영구화 후 2 순위로 합의.
- **(7) `/nf-init --force` 운영 가이드 SKILL.md 명시** — Pane 2 1.5 순위 권고, 별도 tiny PR.
- **(1) NSSF 실제 구현 vs 추가 NF 사이클** — 목표 의존 분기, 사용자 결정 미정 (방법론 KB 품질 → 다른 NF, 제품 runtime → NSSF 구현).
- **(5) 33.501·38.413 운영 결정** — 외부 입력 대기. **(6) worktree-aware manifest/seed hook** — 1 회 더 pain 재발 후.

## 최종 상태

- PR #20 머지 (`f3dbdda`). origin/main 과 로컬 main in-sync.
- canonical 섹션 구조가 `nf-arch-design`/`nf-impl-plan` skill 계약으로 영구화 — template + SKILL.md mandate + exact-match validation.
- `docs/plans/2026-05-18-templates-canonical-permanence-plan.md` Status closed 마킹 (본 retro 와 함께 commit).
- 다음 사이클 trigger 대기.
