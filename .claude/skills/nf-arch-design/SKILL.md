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

1. `handoff/<nf>/contract.yaml`.
2. `design/<nf>/contract/**/*.md` and `design/<nf>/contract/**/*.json`.
3. `design/<nf>/_manifest.yaml` only for NF/spec/profile metadata.

Do not use `dev/<nf>/` as input.
`design/<nf>/module-decomposition/` is a first-class output of this stage, not a legacy artifact. When updating an existing architecture, read it only to preserve user-written sections.

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

design/<nf>/module-decomposition/
└── <Module>.md   # one file per logical module from module-boundaries.md
```

Use the templates in `templates/architecture/` and `templates/module-decomposition/_MODULE.md` as the required section skeleton.
Replace `{{nf}}`, `{{NF}}`, `{{Module}}`, `{{contract_path}}`, and `{{generated_date}}` placeholders.

Every architecture file must carry exactly this canonical section set:

`## Purpose` · `## Inputs (contract)` · `## Boundaries` · `## Decisions` · `## Open Questions` · `## References`

Every `module-decomposition/<Module>.md` file must carry exactly:

`## Responsibility` · `## Inputs` · `## Outputs` · `## State` · `## Decisions` · `## Open Questions` · `## References`

`decisions/ADR-*.md` keeps the ADR record format (`## Context` · `## Decision` · `## Consequences` · `## Open choices` · `## References`), not the architecture section set.

## Architecture rules

- Keep design prose in Korean.
- Preserve 3GPP terms, API names, field names, and schema names in English.
- Within the canonical section set, keep contract-derived facts under `## Inputs (contract)`, `## Boundaries`, and `## Decisions`; put unresolved dev choices under `## Open Questions`.
- Architecture may define logical modules, request flow, state boundaries, error propagation, observability points, and test seams.
- Architecture must not choose OS, programming language, database, HTTP framework, service mesh, deployment substrate, or concrete libraries unless the user explicitly supplies that dev decision.
- When a decision is needed but not contract-derived, write it under `## Open Questions` with status `TBD` or record a decision in `decisions/ADR-*.md`.
- Do not auto-run `/nf-impl-plan`. End by recommending `/nf-impl-plan <nf>` only after the architecture docs are coherent.

## Execution

1. Validate preconditions.
2. Inspect the contract handoff yaml and topic contract files.
3. Create `design/<nf>/architecture/decisions/` and `design/<nf>/module-decomposition/`.
4. Copy the architecture templates into `design/<nf>/architecture/` and one `module-decomposition/_MODULE.md` copy per logical module if the target files do not exist.
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
md = Path('design') / nf / 'module-decomposition'
if not md.is_dir() or not list(md.glob('*.md')):
    raise SystemExit('missing module-decomposition/<Module>.md')
canon = ['## Purpose', '## Inputs (contract)', '## Boundaries',
         '## Decisions', '## Open Questions', '## References']
for p in required:
    if p.startswith('decisions/'):
        continue
    body = (root / p).read_text()
    bad = [h for h in canon if h not in body]
    if bad:
        raise SystemExit(f'{p} missing canonical sections: {bad}')
print('architecture + module-decomposition files present, canonical sections OK')
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
