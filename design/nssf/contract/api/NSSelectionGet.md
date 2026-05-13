---
id: api/NSSelectionGet
status: handoff_ready
generated_sections:
  - api-matrix
  - request-schema
  - response-schema
user_sections:
  - implementation-notes
---

# api/NSSelectionGet

본 토픽 — NSSF 의 NSSelectionGet operation. spec_refs — TS 29.531 §5.2.2.2.1.

## API Matrix

<!-- AUTO:api-matrix:start -->
| field | value |
| --- | --- |
| operationId | NSSelectionGet |
| method | GET |
| path | `/network-slice-information` |
| idempotent | true (HTTP GET semantics) |
| timeout | client default 5s, server target ≤ 200ms p99 (정책 — agent 가 환경에 맞춰 결정 가능) |
| scope | nnssf-nsselection |
| auth | OAuth 2.0 (interface 참조) |
| produces | application/json (200), application/problem+json (4xx/5xx) |
<!-- AUTO:api-matrix:end -->

## Request

<!-- AUTO:request-schema:start -->
Query parameters —

| name | required | type | 의미 |
| --- | --- | --- | --- |
| nf-type | true | NfType (string enum) | 호출자 NF — 보통 AMF |
| nf-id | true | NfInstanceId (UUID) | 호출 NF 의 instance ID |
| slice-info-request-for-registration | conditional | **data-model/SliceInfoForRegistration** | registration 요청 시 (mutually exclusive with -for-pdu-session) |
| slice-info-request-for-pdu-session | conditional | SliceInfoForPduSession | PDU session 요청 시 (본 MVP 범위 밖) |
| home-plmn-id | false | PlmnId | roaming 시 |
| tai | true | Tai | 단말 위치 (cell-level) |
| supported-features | false | hex string | feature negotiation |

본 MVP — `slice-info-request-for-registration` 경로만 구현 ("for-pdu-session" 은 후속 사이클).
<!-- AUTO:request-schema:end -->

## Response

<!-- AUTO:response-schema:start -->
| code | content | schema | 비고 |
| --- | --- | --- | --- |
| 200 | application/json | **data-model/AuthorizedNetworkSliceInfo** | 정상 응답 |
| 400 | application/problem+json | ProblemDetails | error-handling#nsselection-400 |
| 403 | application/problem+json | ProblemDetails | error-handling#nsselection-403 |
| 404 | application/problem+json | ProblemDetails | error-handling#nsselection-404 |
| 500 | application/problem+json | ProblemDetails | recovery 정책은 error-handling 참조 |
<!-- AUTO:response-schema:end -->

## Implementation Notes

<!-- USER:implementation-notes:start -->
- handler 는 stateless — 동일 입력에 동일 출력. session affinity 불필요.
- 인증 검증 → query 파싱 → SelectionEngine 위임 → ProblemDetails or AuthorizedNetworkSliceInfo 직렬화. 자세한 흐름은 module-decomposition/SelectionEngine 참조.
- `slice-info-request-for-registration` 의 JSON 직렬화 query — yaml `parameters.content.application/json.schema` 로 정의됐다 (3GPP 의 "structured query in JSON" 패턴). url-encoded 직렬화 처리 — JSON 문자열로 url-encode 한 뒤 query value 에 넣는다.
- request 의 query 직렬화 검증 (필수 nf-type/nf-id/tai 우선, 그 다음 SliceInfoForRegistration 의 mappingOfNssai 등) 은 client 책임이지만 서버는 위반 시 400 응답.
<!-- USER:implementation-notes:end -->
