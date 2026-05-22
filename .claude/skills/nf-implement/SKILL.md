---
name: nf-implement
description: Public autonomous implementation wrapper for `/nf-implement <nf>` in 5gc-impl-kb. Requires `readiness_pack_ready` PASS (built by `/nf-readiness <nf>`). Use when the user requests autonomous code generation for an NF (e.g. "/nf-implement nssf", "NSSF 구현 시작", "tracer-bullet 시작", "/nf-implement nssf --phase 1 --dry-run"). Defines Phase 0~5 contract (readiness gate → tracer-bullet → task graph normalization → feature waves → contract/security/e2e → hardening), team lane responsibilities (orchestrator / code / reviewer / tester / verifier), stop/resume checkpointing, and the *no spec rediscovery* rule. **본 PR (E3) 은 skeleton/contract 만 정의한다 — NSSF tracer-bullet 실제 실행과 src/generated/tests/CI 산출은 PR G 의 책임.**
argument-hint: "<nf> [--phase <1-5|all>] [--resume] [--dry-run]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-readiness-status.py *) Bash(cat *) Bash(ls *) Bash(find dev/* *) Bash(find design/* *) Bash(find engineering/* *) Bash(git status --short) Bash(mkdir -p *)
---

# nf-implement — public autonomous implementation wrapper

본 skill 은 plan §3 (workflow upgrade, 2026-05-21) 의 public workflow 3 행위 중 세 번째 — `/nf-implement <nf>` 를 정의한다. 사람이 `/nf-readiness <nf>` 로 readiness pack 을 산출한 뒤 본 wrapper 한 번 호출하면 lifecycle 의 *implementation 영역* 이 Phase 1~5 로 자동 실행된다.

> **본 PR (E3, 2026-05-22) 은 wrapper *skeleton* 만 정의한다.** Phase 1~5 의 실제 codegen / build / test / CI 산출은 PR G 의 책임. 본 wrapper 는 Phase 0 gate 확인 → Phase 별 contract 보고 → checkpoint 기록만 한다.

## 입력

- `<nf>` — NF 이름. 같은 NF 의 `dev/<nf>/_readiness_status.yaml` 이 `readiness_pack_ready: PASS` 여야 한다.
- `--phase <1-5|all>` (옵션) — 특정 phase 만 실행 (default `all`).
- `--resume` (옵션) — `dev/<nf>/_implementation_run_state.yaml` 의 `completed_phases` 다음부터 재개.
- `--dry-run` (옵션) — Phase 0 readiness 확인 + 계획 출력만, 실제 phase 실행 안 함.

## 책임 범위

본 wrapper 는 *autonomous implementation orchestration* 이다. *contract / 정책 / state machine* 정의가 본 wrapper 의 책임이며, lifecycle (contract / arch / impl-plan / engineering) 의 *재생성* 은 책임지지 않는다 (lifecycle 산출이 stale 이면 `/nf-readiness <nf>` 로 돌아간다).

- Phase 0 readiness gate (`readiness_pack_ready` PASS 확인).
- Phase 1~5 의 entry/exit gate 와 산출물 contract.
- run state 의 stop/resume/checkpoint.
- *no spec semantic rediscovery* 강제 + audit.
- team lane 의 read 책임 (`team-execution-plan.md`).

## Phase 0 — readiness gate

```bash
.venv/bin/python3 design/scripts/nf-readiness-status.py <nf>
```

본 명령 결과의 `gates.readiness_pack_ready.status == PASS` 만 implementation GO. FAIL 시 다음 조치 안내 후 stop.

- `blocked_by` 의 첫 항목 (예 `impl_ready_for_codegen`) 를 사용자에게 보고.
- 해당 upstream gate 의 `to_pass` (각 status YAML 의 check entry) 안내.
- 다음 행위 — `/nf-readiness <nf>` 재실행 또는 해당 lifecycle skill 직접 호출.

`--dry-run` 이면 Phase 0 결과만 보고 후 phase 계획 출력 + 종료.

## Phase 1 — tracer-bullet

목적 — *가장 작은 vertical slice* 로 전체 toolchain (source layout / generated boundary / build / test) 이 연결됨을 증명.

Phase 1 산출 contract.

- source layout / bootstrap 존재 (예 `src/<nf>/`, `CMakeLists.txt`, build manifest).
- generated / API stub boundary 1 개 (예 `generated/<nf>/api/<Op>.h`).
- 1 endpoint stub (NSSF 의 경우 `NSSelectionGet`) — 합의된 stub response (ProblemDetails / 501 / agreed value) 반환.
- 1~2 smoke / unit / contract test.
- build + test command 실행 및 PASS evidence.
- *spec semantic rediscovery 부재* evidence — run state 의 `spec_reread_audit` (아래 §7) 가 비어있거나 `allowed` 만.

Entry — Phase 0 PASS. Exit — 위 모든 산출 + `tracer_bullet_passed: true` 가 run state 에 기록됨.

본 PR (E3) 은 Phase 1 의 *contract* 만 정의. NSSF 의 실제 Phase 1 산출 (위 list 의 파일들) 은 PR G 의 책임.

## Phase 2 — task graph normalization

목적 — readiness pack 의 `codegen-work-items.yaml` 을 *runnable* 한 DAG / wave 로 확정.

산출.

- task graph (DAG) — items 의 `depends_on` 분석으로 위상 정렬.
- wave 분할 — 동시 실행 가능 그룹.
- lane 할당 — 각 item 의 `owner_lane` 을 team-execution-plan.md 의 lane 정의와 매칭.
- ownership ratify — 어느 lane 이 어느 wave 의 어느 item 을 담당하는지 명시.

Entry — Phase 1 PASS. Exit — task graph + wave + lane assignment 가 dev/<nf>/_implementation_run_state.yaml 에 기록.

## Phase 3 — feature waves

목적 — Phase 2 의 wave 단위로 API / data-model / persistence / security 기능 구현.

산출 (각 wave 별).

- 해당 wave 의 모든 item 의 `expected_files` 생성.
- 해당 wave 의 `tests` 실행 PASS.
- 해당 wave 의 `verification_commands` 실행 evidence.
- changed task id 가 run state 의 `completed_task_ids` 로 이동.

Entry — Phase 2 PASS. Exit — 모든 wave 의 task 완료 + 해당 verification PASS.

## Phase 4 — contract / security / e2e

목적 — 전체 NF 단위의 contract / OpenAPI / OAuth2 / TLS / mTLS / DB migration / integration / e2e 검증.

산출.

- OpenAPI contract test PASS (모든 operation × cause 매트릭스).
- ADR-0004 security baseline 7 항목 verification evidence.
- DB migration 적용 + rollback 테스트.
- integration / e2e 시나리오 PASS (`request-flow.md` 의 핵심 시나리오).

Entry — Phase 3 PASS. Exit — `verification-plan.md` 의 contract / security / e2e gate 모두 PASS.

## Phase 5 — hardening

목적 — 운영 가능 수준의 코드 품질 정리.

산출.

- lint / static analysis 통과.
- perf 기본선 측정 + 회귀 없음.
- logging / error handling / observability seam 완성.
- 미해결 critical gap 0.

Entry — Phase 4 PASS. Exit — `full_nf_done` 신호. wrapper 가 사용자에게 "merge / release 준비 완료" 보고.

## Team lane 역할

본 wrapper 는 `dev/<nf>/team-execution-plan.md` 를 *source of truth* 로 읽는다. 그 안의 lane H2 가 변경 권한과 책임 범위를 정의한다.

team-execution-plan.md 부재 또는 lane 정의 불완전 시 wrapper 는 conservative default 로 stop 하지 않고 다음 lane 을 가정한다.

| lane | 책임 | 쓰기 권한 |
|---|---|---|
| `orchestrator` | phase 진행 / run state / merge | `dev/<nf>/_implementation_run_state.yaml` |
| `code` | feature implementation | `src/<nf>/`, `generated/<nf>/` |
| `reviewer` | contract / security review | review note (PR comment) |
| `tester` | test + CI | `tests/<nf>/` |
| `verifier` | final evidence / gate | verification log |

NF-specific lane override 가 있으면 team-execution-plan.md 정의 우선.

## Stop / resume / checkpoint

run state 파일 — `dev/<nf>/_implementation_run_state.yaml` (gitignored).

### Schema (`implementation-run-state-v1`)

```yaml
schema_version: implementation-run-state-v1
nf: <nf>
readiness_generated_at: <ISO timestamp from _readiness_status.yaml>
current_phase: <0|1|2|3|4|5>
completed_phases: [<int>, ...]
active_task_ids: [<id>, ...]
completed_task_ids: [<id>, ...]
last_checkpoint_at: <ISO timestamp>
last_command: <string>
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

본 PR (E3) 은 schema 만 정의. PR G 가 NSSF 에서 실제 갱신.

### Resume

`--resume` 시 wrapper 동작.

1. `dev/<nf>/_implementation_run_state.yaml` load.
2. `readiness_generated_at` 이 현재 `_readiness_status.yaml` 의 timestamp 와 일치하는지 확인. 다르면 resume 거부 — `/nf-readiness <nf>` 재평가 안내.
3. `completed_phases` 의 다음 phase 부터 재개. 이미 완료된 phase 는 skip.

### Checkpoint

다음 시점에 run state 를 쓴다.

- 각 phase 시작 / 완료.
- 장시간 실행 (build / test) 직전.
- blocker 발생.
- context / usage risk (long-running run 의 안전 멈춤).

## No spec semantic rediscovery

ADR-0004 security baseline + plan §4 의 *autonomous codegen 의 핵심 invariant* — codegen agent 는 `specs/<spec>/` 의 원본 OpenAPI YAML / docx 를 *semantic decision / data model / API matrix 도출* 목적으로 *다시 읽지 않는다*. readiness pack (`api-implementation-matrix.md`, `data-model-implementation-map.md`, `codegen-work-items.yaml`, `spec-to-design-coverage.md` 등) 이 충분한 입력이어야 한다.

### Allowed reads

- `specs/<spec>/` 의 *source trace* (PR 의 evidence 또는 drift check 용 reference). 의미 새로 해석하는 것이 아니라 *이미 결정된 내용이 어디서 왔는지* 확인.
- generator (예 openapi-generator) 의 input 으로 spec YAML 을 *generator-only mode* 로 통과시키는 경우 — 사람/agent 가 의미 해석 안 함.

### Blocked

- "이 schema 가 어떤 의미인지 specs 에서 다시 봐야겠다" 류의 *semantic rediscovery*.
- API operation 의 비공개 행동을 specs 에서 *새로 발견* 하는 시도.
- ADR-0004 security baseline 외의 security 결정을 33.501 / 33.310 / 33.210 에서 *재발견* 하는 시도 (PR #33 의 정책 — security 는 project baseline 으로 흡수).

### Audit

wrapper 는 implementation 동안 `spec_reread_audit` 를 run state 에 append. 각 entry.

- `accessed_path` — specs/<spec>/<file> 경로.
- `purpose` — `allowed` (source trace / generator input) 또는 `blocked` (semantic rediscovery 시도).
- `justification` — 사람이 audit 가능한 짧은 설명.

`blocked` entry 가 1 개라도 발생하면 wrapper 는 즉시 stop + `/nf-readiness <nf>` reopen 안내. blocked rediscovery 가 필요했다는 것은 readiness pack 이 불완전하다는 증거다.

본 PR (E3) 은 정책 / schema 만 정의. PR G 에서 실제 codegen prompt 에 본 rule 을 inject 하고 evidence 를 남긴다.

## Boundary — PR G 와의 명확한 경계

| 항목 | PR E3 (본 PR) | PR G |
|---|---|---|
| `.claude/skills/nf-implement/SKILL.md` | 신설 | 변경 가능 |
| Phase 0~5 contract 문서화 | ✓ | 갱신 |
| run state schema 문서화 | ✓ (정의) | 실제 작성 |
| no spec rediscovery 정책 / audit schema | ✓ (정책) | agent prompt 삽입 + evidence |
| `src/<nf>/` 생성 | ✗ | ✓ (Phase 1 tracer-bullet) |
| `generated/<nf>/` 생성 | ✗ | ✓ |
| `tests/<nf>/` 생성 | ✗ | ✓ |
| build/CMake/CI 변경 | ✗ | ✓ |
| `dev/<nf>/_implementation_run_state.yaml` 실제 갱신 | ✗ | ✓ |

본 PR (E3) 은 SKILL.md 텍스트 + (선택) gitignore entry 정도만 변경. `src/` 등 구현 산출물 0 line.

## Workflow

### 1. 입력 검증
- `<nf>` 비어있으면 묻고 정지.
- `--phase` / `--resume` / `--dry-run` 외 옵션은 정지.

### 2. Phase 0 — readiness gate

```bash
.venv/bin/python3 design/scripts/nf-readiness-status.py <nf>
```

`gates.readiness_pack_ready.status` 확인.

- PASS → Phase 1 진행.
- FAIL → `blocked_by` + `to_pass` 보고 후 stop. `/nf-readiness <nf>` 재실행 안내.

### 3. Phase 1~5 실행 (또는 dry-run plan 출력)

`--dry-run` 이면 각 phase 의 entry / exit / 산출 contract 만 출력 후 종료.

각 phase 진입 시.

1. run state 에 phase 시작 checkpoint.
2. team-execution-plan.md 의 lane 정의 load.
3. phase contract 의 entry gate 확인.
4. phase 산출 생성 / 검증.
5. exit gate 확인 후 run state 에 phase 완료 checkpoint.

### 4. 최종 보고

- 진행한 phase 목록 + 각 산출 / verification evidence.
- `spec_reread_audit` 요약.
- `dev/<nf>/_implementation_run_state.yaml` 의 final state.
- 다음 행위 — Phase 5 PASS 면 "merge / release 준비 완료", 그 외 phase 에서 멈춘 경우 다음 phase 또는 blocker 해소 안내.

## 자주 틀리는 지점

- wrapper 가 readiness_pack_ready FAIL 상태에서 Phase 1 시작 — 금지. 본 wrapper 의 단일 invariant.
- `specs/<spec>/` 의 OpenAPI 를 semantic rediscovery 목적으로 다시 읽음 — `spec_reread_audit.blocked` 로 즉시 stop.
- team-execution-plan.md 와 무관하게 lane 을 hardcode — NF-specific lane override 가 있으면 그것 우선.
- `--resume` 시 readiness 가 갱신됐는지 확인 안 함 — readiness_generated_at 일치 필수.

## 참고 — 본 skill 안에 다시 적지 말 것

- aggregate gate 정의: `design/scripts/nf-readiness-status.py` docstring + ADR-0002.
- readiness pack 9 파일: `nf-impl-plan/SKILL.md`.
- contract implementability 11-check: `nf-impl-status/SKILL.md`.
- security baseline: `docs/adr/ADR-0004-project-security-baseline.md`.
- plan: `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §5.2 / §7.
