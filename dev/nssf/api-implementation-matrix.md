---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-22
---

# NSSF API Implementation Matrix

## Operation Matrix

`handoff/nssf/contract.yaml` 의 모든 API operation 1:1 매핑. 누락된 row 가 있으면 `impl_ready_for_codegen` gate 의 `api_matrix_covers_all_operations` 가 FAIL. phase/wave 는 PR G tracer-bullet (Phase 1) 와 후속 wave 를 구분한다.

| operationId | method | path | handler | data-model | security | persistence | unit test | integration test | phase/wave |
|---|---|---|---|---|---|---|---|---|---|
| NSSelectionGet | GET | /network-slice-information | `nssf_nsselection_get_handler` → `SelectionEngine::select()` | `AuthorizedNetworkSliceInfo`(resp), `SliceInfoForRegistration`/`SliceInfoForPDUSession`/`SliceInfoForUEConfigurationUpdate`(req query/body), `Snssai`/`PlmnId`/`Tai`/`NFType`/`NfInstanceId`/`SupportedFeatures`(refs) | TLS (ADR-0004), OAuth2 scope `nnssf-nsselection` (jwks, libjwt) | read-only — `AvailabilityRepository::find_for_plmn_tai()` (libpq SELECT, no write) | `select_engine_unit` — slice-info validation, snssai allow-list, plmn/tai match | `nsselection_integration` — PostgreSQL fixture + HTTP probe, 200 / 403 / 404 / 414 paths | Phase 1 wave A (tracer-bullet) |
| NSSAIAvailabilityPut | PUT | /nssai-availability/{nfId} | `nssf_nssaiavailability_put_handler` → `AvailabilityEngine::replace(nf_id, payload)` | `NssaiAvailabilityInfo`(req), `AuthorizedNssaiAvailabilityInfo`(resp), `NfInstanceId`(path) | TLS (ADR-0004), OAuth2 scope `nnssf-nssaiavailability` | `AvailabilityRepository::upsert()` (libpq UPSERT, single transaction) + retry-queue enqueue for `NotificationDispatcher` | `availability_engine_unit` — full-replace semantics, AMF reallocation flag, snssai dedup | `nssaiavailability_put_integration` — PostgreSQL fixture, idempotent re-put, notification queue row | Phase 2 wave A (writes-trio) |
| NSSAIAvailabilityPatch | PATCH | /nssai-availability/{nfId} | `nssf_nssaiavailability_patch_handler` → `AvailabilityEngine::patch(nf_id, patch_doc)` | `PatchDocument`(req `application/json-patch+json`, RFC 6902 PatchItem array per 29.531 §6.1.6), `AuthorizedNssaiAvailabilityInfo`(resp), `NfInstanceId`(path) | TLS (ADR-0004), OAuth2 scope `nnssf-nssaiavailability` | `AvailabilityRepository::patch()` (libpq UPDATE with conflict check) + retry-queue enqueue | `availability_engine_patch_unit` — RFC 6902 op/path/value(/from) validation, atomic apply, partial replace | `nssaiavailability_patch_integration` — PostgreSQL fixture, patch then read-back, 412 conflict, 415 on non-json-patch+json content-type | Phase 2 wave A (writes-trio) |
| NSSAIAvailabilityDelete | DELETE | /nssai-availability/{nfId} | `nssf_nssaiavailability_delete_handler` → `AvailabilityEngine::delete(nf_id)` | `NfInstanceId`(path) | TLS (ADR-0004), OAuth2 scope `nnssf-nssaiavailability` | `AvailabilityRepository::delete()` (libpq DELETE) + cascade subscription tombstone via `SubscriptionStore::on_availability_deleted()` | `availability_engine_delete_unit` — cascade rule, 404 absent nf | `nssaiavailability_delete_integration` — PostgreSQL fixture, delete then GET 404, subscription cascade | Phase 2 wave A (writes-trio) |
| NSSAIAvailabilityOptions | OPTIONS | /nssai-availability | `nssf_nssaiavailability_options_handler` → `AvailabilityEngine::options()` | none (response is metadata only, no schema) | TLS (ADR-0004), OAuth2 scope `nnssf-nssaiavailability` | none — static method/CORS metadata derived from contract | `availability_options_unit` — supported methods set | `nssaiavailability_options_integration` — HTTP probe, header `Allow:` exact match | Phase 2 wave A (WI-availability-handlers Options sub-step) |
| NSSAIAvailabilityPost | POST | /nssai-availability/subscriptions | `nssf_subscription_post_handler` → `SubscriptionStore::create(req)` | `NssfEventSubscriptionCreateData`(req), `NssfEventSubscriptionCreatedData`(resp) | TLS (ADR-0004), OAuth2 scope `nnssf-nssaiavailability` | `SubscriptionRepository::insert()` (libpq INSERT, JSONB filter column) + initial snapshot dispatch via `NotificationDispatcher` | `subscription_create_unit` — filter validation, expiry default, expiry/callback contract | `subscription_post_integration` — PostgreSQL fixture, 201 Location header, initial notify probe | Phase 3 wave A (subscription) |
| NSSAIAvailabilityUnsubscribe | DELETE | /nssai-availability/subscriptions/{subscriptionId} | `nssf_subscription_unsubscribe_handler` → `SubscriptionStore::delete(sub_id)` | none (path param only) | TLS (ADR-0004), OAuth2 scope `nnssf-nssaiavailability` | `SubscriptionRepository::delete()` (libpq DELETE) | `subscription_delete_unit` — 404 absent sub, idempotent re-delete | `subscription_delete_integration` — PostgreSQL fixture, delete then list excludes | Phase 3 wave A (subscription) |
| NSSAIAvailabilitySubModifyPatch | PATCH | /nssai-availability/subscriptions/{subscriptionId} | `nssf_subscription_patch_handler` → `SubscriptionStore::patch(sub_id, patch_doc)` | `PatchDocument`(req `application/json-patch+json`, RFC 6902 PatchItem array per 29.531 §6.1.6), `NssfEventSubscriptionCreatedData`(resp) | TLS (ADR-0004), OAuth2 scope `nnssf-nssaiavailability` | `SubscriptionRepository::patch()` (libpq UPDATE with JSONB merge) | `subscription_patch_unit` — RFC 6902 op/path/value(/from) validation, filter narrowing, expiry extension | `subscription_patch_integration` — PostgreSQL fixture, patch then read-back, 412 conflict, 415 on non-json-patch+json content-type | Phase 3 wave A (subscription) |

## Coverage

- 본 matrix 가 `contract.api.operations` 의 모든 entry 를 cover — count 8 = handoff/nssf/contract.yaml api topic 8개 일치 (NSSelectionGet + NSSAIAvailability 7).
- 각 operation 의 `security` 열은 `engineering/nssf/engineering-design.md` 의 `sbi_server_stack` (nghttp2 + TLS app_library), `oauth2_token_validation` (jwks, libjwt) freeze + ADR-0004 baseline 7항목을 반영.
- `persistence` 열은 architecture `state-persistence.md` 의 PostgreSQL/libpq 단일 결정 + repository abstraction (`AvailabilityRepository` / `SubscriptionRepository`) 와 일치. `NSSelectionGet` 만 read-only, 나머지 7 op 은 retry-queue 또는 cascade 동작 포함.
- handler 명명은 `nssf_<resource>_<verb>_handler` 형식. 본 plan 의 work item 의 `expected_files` 가 정확한 path/symbol 을 freeze 한다.
- phase/wave — Phase 1=NSSelectionGet (tracer-bullet, PR G), Phase 2 wave A=NSSAIAvailability handlers (Put/Patch/Delete writes-trio + Options sub-step, all in `WI-availability-handlers`), Phase 3=Subscription (Post/Unsubscribe/SubModifyPatch). `team-execution-plan.md` `## Integration Order` 의 wave 정의와 일치.

## Open Questions

- G-OQ-01 (closed) — `NSSAIAvailabilityPatch` 의 PatchDocument 의미는 `application/json-patch+json` (RFC 6902 PatchItem array) 로 확정. specs/29.531/TS29531_Nnssf_NSSAIAvailability.yaml 의 PATCH requestBody content-type + `PatchDocument: type: array items: $ref: TS29571_CommonData.yaml#/components/schemas/PatchItem` 검증 완료. `open-gaps-and-assumptions.md` G-04 가 확정된 assumption 으로 보존 (위반 RFC 7396 payload 진입 시 415 반환).
- G-OQ-02 (linked to `open-gaps-and-assumptions.md` gap `G-07`). `NSSelectionGet` 의 multi-PLMN/multi-Tai query 시 redirect (307/308) 결정 로직이 `SelectionEngine` 의 in-process 결정인지 operator policy 인지 명확화. 본 사이클은 operator-provided redirect target 으로 가정.

## References

- `handoff/nssf/contract.yaml` — operation 진실 출처.
- `design/nssf/contract/api/` — operation 별 topic 문서 (`NSSelectionGet.md`, `NSSAIAvailability*.md`).
- `design/nssf/architecture/request-flow.md`, `design/nssf/architecture/error-propagation.md`, `design/nssf/architecture/state-persistence.md`.
- `design/nssf/module-decomposition/SelectionEngine.md`, `AvailabilityEngine.md`, `SubscriptionStore.md`, `NotificationDispatcher.md`.
- `engineering/nssf/engineering-design.md` — security/persistence/runtime freeze.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS/OAuth2 baseline 7 항목.
