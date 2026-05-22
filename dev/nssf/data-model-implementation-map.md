---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-22
---

# NSSF Data Model Implementation Map

## Schema Classification

`handoff/nssf/contract.yaml` 의 모든 data-model schema 1:1 매핑. 누락된 row 가 있으면 `impl_ready_for_codegen` gate 의 `data_model_map_covers_all_contract_models` 가 FAIL. row 33 = contract 의 data-model topic 33 개와 일치.

`kind` 값.

- `generated` — openapi-generator(C) 가 생성. wrapper 불필요. `engineering/nssf/engineering-design.md` `schema_codegen` slot freeze.
- `wrapper` — generated struct 위 사람 친화 wrapper 가 필요한 schema (`wrapper_required: true` in contract JSON). 본 사이클은 외부 ref / anyOf / ProblemDetails 표준화 처리.
- `handwritten` — codegen 부적합 또는 비표준 변환이 필요해 사람이 작성. 본 NSSF 사이클은 없음.
- `deferred` — 본 사이클 범위 밖. `open-gaps-and-assumptions.md` 에 `deferred` gap 으로 표시.

`validation class`.

- `format-only` — OpenAPI format (uuid, regex) 만 검증. generated struct 의 setter 단계.
- `business-rule` — NSSF business rule (snssai range, plmn allow-list 등). engine module 에서 추가 검증.
- `enum` — discriminator/enum 값 set.

| schema | kind | target path | c_type_hint | validation class | used_by (op) | phase/wave | gap id |
|---|---|---|---|---|---|---|---|
| AuthorizedNetworkSliceInfo | generated | `src/nssf/generated/models/nf_authorized_network_slice_info.{h,c}` | `struct nf_authorizednetworksliceinfo*` | business-rule (SelectionEngine emit 후 nfId/Snssai cross-check) | NSSelectionGet (resp 200) | Phase 1 wave A | none |
| SliceInfoForRegistration | generated | `src/nssf/generated/models/nf_slice_info_for_registration.{h,c}` | `struct nf_sliceinfoforregistration*` | business-rule (allowed/requested Snssai disjoint check) | NSSelectionGet (req query/body) | Phase 1 wave A | none |
| SliceInfoForPDUSession | generated | `src/nssf/generated/models/nf_slice_info_for_pdu_session.{h,c}` | `struct nf_sliceinfoforpdusession*` | business-rule (DNN + S-NSSAI match) | NSSelectionGet (req query/body) | Phase 1 wave A | none |
| SliceInfoForUEConfigurationUpdate | generated | `src/nssf/generated/models/nf_slice_info_for_ue_configuration_update.{h,c}` | `struct nf_sliceinfoforueconfigurationupdate*` | business-rule (registered/configured Snssai delta) | NSSelectionGet (req query/body) | Phase 1 wave A | none |
| Snssai | generated | `src/nssf/generated/models/nf_snssai.{h,c}` | `struct nf_snssai*` | format-only (sst 0..255, sd hex string) | NSSelectionGet, NSSAIAvailabilityPut/Patch, Post | Phase 1 wave A (cross-phase) | none |
| PlmnId | generated | `src/nssf/generated/models/nf_plmn_id.{h,c}` | `struct nf_plmnid*` | format-only (mcc/mnc digits) | NSSelectionGet, NSSAIAvailabilityPut/Patch | Phase 1 wave A (cross-phase) | none |
| Tai | generated | `src/nssf/generated/models/nf_tai.{h,c}` | `struct nf_tai*` | format-only (plmnId + tac hex) | NSSelectionGet, NSSAIAvailabilityPut/Patch | Phase 1 wave A (cross-phase) | none |
| NfInstanceId | generated | `src/nssf/generated/models/nf_instance_id.{h,c}` | `char*` (uuid string) | format-only (`format: uuid`) | NSSelectionGet, NSSAIAvailabilityPut/Patch/Delete | Phase 1 wave A (cross-phase) | none |
| SupportedFeatures | generated | `src/nssf/generated/models/nf_supported_features.{h,c}` | `char*` (hex string) | format-only (regex `^[A-Fa-f0-9]*$`) | NSSelectionGet (req/resp), AvailabilityPut/Patch (resp) | Phase 1 wave A (cross-phase) | none |
| NFType | wrapper | `src/nssf/wrappers/nf_type_wrapper.{h,c}` | `void*` (anyOf union) | enum (3GPP NFType set + extensible) | NSSelectionGet (req query) | Phase 1 wave A | G-09 |
| NssaiAvailabilityInfo | generated | `src/nssf/generated/models/nf_nssai_availability_info.{h,c}` | `struct nf_nssaiavailabilityinfo*` | business-rule (Tai × Snssai matrix consistency) | NSSAIAvailabilityPut (req) | Phase 2 wave A | none |
| AuthorizedNssaiAvailabilityInfo | generated | `src/nssf/generated/models/nf_authorized_nssai_availability_info.{h,c}` | `struct nf_authorizednssaiavailabilityinfo*` | business-rule (authorized vs requested Snssai diff) | NSSAIAvailabilityPut/Patch (resp 200) | Phase 2 wave A | none |
| PatchDocument | generated | `src/nssf/generated/models/nf_patch_document.{h,c}` | `struct nf_patchdocument_list*` (RFC 6902 PatchItem array) | business-rule (RFC 6902 op/path/value(/from) per 29.531 §6.1.6 + TS29571 PatchItem) | NSSAIAvailabilityPatch, SubModifyPatch | Phase 2 wave A (Avail Patch) / Phase 3 wave A (SubPatch) | G-04 (confirmed) |
| NssfEventSubscriptionCreateData | generated | `src/nssf/generated/models/nf_nssf_event_subscription_create_data.{h,c}` | `struct nf_nssfeventsubscriptioncreatedata*` | business-rule (event filter, callback URI scheme, expiry) | NSSAIAvailabilityPost (req) | Phase 3 wave A | none |
| NssfEventSubscriptionCreatedData | generated | `src/nssf/generated/models/nf_nssf_event_subscription_created_data.{h,c}` | `struct nf_nssfeventsubscriptioncreateddata*` | business-rule (subscriptionId echo + expiry) | NSSAIAvailabilityPost (resp 201), SubModifyPatch (resp 200) | Phase 3 wave A | none |
| 307 | wrapper | `src/nssf/wrappers/nf_problem_details_wrapper.{h,c}` (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum (status code), business-rule (Location header for redirect) | NSSelectionGet | Phase 1 wave A | none |
| 308 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + Location header | NSSelectionGet | Phase 1 wave A | none |
| 400 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + ProblemDetails fields | NSSelectionGet, NSSAIAvailabilityPut/Patch/Delete/Post/Unsubscribe/SubModifyPatch/Options | Phase 1 wave A (cross-phase) | none |
| 401 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + WWW-Authenticate header | NSSelectionGet, NSSAIAvailabilityPut/Patch/Delete/Post/Unsubscribe/SubModifyPatch/Options | Phase 1 wave A (cross-phase) | none |
| 403 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + cause | NSSelectionGet, NSSAIAvailabilityPut/Patch/Delete/Post/Unsubscribe/SubModifyPatch/Options | Phase 1 wave A (cross-phase) | none |
| 404 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + cause | NSSelectionGet, NSSAIAvailabilityPut/Patch/Delete/Post/Unsubscribe/SubModifyPatch/Options | Phase 1 wave A (cross-phase) | none |
| 405 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + Allow header | NSSAIAvailabilityOptions | Phase 2 wave B | none |
| 406 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + Accept negotiation | NSSelectionGet | Phase 1 wave A | none |
| 411 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + Content-Length required | NSSAIAvailabilityPut/Patch/Post/SubModifyPatch | Phase 2 wave A | none |
| 413 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + payload limit | NSSAIAvailabilityPut/Patch/Post/SubModifyPatch | Phase 2 wave A | none |
| 414 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + URI length | NSSelectionGet | Phase 1 wave A | none |
| 415 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + Content-Type required | NSSAIAvailabilityPut/Patch/Post/SubModifyPatch | Phase 2 wave A | none |
| 429 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + Retry-After header | NSSelectionGet, NSSAIAvailabilityPut/Patch/Delete/Post/Unsubscribe/SubModifyPatch/Options | Phase 1 wave A (cross-phase) | none |
| 500 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + cause | NSSelectionGet, NSSAIAvailabilityPut/Patch/Delete/Post/Unsubscribe/SubModifyPatch/Options | Phase 1 wave A (cross-phase) | none |
| 501 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + cause | NSSAIAvailabilityOptions (rare) | Phase 2 wave B | none |
| 502 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + cause | NSSelectionGet | Phase 1 wave A | none |
| 503 | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + Retry-After header | NSSelectionGet, NSSAIAvailabilityPut/Patch/Delete/Post/Unsubscribe/SubModifyPatch/Options | Phase 1 wave A (cross-phase) | none |
| default | wrapper | (shared ProblemDetails wrapper) | `struct nf_problem_details*` | enum + cause fallback | all ops | Phase 1 wave A (cross-phase) | none |

## Wrapper Rationale

wrapper 가 필요한 row 19 (NFType 1 + 17 ProblemDetails 상태코드 + default 1 = 19 row, 단 ProblemDetails 는 단일 wrapper struct `nf_problem_details_wrapper` 공유) 의 이유. data-model topic 총 33 = generated 14 + wrapper 19.

- `NFType` — `complexity_flags: ['anyOf']`. 3GPP 정의 NFType enum (AMF, SMF, NSSF 등) + extensible string. 사람 친화 API 는 enum-or-string union. `nf_type_wrapper.h` 가 enum probe + string fallback 제공.
- `ProblemDetails` (17 status codes + default) — 모든 wrapper 가 `struct nf_problem_details*` 로 resolve. `c_type_hint` 가 일치하므로 단일 wrapper 모듈에서 status-code 별 factory function (`nf_problem_details_make_400()` 등) 으로 처리. wrapper 책임 — RFC 7807 표준 ProblemDetails + 3GPP cause/instance/type 필드 + status-code 별 mandatory header (Retry-After, Location, WWW-Authenticate, Allow) 자동 부착.

운영자 식별자·외부 ref·security profile 처리.

- 외부 common-data schema (29.571 SUPI/GPSI 등) 는 NSSelectionGet 의 SliceInfoForRegistration 안에 *opaque string* 으로 그대로 통과. wrapper 책임 — generated struct 의 raw 문자열을 그대로 store/forward, 의미 해석은 SelectionEngine 의 business rule 단계.
- security profile (mTLS peer identity / OAuth2 token subject) 는 schema 가 아니라 request-context 의 일부. data-model wrapper 의 책임 외 — `team-execution-plan.md` 의 reviewer/verifier lane 이 처리.

## Open Schemas

- 본 사이클의 모든 33 schema 는 `handoff_ready` 상태로 classification 완료. `deferred` schema 없음.
- classification 의미는 spec 검증을 통과한 assumption 으로 보존. `open-gaps-and-assumptions.md` gap 으로 표면화한 항목.
  - `G-04` (confirmed) — `PatchDocument` 는 `application/json-patch+json` (RFC 6902 PatchItem array) 로 29.531 spec 검증 완료. 본 사이클 구현은 RFC 6902 semantics, RFC 7396 merge-patch payload 진입 시 415 반환.
  - `G-09` — `NFType` anyOf 확장의 forward-compat 정책 (모르는 NFType 값 받았을 때 reject vs passthrough). 본 사이클은 passthrough + log warn.

## References

- `handoff/nssf/contract.yaml` — data-model 진실 출처.
- `design/nssf/contract/data-model/*.json` — `complexity_flags`, `wrapper_required`, `c_type_hint`, `validation_hint` per-schema.
- `design/nssf/architecture/module-boundaries.md` — generated vs wrapper boundary.
- `design/nssf/module-decomposition/SelectionEngine.md`, `AvailabilityEngine.md`, `SubscriptionStore.md` — schema 의 module 책임.
- `engineering/nssf/engineering-design.md` `schema_codegen` slot — openapi-generator(C) + cJSON pinned/vendored freeze.
- `docs/adr/ADR-0004-project-security-baseline.md` — wrapper 가 흡수하지 않는 security 경계.
