---
evidence_id: 2026-05-29-nssf-first-dry-run-evidence
date: 2026-05-29
nf: nssf
related_pr: PR-D — autonomous runner first dry-run
related_adr: docs/adr/ADR-0005-autonomous-implementation-policy.md
phase: meta_autonomous_runner_rollout
---

# NSSF — `/nf-implement nssf --dry-run` evidence (PR-D bootstrap)

본 문서는 PR-D 의 *자율 runner 첫 동작 확인* evidence 다. orchestrator 실제
spawn 없이 entry contract (SKILL.md L1~L70) 의 readiness gate + slice picker
2 component 만 수동 호출해 정합 검증한다. 본 PR 머지 후 `/nf-implement nssf`
호출이 본 evidence 와 같은 readiness PASS + 같은 next slice 결정으로 시작
하리라는 *기대 baseline* 이다.

## 환경

- main HEAD = `eda5390` (PR-C 머지 후, 2026-05-29).
- branch = `feat/nf-implement-pr-d-nssf-first-slice`.
- plan amendment 적용 후 — `dev/nssf/plan-amendments/2026-05-29-nssf-wave1-status-sync.md`
  에 의해 `PR-phase1-wave1-schema-bootstrap.status = merged` 정합.

## 1. Readiness gate (Phase 0)

```bash
.venv/bin/python3 design/scripts/nf-readiness-status.py nssf --no-write
```

핵심 출력 — 5 upstream gate PASS:

```
[nf-readiness-status] nssf: gate readiness_pack_ready: PASS
upstream_snapshots:
  - id: handoff_ready              status: PASS
  - id: contract_implementable     status: PASS
  - id: arch_consistent            status: PASS
  - id: impl_ready_for_codegen     status: PASS
  - id: eng_frozen                 status: PASS
```

→ Phase 0 통과. orchestrator dispatch GO.

## 2. Slice picker (D4)

```bash
.venv/bin/python3 design/scripts/nf-implement-slice-status.py nssf --no-gh
```

picker 결정 — next slice:

```yaml
next_slice:
  id: PR-codegen-nssaiavailability-extension
  title: 'feat(nssf): codegen NSSAIAvailability extension — multi-spec emit'
  phase: phase1_wave1
  work_items:
    - WI-codegen-bootstrap
  scope_files:
    - infra/nssf/codegen/regenerate.sh
    - infra/nssf/codegen/type-overrides.yaml
    - infra/nssf/codegen/drift-allowlist.yaml
    - design/scripts/verify-generated-models.py
    - src/nssf/generated/model/**
    - src/nssf/generated/GENERATION_MANIFEST.yaml
    - tests/nssf/unit/test_generated_smoke.c
  depends_on:
    - PR-phase1-wave1-codegen-bootstrap
  required_checks:
    - phase1_wave1_codegen_drift_check
    - pytest_full_suite
    - validate_artifact_catalog
    - execution_control_drift
  estimated_loc: 1200
recommendation: dispatch next slice PR-codegen-nssaiavailability-extension (phase1_wave1).
```

statuses (관련 entry 발췌):

```yaml
PR-phase1-wave1-codegen-bootstrap: merged       # #92
PR-codegen-nssaiavailability-extension: not_started   # next candidate
PR-phase1-wave1-schema-bootstrap: merged        # #101 — amendment 정합 후
PR-phase1-wave1-tls-bootstrap: not_started
PR-phase1-wave1-problem-details-wrapper: merged # #98
PR-phase1-wave1-nftype-wrapper: merged          # #99
```

## 3. Execution-control drift

```bash
.venv/bin/python3 design/scripts/validate-execution-control-drift.py nssf
```

```
[drift] PASS wi_consistency
[drift] PASS phase_consistency
[drift] PASS check_id_consistency
[drift] PASS pr_slice_consistency
[drift] PASS phase_wi_aggregate
[drift] PASS — execution-control sources consistent.
```

## 4. 첫 자율 호출 시 예상 동작

PR-D 머지 후 사용자가 `/nf-implement nssf` 호출하면:

1. SKILL.md L150 의 워크플로우 step 2 — Phase 0 PASS (본 evidence §1 과 동일).
2. SKILL.md L165 워크플로우 step 6 — `nf-orchestrator` subagent spawn.
3. orchestrator 가 본 evidence §2 의 slice (`PR-codegen-nssaiavailability-extension`)
   를 next 로 결정.
4. orchestrator 가 4 lane subagent (nf-code / nf-tester / nf-reviewer /
   nf-verifier) dispatch → branch 생성 → 구현 → 테스트 → review → 4-condition
   gate 통과 → self-merge.
5. ADR-0005 cost cap (chain_depth 20 / total_slices_completed 50 /
   tier2 attempts ≥ 2) 또는 phase 5 종료까지 chain self-respawn 으로 진행.

3-trigger escape (ADR / `eng_frozen` / ADR-0004 영향) 발생 시 즉시 stop +
사람 보고. 본 evidence 의 next slice 는 codegen 확장 — 3-trigger 가능성
낮음 (engineering decision 변경 없이 generator multi-spec emit 만).

## 5. 본 PR-D 의 종료 기준

- 본 amendment + evidence + plan yaml status 정합 = 사람 review/merge.
- 머지 후 *사람의 1 회* `/nf-implement nssf` 호출이 본 evidence §4 의 예상
  대로 진행되면 *자율 runner 정착 완료*. 이후 ADR-0005 의 cost cap 또는
  phase 5 종료까지 사람 개입 = `--status` 관찰 + 3-trigger escape 시 보고
  대응만.

## 6. 본 evidence 의 lifecycle

본 파일은 *historical evidence* — 머지 후 갱신하지 않는다. 다음 자율 dry-run
또는 manual sync 가 필요하면 새 파일 (`<YYYY-MM-DD>-...-evidence.md`) 생성.
