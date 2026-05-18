# ADR-0001 — NF lifecycle and vocabulary

Date: 2026-05-13
Status: Accepted for migration planning

## Context

The repository currently exposes the user-facing workflow as `/nf-init`, `/nf-build`, and `/nf-status`, with reset integrated into `/nf-init --reset`. These names were useful while the project only produced spec-derived design pages, but they are too broad for the full NF development lifecycle.

The main ambiguity is `/nf-build`. It does not compile or build implementation code. It extracts and generates a 3GPP-derived NF contract: markdown/json contract artifacts and a machine-readable handoff file for later development steps.

A separate architecture-design step is required between contract extraction and implementation planning.

## Decision

Define the lifecycle in explicit stages and name canonical user-triggered skills by lifecycle responsibility.

| Stage | Compatibility skill | Canonical skill | Meaning |
|---|---|---|---|
| Spec discovery | `/nf-init <nf> --primary <spec>` | `/nf-spec-discover <nf> --primary <spec>` | Identify primary/ref specs, create or refresh the manifest, and auto-generate the handoff seed when ready. |
| Reset + rediscovery | `/nf-init <nf> --primary <spec> --reset` | `/nf-spec-discover <nf> --primary <spec> --reset` | Archive existing contract artifacts only, then refresh the manifest and auto-generate the seed when ready. |
| Contract extraction | `/nf-build <nf>` | `/nf-contract-build <nf>` | Generate spec-derived contract artifacts and `handoff` contract. |
| Contract validation | `/nf-status <nf>` | `/nf-contract-check <nf>` | Check whether the contract is ready for architecture design. |
| Architecture design | none | `/nf-arch-design <nf>` | Convert contract into detailed module/runtime/state/test architecture. |
| Implementation planning | none | `/nf-impl-plan <nf>` | Convert architecture into implementable tasks and test plan. |

`nf-reset` remains deprecated. Reset is an explicit destructive option of the discovery stage, not an independent lifecycle skill.

## User-triggered vs agent-internal work

User-triggered skills are the public workflow surface. The user chooses the lifecycle stage.

Agent-internal work is the script/tool/check sequence performed inside that skill. A skill may run required same-stage tools, such as `build-handoff.py` and `validate-extraction.py` during contract build. A skill should not silently call the next user-facing lifecycle skill; it should report the result and recommend the next user action.

## Consequences

- Documentation should describe `/nf-build` as contract generation, not code build.
- `handoff_ready` means the contract is ready to start architecture design, not that coding can start without architecture work.
- `design/<nf>/contract/` is the current spec-derived contract location. `handoff/<nf>/contract.yaml` is the current machine-readable handoff contract location; legacy `_handoff.yaml` is retired. `design/<nf>/_contract_seed.yaml` is the auto-generated seed that replaces manual legacy handoff authoring, and `design/<nf>/_contract_status.yaml` is the contract-stage completeness report. `design/<nf>/architecture/` and `dev/<nf>/implementation-plan.md` are the current architecture and implementation-planning locations.
- Existing commands remain compatibility aliases until alias removal is explicitly planned.

## Follow-ups

1. Keep reset integrated into `/nf-init --reset` and `/nf-spec-discover --reset`.

## Migration notes

- 2026-05-13: Added canonical wrapper skills for `/nf-spec-discover`, `/nf-contract-build`, and `/nf-contract-check`; existing `/nf-init`, `/nf-build`, and `/nf-status` remain compatibility aliases.
- 2026-05-13: Moved NSSF topic contract markdown/json artifacts under `design/nssf/contract/`; kept `_handoff_seed.yaml`, `_status.yaml`, and `handoff/nssf/_handoff.yaml` names for compatibility.
- 2026-05-14: Added `/nf-arch-design` skill, reusable architecture templates, and initial NSSF architecture draft documents under `design/nssf/architecture/`.
- 2026-05-14: Added `/nf-impl-plan` skill, reusable dev planning templates, and initial NSSF implementation planning artifacts under `dev/nssf/`.
- 2026-05-14: Renamed the canonical machine-readable handoff output to `handoff/<nf>/contract.yaml`.
- 2026-05-14: Removed legacy `handoff/<nf>/_handoff.yaml` fallback in `validate-extraction.py` and `nf-status.py`; `contract.yaml` is now the only handoff input.
- 2026-05-14: `nf-status.py` measures contract-stage completeness only; architecture and implementation-planning stage status are out of scope and would belong to a separate future skill.
- 2026-05-18: Added that separate future skill — `/nf-arch-status` (`design/scripts/nf-arch-status.py` → `design/<nf>/_arch_status.yaml`, gate `arch_consistent`) and `/nf-impl-status` (`design/scripts/nf-impl-status.py` → `dev/<nf>/_impl_status.yaml`, gate `impl_consistent`). Read-only, discover-based, no semantic judge; Phase 1 = exact canonical sections + required files + frontmatter + tasks schema, cross-ref is advisory WARN. Phase 2 (deep traceability id-relation gate) deferred to a separate cycle. Resolves the 2026-05-14 out-of-scope note above.
- 2026-05-14: Renamed `design/<nf>/_handoff_seed.yaml` → `design/<nf>/_contract_seed.yaml` and `design/<nf>/_status.yaml` → `design/<nf>/_contract_status.yaml` to match the contract-stage vocabulary. Scripts, skills, tests, and `.gitignore` were updated; no compatibility aliases.
