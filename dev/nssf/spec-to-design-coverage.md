---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-22
---

# NSSF Spec → Design Coverage

3GPP spec / OpenAPI topic 별 *contract → architecture → module → dev task → test → engineering decision* trace. autonomous codegen agent 가 원본 spec 을 다시 읽지 않고 구현 판단할 수 있는지 사람이 audit 가능한 view 를 제공.

`spec reread needed?` 컬럼 enum (Pane 2 권고).

- `no` — autonomous codegen agent 가 본 readiness pack 만으로 구현 판단 가능.
- `allowed-trace-only` — spec 원문은 drift detection / source trace / generator input 용도로만 참조 허용. 구현 의미 판단·data model 의미 보강 금지.
- `yes` — 본 readiness pack 만으로 부족, spec 재독해 필요. *자동 blocker* — `open-gaps-and-assumptions.md` 의 `blocker` gap 으로 표면화.

본 표는 8 API op + 핵심 schema 그룹 + cross-cutting 결정 별 trace.

## Coverage Trace

| spec/topic | contract artifact | architecture file | module | dev task id | test ref | eng decision | spec reread needed? |
|---|---|---|---|---|---|---|---|
| 29.531 §6.1 NSSelectionGet | handoff topics[api/NSSelectionGet], design/nssf/contract/api/NSSelectionGet.md | request-flow.md (NSSelectionGet flow), error-propagation.md | SelectionEngine, NotificationDispatcher (n/a for read), ProblemDetailsWrapper | WI-selection-engine, WI-nsselection-handler, WI-problem-details-wrapper | nsselection_integration, nsselection_get_errors, nsselection_response_schema | sbi_server_stack (nghttp2), tls_security, oauth2_token_validation (jwks/libjwt) | no |
| 29.531 §6.2 NSSAIAvailabilityPut | handoff topics[api/NSSAIAvailabilityPut], design/nssf/contract/api/NSSAIAvailabilityPut.md | request-flow.md, state-persistence.md, error-propagation.md | AvailabilityEngine, NotificationDispatcher, AvailabilityRepository | WI-availability-engine, WI-availability-handlers, WI-notification-dispatcher | nssaiavailability_put_integration, availability_engine_replace_unit | persistence (PostgreSQL/libpq), sbi_server_stack | no |
| 29.531 §6.2 NSSAIAvailabilityPatch | handoff topics[api/NSSAIAvailabilityPatch], design/nssf/contract/api/NSSAIAvailabilityPatch.md | request-flow.md, error-propagation.md | AvailabilityEngine, AvailabilityRepository | WI-availability-engine, WI-availability-handlers | nssaiavailability_patch_integration, availability_engine_patch_unit | persistence | no |
| 29.531 §6.2 NSSAIAvailabilityDelete | handoff topics[api/NSSAIAvailabilityDelete], design/nssf/contract/api/NSSAIAvailabilityDelete.md | request-flow.md, state-persistence.md (cascade) | AvailabilityEngine, SubscriptionStore (cascade), AvailabilityRepository | WI-availability-engine, WI-availability-handlers | nssaiavailability_delete_integration | persistence | no |
| 29.531 §6.2 NSSAIAvailabilityOptions | handoff topics[api/NSSAIAvailabilityOptions], design/nssf/contract/api/NSSAIAvailabilityOptions.md | request-flow.md | AvailabilityEngine (options branch) | WI-availability-handlers | nssaiavailability_options_integration, availability_options_unit | sbi_server_stack | no |
| 29.531 §6.3 NSSAIAvailabilityPost (subscriptions) | handoff topics[api/NSSAIAvailabilityPost] | request-flow.md, state-persistence.md, observability.md | SubscriptionStore, NotificationDispatcher | WI-subscription-store, WI-subscription-handlers | subscription_post_integration, subscription_create_unit | persistence, sbi_server_stack, oauth2_token_validation | no |
| 29.531 §6.3 NSSAIAvailabilityUnsubscribe | handoff topics[api/NSSAIAvailabilityUnsubscribe] | request-flow.md, state-persistence.md | SubscriptionStore | WI-subscription-store, WI-subscription-handlers | subscription_delete_integration, subscription_delete_unit | persistence | no |
| 29.531 §6.3 NSSAIAvailabilitySubModifyPatch | handoff topics[api/NSSAIAvailabilitySubModifyPatch] | request-flow.md | SubscriptionStore | WI-subscription-store, WI-subscription-handlers | subscription_patch_integration, subscription_patch_unit | persistence | no |
| 29.531 schemas: NssaiAvailabilityInfo / AuthorizedNssaiAvailabilityInfo | handoff data-model topics + design/nssf/contract/data-model/*.json | module-boundaries.md (generated boundary) | AvailabilityEngine, generated models | WI-codegen-bootstrap, WI-availability-engine | test_generated_codegen_smoke, test_availability_engine_replace | schema_codegen (openapi-generator(C) + cJSON) | allowed-trace-only (generator input only) |
| 29.531 schemas: NssfEventSubscriptionCreate(d)Data | handoff data-model topics + JSON | module-boundaries.md | SubscriptionStore, generated models | WI-codegen-bootstrap, WI-subscription-store | test_generated_codegen_smoke, test_subscription_store_create | schema_codegen | allowed-trace-only |
| 29.531 schemas: SliceInfoForRegistration/PDUSession/UEConfigurationUpdate, AuthorizedNetworkSliceInfo | handoff data-model topics + JSON | module-boundaries.md | SelectionEngine, generated models | WI-codegen-bootstrap, WI-selection-engine | test_generated_codegen_smoke, test_selection_engine_* | schema_codegen | allowed-trace-only |
| 29.531 schemas: Snssai/PlmnId/Tai/NfInstanceId/SupportedFeatures (common) | handoff data-model topics + JSON | module-boundaries.md | (cross-module common) | WI-codegen-bootstrap | test_generated_codegen_smoke | schema_codegen | allowed-trace-only |
| 29.531 schemas: NFType (anyOf) | handoff data-model topics[data-model/NFType] + JSON (complexity_flags=['anyOf']) | module-boundaries.md | NFType wrapper, generated models | WI-nftype-wrapper, WI-codegen-bootstrap | test_nftype_wrapper | schema_codegen | no (G-09 assumption 으로 passthrough 결정) |
| 29.531 schemas: PatchDocument (RFC 6902 array via TS29571 PatchItem) | handoff data-model topics[data-model/PatchDocument] + JSON | module-boundaries.md, error-propagation.md | AvailabilityEngine, SubscriptionStore, generated models | WI-codegen-bootstrap, WI-availability-engine, WI-subscription-store | test_availability_engine_patch, test_subscription_store_patch | schema_codegen | no |
| 29.571 common: ProblemDetails (RFC 7807 + 3GPP extension) | handoff data-model topics 17 status codes + default | error-propagation.md | ProblemDetailsWrapper, generated models | WI-codegen-bootstrap, WI-problem-details-wrapper | test_problem_details_factory, test_problem_details_headers | schema_codegen | allowed-trace-only |
| TLS / mTLS (ADR-0004) | docs/adr/ADR-0004-project-security-baseline.md | runtime-model.md, configuration-strategy.md | TLS context (app_library) | WI-tls-bootstrap | test_tls_context_handshake, test_tls_baseline | tls_security (app_library, openssl) | no |
| inbound OAuth2 (ADR-0004) | docs/adr/ADR-0004 | runtime-model.md | oauth2_jwks (libjwt) | WI-tls-bootstrap | test_oauth2_scope_check, test_jwks_fetch, test_oauth2_inbound | oauth2_token_validation (jwks/libjwt) | no |
| outbound OAuth2 / mTLS (ADR-0004) | docs/adr/ADR-0004 | runtime-model.md, observability.md | NotificationDispatcher | WI-notification-dispatcher | test_notification_dispatcher_outbound, test_oauth2_outbound | sbi_client_stack | no |
| persistence (PostgreSQL/libpq) | architecture decision | state-persistence.md | AvailabilityRepository, SubscriptionRepository, retry_queue | WI-schema-bootstrap, WI-availability-repo, WI-subscription-store | test_schema_bootstrap, test_availability_repo_pg | persistence | no |
| observability (metric/log/trace) | architecture decision | observability.md | metrics, log, trace modules | WI-observability-finalize | test_observability_metrics, test_observability_log, test_observability_trace | (engineering-design pending — G-14 dashboard tuning) | no |

## Gaps

- spec reread needed? = `yes` 행 — 0. autonomous codegen agent 진입 시 spec 재독해 의무 없음. `no_spec_reread_required` check PASS 가능.
- `allowed-trace-only` 행 — 6 (schema generation 그룹). OpenAPI YAML 은 openapi-generator(C) input 으로 *기계 읽기* 만 허용. 구현 의미 판단·data model 의미 보강은 본 readiness pack 의 `data-model-implementation-map.md` + JSON metadata 사용.
- 본 사이클 의문·확정 항목은 `open-gaps-and-assumptions.md` 의 gap id 로 표면화 — G-04 (PatchDocument confirmed RFC 6902), G-07 (redirect target), G-08 (retry tuning), G-09 (NFType), G-10 (subscription expiry), G-11 (amfSetReallocationInd default).

## References

- `handoff/nssf/contract.yaml` — handoff-v2 진실 출처.
- `design/nssf/contract/` — topic 별 contract (api/*, data-model/*, error-handling.md, interface.md).
- `specs/29.531/` — 원본 3GPP (autonomous agent 가 다시 읽지 않는 것이 목표 — `allowed-trace-only` 또는 `no`).
- `engineering/nssf/engineering-design.md` — 13 core slot freeze.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS/OAuth2 baseline.
- `open-gaps-and-assumptions.md` — gap id 의 source.
