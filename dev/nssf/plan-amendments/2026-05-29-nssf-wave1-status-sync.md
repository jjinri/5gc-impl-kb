---
amendment_id: 2026-05-29-nssf-wave1-status-sync
date: 2026-05-29
author: jjinri   # bootstrap manual sync — PR-D 가 사람 review/merge
status: proposed
plan_target: dev/nssf/pr-slicing-plan.yaml
related_adr: docs/adr/ADR-0005-autonomous-implementation-policy.md
three_trigger_escape: false
related_slices:
  - PR-phase1-wave1-schema-bootstrap
---

# NSSF wave 1 status sync — PR-D bootstrap

## 1. 이유

`PR-phase1-wave1-schema-bootstrap` 의 plan 의 `status` 가 `in_progress` 인데
실제 main 에는 `#101` 로 머지되어 있다 (commit `4b3dec2`, 2026-05-29).
nf-implement-slice-status picker 가 본 entry 를 *미완료* 후보로 잘못 평가할
수 있고 (depends_on chain 에서 후속 slice 가 차단), `--no-gh` offline mode
에선 무조건 plan status 만 사용하므로 본 정합 없이는 PR-D 가 시작될 수 없다.

본 amendment 는 *PR-D bootstrap* 의 일부 — orchestrator 가 본격 자율 진행을
시작하기 직전, plan yaml 을 main 실제 상태와 한 번 정합한다. 이후 시점부터는
orchestrator 가 비슷한 drift 발견 시 *자율 plan amendment PR* 를 작성할 책임
이다 (ADR-0005 D5).

## 2. 대안

| 대안 | 거부 이유 |
|---|---|
| gh CLI 동적 조회만 사용 (plan yaml status 무시) | CI / offline 모드에서 picker 결정론 깨짐 + `--no-gh` 옵션 의미 무력화. ADR-0005 D4 "plan + gh 둘 다 진실 출처, gh 우선" 정책 유지 위해 plan yaml 도 정합 필요. |
| status sync 를 orchestrator 의 첫 자율 amendment 로 위임 | chicken-and-egg — picker 가 잘못된 next slice 선택 시 첫 자율 작업이 wrong slice 가 됨. bootstrap 은 사람 손이 안전. |
| plan_history 만 갱신, prs[] entry status 유지 | picker 알고리즘이 prs[].status 만 사용. plan_history 는 audit-only. |

## 3. 영향

### 3.1 plan yaml diff

```diff
   - id: PR-phase1-wave1-schema-bootstrap
     ...
-    status: in_progress
+    status: merged
+    merged_pr: 'https://github.com/jjinri/5gc-impl-kb/pull/101'
```

### 3.2 신규 / 수정 slice 의 scope_files / depends_on

신규 slice 없음. 수정 entry 1 개 — `PR-phase1-wave1-schema-bootstrap` 의
`status` + `merged_pr` 두 필드만.

### 3.3 cascade

- 다른 slice 영향 — 없음. `PR-phase1-wave1-schema-bootstrap` 을 depends_on 으로
  참조하는 후속 slice 가 plan 에 없다 (직접 dependent 부재). NSSF wave 1 의
  뒤따르는 slice (`PR-phase1-wave1-availability-repo` 등) 는 다른 wrapper
  PR 에 의존.
- verification-matrix.yaml 영향 — 없음.
- engineering-design.md / dependency-decisions.yaml 영향 — 없음.
- ADR 신설 필요 — 없음.
- ADR-0004 baseline 영향 — 없음.

### 3.4 picker 결과 (정합 후 예상)

`statuses[PR-phase1-wave1-schema-bootstrap] = merged` → 다음 picker 후보 가
`depends_on` 모두 merged 인 첫 PR 로 갱신. 본 amendment merge 후 `--no-gh`
실행으로 검증 예정.

## 4. Verdict 후 처리

본 amendment 는 *PR-D bootstrap* 의 일부이므로 사람 review/merge. orchestrator
self-merge 미적용. ADR-0005 D5 의 *향후* amendment 부터 self-merge.
