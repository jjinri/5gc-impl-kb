---
nf: {{nf}}
stage: implementation-planning
status: draft
source_architecture: {{architecture_root}}
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{NF}} Open Gaps and Assumptions

모든 gap·assumption 을 6 category 중 하나로 분류. `impl_ready_for_codegen` gate 의 `gaps_classified` 는 본 표의 모든 row 가 category 가 있는지 검사하고, `blocker_gaps_zero` 는 `blocker` row 가 0 인지 검사한다.

## Gaps

| id | category | description | owner | target_resolution |
|---|---|---|---|---|
| G-01 | TODO | TODO | TODO | TODO |

category 값.

- `blocker` — autonomous codegen 시작 전 *반드시* 해결되어야 함. 본 row 가 1 이상이면 `impl_ready_for_codegen` FAIL.
- `deferred` — 본 사이클 범위 밖. 다음 사이클·milestone owner 명시.
- `operator-provided` — 운영자 입력으로 해결 (cert, secret, config). codegen 은 이 의존을 가정하고 진행.
- `library-assumed` — 사용 결정된 third-party library 의 기능에 위임. engineering-design 에 library 인용.
- `test-gap` — test coverage 미달, 후속 test 작업으로 해결.
- `assumption` — 명시적으로 *가정* 으로 기록한 항목. 위반 시 codegen 산출이 무효.

## Summary

`/nf-impl-status` 는 본 표를 다음과 같이 집계한다.

```text
blocker: 0      ← blocker_gaps_zero check
deferred: TODO
operator-provided: TODO
library-assumed: TODO
test-gap: TODO
assumption: TODO
```

## References

- `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §4.4.
- `design-adequacy-checklist.md`, `spec-to-design-coverage.md` — gap 발생 surface.
- `engineering/{{nf}}/engineering-design.md` — library/operator boundary.
