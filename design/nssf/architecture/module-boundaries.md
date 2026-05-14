---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Module Boundaries

## Spec-derived constraints

- `NSSelectionGet` handler 는 GET query parameter 를 받아 `SliceInfoForRegistration` 를 해석해야 한다.
- `nf-type`, `nf-id`, `tai`, registration slice info 누락 또는 형식 오류는 400 ProblemDetails 로 변환한다.
- 정상 응답 builder 는 `AuthorizedNetworkSliceInfo` schema 를 보존한다.
- policy rejection 은 403 `UNAUTHORIZED_NSSAI` 또는 404 `NSSAI_NOT_AVAILABLE` 로 매핑된다.

## Logical modules

| module | responsibility | input | output | spec source |
| --- | --- | --- | --- | --- |
| NSSelectionGet Handler | OAuth 결과 확인, query parsing, content negotiation | HTTP GET request | validated raw request or ProblemDetails | interface, api/NSSelectionGet |
| Request Validator | 필수 query 와 structured JSON query 형식 검증 | raw query map | Selection command or 400 | api/NSSelectionGet |
| Selection Core | requested NSSAI, subscribed NSSAI, PLMN policy 를 비교해 allowed/rejected result 생성 | Selection command, subscription view, policy view | selection result | data-model/SliceInfoForRegistration |
| External NF Gateway | UDM/NRF 같은 외부 NF 조회 boundary 제공 | lookup request | lookup result or boundary error | cross-NF contract |
| Response Builder | success 와 error 를 contract response shape 로 직렬화 | domain result or error | `AuthorizedNetworkSliceInfo` or ProblemDetails | data-model/AuthorizedNetworkSliceInfo, error-handling |

## Boundary rules

- Handler 는 transport, authentication result, request parsing 을 application boundary 로 넘긴다.
- Core domain module 은 transport framework 와 persistence backend 를 직접 알지 않는다.
- Response builder 는 contract schema 와 status code mapping 을 보존한다.
- External NF client boundary 는 retry, timeout, fallback 정책을 명시적으로 노출한다.

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| package/module layout | TBD | logical modules 를 보존해야 한다. |
| dependency injection style | TBD | Selection Core unit test seam 을 보존해야 한다. |
| generated schema type strategy | TBD | contract data model 과 동기화되어야 한다. |
