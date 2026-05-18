# ADR-0001 — {{NF}} architecture baseline

Date: {{generated_date}}
Status: Proposed
Source contract: `{{contract_path}}`

## Context

`{{NF}}` contract validation reached `handoff_ready`, so architecture design can begin.
The architecture must preserve spec-derived API, data model, error handling, and cross-NF constraints while leaving implementation technology choices to the dev stage.

## Decision

Use the architecture document set under `design/{{nf}}/architecture/` plus the module decomposition under `design/{{nf}}/module-decomposition/` as the boundary between contract extraction and implementation planning.
Every architecture file uses the canonical section set (`## Purpose`, `## Inputs (contract)`, `## Boundaries`, `## Decisions`, `## Open Questions`, `## References`); contract-derived facts live under Inputs/Boundaries/Decisions and unresolved dev choices live under Open Questions.

## Consequences

- `/nf-impl-plan {{nf}}` must use these architecture documents as input after they are reviewed.
- Implementation planning must not bypass unresolved `TBD` choices that affect task shape or tests.
- Contract changes require revisiting affected architecture files before implementation planning continues.

## Open choices

| choice | status | note |
| --- | --- | --- |
| runtime and language | TBD | dev-stage decision |
| persistence backend | TBD | dev-stage decision |
| framework and deployment | TBD | dev-stage decision |

## References

- `{{contract_path}}` — contract handoff.
- `design/{{nf}}/architecture/` — architecture document set.
- `design/{{nf}}/module-decomposition/` — module decomposition.
- TODO: 인용 spec clause 를 적는다.
