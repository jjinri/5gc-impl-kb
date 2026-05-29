---
amendment_id: <YYYY-MM-DD>-<short-slug>
date: <YYYY-MM-DD>
author: nf-orchestrator   # 사람이 작성 시 jjinri 등 git user
status: proposed | merged
plan_target: dev/nssf/pr-slicing-plan.yaml
related_adr: docs/adr/ADR-0005-autonomous-implementation-policy.md
three_trigger_escape: false   # ADR 신설 / eng_frozen 변경 / ADR-0004 영향 시 true → 즉시 stop + 사람 보고
related_slices:
  - <PR-id-added>
  - <PR-id-modified>
---

# <title — 무엇을 plan 에 변경하는가>

## 1. 이유

본 amendment 가 필요해진 *trigger event* 와 *왜 plan 갱신 없이는 진행 불가
한가*. 

예시.
- "PR-phase1-wave1-schema-bootstrap (#101) merge 후 plan 의 status 가 stale —
  in_progress 로 남아 next-slice picker 가 잘못된 slice 를 후보로 선택."
- "wave 1 codegen 결과 PatchDocument 가 별도 wrapper slice 가 되어야 LOC 가
  hard_max 안에 들어옴."

## 2. 대안

고려한 다른 접근 + 각 거부 이유. 최소 2 개. 부재하면 nf-reviewer reject.

| 대안 | 거부 이유 |
|---|---|
| <alternative 1> | <reason> |
| <alternative 2> | <reason> |

## 3. 영향

`pr-slicing-plan.yaml` 의 어떤 entry 가 어떻게 변하는지 + cascade.

### 3.1 plan yaml diff (요약)

```diff
- id: PR-existing-slice
+ id: PR-existing-slice
  status: in_progress
- ...
+ status: merged
+ merged_pr: 'https://github.com/jjinri/5gc-impl-kb/pull/101'
```

### 3.2 신규 / 수정 slice 의 scope_files / depends_on

- 신규 slice id — `<PR-id>`.
  - scope_files: <list>.
  - depends_on: <list>.
  - required_checks: <list>.
- 수정 slice — <id> 의 어느 필드가 변경되는가.

### 3.3 cascade

- 다른 slice 에 영향 — 있음 / 없음 + 설명.
- verification-matrix.yaml 에 영향 — 있음 / 없음 + 설명.
- engineering-design.md / dependency-decisions.yaml 에 영향 — 있음 / 없음.
  있음이면 3-trigger escape 발동 (eng_frozen 영향).
- ADR 신설 필요 — 있음 / 없음. 있음이면 3-trigger escape.
- ADR-0004 baseline 영향 — 있음 / 없음. 있음이면 3-trigger escape.

## 4. Verdict 후 처리

- reviewer planner-mode 검사 PASS → orchestrator self-merge.
- 3-trigger escape true → orchestrator stop + 사람 보고. 본 amendment 는
  사람 review/merge 대상.
