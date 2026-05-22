---
name: nf-contract-build
description: Canonical lifecycle skill for generating a 3GPP-derived NF contract in 5gc-impl-kb. Use when the user asks to build/generate/extract/update an NF contract from a ready manifest and handoff seed, including topic/category rebuilds; examples include "/nf-contract-build nssf", "NSSF contract 생성", "generate NSSF contract", "data-model contract만 갱신", or "/nf-contract-build nssf --topic data-model/SliceInfoForRegistration". This is the canonical wrapper name for the existing `/nf-build` workflow and is not an implementation code build.
argument-hint: "<nf> [--<category>] [--topic <topic-id>]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/extract.py *) Bash(.venv/bin/python3 design/scripts/spec-split.py *) Bash(.venv/bin/python3 design/scripts/resolve-yaml-refs.py *) Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(.venv/bin/python3 design/scripts/materialize-contract.py *) Bash(.venv/bin/python3 design/scripts/build-handoff.py *) Bash(.venv/bin/python3 design/scripts/validate-extraction.py *) Bash(mkdir -p *) Bash(ls *) Bash(grep *) Bash(find *)
---

# nf-contract-build — canonical wrapper for `/nf-build`

This skill is the canonical user-facing wrapper for contract extraction/generation.

## Execute

Use the workflow in `../nf-build/SKILL.md` exactly, with these terminology substitutions.

- Report the current user-triggered skill as `/nf-contract-build`.
- Treat `/nf-build` as the compatibility alias.
- Describe the output as **spec-derived NF contract artifacts**, not code build artifacts.
- Generate topic contract artifacts under `design/<nf>/contract/` and the current handoff-v2 yaml at `handoff/<nf>/contract.yaml`.
- Continue to run same-stage validation via `build-handoff.py` and `validate-extraction.py --level basic`.

## Implementability mandate (2026-05-21, PR B)

The generated contract must be *implementation-ready* — codegen agents must not need to re-read original OpenAPI YAML for semantic discovery. Allowed uses of original YAML = `drift check` / `regeneration` / `source-line trace`. Disallowed = implementation judgment / data-model meaning re-derivation / API operation matrix reconstruction. This mandate applies starting with PR B and is enforced by `/nf-contract-check` `contract_implementable` gate (see `nf-contract-check/SKILL.md`).

### API topic mandate

Each `design/<nf>/contract/api/<Operation>.md` must include AUTO-generated sections that fully express:

- method / path (server base URL from `interface` topic)
- query / header / body params with required / optional / default / nullable / enum / format
- security requirement (which scheme / scope / inbound capability needed)
- response status codes × body schema × content-type (full matrix)
- error / ProblemDetails matrix (operation-specific cause enumeration)
- operation-specific validation rules (e.g. JSON Patch op set, structured query format)
- trace source — spec section reference + source line range (anchor / `§6.x.y` / docx clause)

The existing AUTO markers (`api-matrix` / `request-schema` / `response-schema`) cover the structural skeleton. Implementability mandate extends the *completeness bar* — each field/response row must have spec source + implementability-ready detail (not just placeholder).

### Data-model topic mandate

Each `design/<nf>/contract/data-model/<Schema>.md` must include AUTO-generated sections that fully express:

- field table — name / required / nullable / type / enum / array or map cardinality / default
- external `$ref` resolution target (which `<spec>/<file>#<symbol>` was followed)
- `oneOf` / `allOf` / `anyOf` / discriminator handling
- C type mapping hint (e.g. `char*` / `int32_t` / `struct nssf_snssai*` / `enum nf_type`)
- serializer / deserializer hint (cJSON pattern: `cJSON_AddItemToObject` / `cJSON_GetObjectItem`)
- validator hint (which validation rule to apply at runtime)
- generated-vs-wrapper recommendation (`generated` if `openapi-generator` C produces usable code; `wrapper` if pattern needs hand-written wrapper around generated; `handwritten` if generator fails)

### Machine JSON extension (`<topic>.json`)

`resolve-yaml-refs.py --emit-json` output extends with codegen-friendly metadata at the schema level:

- `normalized_schema` — flattened resolved schema (no remaining `$ref` to external).
- `complexity_flags` — `["oneOf", "allOf", "anyOf", "discriminator", "external_ref", "circular"]` subset present.
- `c_type_hint` — recommended C type at top level (driven by openapi-generator C heuristics).
- `validation_hint` — pattern (e.g. `"length: 1-65535"`, `"regex: ^[0-9]+$"`, `"enum"`, `"jsonpatch"`).
- `wrapper_required` — bool. `true` if `complexity_flags` contains `oneOf` / `allOf` / `anyOf` / `discriminator`, OR if external `$ref` unresolvable.

These extensions are required for downstream `/nf-impl-plan` to produce `data-model-implementation-map.md` without re-reading original spec.

## Boundary

This skill does not auto-run `/nf-contract-check`. After generation and same-stage validation, recommend `/nf-contract-check <nf>` as the next user action.
