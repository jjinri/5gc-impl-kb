# ADR-0001 — {{NF}} architecture baseline

Date: {{generated_date}}
Status: Proposed
Source contract: `{{contract_path}}`

## Context

`{{NF}}` contract validation reached `handoff_ready`, so architecture design can begin.
The architecture must preserve spec-derived API, data model, error handling, and cross-NF constraints while leaving implementation technology choices to the dev stage.

## Decision

Use the architecture document set under `design/{{nf}}/architecture/` as the boundary between contract extraction and implementation planning.
Each architecture file separates spec-derived constraints from implementation choices.

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
