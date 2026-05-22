---
name: nf-spec-discover
description: Canonical lifecycle skill for NF spec discovery, manifest refresh, and seed auto-generation in 5gc-impl-kb. Use when the user asks to discover/register/start an NF from a primary 3GPP spec, refresh NF manifest dependencies, generate the handoff seed, or reset and rediscover contract artifacts with `--reset`; examples include "/nf-spec-discover nssf --primary 29.531", "NSSF spec discovery", "create NF manifest", "generate NSSF seed", "rediscover NRF", or "/nf-spec-discover nssf --primary 29.531 --reset". This is the canonical wrapper name for the existing `/nf-init` workflow.
argument-hint: "<nf> --primary <spec> [--reset]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(.venv/bin/python3 design/scripts/nf-seed-gen.py *) Bash(ls *) Bash(cat *) Bash(mkdir -p *) Bash(git mv *) Bash(mv *) Bash(date *) Bash(find design/* *)
---

# nf-spec-discover — internal subroutine (canonical wrapper for `/nf-init`)

> **2026-05-22 amendment (PR A2).** Starting with the `/nf-readiness` + `/nf-implement` public workflow (see `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md`), this skill is *not* the canonical public entrypoint. It is an internal subroutine of `/nf-readiness <nf>` (PR E). Public callers should invoke `/nf-readiness <nf>` and let it resolve `primary_spec` through `design/nf-registry.yaml`. The `--primary <spec>` argument here is *override / bootstrap / internal* use only — it is not the canonical UX. Direct human invocation remains valid for *new-contract* situations (spec subset change, library/DBMS/security policy change) but is no longer the default user workflow.

This skill is the canonical user-facing wrapper for the existing NF spec discovery workflow.

## Execute

Use the workflow in `../nf-init/SKILL.md` exactly, with these terminology substitutions.

- Report the current user-triggered skill as `/nf-spec-discover`.
- Treat `/nf-init` as the compatibility alias.
- Preserve `--reset` semantics exactly: archive current contract artifacts only, preserve manifest/seed/architecture/dev planning, refresh manifest, auto-generate seed when ready, and do **not** auto-run contract build.
- Continue to call `.venv/bin/python3 design/scripts/nf-manifest.py <nf> --primary <spec> --write` as the manifest tool and `.venv/bin/python3 design/scripts/nf-seed-gen.py <nf>` as the ready-state seed auto-generation tool.

## Boundary

This skill ends after manifest creation/refresh/reset and seed auto-generation reporting. Recommend `/nf-contract-build <nf>` as the next user action only when `ready_for_build=true`.
