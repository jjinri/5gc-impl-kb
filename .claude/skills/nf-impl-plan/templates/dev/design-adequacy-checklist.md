---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Design Adequacy Checklist

사람이 audit 가능한 *질문 목록*. 자동 승인이 아니라 audit surface — `/nf-impl-status` 는 본 문서의 *체크 상태 품질* 을 자동 평가하지 않는다.

## Checklist

각 항목 status — `ok` (검토 후 문제 없음) · `concern` (재검토 필요) · `pending` (아직 검토 안 됨).

| id | question | status | note |
|---|---|---|---|
| C-01 | contract 의 모든 API operation 이 `api-implementation-matrix.md` 에 매핑됐는가. | pending | TODO |
| C-02 | 모든 data-model schema 가 `data-model-implementation-map.md` 에 분류됐는가. | pending | TODO |
| C-03 | ADR-0004 security baseline 7 항목이 architecture·engineering-design 에 모두 반영됐는가. | pending | TODO |
| C-04 | `codegen-work-items.yaml` 의 각 item 이 expected_files·tests·verification_commands 를 가지는가. | pending | TODO |
| C-05 | `team-execution-plan.md` 의 lane 분리가 architecture/engineering-design 와 일치하는가. | pending | TODO |
| C-06 | `verification-plan.md` 의 6 gate 가 모두 ADR-0004 와 architecture observability 를 반영하는가. | pending | TODO |
| C-07 | `open-gaps-and-assumptions.md` 의 `blocker` count 가 0 인가. | pending | TODO |
| C-08 | autonomous codegen agent 가 원본 OpenAPI YAML 을 *다시 읽지 않고도* 구현 판단 가능한가. | pending | TODO |
| TODO | TODO | pending | TODO |

## Outstanding

- TODO: status 가 `concern` 인 항목과 그 해결 owner·기한을 적는다.

## References

- `api-implementation-matrix.md`, `data-model-implementation-map.md`, `codegen-work-items.yaml`, `team-execution-plan.md`, `verification-plan.md`, `open-gaps-and-assumptions.md`.
- `docs/adr/ADR-0004-project-security-baseline.md`.
- `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §4.4.
