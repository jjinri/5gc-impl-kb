---
name: nf-contract-build
description: Canonical lifecycle skill for generating a 3GPP-derived NF contract in 5gc-impl-kb. Use when the user asks to build/generate/extract/update an NF contract from a ready manifest and handoff seed, including topic/category rebuilds; examples include "/nf-contract-build nssf", "NSSF contract 생성", "generate NSSF contract", "data-model contract만 갱신", or "/nf-contract-build nssf --topic data-model/SliceInfoForRegistration". This is the canonical wrapper name for the existing `/nf-build` workflow and is not an implementation code build.
argument-hint: "<nf> [--<category>] [--topic <topic-id>]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/extract.py *) Bash(.venv/bin/python3 design/scripts/spec-split.py *) Bash(.venv/bin/python3 design/scripts/resolve-yaml-refs.py *) Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(.venv/bin/python3 design/scripts/build-handoff.py *) Bash(.venv/bin/python3 design/scripts/validate-extraction.py *) Bash(mkdir -p *) Bash(ls *) Bash(grep *) Bash(find *)
---

# nf-contract-build — canonical wrapper for `/nf-build`

This skill is the canonical user-facing wrapper for contract extraction/generation.

## Execute

Use the workflow in `../nf-build/SKILL.md` exactly, with these terminology substitutions.

- Report the current user-triggered skill as `/nf-contract-build`.
- Treat `/nf-build` as the compatibility alias.
- Describe the output as **spec-derived NF contract artifacts**, not code build artifacts.
- Continue to generate current handoff-v2 artifacts in their existing paths until the path-migration phase starts.
- Continue to run same-stage validation via `build-handoff.py` and `validate-extraction.py --level basic`.

## Boundary

This skill does not auto-run `/nf-contract-check`. After generation and same-stage validation, recommend `/nf-contract-check <nf>` as the next user action.
