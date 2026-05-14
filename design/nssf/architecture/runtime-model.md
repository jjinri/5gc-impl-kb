---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Runtime Model

## Spec-derived constraints

- `NSSelectionGet` 는 HTTP GET 이므로 동일 입력에 대해 side effect 없는 처리 모델을 우선한다.
- OAuth 2.0 client credentials 검증 결과와 `3gpp-Sbi-*` header 전달을 request context 에 보존한다.
- JSON response 와 ProblemDetails response 를 모두 content negotiation boundary 에서 처리한다.
- External NF lookup 은 NSSF response latency 와 failure mapping 에 영향을 준다.

## Runtime boundaries

| boundary | responsibility | concurrency concern | failure mode |
| --- | --- | --- | --- |
| inbound SBI | HTTP/2 stream 처리와 auth context 전달 | per-request isolation | auth failure or malformed query |
| contract validation | query schema 와 required field 검증 | shared state 없음 | 400 ProblemDetails |
| core selection logic | NSSAI intersection 과 policy decision | pure function 형태 권장 | 403 or 404 decision |
| external NF client | UDM/NRF lookup boundary | connection pool and timeout budget | 500 or fallback candidate |

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| process/thread model | TBD | request isolation 과 cancellation 을 보존해야 한다. |
| in-memory cache policy | TBD | stale subscription or NSI data 허용 여부를 명시해야 한다. |
| backpressure mechanism | TBD | 5xx error mapping 과 observability 를 보존해야 한다. |
