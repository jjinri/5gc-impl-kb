# ADR-0001 — NF lifecycle and vocabulary

Date: 2026-05-13
Status: Accepted; slimmed 2026-05-26 after lifecycle policy extraction

## Context

The repository currently exposes the user-facing workflow as `/nf-init`, `/nf-build`, and `/nf-status`, with reset integrated into `/nf-init --reset`. These names were useful while the project only produced spec-derived design pages, but they are too broad for the full NF development lifecycle.

The main ambiguity is `/nf-build`. It does not compile or build implementation code. It extracts and generates a 3GPP-derived NF contract: markdown/json contract artifacts and a machine-readable handoff file for later development steps.

A separate architecture-design step is required between contract extraction and implementation planning.

## Decision

Normative source: [`design/policies/lifecycle.yaml`](../../design/policies/lifecycle.yaml) — lifecycle stages, gates, canonical skills, work boundary, and semantics. This ADR preserves rationale and migration history only.

## Consequences

- `/nf-contract-build` is contract generation, not implementation code build.
- `handoff_ready` means the contract is ready to start architecture design, not that autonomous code generation can start without further stages (`readiness_pack_ready` is the aggregate GO signal).

## Migration notes

- 2026-05-13: Added canonical wrapper skills for `/nf-spec-discover`, `/nf-contract-build`, and `/nf-contract-check`; existing `/nf-init`, `/nf-build`, and `/nf-status` remained compatibility aliases. *(Aliases removed 2026-05-26 — see below.)*
- 2026-05-13: Moved NSSF topic contract markdown/json artifacts under `design/nssf/contract/`; kept `_handoff_seed.yaml`, `_status.yaml`, and `handoff/nssf/_handoff.yaml` names for compatibility.
- 2026-05-14: Added `/nf-arch-design` skill, reusable architecture templates, and initial NSSF architecture draft documents under `design/nssf/architecture/`.
- 2026-05-14: Added `/nf-impl-plan` skill, reusable dev planning templates, and initial NSSF implementation planning artifacts under `dev/nssf/`.
- 2026-05-14: Renamed the canonical machine-readable handoff output to `handoff/<nf>/contract.yaml`.
- 2026-05-14: Removed legacy `handoff/<nf>/_handoff.yaml` fallback in `validate-extraction.py` and `nf-status.py`; `contract.yaml` is now the only handoff input.
- 2026-05-14: `nf-status.py` measures contract-stage completeness only; architecture and implementation-planning stage status are out of scope and would belong to a separate future skill.
- 2026-05-18: Added that separate future skill — `/nf-arch-status` (`design/scripts/nf-arch-status.py` → `design/<nf>/_arch_status.yaml`, gate `arch_consistent`) and `/nf-impl-status` (`design/scripts/nf-impl-status.py` → `dev/<nf>/_impl_status.yaml`, gate `impl_consistent`). Read-only, discover-based, no semantic judge; Phase 1 = exact canonical sections + required files + frontmatter + tasks schema, cross-ref is advisory WARN. Phase 2 (deep traceability id-relation gate) deferred to a separate cycle. Resolves the 2026-05-14 out-of-scope note above.
- 2026-05-14: Renamed `design/<nf>/_handoff_seed.yaml` → `design/<nf>/_contract_seed.yaml` and `design/<nf>/_status.yaml` → `design/<nf>/_contract_status.yaml` to match the contract-stage vocabulary. Scripts, skills, tests, and `.gitignore` were updated; no compatibility aliases.
- 2026-05-19: Added the Engineering Design Freeze stage between implementation planning and autonomous code generation (ADR-0002). `/nf-eng-design` generates `engineering/<nf>/engineering-design.md`; `/nf-eng-status` (`design/scripts/nf-eng-status.py` → `engineering/<nf>/_engineering_status.yaml`) reports gate `eng_frozen` (deterministic, blocking) and `advisory.impl_plan_alignment` (non-blocking). *(Historical: said "`eng_frozen` is the sole GO signal for autonomous code generation"; superseded by `lifecycle.yaml.gates.readiness_pack_ready` aggregate — `eng_frozen` is one of five required sub-gates.)* Inventory = `design/schemas/engineering-core-slots.yaml` profile ∪ per-NF deferral register; no validator hardcoding.
- 2026-05-26: Slimmed normative content into `design/policies/lifecycle.yaml`. Removed Decision stages table (now `lifecycle.yaml.stages`) and User-triggered vs agent-internal section (now `lifecycle.yaml.work_boundary`). Removed `/nf-init`·`/nf-build`·`/nf-status` compatibility skill surface; reset semantics retained on canonical `/nf-spec-discover --reset` only (resolves the original 2026-05-13 follow-up to keep reset as a `--reset` option, not an independent skill).
