# 2026-05-18 — nf-arch-status / nf-impl-status validator skill summary

> **Historical record — 2026-05-21 note.** 본 retro 후속 항목의 "33.501·38.413 운영 결정 외부 입력 대기" 는 2026-05-21 사이클 (PR #33~#36) 에서 33.501 만 `docs/adr/ADR-0004-project-security-baseline.md` 으로 흡수 (38.413 운영 보류 유지). 본 retro 의 validator skill 작업 자체는 변동 없음.

Status: complete
Current objective: architecture·implementation-planning 산출의 자기 일관성을 측정하는 read-only validator skill 2 개를 신설한 사이클을 한 문서로 남긴다.

## 한 줄 요약

`docs/plans/2026-05-18-arch-impl-status-validator-plan.md` 를 단일 PR (#21) 로 실행했다. ADR-0001 L54 가 예고한 "separate future skill" 을 신설 — `/nf-arch-status` (`design/<nf>/_arch_status.yaml`, gate `arch_consistent`) · `/nf-impl-status` (`dev/<nf>/_impl_status.yaml`, gate `impl_consistent`). PR #20 으로 영구화한 canonical 섹션 계약을 독립 실행 가능한 validator 로 승격했다.

## 사이클 진행

### 후보 결정 (Pane 2 비교)

- retro 2026-05-18(templates) "다음 사이클 후보" 7 건을 Pane 1 정리 → `/pane-send 2` → `/pane-compare 2`.
- Pane 2 우선순위 (3)→(7)→(1) 타당 확인, 분기 0. 보강 2 — validator 과적합 완화책 (discover 기반·semantic judge 제외·Phase 1/2 분할), (1) 구현 전 kickoff plan.
- 사용자 결정 — (3) validator skill 진행.

### 구현 (PR #21)

- worktree 격리. `nf-status.py` 의 check/gate/render 형태를 모델로 차용.
- `nf-arch-status.py` — gate `arch_consistent` (Tier1 5 binary AND), `module_boundaries_xref` advisory WARN non-gate.
- `nf-impl-status.py` — gate `impl_consistent` (Tier1 4 AND), `traceability_xref` advisory WARN.
- 2 SKILL.md (measure-only, read-only, `/nf-status` 책임 분리표), ADR-0001 Follow-ups 기록 (L54 resolve), `.gitignore` 재생성물 등록.
- Pane 2 완화책 사전 반영 — read-only, discover glob (NSSF 하드코딩 0), semantic judge 제외, Phase 1 한정.

### 리뷰 사이클 (2 라운드 second-opinion)

- 1 차 `$git-pr-review 21` (Pane 2) — Verdict **request changes**, Finding [Medium]: `manual_overrides.pass_anyway` 가 문서/출력상 "진짜 우회" 였으나 scripts 가 읽지/적용 안 함 → 사용자가 생성 YAML 수정해도 gate 계속 FAIL (doc-code 괴리). + question: `traceability_xref` 가 plan 의 test-matrix.md 도 봐야.
- 반영 (commit `6b60a74`) — `load_prev_pass_anyway` + `apply_overrides` ({check,reason,confirmed_by} 필수) + `compute_gates` overridden 제외 + render 의 `overridden: true` 표기. `traceability_xref` test-matrix.md 도 스캔.
- **반영 중 근본 버그 발견** — render 의 수동 f-string 따옴표 (`current: "{val}"`) 가 FAIL `current` 의 list repr (내부 `"`·`[` 포함) 에서 *invalid YAML* 을 생성. 그러면 `load_prev_pass_anyway` 의 `safe_load` 가 실패해 우회가 *영영* 무효였다. Medium 의 진짜 근본 원인. 전체 document 를 `yaml.safe_dump` 단일 직렬화로 교체 — round-trip 안전.
- 2 차 follow-up — Verdict **approve / no blocking findings** (이전 사이클 패턴). 머지 `7974e5e` → 로컬 main FF → 원격·로컬 브랜치 삭제 → worktree 제거.

## 잘된 점

- **Pane 2 완화책 사전 반영이 리뷰 범위를 축소했다.** discover 기반·semantic judge 제외·Phase 1/2 분할을 plan 단계에서 박아, 1 차 리뷰가 그 4 축을 "타당" 으로 빠르게 통과하고 *진짜 결함 1 건* 에 집중했다.
- **테스트 하네스가 근본 버그를 노출했다.** Medium (pass_anyway inert) 을 고치려 override 주입 smoke 를 짜다가, 그 smoke 의 `safe_load` 가 *내 생성 산출* 에서 깨졌다. 표면 결함 수정이 더 깊은 직렬화 버그를 드러낸 사례 — 수정 검증을 *실제 재파싱* 으로 했기에 잡혔다.
- **doc-code 괴리 패턴 2 회 연속 적발.** PR #20 = "exactly … set" 선언 vs presence-only 검사. PR #21 = "진짜 우회" 문서 vs inert pass_anyway. 둘 다 *문서가 약속한 동작을 코드가 안 함*. second-opinion 리뷰가 이 류를 일관되게 잡는다 — Pane 2 검토가 구조·문서 일치를 본다는 신호.

## 영구화 효과

- 4 단계 lifecycle status 가 완성 — contract (`/nf-status`) · architecture (`/nf-arch-status`) · implementation-planning (`/nf-impl-status`). 각 단계가 독립 read-only gate 를 가져, 다음 NF 사이클이 단계별 자기 일관성을 기계 검증한다.
- `manual_overrides.pass_anyway` 가 *실제로 동작* — exact-match 엄격성 (PR #20 영구화 목적) 의 escape hatch 가 문서대로 작동, 사용자 명시 결정 ({reason, confirmed_by}) 으로만.

## 다음 사이클 후보

- **Phase 2 — deep traceability id-relation gate 化.** 본 사이클이 advisory WARN 으로 남긴 `traceability_xref`·`module_boundaries_xref` 를 contract topic ↔ module ↔ test id 무결성 gate 로 강화. 별도 사이클.
- **(7) `/nf-init --force` 운영 가이드 SKILL.md tiny PR** — Pane 2 1.5 순위, 미실행.
- **(1) NSSF 구현 vs 다른 NF** — 목표 의존 분기, 사용자 결정 미정. 구현 착수 시 Pane 2 권고대로 짧은 implementation kickoff plan 선행.
- **canonical 상수 ↔ SKILL.md mandate drift 방지** — 현재 주석/문서 약속뿐. 단일 source 추출 또는 drift 검사 자동화 후보 (Pane 2 1 차 리뷰 (2) 지적).
- **(5) 33.501·38.413 운영 결정** 외부 입력 대기. **(6) worktree-aware hook** 재발 후.

## 도구·운영 관찰

- `nf-status.py` 도 `manual_overrides.pass_anyway` 를 출력만 하고 적용 안 한다 (본 신규 script 가 그 형태를 mirror 했다가 같은 결함 상속). 본 사이클은 신규 script 만 고쳤다 (plan §제외 — nf-status 변경 별도 책임). contract 단계도 같은 escape hatch 가 inert 라는 점은 별도 사이클 후보.
- 2 라운드 second-opinion 패턴 (request changes → 반영 → approve) 이 PR #20·#21 연속 적용. 단일 PR 안에서 결함 적발·수정·재검증이 완결되는 사이클이 안정화됐다.

## 최종 상태

- PR #21 머지 (`7974e5e`). origin/main 과 로컬 main in-sync.
- 4 단계 lifecycle status 완성. `pass_anyway` 실동작 + YAML round-trip 안전.
- `docs/plans/2026-05-18-arch-impl-status-validator-plan.md` Status closed 마킹 (본 retro 와 함께 commit).
- 다음 사이클 trigger 대기.
