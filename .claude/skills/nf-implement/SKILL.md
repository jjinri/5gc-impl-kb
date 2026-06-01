---
name: nf-implement
description: Public autonomous implementation runner for `/nf-implement <nf>` in 5gc-impl-kb. Requires `readiness_pack_ready` PASS (built by `/nf-readiness <nf>`). Use when the user requests autonomous code generation for an NF (e.g. "/nf-implement nssf", "NSSF 구현 시작", "tracer-bullet 시작", "/nf-implement nssf --phase 1 --dry-run"). Acts as a launcher/preflight: after the readiness gate it starts `nf-orchestrator` as a *main-thread* agent (`claude --agent nf-orchestrator`), which loops next-slice pick → 4 lane dispatch (code/reviewer/tester/verifier) → self-merge under the 4-condition gate → checkpoint/resume → until phase 5 done, cost cap, or 3-trigger escape (ADR-0005). Defines the entry contract, state schema, human stop/observe surface, and the *no spec semantic rediscovery* invariant. Implementation policy = ADR-0005, infrastructure = PR-B (#104).
argument-hint: "<nf> [--phase <1-5|all>] [--resume] [--dry-run] [--stop] [--status]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-readiness-status.py *) Bash(.venv/bin/python3 design/scripts/nf-implement-slice-status.py *) Bash(.venv/bin/python3 design/scripts/nf-implement-control.py *) Bash(cat *) Bash(ls *) Bash(find dev/* *) Bash(find design/* *) Bash(find engineering/* *) Bash(git status --short) Bash(git log --oneline -n *) Bash(gh pr list *) Bash(gh pr view *) Bash(mkdir -p *)
---

# nf-implement — autonomous implementation runner

본 skill 은 `/nf-implement <nf>` 의 *launcher / preflight* 다. 호출 시 정책 +
입력 검증 + Phase 0 readiness 확인 후 **`nf-orchestrator` 를 main-thread agent
로 기동** (`claude --agent nf-orchestrator`) 해서 자율 implementation loop 를
시작한다. orchestrator 는 *subagent 가 아니라 main-thread agent* 이므로 lane
subagent 4 종 (`nf-code` / `nf-reviewer` / `nf-tester` / `nf-verifier`) 을 `Agent`
tool 로 dispatch 할 수 있고, self-merge (ADR-0005 D1) 까지 자율로 처리한다.

> **왜 main-thread agent 인가.** harness 제약상 subagent 는 subagent 를 spawn 하지
> 못한다 (Agent tool 이 subagent unavailable tools). SKILL 이 orchestrator 를
> subagent 로 spawn 하면 orchestrator 가 lane 을 다시 spawn 하는 2-level nesting 이
> 되어 작동 불가. 따라서 orchestrator 는 `claude --agent nf-orchestrator` 로 전용
> pane/process 에서 main-thread 로 기동한다 (ADR-0005 D3).

정책 source — [`docs/adr/ADR-0005-autonomous-implementation-policy.md`](../../../docs/adr/ADR-0005-autonomous-implementation-policy.md).
인프라 source — PR-B (#104) 의 5 agent 정의 (orchestrator + 4 lane) + slice picker
+ security pattern yaml.

## 입력

- `<nf>` — NF 이름. 같은 NF 의 `dev/<nf>/_readiness_status.yaml` 이
  `readiness_pack_ready: PASS` 여야 한다.
- `--phase <1-5|all>` (옵션) — 특정 phase 만 실행. 기본 `all`.
- `--resume` (옵션) — `dev/<nf>/_implementation_run_state.yaml` 의
  `completed_phases` 다음부터 재개.
- `--dry-run` (옵션) — Phase 0 + next-slice picker 출력 + 계획만, 실제 dispatch
  없음.
- `--stop` (옵션) — graceful stop. state 의 `stop_requested: true` 기록 → 진행
  중 slice 머지 후 loop 종료. orchestrator 가 활성 시점에 신호 수신.
- `--status` (옵션) — `nf-implement-slice-status.py` 호출 + state 요약 출력,
  loop 시작 없음.

## 책임 범위

본 runner 는 *autonomous implementation orchestration* 의 *entry contract* 다.

- Phase 0 readiness gate 확인 (`readiness_pack_ready` PASS).
- 입력 검증 + state file 호환 검증.
- `nf-orchestrator` 를 main-thread agent 로 launch (`claude --agent
  nf-orchestrator`). 이후 checkpoint/resume 은 harness compaction + `--resume`
  재기동 책임.
- 사람 인터페이스 명세 (`--stop` / `--status` / 라벨).
- *no spec semantic rediscovery* invariant 강제.
- team-execution-plan.md 의 lane override 를 orchestrator dispatch prompt 에
  inject.

본 runner 가 *직접 하지 않는* 것 — lifecycle 산출 재생성 (`/nf-readiness <nf>`
책임), LLM judgment slice 선택 (slice picker 가 deterministic), 머지 결정
(orchestrator self-merge gate).

## Phase 0 — readiness gate

```bash
.venv/bin/python3 design/scripts/nf-readiness-status.py <nf>
```

본 명령 결과의 `gates.readiness_pack_ready.status == PASS` 만 GO. FAIL 시 다음
조치 안내 후 stop.

- `blocked_by` 의 첫 항목 (예 `impl_ready_for_codegen`) 를 사용자에게 보고.
- 해당 upstream gate 의 `to_pass` (각 status YAML 의 check entry) 안내.
- 다음 행위 — `/nf-readiness <nf>` 재실행 또는 해당 lifecycle skill 직접 호출.

`--dry-run` 이면 Phase 0 결과 + slice picker 출력 + lane dispatch 계획만 보고
후 종료.

## Phase 1~5 contract (orchestrator 의 책임)

본 runner 는 entry contract 만 정의한다. 각 phase 의 산출·gate 명세는
`dev/<nf>/verification-plan.md` + `verification-matrix.yaml` + ADR-0005 D8 의
retry tier 가 진실 출처.

| phase | 목적 | exit gate |
|---|---|---|
| 1 (tracer + wave) | 최소 vertical slice + wave 1 full feature wiring | `cmake + ctest` PASS, `tracer_bullet_passed: true`. |
| 2 | NSSAIAvailability writes | Put/Patch/Delete/Options integration test PASS. |
| 3 | subscription | 3 op integration test PASS + initial snapshot dispatch 검증. |
| 4 | contract / security / e2e | 8 op × 18 cause matrix green, ADR-0004 7항목 evidence emit, e2e scenario green. |
| 5 | hardening | metric/log/trace seam production-capable, label cardinality / log redaction / trace sampler freeze. |

orchestrator 가 phase 진입 시 verification-matrix 의 해당 wave 의 required gate
를 nf-verifier 에 전달한다.

## State file schema — implementation-run-state-v2

`dev/<nf>/_implementation_run_state.yaml` (gitignored, orchestrator 가 단독
write).

```yaml
schema_version: implementation-run-state-v2
nf: <nf>
readiness_generated_at: <ISO timestamp from _readiness_status.yaml>
current_phase: <0|1|2|3|4|5>
completed_phases: [<int>, ...]
active_task_ids: [<id>, ...]
completed_task_ids: [<id>, ...]
last_checkpoint_at: <ISO timestamp>
last_command: <string>
resume_count: <int>                   # ADR-0005 D6 — --resume 누적 횟수 cap source (이전 chain_depth).
run_epoch: <int>                      # resume 시퀀스 번호 (resume 마다 +1), audit field.
total_slices_completed: <int>         # cost cap source 2.
current_slice:
  id: <slice id>
  pr_url: <gh URL or null>
  branch: <local branch>
current_slice_attempts:
  tier0: <int>
  tier1: <int>
  tier2: <int>
stop_requested: <bool>                # --stop 신호. orchestrator 가 현재 slice merge 후 종료.
validation_evidence:
  - phase: <int>
    artifact: <path>
    note: <string>
blockers:
  - phase: <int>
    description: <string>
    to_resolve: <string>
spec_reread_audit:
  - accessed_path: specs/<spec>/<file>
    purpose: <allowed|blocked>
    justification: <string>
next_step: <string>
```

v1 → v2 변경:
- `resume_count`, `run_epoch`, `total_slices_completed`, `current_slice`,
  `current_slice_attempts`, `stop_requested` 신설.
- orchestrator 가 `--resume` 시 v1 → v2 자동 변환 가능 (누락 필드 = 0/false/null).

## Workflow

### 1. 입력 검증
- `<nf>` 비어있으면 stop.
- 정의된 옵션 외 사용 시 stop.
- `--stop` / `--status` 단독은 `<nf>` 만 필요.

### 2. Phase 0 readiness gate

```bash
.venv/bin/python3 design/scripts/nf-readiness-status.py <nf>
```

PASS → 3 단계. FAIL → blocked_by + to_pass 보고 후 stop.

### 3. `--status` 처리 (있을 때 단독)

```bash
.venv/bin/python3 design/scripts/nf-implement-slice-status.py <nf>   # next-slice + counters
.venv/bin/python3 design/scripts/nf-implement-control.py <nf> --status   # control state
```

slice picker 결과 + control state 요약 출력. orchestrator launch 없음.

### 4. `--stop` 처리 (있을 때 단독)

```bash
.venv/bin/python3 design/scripts/nf-implement-control.py <nf> --stop
```

state file 의 `stop_requested = true` (+ `stop_requested_at`) 를 외부에서 기록한다.
A′ 에서 orchestrator 는 별도 process 이므로, main pane 의 본 SKILL 이 control
script 로 state 를 mutate 하고 orchestrator 가 매 loop iteration 시작 시 flag 를
poll 해 현재 slice 머지 후 graceful 종료한다. 이미 종료 상태면 안내만.
재기동 전 해제는 `nf-implement-control.py <nf> --clear-stop`.

### 5. `--dry-run` 처리

Phase 0 PASS + slice picker 결과 + orchestrator dispatch prompt 계획만 출력
후 종료. 실제 dispatch 없음.

### 6. orchestrator launch (default)

orchestrator 는 *main-thread agent* 로 기동한다 — `Agent` tool 의 subagent spawn
이 아니다 (2-level nesting 금지, ADR-0005 D3). 전용 pane/process 에서:

```bash
claude --agent nf-orchestrator "<nf> [--resume] [--phase <n>]"
```

launch 시 orchestrator 가 받아야 할 context (인자/초기 prompt 로 전달):

- `<nf>` 이름.
- `--resume` 또는 fresh start 여부.
- `dev/<nf>/_implementation_run_state.yaml` path (있으면 last_checkpoint
  metadata 만).
- `dev/<nf>/pr-slicing-plan.yaml` path.
- `dev/<nf>/team-execution-plan.md` 의 user_sections override 발췌.
- ADR-0005 의 cost cap 값 (resume_count 20 / total_slices_completed 50 /
  tier2 ≥ 2).
- ADR-0004 security baseline 의 의무 7 항목 요약.
- `infra/security/baseline-patterns.yaml` path.

> **Launch 메커니즘 (현재 = 수동 baseline).** 본 SKILL 은 preflight (Phase 0 +
> 입력 검증 + launch context 조립) 까지 수행하고, 위 `claude --agent
> nf-orchestrator` 명령을 사람이 전용 pane/터미널에서 실행하도록 안내한다.
> tmux new-pane / background process 자동 launch 는 *deferred follow-up* —
> 메커니즘 확정 후 SKILL allowlist 에 해당 launch 명령 (`tmux`/`bg`) 을 추가한다
> (handoff #108 §운영 디테일). 무거운 gh/git/state-write 권한은 SKILL 이 아니라
> orchestrator agent tools 에 잔류한다.

orchestrator 가 loop 시작 후 본 wrapper 는 종료. orchestrator 는 phase 5 done /
cost cap 도달 / 3-trigger escape 까지 진행하며, context 한계 시 checkpoint 후
`--resume` 으로 재기동된다.

### 7. 최종 보고

orchestrator 가 종료 시 main session 으로 final summary push (살아있을 때).

- 진행한 phase 목록.
- 머지된 PR 목록 (라벨 `nf-implement-autonomous` + `nf:<nf>`).
- `spec_reread_audit` 요약.
- `dev/<nf>/_implementation_run_state.yaml` 의 final state.
- 다음 행위 — Phase 5 PASS 면 "merge / release 준비 완료", cost cap 도달 시
  cap 종류 + resume 안내, blocker 발생 시 첫 blocker.

## 자율 boundary

ADR-0005 D1~D8 가 boundary 의 source.

- **self-merge 허용** (D1) — orchestrator 의 `gh pr merge --squash --delete-branch`.
- **4-condition gate** (D2) — required_checks PASS + reviewer subagent approve
  + MERGEABLE + branch up-to-date.
- **main-thread orchestrator + 4 lane** (D3) — orchestrator (main-thread agent)
  + code / reviewer / tester / verifier subagent. lane 기본 직렬, 병렬은
  read-only/worktree 격리 시만. NF-specific override 는 team-execution-plan.md
  user_sections.
- **plan-driven slice picker** (D4) — `nf-implement-slice-status.py` 가 단일
  진실 출처. LLM judgment 금지.
- **plan amendment self-merge + 3-trigger escape** (D5) — ADR 신설 /
  `eng_frozen` 변경 / ADR-0004 영향 시 즉시 stop.
- **checkpoint/resume + cost cap** (D6) — context 한계 시 harness compaction +
  state checkpoint, `--resume` 재기동 (chain self-respawn 폐기). resume_count 20 /
  total_slices_completed 50 / 동일 slice tier2 ≥ 2 셋 중 1 도달 시 stop.
- **2-layer security drift** (D7) — `infra/security/baseline-patterns.yaml` 의
  Layer A (block) + Layer B (advisory). reviewer subagent 가 review-time 검사.
- **3-tier retry** (D8) — tier0 flake / tier1 auto-fix / tier2 diagnose /
  tier3 stop. 동일 slice tier2 ≥ 2 시 자동 tier3.

## 사람 인터페이스

- `--status` — `nf-implement-slice-status.py` + state 요약.
- `--stop` — graceful stop (현재 slice merge 후 종료).
- 라벨 — orchestrator 가 모든 PR 에 `nf-implement-autonomous` + `nf:<nf>`
  부착. `gh pr list --label nf-implement-autonomous` 로 외부 audit.
- main session 보고 — orchestrator 가 loop 종료 시 final summary push.
- audit log — `dev/<nf>/_implementation_run_state.yaml` (gitignored, 사람 tail
  가능) + `dev/<nf>/_verification_log.yaml` (verifier append only).

## 3-trigger escape (사람 개입 필수)

ADR-0005 D5. 다음 중 1+ 발생 시 orchestrator 즉시 stop + 사람 보고 + 사람
review/merge 필요.

1. ADR 신설 동반 변경.
2. `eng_frozen` decision 변경 (engineering-design.md 갱신).
3. ADR-0004 security baseline 영향.

본 wrapper 는 escape 발생 시 main session 으로 보고만 — escape 결정은
orchestrator 의 책임.

## No Spec Semantic Rediscovery (invariant)

ADR-0005 §No Spec Semantic Rediscovery + ADR-0004 baseline + 본 runner 의 핵심
invariant. codegen agent 는 `specs/<spec>/` 의 원본 OpenAPI / docx 를 *semantic
decision / data model / API matrix 도출* 목적으로 *다시 읽지 않는다*. readiness
pack (`api-implementation-matrix.md`, `data-model-implementation-map.md`,
`codegen-work-items.yaml`, `spec-to-design-coverage.md` 등) 이 충분한 입력이어야
한다.

### Allowed reads

- `specs/<spec>/` 의 *source trace* (PR evidence 또는 drift check 용 reference).
- generator (openapi-generator) 의 input 으로 spec YAML 을 *generator-only mode*
  로 통과.

### Blocked

- "이 schema 가 어떤 의미인지 specs 에서 다시 봐야겠다" 류의 *semantic rediscovery*.
- API operation 의 비공개 행동을 specs 에서 *새로 발견* 하는 시도.
- ADR-0004 외의 security 결정을 33.501 / 33.310 / 33.210 에서 *재발견* 하는 시도.

### Audit

orchestrator 가 implementation 동안 `spec_reread_audit` 에 entry append.
`blocked` entry 1개라도 발생 시 즉시 stop + `/nf-readiness <nf>` reopen 안내.

## 자주 틀리는 지점

- readiness FAIL 상태에서 orchestrator dispatch — 금지. Phase 0 단일 invariant.
- `--resume` 시 readiness 가 갱신됐는지 확인 안 함 — `readiness_generated_at`
  일치 필수. 불일치 = `/nf-readiness <nf>` 재실행 안내.
- orchestrator 가 LLM judgment 으로 slice 선택 — 금지. `nf-implement-slice-status.py`
  가 단일 진실 출처.
- 본 wrapper 가 코드/테스트 직접 편집 — 금지. lane subagent 책임.
- resume hand-off 에 *주관적 진행 요약* 의존 — checkpoint 는 state file 의
  last_checkpoint metadata (last_checkpoint_at + completed_phases +
  current_slice) 가 단일 출처. prompt 의 산문 요약에 의존 금지.
- `--stop` 신호 무시하고 무한 진행 — 금지. orchestrator 가 매 iteration 시작
  시 stop_requested 확인.
- 본 runner 가 next-slice picker 결과를 *override* — 가능하지만 사람의 plan
  amendment PR 필요. 직접 `_implementation_run_state.yaml` 편집 금지.

## 참고 — 본 skill 안에 다시 적지 말 것

- aggregate gate 정의 — `design/scripts/nf-readiness-status.py` docstring +
  ADR-0002.
- readiness pack 9 파일 — `nf-impl-plan/SKILL.md`.
- contract implementability 11-check — `nf-impl-status/SKILL.md`.
- security baseline mandate — `docs/adr/ADR-0004-project-security-baseline.md`
  + `design/policies/security-baseline.yaml`.
- security drift pattern — `infra/security/baseline-patterns.yaml`.
- 자율 runner 정책 — `docs/adr/ADR-0005-autonomous-implementation-policy.md`.
- lane subagent prompt template — `.claude/agents/nf-<lane>.md`.
- next-slice picker 알고리즘 — `design/scripts/nf-implement-slice-status.py`
  docstring.
