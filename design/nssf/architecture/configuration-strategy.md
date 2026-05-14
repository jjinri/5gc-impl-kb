---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/_handoff.yaml
generated_date: 2026-05-14
---

# NSSF Configuration Strategy

## Spec-derived constraints

- API root, apiVersion, TLS/OAuth settings, external NF endpoint discovery 는 runtime configuration 이 필요하다.
- Slice selection policy 는 requested NSSAI, subscribed NSSAI, serving PLMN policy 를 함께 고려해야 한다.
- `supported-features` 는 optional feature negotiation input 으로 보존한다.

## Configuration keys

| key | type | default | source | runtime reload |
| --- | --- | --- | --- | --- |
| `api_root` | URI | none | operator deployment | yes if framework supports it |
| `oauth_token_issuer` | URI/service ref | NRF | interface contract | yes |
| `server_timeout_ms` | integer | TBD | API matrix policy | yes |
| `external_nf_timeout_ms` | integer | TBD | architecture policy | yes |
| `slice_policy_source` | service/file ref | TBD | operator policy | TBD |
| `supported_features` | hex string | empty | API contract | yes |

## Policy boundaries

- Spec 에서 정한 API path, schema, status code 는 configuration 으로 바꾸지 않는다.
- Operator policy 는 slice allow/deny 와 fallback behavior 의 입력일 수 있다.
- Timeout default 는 dev 단계에서 target runtime 과 external NF dependency 를 보고 확정한다.

## Implementation choices

| choice | status | constraint |
| --- | --- | --- |
| config file format | TBD | documented key semantics 를 보존해야 한다. |
| secret source | TBD | OAuth/TLS credential boundary 와 충돌하지 않는다. |
| dynamic reload mechanism | TBD | partial update failure handling 이 필요하다. |
