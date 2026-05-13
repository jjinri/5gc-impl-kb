---
name: nf-spec-discover
description: Canonical lifecycle skill for NF spec discovery and manifest generation in 5gc-impl-kb. Use when the user asks to discover/register/start an NF from a primary 3GPP spec, refresh NF manifest dependencies, or reset and rediscover an NF with `--reset`; examples include "/nf-spec-discover nssf --primary 29.531", "NSSF spec discovery", "create NF manifest", "rediscover NRF", or "/nf-spec-discover nssf --primary 29.531 --reset". This is the canonical wrapper name for the existing `/nf-init` workflow.
argument-hint: "<nf> --primary <spec> [--reset]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(ls *) Bash(cat *) Bash(mkdir -p *) Bash(git mv *) Bash(mv *) Bash(date *) Bash(find design/* *)
---

# nf-spec-discover — canonical wrapper for `/nf-init`

This skill is the canonical user-facing wrapper for the existing NF spec discovery workflow.

## Execute

Use the workflow in `../nf-init/SKILL.md` exactly, with these terminology substitutions.

- Report the current user-triggered skill as `/nf-spec-discover`.
- Treat `/nf-init` as the compatibility alias.
- Preserve `--reset` semantics exactly: archive existing NF artifacts, regenerate manifest, and do **not** auto-run contract build.
- Continue to call `.venv/bin/python3 design/scripts/nf-manifest.py <nf> --primary <spec> --write` as the implementation tool.

## Boundary

This skill ends after manifest creation/refresh/reset reporting. Recommend `/nf-contract-build <nf>` as the next user action only when `ready_for_build=true`.
