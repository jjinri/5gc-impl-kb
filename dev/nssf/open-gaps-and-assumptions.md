---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-22
---

# NSSF Open Gaps and Assumptions

모든 gap·assumption 을 6 category 중 하나로 분류. `impl_ready_for_codegen` gate 의 `gaps_classified` 는 본 표의 모든 row 가 category 가 있는지 검사하고, `blocker_gaps_zero` 는 `blocker` row 가 0 인지 검사한다.

본 표의 description 은 non-blocker row 모두 owner/phase/trigger/mitigation 4 정보를 포함한다 (Pane 2 권고). `readiness-followup` 의미 항목은 GAP_CATEGORIES enum 6종 한정으로 `deferred` 로 분류하고 description 앞에 `readiness-followup:` prefix 를 둔다.

## Gaps

| id | category | description | owner | target_resolution |
|---|---|---|---|---|
| G-01 | operator-provided | mTLS server/client certificate, OAuth2 JWKS endpoint URL, peer NF certificate trust roots. trigger=Phase 1 deploy. mitigation=engineering-design TLS slot + ADR-0004 baseline 이 capability 보장, 인증 자료는 운영자 config 로 주입. | ops | Phase 1 deploy (PR G 단계의 runtime config) |
| G-02 | operator-provided | PostgreSQL connection DSN (host/port/db/user/password 또는 client cert path). trigger=PostgreSQL fixture 실행. mitigation=`store.dsn` config key 가 architecture/state-persistence.md 결정대로 mandatory, 운영자 sealed-secret 또는 env 로 주입. | ops | Phase 1 deploy |
| G-03 | library-assumed | openapi-generator(C) 가 `handoff/nssf/contract.yaml` + OpenAPI source 로 NSSF contract 14 generated data-model schema (+ TS29571 PatchItem 같은 외부 dependency) 를 정확히 emit + cJSON serializer 생성. 정확한 산출 count 는 build step 에서 validation. trigger=Phase 1 codegen step. mitigation=`engineering/nssf/engineering-design.md` `schema_codegen` slot (openapi-generator(C) + cJSON pinned/vendored) freeze + smoke test (work item `WI-codegen-bootstrap`). | code lane | Phase 1 (PR G) |
| G-04 | assumption | `PatchDocument` 의미 = `application/json-patch+json` (RFC 6902 PatchItem array) confirmed against specs/29.531/TS29531_Nnssf_NSSAIAvailability.yaml — PATCH requestBody 의 content-type 이 `application/json-patch+json`, `PatchDocument: type: array items: $ref: TS29571_CommonData.yaml#/components/schemas/PatchItem`. trigger=NSSAIAvailabilityPatch / SubModifyPatch 구현. mitigation=`api-implementation-matrix.md` NSSAIAvailability Patch row + `codegen-work-items.yaml` WI-availability-engine / WI-subscription-store acceptance 가 RFC 6902 op/path/value(/from) semantics 로 통일. 위반 (RFC 7396 merge-patch payload 진입) 시 415 Unsupported Media Type 반환. | code lane | Phase 2 wave A (Avail Patch) 시점 검증 |
| G-05 | library-assumed | cJSON vendored copy 가 `engineering/nssf/engineering-design.md` `schema_codegen` slot 의 pinned 버전과 일치. trigger=Phase 1 build. mitigation=engineering-design 인용 + CMake 의 cJSON 의존 freeze. | code lane | Phase 1 (PR G) |
| G-06 | operator-provided | libjwt JWKS endpoint URL + JWKS cache TTL. trigger=Phase 1 inbound OAuth2 token validation. mitigation=engineering-design `oauth2_token_validation` slot (jwks, libjwt) + ADR-0004 baseline 가 capability 보장, 실제 URL/TTL 은 운영자 config 로 주입. | ops | Phase 1 deploy |
| G-07 | assumption | `NSSelectionGet` 의 307/308 redirect target 결정은 operator-provided redirect table 로부터 read. trigger=NSSelectionGet 의 multi-PLMN/TAI 분기. mitigation=`SelectionEngine` 의 redirect step 은 config-driven, operator 가 `nssf_redirect_targets` table 을 제공. 본 사이클은 in-process algorithm 으로 redirect 결정하지 않음. | code lane | Phase 1 (PR G) Open Questions G-OQ-02 |
| G-08 | deferred | `readiness-followup:` NotificationDispatcher 의 retry policy (exponential backoff jitter, max-attempts, dead-letter table) 의 운영 친화 튜닝. 본 사이클은 architecture state-persistence.md 의 table-as-queue + FOR UPDATE SKIP LOCKED 기본 동작으로 진행. trigger=Phase 4 (contract/security/e2e) hardening. mitigation=Phase 1~3 은 동기 dispatch + 한 차례 재시도, Phase 4 에서 정책 튜닝 wave. | code lane | Phase 4 (PR G 후속) |
| G-09 | assumption | `NFType` anyOf (3GPP enum + extensible string) 에서 *모르는* NFType 값을 받으면 reject 하지 않고 *passthrough + log warn*. trigger=NSSelectionGet 의 nfType query param. mitigation=`NFType` wrapper 의 forward-compat 정책. 위반 (codegen 이 strict-enum reject) 시 신규 NFType 출시 시 호환성 깨짐. | code lane | Phase 1 (PR G) |
| G-10 | assumption | Subscription default expiry 는 operator 가 `subscription.default_expiry` config 로 결정 (없으면 24h). 29.531 spec 은 expiry mandatory 가 아니라 NSSF 결정. trigger=NSSAIAvailabilityPost. mitigation=engineering-design 의 configuration_strategy 에 default 24h 명시 + override 가능. | code lane | Phase 3 wave A |
| G-11 | assumption | `NssaiAvailabilityInfo.amfSetReallocationInd` flag 의 default 는 false (AMF reallocation 요구하지 않음) per 29.531 §6.1.6. trigger=NSSAIAvailabilityPut 의 amfSetReallocationInd 누락. mitigation=generated struct default value freeze + business-rule unit test. | code lane | Phase 2 wave A |
| G-12 | library-assumed | PostgreSQL schema migration 은 단일 SQL bootstrap (`infra/nssf/schema.sql`) + 운영자 수동 적용. flyway/liquibase 같은 migration framework 의존 없음. trigger=Phase 1 deploy. mitigation=architecture state-persistence.md 결정 + `WI-schema-bootstrap` work item. | ops + code lane | Phase 1 deploy |
| G-13 | test-gap | `verification-plan.md` 의 Security gate 가 ADR-0004 baseline 7항목을 모두 cover 하지만, mTLS 가 *peer NF 인증* 까지 검증하는 e2e 시나리오는 mocked peer 만 사용. 실제 다른 NF (예 AMF) 와의 peer cert chain 검증은 본 사이클 범위 외. trigger=Phase 4 e2e. mitigation=mocked peer + cert fixture 가 Phase 1~3 충족, 실제 NF interop 은 후속 milestone. | tester + verifier lane | Phase 4 (PR G 후속) |
| G-14 | deferred | `readiness-followup:` observability dashboard query / alert threshold 의 production tuning. 본 사이클은 architecture observability.md 의 metric/log/trace seam 만 정의, dashboard 표현·alert 조건은 운영 도입 후 튜닝. trigger=Phase 5 hardening. mitigation=Phase 1~4 가 signal emit 정확성 보장, 운영 도입 후 threshold 결정. | verifier + ops | Phase 5 (PR G 후속) |

## Summary

`/nf-impl-status` 는 본 표를 다음과 같이 집계한다.

```text
blocker: 0      ← blocker_gaps_zero check
deferred: 2
operator-provided: 3
library-assumed: 3
test-gap: 1
assumption: 5
```

총 14 gap. blocker 0 — `impl_ready_for_codegen` 의 `blocker_gaps_zero` 와 `no_spec_reread_required` 가 PASS 가능. 14 gap 모두 6 enum 중 분류 — `gaps_classified` PASS 가능.

## References

- `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §4.4.
- `design-adequacy-checklist.md`, `spec-to-design-coverage.md` — gap 발생 surface.
- `engineering/nssf/engineering-design.md` — library/operator boundary 결정.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS/OAuth2 capability mandate.
- `design/nssf/architecture/state-persistence.md`, `observability.md`, `error-propagation.md`.
