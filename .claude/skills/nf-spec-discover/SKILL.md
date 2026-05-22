---
name: nf-spec-discover
description: Internal lifecycle subroutine / compatibility wrapper for NF spec discovery, manifest refresh, and seed auto-generation in 5gc-impl-kb. Public callers should use `/nf-readiness <nf>` (resolves `primary_spec` via `design/nf-registry.yaml`); direct invocation here is for override / bootstrap / new-contract situations only. Use when the user explicitly asks to override registry resolution, bootstrap a never-discovered NF, refresh NF manifest dependencies, generate the handoff seed, or reset and rediscover contract artifacts with `--reset`; override examples include "/nf-spec-discover nssf --primary 29.531 --reset" or "rediscover NRF with a different primary spec". Compatibility alias for the existing `/nf-init` workflow.
argument-hint: "<nf> --primary <spec> [--reset]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(.venv/bin/python3 design/scripts/nf-seed-gen.py *) Bash(ls *) Bash(cat *) Bash(mkdir -p *) Bash(git mv *) Bash(mv *) Bash(date *) Bash(find design/* *)
---

# nf-spec-discover — internal subroutine (canonical wrapper for `/nf-init`)

> **2026-05-22 amendment (PR A2).** Starting with the `/nf-readiness` + `/nf-implement` public workflow (see `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md`), this skill is *not* the canonical public entrypoint. It is an internal subroutine of `/nf-readiness <nf>` (PR E). Public callers should invoke `/nf-readiness <nf>` and let it resolve `primary_spec` through `design/nf-registry.yaml`. The `--primary <spec>` argument here is *override / bootstrap / internal* use only — it is not the canonical UX. Direct human invocation remains valid for *new-contract* situations (spec subset change, library/DBMS/security policy change) but is no longer the default user workflow.

This skill is an *internal lifecycle subroutine / compatibility wrapper* for the existing NF spec discovery workflow. Public users invoke `/nf-readiness <nf>` instead; that wrapper resolves `primary_spec` from `design/nf-registry.yaml` and calls this skill internally. Direct invocation here is only for *override / bootstrap / new-contract* situations (registry missing or low-confidence, spec subset change, library/DBMS/security policy change).

## Execute

Use the workflow in `../nf-init/SKILL.md` exactly, with these terminology substitutions.

- Report the current user-triggered skill as `/nf-spec-discover`.
- Treat `/nf-init` as the compatibility alias.
- Preserve `--reset` semantics exactly: archive current contract artifacts only, preserve manifest/seed/architecture/dev planning, refresh manifest, auto-generate seed when ready, and do **not** auto-run contract build.
- Continue to call `.venv/bin/python3 design/scripts/nf-manifest.py <nf> --primary <spec> --write` as the manifest tool and `.venv/bin/python3 design/scripts/nf-seed-gen.py <nf>` as the ready-state seed auto-generation tool.

## Boundary

This skill ends after manifest creation/refresh/reset and seed auto-generation reporting. Recommend `/nf-contract-build <nf>` as the next user action only when `ready_for_build=true`.
