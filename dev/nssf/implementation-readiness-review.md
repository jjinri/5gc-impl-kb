---
nf: nssf
stage: implementation-planning
status: draft
source_architecture: design/nssf/architecture
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-22
---

# NSSF Implementation Readiness Review

사람이 *전체 설계 적정성* 을 빠르게 audit 하기 위한 executive review. `/nf-impl-status` 는 본 문서의 *내용 품질* 을 자동 승인하지 않는다 — 존재·구조·traceability 만 검사한다. 본 review 의 GO/NO-GO 판정은 사람 책임.

## Executive Summary

- NSSF 는 3GPP 29.531 Nnssf_NSSelection (slice selection) + Nnssf_NSSAIAvailability (availability tracking, subscription) 두 service 를 제공. 본 implementation 은 8 API operation + 33 data-model schema 의 autonomous code generation 으로 시작한다.
- 핵심 모듈 4종 — SelectionEngine (NSSelectionGet), AvailabilityEngine (PUT/PATCH/DELETE/OPTIONS), SubscriptionStore (POST/DELETE/PATCH subscriptions), NotificationDispatcher (outbound retry queue).
- runtime — nghttp2 server + h2 (TLS via app_library, openssl backend) + h2c fallback, PostgreSQL/libpq 단일 persistence backend, libjwt JWKS inbound OAuth2, openapi-generator(C) + cJSON pinned/vendored schema codegen.
- 완성 시 — UE 등록 시 AMF 가 NSSF 로 slice 선택 요청, NSSF 가 availability snapshot 기반 AuthorizedNetworkSliceInfo 응답. AMF 가 availability data 변경 시 subscription notify.

## Scope Confirmation

- API operation 8개 (포함) — `api-implementation-matrix.md` ## Operation Matrix 참조.
  - Phase 1 wave A — NSSelectionGet (tracer-bullet).
  - Phase 2 wave A — NSSAIAvailability Put / Patch / Delete (writes-trio).
  - Phase 2 wave B — NSSAIAvailability Options (read-trio).
  - Phase 3 wave A — NSSAIAvailability subscription Post / Unsubscribe / SubModifyPatch.
- data-model schema 33개 (포함) — `data-model-implementation-map.md` ## Schema Classification 참조. generated 14 + wrapper 19 (1 NFType + 17 ProblemDetails 상태코드 + 1 default) + handwritten 0 + deferred 0.
- 외부 의존성.
  - 운영자 입력 — TLS cert (G-01), PostgreSQL DSN (G-02), libjwt JWKS endpoint (G-06).
  - 라이브러리 — openapi-generator(C), cJSON, nghttp2, libpq, libjwt, openssl, app_library TLS facade.
  - 외부 NF — AMF (NSSelectionGet caller, availability subscription consumer), 다른 5G NF 가 cross-NF spec 통해 NSSF 조회 가능.
- 명시적 범위 제외 — 별도 NF spec (33.501/33.310/33.210 etc.) 의존 추가, 33-series spec 산출 (project security baseline ADR-0004 가 source).

## Risk Assessment

- 기술 risk.
  - R-TECH-01 (low) — `PatchDocument` 의미는 29.531 spec 검증으로 `application/json-patch+json` (RFC 6902 PatchItem array) 확정 (G-04 confirmed). RFC 7396 merge-patch payload 진입 시 415 reject. AMF caller 가 spec 미준수 시 reject 예상 — operational expected behavior.
  - R-TECH-02 (low) — `NFType` anyOf forward-compat — G-09 passthrough + log warn 로 mitigate. 위반 (strict enum reject) 시 신규 NFType 출시에서 비호환.
  - R-TECH-03 (medium) — NotificationDispatcher retry 정책 — Phase 1~3 은 동기 dispatch + 1회 재시도 (G-08), Phase 4 hardening 에서 튜닝. Phase 1~3 동안 운영 부하 spike 시 retry_queue 쌓일 가능성.
- 운영 risk.
  - R-OPS-01 (high) — operator 가 G-01 / G-02 / G-06 (cert, DSN, JWKS) 적시 제공 못 하면 Phase 1 deploy 차단. 운영팀에 입력 의무 명시 필요.
  - R-OPS-02 (medium) — PostgreSQL schema migration 은 단일 SQL bootstrap (G-12), 운영자 수동 적용. flyway 같은 framework 없음. schema 변화 시 운영 절차 필수.
- schedule risk.
  - R-SCHED-01 (medium) — Phase 1 tracer-bullet 의 work item 8종 의존 그래프상 직렬 부분이 길다 (codegen-bootstrap → schema-bootstrap → tls-bootstrap → availability-repo → selection-engine → nsselection-handler). codegen-bootstrap 실패 시 전 phase blocked.
  - R-SCHED-02 (low) — Phase 4 contract/security test 는 8 op × 18 cause matrix 144 cell + ADR-0004 7 항목 — test workload 큼, tester/verifier lane 자원 필요.

## Decision Audit

- ADR-0004 security baseline 7 항목 — architecture / engineering-design 반영 검증.
  - (1) TLS 의무 — engineering-design `tls_security` slot `mode=enabled, tls_provider=app_library, tls_dependency=openssl` 일치.
  - (2) inbound OAuth2 — engineering-design `oauth2_token_validation` slot `enabled=true, token_validation_strategy=jwks, lib=libjwt` 일치.
  - (3) outbound OAuth2 — engineering-design `sbi_client_stack` slot `outbound TLS/mTLS/OAuth2 production-capable` 일치.
  - (4) dev disable 한정 — architecture `configuration-strategy.md` 의 profile (dev/prod) 분기 결정 반영.
  - (5) library mandate — sub-300 LOC handcraft TLS/OAuth2 금지, app_library / libjwt 사용 freeze.
  - (6) operator-provided — G-01 / G-06 gap 으로 명시.
  - (7) production-capable — `WI-security-tests` 가 mocked peer / mocked JWKS 로 fail-closed 시나리오 검증.
- engineering-design 13 core slot 결정 요약 (post PR #36 ratify).
  - `sbi_server_stack` — nghttp2 server + h2 (TLS via app_library) + h2c fallback.
  - `tls_security` — mode=enabled, tls_provider=app_library, tls_dependency=openssl.
  - `oauth2_token_validation` (inbound) — jwks via libjwt.
  - `sbi_client_stack` — nghttp2 client + outbound TLS / mTLS / OAuth2 production-capable.
  - `persistence` — PostgreSQL/libpq 단일 backend.
  - `schema_codegen` — openapi-generator(C) + cJSON pinned/vendored.
  - 나머지 7 slot — 본 사이클 변경 없음, `engineering/nssf/engineering-design.md` ## Decisions 참조.

## Recommendation

본 사이클 권고 — **GO (Phase 1 tracer-bullet 진입 가능)**, 단 다음 운영 입력 확보 조건부.

근거.

- contract_implementable PASS (7 check), arch_consistent PASS, eng_frozen PASS — 본 readiness pack 이 통과하면 readiness_pack_ready PASS.
- 8 op × 33 schema 가 모두 work item 그래프에 매핑되고 PR G NSSelectionGet tracer-bullet 의 work item 의존 그래프가 명확.
- ADR-0004 security baseline 7 항목 모두 engineering-design + work item + verification gate 에 반영.
- gap 14개 중 blocker 0 — autonomous codegen 진입 가능.

조건 (운영팀 사전 확보).

- C1 — Phase 1 deploy 전 G-01 (TLS cert), G-02 (PostgreSQL DSN), G-06 (libjwt JWKS endpoint) 운영자 제공 확정.
- C2 — Phase 4 의 mocked peer / mocked JWKS fixture 운영 환경 확정 (tester/verifier lane 자원).

후속 사이클 결정 권고.

- F2 readiness pack 의 deferred 항목 2건 (G-08 retry policy tuning, G-14 observability dashboard) 은 Phase 4~5 와 함께 운영 도입 후 별도 milestone.
- canonical 의 `implementation_guidance_quality` subjective judge 는 본 PR scope 외, PR G 또는 별도 사이클에서 처리.

## References

- `design/nssf/architecture/overview.md`, `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md`.
- `engineering/nssf/engineering-design.md` — 13 core slot freeze.
- `docs/adr/ADR-0004-project-security-baseline.md` — 7 항목 baseline.
- `api-implementation-matrix.md`, `data-model-implementation-map.md`, `codegen-work-items.yaml`, `team-execution-plan.md`, `verification-plan.md`, `open-gaps-and-assumptions.md`.
