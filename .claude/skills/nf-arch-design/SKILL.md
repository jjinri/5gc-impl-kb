---
name: nf-arch-design
description: Canonical lifecycle skill for converting a handoff-ready NF contract into detailed architecture design in 5gc-impl-kb. Use when the user asks to design NF architecture from a validated contract, create module/runtime/state/test architecture docs, or start the architecture phase after `/nf-contract-check`; examples include "/nf-arch-design nssf", "NSSF architecture 설계", "contract 기반 상세 아키텍처 만들어줘", or "turn this NF contract into architecture docs".
argument-hint: "<nf>"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-status.py *) Bash(cat *) Bash(ls *) Bash(find design/*) Bash(find handoff/*) Bash(mkdir -p *) Bash(cp *) Bash(grep *) Bash(sed *) Bash(python3 *)
---

# nf-arch-design

This skill is the canonical user-facing architecture design stage.
It converts a handoff-ready, spec-derived NF contract into architecture documents under `design/<nf>/architecture/`.

## Inputs

Read only these NF-specific inputs.

1. `handoff/<nf>/contract.yaml` if it exists.
2. Otherwise legacy `handoff/<nf>/_handoff.yaml` while the compatibility fallback remains active.
3. `design/<nf>/contract/**/*.md` and `design/<nf>/contract/**/*.json`.
4. `design/<nf>/_manifest.yaml` only for NF/spec/profile metadata.

Do not use `dev/<nf>/` as input.
Do not copy or depend on legacy `design/<nf>/module-decomposition/` unless a user explicitly asks to migrate that legacy artifact.
That legacy folder is pending a separate compatibility-aware migration.

## Preconditions

1. Parse `<nf>` from the user request.
2. Run `.venv/bin/python3 design/scripts/nf-status.py <nf> --no-write`.
3. Continue only if the `handoff_ready` gate is PASS.
4. If `handoff_ready` is not PASS, stop after reporting blockers and recommend `/nf-contract-build <nf>` or `/nf-contract-check <nf>` as appropriate.

`canonical` does not need to pass for this stage. A missing `implementation_guidance_quality` judge is not an architecture blocker when `handoff_ready` passes.

## Output files

Create or update this architecture set.

```text
design/<nf>/architecture/
├── overview.md
├── module-boundaries.md
├── request-flow.md
├── runtime-model.md
├── state-persistence.md
├── configuration-strategy.md
├── error-propagation.md
├── observability.md
├── test-strategy.md
└── decisions/
    └── ADR-0001-architecture-baseline.md
```

Use the templates in `templates/architecture/` as the required section skeleton.
Replace `{{nf}}`, `{{NF}}`, `{{contract_path}}`, `{{status_path}}`, and `{{generated_date}}` placeholders.

## Architecture rules

- Keep design prose in Korean.
- Preserve 3GPP terms, API names, field names, and schema names in English.
- Separate **Spec-derived constraints** from **Implementation choices** in every document.
- Architecture may define logical modules, request flow, state boundaries, error propagation, observability points, and test seams.
- Architecture must not choose OS, programming language, database, HTTP framework, service mesh, deployment substrate, or concrete libraries unless the user explicitly supplies that dev decision.
- When a decision is needed but not contract-derived, write it under **Implementation choices** with status `TBD` or record a decision in `decisions/ADR-*.md`.
- Do not auto-run `/nf-impl-plan`. End by recommending `/nf-impl-plan <nf>` only after the architecture docs are coherent.

## Execution

1. Validate preconditions.
2. Inspect the contract handoff yaml and topic contract files.
3. Create `design/<nf>/architecture/decisions/`.
4. Copy the template files into `design/<nf>/architecture/` if the target files do not exist.
5. Populate sections with contract-derived facts and explicit `TBD` implementation choices.
6. Preserve existing user-written sections when updating an existing architecture document.
7. Run lightweight validation.

Recommended validation.

```bash
git diff --check
python3 - <<'PY'
from pathlib import Path
nf = '<nf>'
root = Path('design') / nf / 'architecture'
required = [
    'overview.md',
    'module-boundaries.md',
    'request-flow.md',
    'runtime-model.md',
    'state-persistence.md',
    'configuration-strategy.md',
    'error-propagation.md',
    'observability.md',
    'test-strategy.md',
    'decisions/ADR-0001-architecture-baseline.md',
]
missing = [p for p in required if not (root / p).exists()]
if missing:
    raise SystemExit(f'missing architecture files: {missing}')
print('architecture files present')
PY
```

## Report

Report these items.

- Source contract path used.
- Created or updated architecture files.
- Contract-derived constraints captured.
- Implementation choices left as TBD.
- Validation commands and results.
- Next recommended user-triggered skill.
