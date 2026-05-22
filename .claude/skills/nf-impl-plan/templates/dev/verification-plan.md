---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Verification Plan

`/nf-implement {{nf}}` 의 verification gate 6종. 각 gate 는 unit/integration/contract/security/e2e/observability 한 종류씩 책임.

## Unit

- 범위 — 개별 module 함수·class·utility.
- pass 조건 — TODO 비율 기준 (예 `coverage >= 80%`), 모든 unit test green.
- tooling — TODO (engineering-design 의 test framework 결정 인용).

## Integration

- 범위 — module 간 SBI/service 인터페이스, persistence backend, configuration loading.
- pass 조건 — TODO (예 main API endpoint 응답 schema 일치, persistence round-trip 검증).
- tooling — TODO.

## Contract

- 범위 — `handoff/{{nf}}/contract.yaml` 정합성, problem-details matrix, error response schema.
- pass 조건 — contract test 가 모든 API operation × cause 매트릭스를 cover.
- tooling — TODO (contract test runner 결정).

## Security

- 범위 — ADR-0004 security baseline 7 항목 (mTLS/inbound OAuth2/outbound OAuth2/dev disable/library mandate/operator policy).
- pass 조건 — 각 baseline 항목별 verification command + 결과 evidence.
- tooling — TODO (예 OpenSSL probe, JWKS fetch test, scope 검증 fuzz).

## End-to-End

- 범위 — service flow 시나리오, multi-NF interaction (필요 시 mocked peer).
- pass 조건 — `request-flow.md` 의 핵심 시나리오 green.
- tooling — TODO.

## Observability

- 범위 — `{{architecture_root}}/observability.md` 의 metric/log/trace seam.
- pass 조건 — 각 seam 이 production-capable signal emit, dashboard query 검증.
- tooling — TODO.

## References

- `{{architecture_root}}/test-strategy.md`, `{{architecture_root}}/observability.md`.
- `engineering/{{nf}}/engineering-design.md` — test/verification tooling freeze.
- `docs/adr/ADR-0004-project-security-baseline.md`.
- `codegen-work-items.yaml` — work item 의 `tests`/`verification_commands` 가 본 plan 의 gate 를 가리킨다.
