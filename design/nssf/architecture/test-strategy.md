---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/_handoff.yaml
generated_date: 2026-05-14
---

# NSSF Test Strategy

## Spec-derived constraints

- API contract, data model JSON, error matrix, and external NF boundaries are the primary test source.
- `handoff_ready` must remain PASS after architecture changes that touch contract assumptions.
- Implementation tests must distinguish client input errors, policy rejection, slice availability miss, and internal failure.

## Test seams

| seam | test type | contract source | expected evidence |
| --- | --- | --- | --- |
| request validation | unit/integration | api/NSSelectionGet | required query and structured JSON cases |
| core decision logic | unit | SliceInfoForRegistration, AuthorizedNetworkSliceInfo | allowed and rejected NSSAI cases |
| error mapping | unit/integration | error-handling | 400, 403, 404, 500 ProblemDetails cases |
| external NF client boundary | contract/mock | cross-NF dependency | timeout, unavailable, invalid response cases |
| observability | integration | interface and error-handling | correlation fields and redaction checks |

## Acceptance matrix

| scenario | input | expected result | source |
| --- | --- | --- | --- |
| valid registration selection | required query plus valid SliceInfoForRegistration | 200 AuthorizedNetworkSliceInfo | api/NSSelectionGet |
| missing required query | omit nf-id or tai | 400 INVALID_QUERY_PARAM | error-handling |
| unauthorized requested NSSAI | requested set outside policy | 403 UNAUTHORIZED_NSSAI | error-handling |
| unavailable slice instance | allowed S-NSSAI but no NSI | 404 NSSAI_NOT_AVAILABLE | error-handling |
| downstream failure | UDM or NRF timeout | 500 SYSTEM_FAILURE or fallback path | error-handling |

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| test framework | TBD | contract fixtures remain readable by agents. |
| mock strategy | TBD | external NF boundary behavior is explicit. |
| CI placement | TBD | contract regression can run before implementation planning completion. |
