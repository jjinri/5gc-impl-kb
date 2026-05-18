---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Implementation Plan

## Scope

본 implementation plan 은 architecture design 을 *구현 작업 단위* 로 변환한다. 소스 코드·dependency file·build system 은 이 단계에서 만들지 않는다.

포함 범위.

- TODO: 이번 plan 이 다루는 API operation, module, external boundary 를 적는다.

제외 범위.

- 구현 언어·런타임·HTTP 라이브러리·TLS·persistence backend 선택 (각 task 의 `owner: dev` 가 결정).
- TODO: 운영 결정 보류로 제외한 범위와 후속 milestone 을 적는다.

입력.

- Architecture root — `{{architecture_root}}`.
- Contract handoff — `{{contract_path}}`.
- Contract 산출 — `design/{{nf}}/contract/`.

## Phases

| Phase | 목표 | 핵심 module | 핵심 task prefix |
|---|---|---|---|
| TODO | TODO | TODO | TODO |

Phase 간 의존성.

- TODO: phase 선후 관계와 병렬 가능 구간을 적는다.

## Test Plan

본 plan 단계는 *test seam* 만 정의한다. 구체 test case 는 `test-matrix.md` 가 진실 출처다.

| 종류 | 범위 | seam |
|---|---|---|
| unit | TODO | TODO |
| module-integration | TODO | TODO |
| contract | TODO | TODO |
| end-to-end scenario | TODO | TODO |

Phase ↔ test 매핑.

- TODO: 각 phase 가 닫는 test 종류를 적는다.

## Open Risks

- TODO: 미해결 기술 결정 (language/runtime, HTTP/SBI framework, persistence/cache backend, telemetry stack) 을 owner 와 blocking task 와 함께 적는다.
- TODO: architecture 의 Open Questions 중 dev 진행을 막는 항목을 적는다.

## References

- `{{architecture_root}}/` — architecture document set + `decisions/`.
- `design/{{nf}}/module-decomposition/` — module 상세.
- `{{contract_path}}` — contract handoff.
- `tasks.yaml`, `test-matrix.md`, `traceability.md`.
- TODO: 본 사이클 plan 문서 (`docs/plans/...`) 를 적는다.
