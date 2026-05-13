# ADR-0001 — NF lifecycle and vocabulary

Date: 2026-05-13
Status: Accepted for migration planning

## Context

The repository currently exposes the user-facing workflow as `/nf-init`, `/nf-build`, and `/nf-status`, with reset integrated into `/nf-init --reset`. These names were useful while the project only produced spec-derived design pages, but they are too broad for the full NF development lifecycle.

The main ambiguity is `/nf-build`. It does not compile or build implementation code. It extracts and generates a 3GPP-derived NF contract: markdown/json contract artifacts and a machine-readable handoff file for later development steps.

A separate architecture-design step is required between contract extraction and implementation planning.

## Decision

Define the lifecycle in explicit stages and name future user-triggered skills by lifecycle responsibility.

| Stage | User-triggered skill today | Future canonical skill | Meaning |
|---|---|---|---|
| Spec discovery | `/nf-init <nf> --primary <spec>` | `/nf-spec-discover <nf> --primary <spec>` | Identify primary/ref specs and create or refresh the manifest. |
| Reset + rediscovery | `/nf-init <nf> --primary <spec> --reset` | `/nf-spec-discover <nf> --primary <spec> --reset` | Archive existing contract artifacts, then recreate the manifest. |
| Contract extraction | `/nf-build <nf>` | `/nf-contract-build <nf>` | Generate spec-derived contract artifacts and `handoff` contract. |
| Contract validation | `/nf-status <nf>` | `/nf-contract-check <nf>` | Check whether the contract is ready for architecture design. |
| Architecture design | none yet | `/nf-arch-design <nf>` | Convert contract into detailed module/runtime/state/test architecture. |
| Implementation planning | none yet | `/nf-impl-plan <nf>` | Convert architecture into implementable tasks and test plan. |

`nf-reset` remains deprecated. Reset is an explicit destructive option of the discovery stage, not an independent lifecycle skill.

## User-triggered vs agent-internal work

User-triggered skills are the public workflow surface. The user chooses the lifecycle stage.

Agent-internal work is the script/tool/check sequence performed inside that skill. A skill may run required same-stage tools, such as `build-handoff.py` and `validate-extraction.py` during contract build. A skill should not silently call the next user-facing lifecycle skill; it should report the result and recommend the next user action.

## Consequences

- Documentation should describe `/nf-build` as contract generation, not code build.
- `handoff_ready` means the contract is ready to start architecture design, not that coding can start without architecture work.
- `design/<nf>/contract/`, `design/<nf>/architecture/`, `handoff/<nf>/contract.yaml`, and `dev/<nf>/implementation-plan.md` are the target structure for later phases.
- Existing commands remain compatibility aliases until the corresponding canonical skills exist.

## Follow-ups

1. Add canonical skill wrappers for `/nf-contract-build` and `/nf-contract-check`.
2. Move contract artifacts under `design/<nf>/contract/` after tests are prepared.
3. Add `/nf-arch-design` and `/nf-impl-plan` skills with templates.
4. Keep reset integrated into `/nf-init --reset` and future `/nf-spec-discover --reset`.

## Migration notes

- 2026-05-13: Added canonical wrapper skills for `/nf-spec-discover`, `/nf-contract-build`, and `/nf-contract-check`; existing `/nf-init`, `/nf-build`, and `/nf-status` remain compatibility aliases.
