---
plan: nf-implement-autonomous-runner
date: 2026-05-29
status: draft
trigger: |
  `/nf-implement` SKILL.md (E3, 2026-05-22) 가 contract/skeleton 만 정의.
  Phase 1 wave 1 (#87/#92/#98/#99/#101) 까지 사람이 PR 사이클을 매뉴얼로 수행.
  CONTEXT.md L59 정의 — "`/nf-implement` 는 실제 source/test/CI 를 *만든다*" — 와 어긋남.
inputs:
  - .claude/skills/nf-implement/SKILL.md
  - dev/nssf/pr-slicing-plan.yaml
  - dev/nssf/agent-execution-plan.yaml
  - dev/nssf/team-execution-plan.md
  - dev/nssf/verification-matrix.yaml
  - dev/nssf/_implementation_run_state.yaml
  - docs/adr/ADR-0004-project-security-baseline.md
  - CONTEXT.md
  - CLAUDE.md
related:
  - docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md
---

# /nf-implement autonomous runner — 설계 freeze

본 문서는 2026-05-29 grill 의 11 분기 결정 + 4 PR rollout spec freeze.

## 0. 문제

`/nf-implement` 가 자율 runner 가 되지 않아 사람이 매 PR 사이클 (1 slice = 1 PR) 을
매뉴얼 orchestration. user 의 표현 — *"Phase 5 까지 사람이 다음 schema-bootstrap
해 라고 지시하지 않아도 agent 가 plan 읽고 다음 일을 선택해야 의미가 산다"*.

본 plan 의 목적 = `/nf-implement <nf>` 한 번 호출로 readiness pack PASS 이후
phase 5 종료 또는 본질 blocker 까지 자율 진행하게 만드는 것.

## 1. 결정사항 (Q1~Q11)

### Q1 — autonomy frontier
**self-merge 허용**. orchestrator main agent (subagent) 가 `gh pr merge --squash`
권한을 가짐. CLAUDE.md / memory `feedback_merge_pr` 단서 갱신.

### Q2 — runner topology
**Claude session 안의 sub-agent**. 사람이 `/nf-implement <nf>` 1회 호출 →
orchestrator subagent loop. 5 신규 NF-agnostic agent 정의:
`.claude/agents/nf-{orchestrator,code,reviewer,tester,verifier}.md`.

### Q3 — next-slice picker
**`pr-slicing-plan.yaml` topo sort + `gh` 동적 조회**. 매 iteration 마다
`gh pr view --json state,mergeStateStatus,statusCheckRollup` → next = `depends_on`
모두 `merged` AND 자신 `status != merged` 의 첫 매칭. plan yaml 의 `status` 필드는
*사람 history*, runtime 은 gh 가 진실 출처. 신규 script
`design/scripts/nf-implement-slice-status.py`.

### Q4 — self-merge gate (4 condition)
1. PR 의 `required_checks` 전부 PASS (verification-matrix.yaml).
2. reviewer subagent 가 ADR-0004 security baseline + contract drift + scope
   (changed_paths ⊆ slice scope_files) 검사 PASS.
3. `mergeable: MERGEABLE`.
4. branch up-to-date with main.

머지 = `gh pr merge --squash --delete-branch`.

**Auto-revert**: 머지 후 main CI red 감지 → 자동 revert PR (reviewer skip +
auto-merge) + orchestrator stop + blocker 기록.

**Post-merge sync**: 다음 slice 시작 전 `git fetch origin main && git rebase main` 자동.

### Q5 — out-of-scope 발견 처리
**Plan-first** — out-of-scope 감지 시 orchestrator 가 `pr-slicing-plan.yaml`
신규 entry / `depends_on` 갱신 PR 먼저 작성 → reviewer (planner-mode) 가
ADR-0001 vocab + slice policy alignment 검사 → self-merge → next-slice 재계산.

**Plan amendment template**: `dev/<nf>/plan-amendments/<date>-<slug>.md` 에 변경
이유 + alternatives + impact 1-pager.

**3-trigger escape hatch** — 다음 중 1개 이상이면 orchestrator stop + 사람 보고:
1. ADR 신설 동반 변경.
2. `eng_frozen` decision 변경 (engineering-design.md 갱신).
3. security baseline 영향 (ADR-0004 7항목 중 1개).

### Q6 — context window / long-run
**Chain self-respawn**. orchestrator 가 context 70% 도달 시:
1. `_implementation_run_state.yaml` checkpoint.
2. `Agent` tool 로 동일 type 신규 orchestrator subagent spawn — hand-off
   prompt 는 state file path + last_checkpoint_at + chain_depth 만.
3. 본인 exit.

**Hand-off prompt 표준**:
```
context: /nf-implement <nf> --resume
state_file: dev/<nf>/_implementation_run_state.yaml
last_checkpoint: <ISO>
last_phase: <N>
last_slice_pr: <#URL>
chain_depth: <N>
instruction: read state_file → call next-slice picker → continue loop until phase 5 done or 3-trigger escape (Q5).
```

**Depth cap = 20**. 도달 시 stop + 사람 보고.

**사람 인터페이스**: GitHub PR 알림 (라벨 `nf-implement-autonomous`) + main session
final summary (살아있을 때).

### Q7 — lane subagent 정의
**5 NF-agnostic agent + Bash 명령 화이트리스트 + reviewer write 권한 0**:

| lane | allowed Bash | Edit/Write | 기타 |
|---|---|---|---|
| nf-orchestrator | `.venv/bin/python3 design/scripts/*`, `gh pr *`, `git *`, `cat *`, `ls *` | `dev/<nf>/_implementation_run_state.yaml`, `pr-slicing-plan.yaml` | Agent (다른 4 lane dispatch) |
| nf-code | `cmake *`, `ctest *`, `git *`, `gh pr view *` | Edit/Write 자유 (review-time scope check) | Read |
| nf-reviewer | `gh *`, `git diff *`, `git log *` | *없음* | Read |
| nf-tester | `ctest *`, `cmake *`, `gcov *`, `git *` | tests/<nf>/** (review-time scope check) | Read |
| nf-verifier | `.venv/bin/python3 design/scripts/*`, `gh pr checks *`, `git *` | verification log | Read |

write-scope 강제는 *reviewer review-time scope check* 가 primary. Claude Code
`allowed-tools` 의 Edit/Write path filter 는 약함 → 의존하지 않음.

Dispatching pattern = `superpowers:dispatching-parallel-agents` 재사용.

### Q8 — failure 재시도 정책 (3-tier)
- **tier 0 (flake)**: CI red 1회는 `gh pr checks` re-run 1회.
- **tier 1 (auto-fix)**: build/test/lint 실패는 code (또는 tester) subagent
  재호출 + failure log prompt. max 2 attempt.
- **tier 2 (diagnose)**: tier 1 실패 또는 reviewer reject 본질적 → `diagnose`
  skill 호출 → root cause 분석 → (a) 코드 수정 1 attempt 또는
  (b) plan amendment PR (Q5) 또는 (c) tier 3 전이.
- **tier 3 (stop)**: engineering decision / security baseline / `eng_frozen`
  영향 → blocker 기록 + 사람 보고. 동일 slice 의 tier 2 도달 횟수 ≥ 2 시 자동
  tier 3.

**Counter schema** — `_implementation_run_state.yaml.current_slice_attempts:
{tier0: N, tier1: N, tier2: N}`. slice merge 시 reset.

**Flake vs 본질**:
- 동일 step + 동일 메시지 2회 연속 = 본질 (tier 1 진입).
- 다른 step 또는 다른 메시지 = flake (tier 0 1회 후 결정).

**Merge conflict 2-class**:
- *trivial* (다른 파일/다른 함수/import order) = auto-rebase + retry.
- *semantic* (같은 함수/같은 줄) = tier 2.

**Failure log**:
- `_implementation_run_state.yaml.blockers[]` (요약).
- `dev/<nf>/_failure_log.yaml` (gitignored, attempt 별 stdout/stderr/exit).
- 사람 audit 표면 = GitHub PR comment + main session push.

### Q9 — rollout (4 PR 시퀀스)
| PR | 책임 | self-merge |
|---|---|---|
| **PR-A** | ADR-0005 신설 + CLAUDE.md 의사결정 § 갱신 + CONTEXT.md glossary + memory feedback_merge_pr 갱신. 정책 freeze. | ✗ 사람 review |
| **PR-B** | 5 `.claude/agents/nf-{lane}.md` 신설 + `design/scripts/nf-implement-slice-status.py` + `_implementation_run_state.yaml` schema 확장 + `infra/security-baseline-patterns.yaml`. infrastructure. | ✗ 사람 review |
| **PR-C** | `/nf-implement` SKILL.md skeleton → runner rewrite + `dev/<nf>/plan-amendments/` template. runner contract. | ✗ 사람 review |
| **PR-D** | NSSF 첫 자율 slice — dry-run 검증 + 1 slice 실전 자율 진행. | ✗ 사람 review (bootstrap) |

PR-A merged 이후 *NSSF feature slice* 가 self-merge 대상. 본 4 PR 자체는 meta-layer.

### Q10 — human stop/observe interface
**3-control + 라벨 + cost cap**:
- `/nf-implement <nf>` = start.
- `/nf-implement <nf> --stop` = graceful — state file 에 `stop_requested: true`
  기록 → 현재 slice merge 후 chain 종료.
- `/nf-implement <nf> --status` = `nf-implement-slice-status.py` 호출,
  next-slice + 현재 open PR + chain_depth + last_checkpoint +
  total_slices_completed + tier counters 출력.
- orchestrator 가 모든 PR 에 라벨 `nf-implement-autonomous` + `nf:<name>` 부착.
- cost cap = chain_depth 20 (Q6) + total_slices_completed 50 + 동일 slice tier2
  attempts ≥ 2 (Q8). 셋 중 하나 도달 시 stop.

**Multi-NF**: NF 별 격리 (state file 도 NF 별), 동시 진행 가능. 단 권고 = NF
1개 / session. 강제 아님.

**spec_reread_audit chain 누적**: hand-off prompt 에 audit summary 포함, fresh
subagent 가 append. `blocked` entry 1개 = 즉시 stop (현 SKILL.md 정책 유지).

### Q11 — security baseline drift 검출
**2-layer**:
- **Layer A (deny-pattern grep)**: `infra/security-baseline-patterns.yaml` 에
  ADR-0004 7항목 별 deny / allow regex. reviewer 가 `git diff` 에 deny 매칭 →
  reject.
- **Layer B (semantic config-source check)**: TLS/OAuth2 init call 의 인자가
  operator-config (`config.tls.cert_path` 류) 인지 hardcode 인지 검사. hardcode
  block, *AST 한계로 false negative 허용* — primary block 은 Layer A.

**Pattern yaml schema**:
```yaml
{adr_item: <ADR-0004 §N>, deny: [regex...], allow: [regex...],
 severity: block|advisory, applies_to: [file_glob...]}
```

**Retro-test CI gate**: PR-B 에 retro fixture (과거 NSSF PR #87/#92/#98/#99/#101
patch) 적용 → false positive 0 검증. 새 pattern 추가 시 fixture 의무.

**Advisory** (block 아님): ADR-0004 evidence emit 동반 여부 (metric/log).

## 2. CONTEXT.md glossary 신규 용어

| 용어 | 정의 |
|---|---|
| Autonomous Implementation Orchestrator | `/nf-implement` 호출 시 Claude session 안에서 sub-agent 로 도는 entity. plan 읽고 next-slice 선택, lane subagent dispatch, PR open/merge, chain self-respawn. |
| Lane Subagent | 5 NF-agnostic agent (orchestrator/code/reviewer/tester/verifier). `.claude/agents/nf-{name}.md`. |
| Slice Picker | `pr-slicing-plan.yaml` topo sort + gh 상태 조회 → next PR slice. `design/scripts/nf-implement-slice-status.py`. |
| Chain Self-Respawn | context window 한계 시 orchestrator 가 동일 type subagent 새로 spawn 후 본인 exit. depth cap 20. |
| Self-Merge | orchestrator 의 `gh pr merge --squash --delete-branch` 자동. ADR-0005 가 권한 부여. |
| Plan Amendment | `pr-slicing-plan.yaml` 갱신 PR. `dev/<nf>/plan-amendments/<date>-<slug>.md` 동반. orchestrator self-merge 가능 단 3-trigger escape 미적용 시. |

## 3. ADR 신설

[`docs/adr/ADR-0005-autonomous-implementation-policy.md`](../adr/ADR-0005-autonomous-implementation-policy.md)

자율 권한 부여 + 5 lane + chain + drift 검출 의 결정 justification + alternatives.

## 4. Memory 갱신

- `feedback_merge_pr.md` — orchestrator `gh pr merge --squash` 예외 단서 추가.
  사람 직접 main push 금지는 유지.
- `project_autonomous_runner_grill_decisions.md` (신규) — Q1~Q11 결정 요약 +
  본 plan 링크.

## 5. PR-A/B/C/D scope_files 초안

### PR-A (정책 freeze)
- `docs/adr/ADR-0005-autonomous-implementation-policy.md` (신규)
- `docs/plans/2026-05-29-nf-implement-autonomous-runner-grill.md` (본 파일)
- `CLAUDE.md` (의사결정 § 단서)
- `CONTEXT.md` (glossary 6 용어 추가)
- memory (PR 무관 — 별도 commit)

depends_on: []

### PR-B (infrastructure)
- `.claude/agents/nf-orchestrator.md`
- `.claude/agents/nf-code.md`
- `.claude/agents/nf-reviewer.md`
- `.claude/agents/nf-tester.md`
- `.claude/agents/nf-verifier.md`
- `design/scripts/nf-implement-slice-status.py`
- `design/scripts/lib/` (필요 시)
- `infra/security-baseline-patterns.yaml`
- `tests/security_patterns/` (retro-test fixture)
- `.github/workflows/security-baseline-retro.yml` (CI gate)
- `_implementation_run_state.yaml` schema 확장 문서 — SKILL.md 안에 inline (PR-C
  에서 갱신).

depends_on: [PR-A]

required_checks (예상):
- pytest_full_suite
- validate_artifact_catalog
- security_baseline_retro_fixture_pass

### PR-C (runner contract)
- `.claude/skills/nf-implement/SKILL.md` (skeleton → runner rewrite)
- `dev/nssf/plan-amendments/.gitkeep` (template dir)
- `dev/nssf/plan-amendments/_template.md` (template)
- `dev/nssf/pr-slicing-plan.yaml` (autonomous-runner-rollout pack section
  추가 — PR-A/B/C/D entry).

depends_on: [PR-B]

### PR-D (NSSF first autonomous slice)
- 본 PR 자체는 *결과 PR* — orchestrator 가 만든 slice 가 PR-D 로 머지됨.
- dry-run evidence 동반:
  - `dev/nssf/_implementation_run_state.yaml` (gitignored, but evidence 첨부).
  - `gh pr list --label nf-implement-autonomous` 출력.
- 첫 slice 의 *내용* 은 next-slice picker 결과에 따라 결정 (P2b PatchDocument
  wrapper 또는 P3 backlog 후보).

depends_on: [PR-C]

bootstrap exception — 본 4 PR 자체는 *사람 review/merge*. self-merge 권한은
PR-A merge 이후 NSSF feature slice 에만 적용.

## 6. 검증 / 종료 기준

본 plan 의 종료 기준 = PR-A/B/C/D 4건 모두 merged. 그 이후:
- 검증 = `/nf-implement nssf --dry-run` 으로 next-slice picker + lane allocation
  + cost cap 출력 확인.
- 실전 = `/nf-implement nssf` 1회 호출 → orchestrator 가 1 slice 자율 진행
  (NSSF feature slice 머지까지).
- 회귀 검증 = NSSF Phase 1 wave 1 잔여 slice (P2b 또는 P3 backlog) 가 자율
  머지 후 main green 유지.

## 7. 자주 틀리는 지점

- 본 plan 의 4 PR 자체에 self-merge 적용 — 금지. bootstrap 은 사람 review.
- pattern yaml (Q11) 을 ADR-0004 본문과 분리 갱신 — 금지. ADR-0004 변경 시
  yaml 도 동기 갱신 PR 동반.
- chain hand-off 에 *주관적 진행 요약* 포함 — 금지. state file path + checkpoint
  metadata 만. fresh subagent 가 plan 다시 읽음.
- next-slice picker 결과를 사람이 *override* — 가능하지만 plan amendment PR
  필요. 직접 `_implementation_run_state.yaml` 편집 금지.
