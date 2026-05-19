# NSSF architecture 재생성 — 사이클 회고 (2026-05-19)

plan: `docs/plans/2026-05-18-nssf-architecture-regen-plan.md` (closed)
PR: #25 (arch caveat + plan/follow-up, merge `b1694ab`) · #26 (CLAUDE.md 의사결정 제시 정책, merge `04d3ea9`)
직전 사이클: `docs/retros/2026-05-18-nf-build-fresh-full-materialization-summary.md` (fresh-full materialization — 본 사이클의 의도적 후속)

## 한 일

fresh-full materialization (PR #24) 으로 재생성한 NSSF contract 를 입력으로 (E) `/nf-arch-design nssf` → (F) `/nf-arch-status nssf` 를 수행. canonical architecture baseline (PR #16/#20) 과의 정합성을 *의도적으로* 검토.

- (P3) contract 재생성 — manifest manual_overrides (33.501·38.413 운영 보류) *기존 ratify 결정 복원* 후 `ready_for_build=true`, materialize 25/25 handoff_ready, validate basic 13/0, `/nf-contract-check` handoff_ready PASS.
- (E) architecture 재생성 — canonical 의 contract-derived 구조 사실 (8 API op·25 topic·data-model chain·4 module 분해) 이 재생성 contract 와 *일치*. architecture 10파일 본문·`generated_date` **무변경**. blind 템플릿 재생성으로 canonical 사람 산문을 파괴하지 않음.
- 직접 내용 검증으로 fresh-full 추출 두 곳이 얕음을 적발 (지표·gate 통과 안주 금지) — `error-handling` topic 은 status code 나열만 (operation×cause 매핑 부재), `interface` topic 은 materializer `auto_interface` first-YAML-only 구현이라 첫 primary YAML 기반 partial.
- (F) `arch_consistent` gate PASS.

## 결정·근거

- **재생성은 caveat-only, 내용 diff 0 이 정직한 결과.** 구조 사실 일치 + 상위 추상(module/flow/runtime) 보존. cause matrix 등은 spec(29.531)-valid manual augmentation 으로 유지하되 *source label* 을 contract-derived 가 아니라 spec-valid/manual + current-generated non-traceable 로 분리.
- **tooling fix 는 scope 외 별도 사이클.** `auto_interface` all-primary-YAML service-summary merge / `auto_error` deep extraction → plan `## Follow-up` 기록. 명칭 two-service 금지, NF 별 yaml 개수 유동.
- **관심사 분리 2 PR.** arch caveat (#25) 와 전역 CLAUDE.md 의사결정 정책 (#26) 은 리뷰 축·머지 리스크가 달라 별 PR (Pane 2 PR 구조 검토 권고 옵션2).
- **신규 정책 — 의사결정 제시 시 Pane 2 리뷰 옵션 의무화** (CLAUDE.md, memory `feedback-decision-pane2-review`).

## Pane 2 second-opinion 기여 (다회 라운드)

- (E) 결론 caveat-only 타당성 + interface gap 을 *필수 caveat 로 격상* + root cause 를 `auto_interface` first-YAML-only 로 특정 (코드 검증됨).
- "two YAML/service" 과적합 차단 → `primary_files.yamls` 전체 일반화.
- PR #25 review Medium 1건 — caveat under-apply 로 arch set 내 자기모순·stale 잔존 적발. 6+3 곳 일관화로 해소.
- "interface topic 자체가 first-YAML-only" 표현 정정 → "현 generated interface output 이 materializer 구현 한계로 partial" 로 source 좁힘. interface topic 의도(전체 service-level 표면 대표)와 구현 한계 분리.

## 함정·교훈

- **validate 통과 ≠ 산출 내용 정상.** validate basic 13/0·gate PASS 여도 fresh-full 추출이 얕을 수 있음 — 산출 *내용* 직접 검증이 caveat 의 근거였다.
- **caveat 는 arch set 전체 일관해야.** 2곳만 적용 시 같은 set 내 반대 claim·자기모순 발생 (Pane 2 Medium). source-label 정정은 *영향 받는 모든 ## Inputs* 에.
- **표현 정밀도.** "topic 자체가 X" vs "현 generated output 이 구현 한계로 X" 는 의미가 다르다 — 한계를 정책/의도로 오인시키지 않게 source 를 좁힐 것.
- **handoff 가정 검증.** handoff 는 local main 을 "aec753c clean ahead 1" 로 기술했으나 실제는 Pane2 *미커밋 작업* 동반 → 자동 rebase-sync 가 타 agent 작업 교란 위험. handoff 기술과 실제 상태 불일치 시 진행 전 surface.

## 미결·후속

- **local main sync 미완.** 메인 repo 에 Pane2 미커밋 작업 (`.codex/skills/git-pr-review/SKILL.md`, `ONBOARDING.md`) 존재로 자동 rebase 제외. Pane2/사용자가 자기 작업 정리 후 `git pull --rebase` (aec753c + 미커밋 보존).
- **tooling follow-up** (별도 사이클) — plan `## Follow-up` 의 `auto_interface` all-primary-YAML service-summary merge / `auto_error` deep extraction.
- **재생성 검증 패턴 영구화 후보** — "구조 일치 시 caveat-only, 내용 직접 검증, source-label 분리" 가 향후 NF arch 재생성에 일반 적용 가능한지 차기 검토.
