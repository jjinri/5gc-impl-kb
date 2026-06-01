---
id: ADR-0005
title: Autonomous Implementation Policy — orchestrator self-merge + main-thread orchestrator loop + 4 lane subagent
status: proposed
date: 2026-05-29
supersedes: []
related:
  - docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md
  - docs/adr/ADR-0002-engineering-design-freeze.md
  - docs/adr/ADR-0004-project-security-baseline.md
  - docs/plans/2026-05-29-nf-implement-autonomous-runner-grill.md
---

# ADR-0005 — Autonomous Implementation Policy

## Context

`/nf-implement` SKILL (E3, 2026-05-22) 가 contract/skeleton 만 정의. NSSF Phase
1 wave 1 (#87/#92/#98/#99/#101) 까지 사람이 매 PR 사이클을 매뉴얼 orchestration.
CONTEXT.md L59 정의 — "/nf-implement 는 실제 source/test/CI 를 *만든다*" — 와
실제 동작 (사람이 매 단계 지시) 사이에 gap.

User 의 요구 — *"Phase 5 까지 사람이 다음 schema-bootstrap 해 라고 지시하지
않아도, agent 가 plan 읽고 다음 일을 선택해야 /nf-implement 의 의미가 산다"*.

본 ADR 은 자율 runner 의 권한·boundary·실패 처리·관찰 표면을 freeze 한다.

## Decision

다음 8 항목을 본 repo 의 autonomous implementation policy 로 ratify.

### D1. orchestrator self-merge 권한 부여

`/nf-implement <nf>` 호출 시 launcher 가 기동한 main-thread orchestrator agent 가
`gh pr merge --squash --delete-branch` 권한을 가진다. 사람의 review/merge 가 매 PR
사이클의 gating 단계가 아니다.

본 결정은 기존 memory `feedback_merge_pr` 의 *"원격 main 직접 push 금지, PR
머지 사이클 의무"* 와 호환 — orchestrator 의 `gh pr merge` 도 PR 사이클 일부.
사람이 직접 main 에 push 하는 금지는 유지.

### D2. self-merge 4-condition gate

orchestrator 의 self-merge 직전 검증:
1. PR 의 `required_checks` 전부 PASS.
2. reviewer subagent 가 ADR-0004 security baseline + contract drift + scope
   (changed_paths ⊆ slice scope_files) 검사 PASS.
3. `mergeable: MERGEABLE`.
4. branch up-to-date with main.

### D3. main-thread orchestrator loop + 4 NF-agnostic lane subagent

orchestrator 는 `claude --agent nf-orchestrator` 로 기동한 *main-thread* agent 며
loop 를 돈다 — subagent 가 아니다. harness 제약상 subagent 는 subagent 를 spawn
하지 못하므로 (Agent tool 이 subagent unavailable tools), orchestrator 가
main-thread 여야 4 lane 을 dispatch 할 수 있다.

4 lane (`nf-{code,reviewer,tester,verifier}`) 은 orchestrator 가 `Agent` tool 로
dispatch 하는 *1-level* subagent. `.claude/agents/nf-{orchestrator,code,reviewer,
tester,verifier}.md` 정의 — orchestrator agent 정의는 유지하되 launcher 가
main-thread 로 기동한다. NF 별 override 는 `dev/<nf>/team-execution-plan.md` 의
user_sections 본문에서 prompt context 로 inject. reviewer subagent 는 Edit/Write
권한 0.

lane 실행은 기본 직렬 (code → tester → reviewer/verifier). 병렬 dispatch 는
read-only lane (reviewer/verifier) 또는 worktree 로 격리된 lane 에 한정 — 공유
write scope 를 가진 lane 을 병렬화하지 않는다.

### D4. Plan-driven next-slice picker

next-slice = `pr-slicing-plan.yaml` 의 topo sort + `gh pr view` 동적 상태 조회로
deterministic 산출. orchestrator 의 LLM judgment 으로 선택 금지. 신규 script
`design/scripts/nf-implement-slice-status.py` 가 picker 의 단일 진실 출처.

### D5. Plan amendment self-merge + 3-trigger escape

Plan out-of-scope 발견 시 orchestrator 가 `pr-slicing-plan.yaml` 갱신 PR (plan
amendment) 도 self-merge 가능. 단 다음 중 1개 이상이면 즉시 stop + 사람 보고:
1. ADR 신설 동반 변경.
2. `eng_frozen` decision 변경.
3. ADR-0004 security baseline 영향.

### D6. Checkpoint/resume + cost cap

Context window 한계 처리 = harness compaction + state checkpoint + `--resume`.
chain self-respawn (동일 type subagent 재spawn) 은 *폐기* — main-thread
orchestrator 는 harness 가 context 를 compaction 하며, 한계 도달 시 사람이
`/nf-implement <nf> --resume` 으로 재기동하면 state checkpoint 에서 이어간다.
hand-off 매개는 `dev/<nf>/_implementation_run_state.yaml` 의 checkpoint
(last_checkpoint_at + completed_phases + current_slice) 단독.

Cost cap (셋 중 1개 도달 시 stop):
- resume_count 20 (이전 chain_depth — resume 누적 횟수 cap).
- total_slices_completed 50.
- 동일 slice tier2 attempts ≥ 2.

run_epoch = resume 시퀀스 번호 (resume 마다 +1), state 의 audit field.

### D7. 2-layer security baseline drift 검출

- Layer A = `infra/security-baseline-patterns.yaml` 의 deny/allow regex grep
  (primary block source).
- Layer B = TLS/OAuth2 init call 의 config-source semantic check (AST 한계로
  false negative 허용).

pattern yaml 은 reviewed lifecycle artifact (사람 ratify 후 git tracked). ADR-0004
변경 시 동기 갱신 의무.

### D8. 3-tier retry policy

self-merge 4-condition (D2) 중 ANY false, 또는 CI red / lane 실패 시 retry tier
진입. 본 정책이 retry 의 active behavior source 다 (orchestrator 가 직접 참조).

- tier 0 (flake): CI red 1회 = re-run 1회. 동일 step + 동일 메시지 2회 연속 시
  tier 1.
- tier 1 (auto-fix): nf-code / nf-tester subagent 재호출 + failure log prompt.
  max 2 attempt.
- tier 2 (diagnose): `diagnose` skill 호출 → root cause → (a) 수정 1 attempt
  또는 (b) plan amendment PR (D5) 또는 (c) tier 3.
- tier 3 (stop): blocker 기록 + 사람 보고. 동일 slice tier 2 ≥ 2 시 자동 tier 3.

counter = `state.current_slice_attempts: {tier0, tier1, tier2}`, slice merged 시
reset.

## Consequences

### 긍정

- `/nf-implement <nf>` 1회 호출로 phase 5 종료 또는 본질 blocker 까지 자율 진행.
- 사람 일감 = `--status` 관찰 + `--stop` graceful 중단 + plan/ADR/engineering
  decision 변경 검토 (3-trigger).
- 사람 매 단계 지시 패턴 제거 — user 요구 충족.

### 위험

- self-merge 가 wrong code 머지 가능성 — 4-condition gate (D2) + auto-revert
  (plan §1 Q4) + retro-test CI (D7) 이 완화책. 그러나 ADR-0004 가 아직 다루지
  않는 신종 vulnerability 는 검출 불가.
- resume hand-off (state checkpoint) 가 불완전하면 재기동이 잘못된 slice 에서
  이어질 수 있음 — `--status` 로 사람이 checkpoint 확인 가능. resume_count cap 20
  이 무한 resume 방지.
- plan amendment self-merge 가 plan 의 사람 의도와 drift 가능 — git log 가 모든
  amendment PR 의 history 유지, reviewer subagent 가 ADR-0001 vocab 검사.

### Trade-offs

- 자율성 ↑ ↔ 즉시 audit 표면 ↓. 사람이 매 PR review 하지 않으므로 사후 audit
  표면 (git log + `--status` + 라벨) 에 의존.
- deterministic picker (D4) ↔ runtime 유연성. plan amendment PR 우회로가 유연성
  복구하지만 plan 의 사람 의도 가시화 비용 (amendment template) 발생.

## Alternatives considered

### Alt-1. PR open 까지 자율, 사람 review/merge

memory `feedback_merge_pr` 와 직접 호환. 사람 일감 = 매 PR review/merge button.
거부 이유 — user 가 self-merge 허용 명시 + Phase 5 까지의 PR 수 (예상 ≥ 20) 가
사람 review 부담을 자율 의미 무효화 수준으로 키움.

### Alt-2. wave 단위 사람 confirm

각 wave 시작 전 사람 confirm. 안전하지만 user 가 지적한 "phase 5 까지 사람 매
단계 지시" 패턴과 동치. 거부.

### Alt-3. Ouroboros / GitHub Actions cron headless

진짜 unattended. 본 repo skill 시스템과 격리 + API key/billing 별도 + 디버깅
표면 멀어짐. 본 단계 over-engineering. 거부.

### Alt-4. NF-specific subagent (`.claude/agents/<nf>-{lane}.md` × NF)

NF 마다 5 종 ×N 파일 폭발 + lane 책임이 NF-공통이므로 중복. 거부 — NF-agnostic
+ team-execution-plan.md user_sections 로 override.

### Alt-5. plan 무시 + LLM judgment 으로 slice 선택

비-deterministic. resume/replay/audit 불가. 거부.

## References

- 본 plan = `docs/plans/2026-05-29-nf-implement-autonomous-runner-grill.md` (Q1~Q11
  분기 + rollout spec).
- 기존 memory `project_workflow_upgrade_status.md` — F1/F2/G plan 의 G 책임 영역
  과 본 ADR 의 D 항목 매핑.
- ADR-0001 (lifecycle vocab) — D5 의 plan amendment reviewer 검사 source.
- ADR-0004 (security baseline) — D7 의 drift pattern source.
