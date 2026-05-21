---
name: nf-contract-check
description: Canonical lifecycle skill for checking whether a generated NF contract is ready for architecture design in 5gc-impl-kb. Use when the user asks to validate/check/score/status an NF contract, inspect handoff readiness, or list blocking extraction issues; examples include "/nf-contract-check nssf", "NSSF contract 검증", "handoff_ready 확인", "is this NF contract ready for architecture design?". This is the canonical wrapper name for the existing `/nf-status` workflow.
argument-hint: "<nf> [--no-write]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-status.py *) Bash(cat *) Bash(ls *)
---

# nf-contract-check — canonical wrapper for `/nf-status`

This skill is the canonical user-facing wrapper for contract validation.

## Execute

Use the workflow in `../nf-status/SKILL.md` exactly, with these terminology substitutions.

- Report the current user-triggered skill as `/nf-contract-check`.
- Treat `/nf-status` as the compatibility alias.
- Interpret `handoff_ready` as "ready to start architecture design," not "ready to implement without architecture work."
- Interpret `contract_implementable` (PR B, 2026-05-21) as "ready for codegen without re-reading original OpenAPI YAML for semantic discovery."
- Continue to call `.venv/bin/python3 design/scripts/nf-status.py <nf> [--no-write]` as the implementation tool.

## `contract_implementable` gate (PR B, 2026-05-21)

Aggregate of seven implementability checks defined in `nf-status.py`:

| Check | Criterion |
|---|---|
| `api_operation_complete` | Each `design/<nf>/contract/api/<Operation>.md` has method/path/security/full response matrix/error matrix/operation-specific validation rule/spec source line. |
| `data_model_field_tables_complete` | Each `design/<nf>/contract/data-model/<Schema>.md` has a complete field table (name/required/nullable/type/enum/cardinality/default) — no `TBD` / empty rows. |
| `external_refs_resolved_or_classified` | Each `data-model/<Schema>.json` has zero `unresolved_refs`, OR unresolved refs are explicitly classified as `external` / `operator-provided` / `deferred` with rationale. |
| `schema_complexity_classified` | Each `data-model/<Schema>.json` has `complexity_flags` populated (may be empty list if schema is simple). |
| `generated_wrapper_boundary_declared` | Each `data-model/<Schema>.json` has `wrapper_required` boolean explicitly set. Each `data-model/<Schema>.md` has `generated-vs-wrapper` recommendation. |
| `problem_details_matrix_complete` | `error-handling` topic enumerates 8 operation × cause matrix (not just status code list) — handoff yaml `error_handling.operations[<op>].causes` populated. |
| `no_spec_reread_required_for_implementation` | Aggregate sentinel — `unresolved_refs` zero across all data-model JSONs + all api operations have full response/error matrix + no `TODO`/`TBD` in AUTO sections of contract topics. |

PASS criteria — all seven checks PASS (or NOT_APPLICABLE per profile). Each check has `to_pass` actionable item pointing back to `/nf-contract-build` or to `/nf-impl-plan` Human Review Pack for gap classification.

This gate is *additive* to existing `handoff_ready` — both should PASS before architecture design (`handoff_ready`) and before implementation planning (`contract_implementable`).

## Boundary

This skill measures contract readiness only. It does not modify contract artifacts and does not auto-run `/nf-arch-design`. Recommend `/nf-arch-design <nf>` only when `handoff_ready` PASSes; recommend `/nf-impl-plan <nf>` only when `contract_implementable` PASSes.
