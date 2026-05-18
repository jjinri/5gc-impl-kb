---
name: nf-impl-plan
description: Canonical lifecycle skill for converting reviewed NF architecture documents into implementation planning artifacts in 5gc-impl-kb. Use when the user asks to create implementation tasks, test matrix, traceability, or a dev plan from `design/<nf>/architecture/`; examples include "/nf-impl-plan nssf", "NSSF 구현 계획 만들어줘", "architecture를 dev tasks로 쪼개줘", or "create implementation plan from NSSF architecture".
argument-hint: "<nf>"
allowed-tools: Bash(cat *) Bash(ls *) Bash(find design/*) Bash(find handoff/*) Bash(find dev/*) Bash(mkdir -p *) Bash(cp *) Bash(grep *) Bash(sed *) Bash(python3 *) Bash(.venv/bin/python3 design/scripts/nf-status.py *)
---

# nf-impl-plan

This skill is the canonical user-facing implementation planning stage.
It converts reviewed architecture documents into planning artifacts under `dev/<nf>/`.
It does not write implementation source code.

## Inputs

Read these NF-specific inputs.

1. `design/<nf>/architecture/*.md` and `design/<nf>/architecture/decisions/*.md`.
2. `handoff/<nf>/contract.yaml`.
3. `design/<nf>/contract/**/*.md` and `design/<nf>/contract/**/*.json` for source traceability.
4. `design/<nf>/_manifest.yaml` only for NF/spec/profile metadata.

Do not use `dev/<nf>/` as an input source unless updating an existing plan and preserving user-written sections.

## Preconditions

1. Parse `<nf>` from the user request.
2. Verify `design/<nf>/architecture/overview.md` exists.
3. Verify the architecture set contains these files.

```text
overview.md
module-boundaries.md
request-flow.md
runtime-model.md
state-persistence.md
configuration-strategy.md
error-propagation.md
observability.md
test-strategy.md
decisions/ADR-0001-architecture-baseline.md
```

4. Run `.venv/bin/python3 design/scripts/nf-status.py <nf> --no-write` and confirm `handoff_ready` is PASS.
5. If architecture files are missing, stop and recommend `/nf-arch-design <nf>`.
6. If `handoff_ready` is not PASS, stop and recommend `/nf-contract-check <nf>`.

`canonical` does not need to pass for this stage. A missing `implementation_guidance_quality` judge is not an implementation-planning blocker when `handoff_ready` and architecture docs exist.

## Output files

Create or update this planning set.

```text
dev/<nf>/
├── implementation-plan.md
├── tasks.yaml
├── test-matrix.md
└── traceability.md
```

Use the templates in `templates/dev/` as the required section skeleton.
Replace `{{nf}}`, `{{NF}}`, `{{architecture_root}}`, `{{contract_path}}`, and `{{generated_date}}` placeholders.

Each planning file must carry exactly its canonical section set:

- `implementation-plan.md` — `## Scope` · `## Phases` · `## Test Plan` · `## Open Risks` · `## References`
- `test-matrix.md` — `## Purpose` · `## Test Inventory` · `## Coverage Rules` · `## Open Questions` · `## References`
- `traceability.md` — `## Contract → Module` · `## Module → Test` · `## Open Gaps` · `## References`
- `tasks.yaml` — keep the `impl-plan-v1` schema unchanged.

## Planning rules

- Keep prose in Korean.
- Preserve 3GPP terms, API names, field names, module names, and schema names in English.
- Every task must trace to at least one architecture file or contract artifact.
- Every test item must trace to at least one of API contract, error matrix, data model, observability requirement, or architecture decision.
- Plan source code work only as tasks. Do not create source code, package layout, dependency files, or build system files in this skill.
- Mark unresolved technology choices as `TBD` with a decision owner instead of guessing.
- Do not auto-run implementation or verification workflows. End by reporting the implementation plan and remaining decisions.

## Execution

1. Validate preconditions.
2. Inspect architecture, contract, and handoff inputs.
3. Create `dev/<nf>/`.
4. Copy templates if target files do not exist.
5. Populate the plan, task graph, test matrix, and traceability links.
6. Preserve existing user-written sections when updating an existing plan.
7. Run lightweight validation.

Recommended validation.

```bash
git diff --check
python3 - <<'PY'
from pathlib import Path
import yaml
nf = '<nf>'
root = Path('dev') / nf
required = ['implementation-plan.md', 'tasks.yaml', 'test-matrix.md', 'traceability.md']
missing = [p for p in required if not (root / p).exists()]
if missing:
    raise SystemExit(f'missing implementation planning files: {missing}')
tasks = yaml.safe_load((root / 'tasks.yaml').read_text())
for task in tasks.get('tasks', []):
    if not task.get('trace_to'):
        raise SystemExit(f"task has no trace_to: {task.get('id')}")
canon = {
    'implementation-plan.md': ['## Scope', '## Phases', '## Test Plan', '## Open Risks', '## References'],
    'test-matrix.md': ['## Purpose', '## Test Inventory', '## Coverage Rules', '## Open Questions', '## References'],
    'traceability.md': ['## Contract → Module', '## Module → Test', '## Open Gaps', '## References'],
}
for fn, want in canon.items():
    got = [ln.strip() for ln in (root / fn).read_text().splitlines()
           if ln.startswith('## ')]
    if got != want:
        raise SystemExit(f'{fn} canonical sections mismatch: {got} != {want}')
print('implementation planning files present, traceable, canonical sections exact-match OK')
PY
```

## Report

Report these items.

- Architecture root used.
- Created or updated planning files.
- Number of tasks and test cases.
- Technology choices left as TBD.
- Validation commands and results.
- Next recommended action.
