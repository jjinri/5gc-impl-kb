# Handoff — ADR-0005 dispatch 토폴로지를 Option A 로 전환

## 결정 상태 — **A 확정** (Pane 1 + Pane 2 합의)

`/nf-implement` 자율 runner 를 **Option A (main-session orchestration)** 로 재작성.
양 pane second-opinion 합의 완료. Pane 2 가 D3/D6 외 7 함정 추가 → 스코프 확장됨.
"D3/D6만 수정"은 **불충분**.

## 근본 원인 (확정 + 공식 docs 근거)

현 ADR-0005 = **2-level subagent nesting** → Claude Code harness 에서 작동 불가:

- `/nf-implement` SKILL 이 `nf-orchestrator` 를 subagent 로 spawn → orchestrator 가
  4 lane 을 다시 subagent 로 dispatch + D6 chain self-respawn (subagent→subagent).
- harness 제약: **subagent 는 subagent spawn 못 함**. `Agent` tool 이 subagent
  unavailable tools 에 포함. main-thread agent 만 `Agent(...)` 로 subagent spawn 가능.
- 근거: Claude Code docs — https://code.claude.com/docs/en/sub-agents ,
  https://code.claude.com/docs/en/agent-sdk/subagents ("Subagents cannot spawn
  their own subagents", "agent runs as main thread … can spawn subagents").

근거 위치: `.claude/agents/nf-orchestrator.md:10,49,117-118`, ADR-0005 D3/D6,
nf-implement `SKILL.md:11-14`.

## Option A topology

main session = orchestrator loop, 4 lane (`nf-code`/`nf-tester`/`nf-reviewer`/
`nf-verifier`) = **1-level subagent**. Anthropic 공식 패턴 (Skills + chain-from-main)
정합. D1/D2/D4/D5/D7 무영향 (self-merge / 4-condition gate / picker / amendment /
drift 전부 main 에서 동작).

## launch 방식 — **A′ 확정** (2026-05-29)

`claude --agent nf-orchestrator` 를 전용 pane/process 로 실행 → main-thread 이므로
4 lane subagent spawn 가능. interactive session 자유.

A′ 가 단순화하는 것 (pure A 대비):
- orchestrator 가 main-thread **agent 로 생존** → `nf-orchestrator.md` 는 agent 정의
  *유지*. `tools: Agent`(L4) 가 이제 **정당** (main-thread agent 는 spawn 가능).
- git/gh/state-write 권한은 **agent tools 에 그대로** (`nf-orchestrator.md:4` 이미
  `Bash, Read, Edit, Write, ...`). → trap #1 의 무거운 권한이 SKILL 이 아니라 agent 에.
- `/nf-implement` SKILL = **launcher 역할** — readiness 검증 후 전용 pane/process 에서
  `claude --agent nf-orchestrator` 기동. SKILL allowlist 는 launch 명령(tmux/bg)만 추가.

남은 운영 디테일 (다음 세션): launch 메커니즘 — tmux new-pane vs background process vs
사용자 수동 실행 안내 중 택. --stop/--status 는 interactive session 의 외부 state-file
mutation script (orchestrator 가 poll, `SKILL.md:28-29` 전제와 일치).

## 실행 스코프 (확장됨 — Pane 2 7 함정 반영)

| # | 대상 | 변경 | 검증 |
|---|---|---|---|
| - | ADR-0005 D3 | "5 lane subagent" → "main orchestrator loop + 4 lane subagent" | |
| - | ADR-0005 D6 | chain self-respawn **폐기**. context = harness compaction + state checkpoint + `--resume`. chain_depth → resume_count/run_epoch 로 의미 축소 | |
| - | ADR-0005 title/D1 | "5 lane / chain self-respawn" 표현 제거 | |
| 1 | `SKILL.md:5` allowed-tools | **A′**: SKILL=launcher → launch 명령(tmux new-pane / bg `claude --agent nf-orchestrator`)만 추가. 무거운 gh/git/state-write 는 agent tools 에 잔류(이미 보유) | ✅ 확인 |
| 2 | 용어 drift 8 파일 | `CLAUDE.md` `CONTEXT.md` `ADR-0005` `docs/plans/2026-05-29-...grill.md` `SKILL.md` `nf-orchestrator.md` `nf-implement-slice-status.py` `.claude/skills/nf-impl-plan/templates/dev/team-execution-plan.md` | ✅ grep |
| 3 | `nf-orchestrator.md` | **A′**: agent 정의 *유지*, `claude --agent` 로 main-thread 기동. `tools: Agent`(L4) 정당. prose 만 정정 — "main session 안에서 sub-agent 로 spawn"(L10) 제거, chain self-respawn(L49 / loop step 5) 삭제 | ✅ 확인 |
| 4 | --stop/--status | 외부 state-file mutation script 신설 (A/A′ 공통) | 설계 |
| 5 | lane 병렬 정책 | 기본 code→tester→(reviewer/verifier). 병렬은 read-only lane 또는 worktree 격리 시만. ADR/agent 에 명시 | |
| 6 | lane dispatch prompt | slice id + scope_files + readiness excerpt + state path + **verdict schema** 매번 inject (parent history 없음). `nf-orchestrator.md:120-131` 확장 | |
| 7 | D8 dangling ref | ADR 은 D1–D7. `nf-orchestrator.md:66,85` + `SKILL.md:69,206` 의 "D8" → retry tier 를 **ADR D8 로 승격** 또는 "plan §1 Q8" 로 정정 | ✅ 확인 4곳 |
| - | `nf-implement-slice-status.py` | chain_depth emit/read → resume 의미로 (코드 변경) | ✅ grep |

**주의** — `.claude/worktrees/nf-implement-preflight-audit/` 는 별도 worktree
(untracked). 위 grep 결과의 worktree 경로는 **건드리지 말 것** (memory
[[feedback_worktree_abs_path]]). canonical tree 만 편집.

## 추천 skill (다음 세션)

- ADR/문서 편집 = 직접 Edit (lifecycle skill 아님).
- PR 사이클 = memory [[feedback_merge_pr]] (push branch + `gh pr create`).
- PR 전 doc claim 코드 검증 = memory [[feedback_doc_code_accuracy]].
- 다파일 변경이므로 `superpowers:writing-plans` 로 PR 슬라이싱 고려 (ADR 개정 PR /
  SKILL+agent reframe PR / 용어drift PR 분리 가능).

## 진행 상태 / 참조

- memory: [[project_autonomous_runner_grill_decisions]] (blocker + A pivot 기록됨),
  [[project_nssf_phase1_wave1_wrappers_status]].
- plan: `docs/plans/2026-05-29-nf-implement-autonomous-runner-grill.md`.
- branch: main, 미커밋 (분석/송신/prep 만). ADR 개정은 새 branch.
- Pane 2 review 원문 = 본 세션 마지막 user turn.
