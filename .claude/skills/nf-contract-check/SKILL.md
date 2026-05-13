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
- Interpret `handoff_ready` as “ready to start architecture design,” not “ready to implement without architecture work.”
- Continue to call `.venv/bin/python3 design/scripts/nf-status.py <nf> [--no-write]` as the implementation tool.

## Boundary

This skill measures contract readiness only. It does not modify contract artifacts and does not auto-run `/nf-arch-design`. Recommend `/nf-arch-design <nf>` only when handoff_ready passes.
