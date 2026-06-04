---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: '2026-05-26'
generator: design/scripts/nf-readiness-pack-generate.py
source_readiness_config: design/nssf/readiness-config.yaml
generated_sections:
  - gaps-table
user_sections:
  - intro-note
  - summary-body
  - references-body
---

# NSSF Open Gaps and Assumptions

<!-- USER:intro-note:start -->
모든 gap·assumption 을 6 category 중 하나로 분류. `impl_ready_for_codegen` gate 의 `gaps_classified` 는 본 표의 모든 row 가 category 가 있는지 검사하고, `blocker_gaps_zero` 는 `blocker` row 가 0 인지 검사한다.

본 표의 description 은 non-blocker row 모두 owner/phase/trigger/mitigation 4 정보를 포함한다 (Pane 2 권고). `readiness-followup` 의미 항목은 GAP_CATEGORIES enum 6종 한정으로 `deferred` 로 분류하고 description 앞에 `readiness-followup:` prefix 를 둔다.
<!-- USER:intro-note:end -->

## Gaps

<!-- AUTO:gaps-table:start -->
| id | category | description | owner | target_resolution |
|---|---|---|---|---|
| G-01 | operator-provided | mTLS server/client certificate, OAuth2 JWKS endpoint URL, peer NF certificate trust roots. trigger=Phase 1 deploy. mitigation=engineering-design TLS slot + ADR-0004 baseline 이 capability 보장, 인증 자료는 운영자 config 로 주입. | ops | Phase 1 deploy (PR G 단계의 runtime config) |
| G-02 | operator-provided | PostgreSQL connection DSN (host/port/db/user/password 또는 client cert path). trigger=PostgreSQL fixture 실행. mitigation=`store.dsn` config key 가 architecture/state-persistence.md 결정대로 mandatory, 운영자 sealed-secret 또는 env 로 주입. | ops | Phase 1 deploy |
| G-03 | library-assumed | openapi-generator(C) 가 handoff/nssf/contract.yaml + OpenAPI source 로 NSSF contract 14 generated data-model schema + cJSON serializer 를 정확히 emit. trigger=Phase 1 codegen step. mitigation=engineering-design `schema_codegen` slot freeze + smoke test (WI-codegen-bootstrap). | code lane | Phase 1 (PR G) |
| G-04 | assumption | `PatchDocument` 의미 = `application/json-patch+json` (RFC 6902 PatchItem array). trigger=NSSAIAvailabilityPatch / SubModifyPatch 구현. mitigation=api-implementation-matrix.md + codegen-work-items.yaml WI-availability-engine / WI-subscription-store acceptance. | code lane | Phase 2 wave A (Avail Patch) 시점 검증 |
| G-05 | library-assumed | cJSON vendored copy 가 engineering-design `schema_codegen` slot pinned 버전과 일치. trigger=Phase 1 build. mitigation=engineering-design 인용 + CMake 의 cJSON 의존 freeze. | code lane | Phase 1 (PR G) |
| G-06 | operator-provided | libjwt JWKS endpoint URL + JWKS cache TTL. trigger=Phase 1 inbound OAuth2 token validation. mitigation=engineering-design `oauth2_token_validation` slot + ADR-0004 baseline 이 capability 보장, 실제 URL/TTL 은 운영자 config 로 주입. | ops | Phase 1 deploy |
| G-07 | assumption | `NSSelectionGet` 의 307/308 redirect target 결정은 operator-provided redirect table 로부터 read. trigger=NSSelectionGet 의 multi-PLMN/TAI 분기. mitigation=`SelectionEngine` 의 redirect step 은 config-driven. | code lane | Phase 1 (PR G) Open Questions G-OQ-02 |
| G-08 | deferred | readiness-followup: NotificationDispatcher 의 retry policy (exponential backoff jitter, max-attempts, dead-letter table) 의 운영 친화 튜닝. trigger=Phase 4 hardening. mitigation=Phase 1~3 은 동기 dispatch + 한 차례 재시도, Phase 4 에서 정책 튜닝. | code lane | Phase 4 (PR G 후속) |
| G-09 | assumption | `NFType` anyOf (3GPP enum + extensible string) 에서 모르는 NFType 값을 받으면 reject 하지 않고 passthrough + log warn. trigger=NSSelectionGet 의 nfType query param. mitigation=`NFType` wrapper 의 forward-compat 정책. | code lane | Phase 1 (PR G) |
| G-10 | assumption | Subscription default expiry 는 operator 가 `subscription.default_expiry` config 로 결정 (없으면 24h). 29.531 spec 은 expiry mandatory 가 아니라 NSSF 결정. trigger=NSSAIAvailabilityPost. mitigation=engineering-design configuration_strategy 에 default 24h 명시 + override 가능. | code lane | Phase 3 wave A |
| G-11 | assumption | `NssaiAvailabilityInfo.amfSetReallocationInd` flag 의 default 는 false. trigger=NSSAIAvailabilityPut 의 amfSetReallocationInd 누락. mitigation=generated struct default value freeze + business-rule unit test. | code lane | Phase 2 wave A |
| G-12 | library-assumed | PostgreSQL schema migration 은 단일 SQL bootstrap (`infra/nssf/schema.sql`) + 운영자 수동 적용. flyway/liquibase 같은 migration framework 의존 없음. trigger=Phase 1 deploy. mitigation=architecture state-persistence.md 결정 + WI-schema-bootstrap. | ops + code lane | Phase 1 deploy |
| G-13 | test-gap | verification-plan.md 의 Security gate 가 ADR-0004 baseline 7항목을 모두 cover 하지만, mTLS 가 peer NF 인증까지 검증하는 e2e 시나리오는 mocked peer 만 사용. trigger=Phase 4 e2e. mitigation=mocked peer + cert fixture 가 Phase 1~3 충족, 실제 NF interop 은 후속 milestone. | tester + verifier lane | Phase 4 (PR G 후속) |
| G-14 | deferred | readiness-followup: observability dashboard query / alert threshold 의 production tuning. trigger=Phase 5 hardening. mitigation=Phase 1~4 가 signal emit 정확성 보장, 운영 도입 후 threshold 결정. | verifier + ops | Phase 5 (PR G 후속) |
<!-- AUTO:gaps-table:end -->

## Summary

<!-- USER:summary-body:start -->
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

### G-08 addendum (2026-06-04, post-#135 notification-dispatcher eyeball)

본 addendum 는 AUTO gaps-table 의 G-08 row (deferred, source = `design/nssf/readiness-config.yaml` `gaps.rows`) 를 USER 영역에서 보강한다. AUTO row 자체 mitigation 텍스트 enrich 는 readiness regen 이 필요해 본 housekeeping scope (`dev/nssf/** + .gitignore`) 밖이다 — 본 USER addendum 으로 추적한다.

- **이미 G-08 가 커버**. exponential backoff jitter / max-attempts / dead-letter table 의 Phase-4 튜닝. #135 는 Phase 1~3 minimum (동기 dispatch + 한 차례 재시도) 만 구현.
- **신규 (추가)**. fail-closed `NSSF_DISPATCH_FORBIDDEN` (oauth2 token 획득 실패 등) 시 row 를 `requeue NOW()` 로 재무장하므로, caller 가 dispatch 를 즉시 재호출하면 **hot-loop** (busy retry) 위험이 있다. #135 는 이를 *caller-contract* 로 막는다 — `notification_dispatcher.h` 의 `dispatch_pending` doc 에 **"caller MUST NOT tight-loop on FORBIDDEN/retriable status"** 명시 + dispatch 는 call-driven (호출당 최대 1 row, 내부 polling loop 없음).
- **blocker 승격 조건**. Phase-4 resilience slice (`PR-phase4-dispatcher-resilience`, G-08 + F3 fold) 가 dispatcher 에 polling/worker loop 를 도입하면, 위 hot-loop 가 caller-contract 만으로 막히지 않으므로 **G-08 은 그 시점에 deferred → blocker 로 승격** (backoff/dead-letter 가 loop 의 전제조건이 됨). loop 없이 call-driven 유지 시 deferred 유지.

### Reliability 제약 (2026-06-04, sequencing amendment phase3-first)

`PR-phase3-fanout-integration` 가 engine→store→dispatcher publish-seam 을 main.c 에 배선하면 (B3 not-ready path 대체) live subscription 의 outbound 가 활성화된다. 이 시점부터 outbound OAuth2 token 의 transient failure (5xx/timeout/429) 가 **single-shot** 으로 처리된다 (#135 G-08 minimum — 한 차례 재시도 후 stop). 현재는 polling loop 없음 + fail-closed/queue 보존이라 **security blocker 아님** (token 없으면 outbound 금지, row 유실 없음).

- **명문화된 제약**. `PR-phase2-oauth2-resilience` (+ `PR-phase2-config-hardening`) 는 **Phase-4/e2e 진입 전 또는 dispatcher worker/polling loop 도입 전 반드시 close** 해야 한다 (F3/G-08 연결). 그 전까지 fan-out 은 call-driven single-shot 으로 동작한다.
- **plan 강제 (depends_on-ENFORCED, round-2)**. 위 backlog 는 `PR-phase3-fanout-integration` depends_on gate 로 phase3 integration 뒤에만 picker surface 된다. 추가로 **모든 Phase-4/e2e gate slice (`PR-phase4-contract-tests` / `PR-phase4-security-tests` / `PR-phase4-e2e-tests`) 와 worker-loop slice (`PR-phase4-dispatcher-resilience`) 의 depends_on 에 `PR-phase2-oauth2-resilience` + `PR-phase2-config-hardening` 를 hard dependency 로 추가**했다 (round-1 의 note-only → round-2 enforced). topo 검증: oauth2-resilience/config-hardening 가 모든 phase4 slice 앞에 정렬, cycle 없음 (drift PASS). 즉 resilience close 전엔 Phase-4 진입 자체가 picker 상 불가능하다.
<!-- USER:summary-body:end -->

## References

<!-- USER:references-body:start -->
- `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §4.4.
- `design-adequacy-checklist.md`, `spec-to-design-coverage.md` — gap 발생 surface.
- `engineering/nssf/engineering-design.md` — library/operator boundary 결정.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS/OAuth2 capability mandate.
- `design/nssf/architecture/state-persistence.md`, `observability.md`, `error-propagation.md`.
<!-- USER:references-body:end -->
