---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Architecture Overview

## Purpose

본 문서는 handoff-ready contract 를 architecture 책임으로 변환한다 — *어떤 모듈* 이 *어떤 책임* 을 *어떤 경계* 로 가지는지 정의한다. 구현 언어·런타임·DBMS·배포·라이브러리 선택은 dev 단계의 책임이다.

## Inputs (contract)

- `{{contract_path}}` (handoff-v2).
- TODO: 본 architecture 가 의존하는 API topic id 를 적는다.
- TODO: 본 architecture 가 의존하는 핵심 data-model topic id 를 적는다.
- TODO: 공통 topic (`interface`, `error-handling` 등) 의존을 적는다.

## Boundaries

본 architecture 가 다루는 것.

- TODO: 이번 pass 가 다루는 SBI service·operation 을 적는다.
- TODO: inbound/outbound 처리 범위를 적는다.
- TODO: module 분해 대상을 적는다.

본 architecture 가 다루지 않는 것.

- 구현 언어·런타임·DBMS·HTTP 라이브러리 선택 (dev 단계).
- TODO: 운영 결정 보류로 제외한 spec 깊이를 `_manifest.yaml` `manual_overrides.exclude` 근거와 함께 적는다.
- 배포 토폴로지·service mesh / sidecar 선택.

## Decisions

| decision | 내용 |
| --- | --- |
| architecture 경계 | contract extraction 과 implementation planning 사이의 경계는 `design/{{nf}}/architecture/` 문서 집합이다. |
| module 산출 | logical module 분해는 `design/{{nf}}/module-decomposition/<Module>.md` 에 둔다. |
| 구현 선택 위임 | language/runtime, persistence backend, HTTP/SBI framework, deployment topology 는 dev 단계 `TBD`. contract 에서 결정하지 않는다. |
| TODO | TODO: 본 NF 고유의 baseline 결정을 추가한다. |

## Open Questions

- TODO: 다음 사이클 또는 dev 단계로 미루는 미해결 항목을 적는다.
- TODO: 운영 결정 보류 (보안 profile, cross-NF 지원 여부 등) 를 적는다.

## References

- `{{contract_path}}` — contract handoff.
- `design/{{nf}}/contract/` — contract topic 산출.
- `design/{{nf}}/architecture/decisions/ADR-0001-architecture-baseline.md`.
- 본 architecture set — `module-boundaries.md`, `request-flow.md`, `runtime-model.md`, `state-persistence.md`, `configuration-strategy.md`, `error-propagation.md`, `observability.md`, `test-strategy.md`.
- TODO: 인용 spec clause 를 적는다.
