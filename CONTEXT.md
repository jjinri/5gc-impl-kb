# 5gc-impl-kb context

본 repo 는 3GPP spec 을 NF 구현용 **implementation knowledge base** 로 변환한다. 이 문서는 현재 workflow 의 용어집이다.

## Canonical terms

**Implementation Knowledge Base (KB)**
AI agent 가 NF 코드를 작성하고 사람이 설계를 검토하기 위해 읽는 tracked 산출물 집합. NF별 KB 는 주로 `design/<nf>/architecture/**`, `design/<nf>/module-decomposition/**`, `engineering/<nf>/engineering-design.md`, `engineering/<nf>/dependency-decisions.yaml`, `dev/<nf>/` readiness/execution/prep pack 으로 구성된다.

**Source Input**
사람이 외부에서 확보해 `specs/<spec>/` 에 넣는 원본 docx/yaml/pdf. lifecycle 이 새로 시작되는 clean 상태의 필수 입력이다.

**Local Reproducible Artifact**
동일 source input + script/skill 로 재생성 가능한 cache 산출. 예: `design/<nf>/_manifest.yaml`, `design/<nf>/_contract_seed.yaml`, `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml`, `_*_status.yaml`. git source of truth 가 아니다.

**Reviewed Lifecycle Artifact**
skill 이 생성할 수 있지만 PR review/ratify 후 다음 단계의 source of truth 로 쓰이는 tracked 산출. 예: architecture 문서, `engineering-design.md`, `dev/<nf>/` readiness pack.

**Readiness Pack**
`/nf-impl-plan` 이 만드는 `dev/<nf>/` implementation KB. Agent Execution Pack 과 Human Review Pack 을 포함하며 `impl_ready_for_codegen` 의 입력이다.

**Agent Execution Pack**
구현 agent 가 코드 생성을 위해 읽는 readiness pack 부분. 대표 파일: `api-implementation-matrix.md`, `data-model-implementation-map.md`, `codegen-work-items.yaml`, `team-execution-plan.md`, `verification-plan.md`.

**Execution Control Pack**
`readiness_pack_ready PASS` 이후 장기 자율 구현을 lane/write-scope/검증/PR 단위로 제어하는 tracked `dev/<nf>/` 산출물. 대표 파일: `agent-execution-plan.yaml`, `verification-matrix.yaml`, `pr-slicing-plan.yaml`. `/nf-implement` 와 team/runtime agent 가 work item 을 실제 PR slice 로 나눌 때 읽는다.

**Autonomous Implementation Prep Pack**
Phase 1 tracer-bullet 이후 또는 feature codegen 진입 직전에 만드는 tracked 준비 산출물. Dependency ratify, CMake dependency mapping, runtime config schema/example, operator input registry, codegen config/drift allowlist, generated boundary manifest, fixture/golden data, error-cause catalog, migration manifest, failure recovery guide 를 포함한다. 목적은 AI agent 가 추가 질문 없이 권장 기본값으로 실제 코드 작업을 진행하게 하는 것이다.

**Human Review Pack**
사람이 설계 충분성, trace, gap 분류를 검토하기 위해 읽는 readiness pack 부분. 대표 파일: `implementation-readiness-review.md`, `design-adequacy-checklist.md`, `spec-to-design-coverage.md`, `open-gaps-and-assumptions.md`.

**Engineering Design**
spec 에서 자동 도출할 수 없는 library/DB/runtime/tool/operator-policy 결정. `engineering/<nf>/engineering-design.md` 가 source of truth 다.

**Dependency Decisions**
Engineering Design 을 machine-readable 형태로 보강하는 ratified dependency stack. `engineering/<nf>/dependency-decisions.yaml` 에 library 선택, version/pinning, usage boundary, deferral 을 기록한다. `engineering-design.md` 와 모순되면 `engineering-design.md` 의 사람 ratify 결정이 우선이고 yaml 을 보정한다.

**Generated Boundary Manifest**
hand-written boundary stub 과 future openapi-generator 산출의 경계를 추적하는 manifest. 예: `src/<nf>/generated/GENERATION_MANIFEST.yaml`, `infra/<nf>/codegen/drift-allowlist.yaml`. 구현 의미 재발견이 아니라 generator drift/source trace 제어에 쓰인다.

**eng_frozen**
Engineering Design 의 technology decision freeze gate. 최종 implementation GO 가 아니라 `readiness_pack_ready` 의 구성요소다.

**readiness_pack_ready**
`/nf-implement <nf>` 시작 가능성을 뜻하는 aggregate gate. 다음 gate 가 모두 PASS 해야 한다: `handoff_ready`, `contract_implementable`, `arch_consistent`, `impl_ready_for_codegen`, `eng_frozen`.

**No Spec Semantic Rediscovery**
구현 agent 가 원본 spec/OpenAPI 를 의미 보강이나 data model 재해석 목적으로 다시 읽으면 readiness 실패로 본다. 원본은 generator input, drift/source trace 확인에만 허용된다.

**Project Security Baseline**
모든 NF 구현이 따라야 하는 TLS/mTLS/OAuth2 production-capable code path 의무. Source = `docs/adr/ADR-0004-project-security-baseline.md`. 33.501/33.310/33.210 세부는 operator-provided config + maintained library compliance 로 다루며 NF별 lifecycle dependency 로 끌어들이지 않는다.

**Autonomous Implementation Orchestrator**
`/nf-implement <nf>` 가 `claude --agent nf-orchestrator` 로 기동하는 *main-thread* agent. plan 을 읽고 next-slice 를 선택, 4 lane subagent 를 dispatch, PR 을 open / merge, 그리고 context 한계 시 checkpoint 후 `--resume` 으로 재기동된다. harness 제약상 subagent 는 subagent 를 spawn 하지 못하므로 orchestrator 는 main-thread 여야 lane 을 dispatch 할 수 있다. Source = `docs/adr/ADR-0005-autonomous-implementation-policy.md`.

**Lane Subagent**
orchestrator (main-thread) 가 `Agent` tool 로 dispatch 하는 4 NF-agnostic 1-level subagent (`code`, `reviewer`, `tester`, `verifier`). 정의는 `.claude/agents/nf-<lane>.md` (orchestrator agent 정의도 같은 위치에 있으나 lane 이 아니라 main-thread 로 launch). NF-specific lane override 는 `dev/<nf>/team-execution-plan.md` 의 user_sections 본문이 prompt context 로 제공한다. reviewer subagent 는 Edit/Write 권한이 0 이다. lane 실행은 기본 직렬 (code → tester → reviewer/verifier), 병렬은 read-only lane 또는 worktree 격리 시만.

**Slice Picker**
`pr-slicing-plan.yaml` 의 topo sort + `gh pr view` 동적 상태 조회로 next PR slice 를 deterministic 산출하는 단일 진실 출처. 구현 = `design/scripts/nf-implement-slice-status.py`. orchestrator 의 LLM judgment 으로 slice 를 선택하지 않는다.

**Checkpoint/Resume**
main-thread orchestrator 가 context window 한계 시 harness compaction 으로 진행하고, 한계 도달 시 사람이 `/nf-implement <nf> --resume` 으로 재기동하면 state checkpoint (last_checkpoint_at + completed_phases + current_slice) 에서 이어가는 패턴. chain self-respawn (동일 type subagent 재spawn) 은 폐기됐다. resume_count cap = 20, run_epoch = resume 시퀀스 번호.

**Self-Merge**
orchestrator 의 `gh pr merge --squash --delete-branch` 자동. ADR-0005 D1 이 권한을 부여하고 D2 의 4-condition gate (`required_checks` PASS + reviewer approve + `MERGEABLE` + branch up-to-date) 가 검증한다. 사람이 직접 main 에 push 하는 금지는 그대로 유지된다.

**Plan Amendment**
`pr-slicing-plan.yaml` 갱신 PR. orchestrator 가 out-of-scope 발견 시 작성하며 `dev/<nf>/plan-amendments/<date>-<slug>.md` 1-pager (이유 + alternatives + impact) 를 동반한다. self-merge 가능. 단 ADR 신설 / `eng_frozen` 변경 / ADR-0004 영향 중 1 이상이면 즉시 stop + 사람 보고.

## Relationships

- `specs/` 는 source input 이다.
- `/nf-readiness <nf>` 는 source input 을 NF별 KB/readiness pack 으로 변환한다.
- `/nf-implement <nf>` 는 readiness pack + execution-control pack + autonomous-prep pack 을 읽어 실제 source/test/CI 를 만든다.
- `eng_frozen` 은 necessary but not sufficient 이다. 최종 GO 는 `readiness_pack_ready` 다.
- Execution Control Pack 과 Autonomous Implementation Prep Pack 은 `readiness_pack_ready` 이후 자율 구현 속도와 재현성을 높이는 tracked 보강 산출물이다. readiness gate 자체의 대체물이 아니다.
- Human Review Pack 은 자동 gate 의 대상이 아니라 사람이 audit 하는 표면이다. 단, 파일 존재·frontmatter·traceability 같은 구조는 `nf-impl-status.py` 가 검사한다.
- Local reproducible artifact 는 중요하지만 git source of truth 가 아니다. 필요하면 skill/script 로 재생성한다.
- Autonomous Implementation Orchestrator 는 readiness pack PASS 상태에서만 시작한다. orchestrator 는 spec 의미를 다시 읽지 않고 (No Spec Semantic Rediscovery), Plan Amendment 도 reviewer subagent 검사를 거친다.

## Avoid ambiguous wording

| 피할 표현 | 대신 사용할 표현 |
|---|---|
| design output | spec-derived contract / architecture / engineering design 중 하나로 구체화 |
| implementation plan 이 곧 개발 설계 | implementation planning 과 engineering design 을 분리 |
| eng_frozen 이 codegen GO | readiness_pack_ready 가 implementation GO |
| generated file = gitignore | local reproducible artifact 또는 reviewed lifecycle artifact 로 구분 |
| spec 다시 읽어서 구현 | readiness pack 보강 후 구현 |
