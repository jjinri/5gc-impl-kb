---
id: interface
status: handoff_ready
generated_sections:
  - auth-block
  - transport-block
user_sections:
  - implementation-notes
---

# Interface

본 토픽 — NSSF (Nnssf) 의 API URI, transport, 인증, 헤더 규약. spec_refs — TS 29.531 §6.1.1, §6.1.5.

## Auth

<!-- AUTO:auth-block:start -->
| field | value |
| --- | --- |
| auth_scheme | OAuth 2.0 client credentials (RFC 6749) |
| token_issuer | NRF |
| scope_naming | `<service-name>` (e.g. `nnssf-nsselection`) |
| spec | TS 33.501 §13 (5G security) + TS 29.500 (SBI) |
<!-- AUTO:auth-block:end -->

## Transport

<!-- AUTO:transport-block:start -->
| field | value |
| --- | --- |
| transport | HTTP/2 over TLS (RFC 9113) |
| base_url | `{apiRoot}/nnssf-nsselection/<apiVersion>` |
| apiVersion | `v1` (spec TS 29.531 v18.x — yaml info.version) |
| content_type | `application/json` (request, response) + `application/problem+json` (error) |
| headers | `3gpp-Sbi-*` family per TS 29.500 §5.2.3 |
<!-- AUTO:transport-block:end -->

## Implementation Notes

<!-- USER:implementation-notes:start -->
- TLS termination 위치 — service mesh (envoy) 또는 NF 내장 둘 다 허용 (`may_decide`). 단 `3gpp-Sbi-*` 헤더 전달은 보장.
- 본 NSSF MVP 는 NSSelectionGet 1 API 만. 추가 service (NSSAIAvailability 등) 는 후속 사이클.
- OAuth token caching — NRF 호출 횟수 줄이려면 expiry-aware cache 권장. cache 라이브러리는 `may_decide`.
<!-- USER:implementation-notes:end -->
