---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Implementation Readiness Review

사람이 *전체 설계 적정성* 을 빠르게 audit 하기 위한 executive review. `/nf-impl-status` 는 본 문서의 *내용 품질* 을 자동 승인하지 않는다 — 존재·구조·traceability 만 검사한다.

## Executive Summary

- 본 NF 의 핵심 목표·범위·완성 시 사용자가 얻을 기능 (3~5줄).
- TODO.

## Scope Confirmation

- 포함된 API operation, data-model, 외부 의존성을 한눈에 확인 (`api-implementation-matrix.md`, `data-model-implementation-map.md` 인용).
- TODO.

## Risk Assessment

- 기술 risk, 운영 risk, schedule risk 를 `open-gaps-and-assumptions.md` 의 gap id 와 묶어 적는다.
- TODO.

## Decision Audit

- ADR-0004 security baseline 7 항목이 architecture/engineering-design 에 모두 반영됐는지 확인.
- engineering-design 의 13 core slot 결정 요약 + 본 사이클에 변경된 항목.
- TODO.

## Recommendation

- 사람의 GO/NO-GO 권고와 그 근거. `/nf-implement` 시작 전 사람이 마지막으로 보는 신호.
- TODO.

## References

- `{{architecture_root}}/overview.md`, `decisions/`.
- `engineering/{{nf}}/engineering-design.md`.
- `docs/adr/ADR-0004-project-security-baseline.md`.
- 본 pack 의 다른 문서들 (`api-implementation-matrix.md` 외).
