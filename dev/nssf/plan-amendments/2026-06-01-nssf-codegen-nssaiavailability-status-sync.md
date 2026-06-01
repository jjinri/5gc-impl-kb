---
amendment_id: 2026-06-01-nssf-codegen-nssaiavailability-status-sync
date: 2026-06-01
author: nf-orchestrator
status: proposed
plan_target: dev/nssf/pr-slicing-plan.yaml
related_adr: docs/adr/ADR-0005-autonomous-implementation-policy.md
three_trigger_escape: false   # ADR 신설 / eng_frozen 변경 / ADR-0004 영향 모두 없음
related_slices:
  - PR-codegen-nssaiavailability-extension
---

# NSSF codegen NSSAIAvailability extension status sync — #110

## 1. 이유

`PR-codegen-nssaiavailability-extension` 가 main 에 `#110` 로 머지되었다
(첫 fully-autonomous orchestrator slice — readiness gate → 4 lane dispatch →
4-condition self-merge gate PASS). 그런데 plan 의 해당 entry `status` 가
`not_started` 로 남아 있고 `merged_pr` URL 이 없다.

`nf-implement-slice-status.py` 의 `effective_status()` 는 `merged_pr` URL 이
있을 때만 gh 동적 조회를 한다 (`gh_pr_state(pr.get("merged_pr") or "")`).
URL 부재 시 plan 의 `status` 로 fallback. 따라서 본 정합 없이는 picker 가
*이미 머지된* slice 를 계속 next 후보로 반환하여 loop 가 advance 하지 못한다
(실측 — 머지 직후 picker 가 동일 slice 재반환).

## 2. 대안

| 대안 | 거부 이유 |
|---|---|
| picker 가 gh 로 *모든* PR 의 머지 상태를 title/branch 매칭으로 자동 탐지 | picker 알고리즘 변경 = 별도 infra PR. 현 결정론 (plan = 의도 source, merged_pr = trace) 깨뜨림. 본 status sync 범위 밖. |
| orchestrator 가 plan yaml 을 self-merge 하여 즉시 advance | ADR-0005 D5 "plan amendment self-merge 도 금지" 위배. plan = 사람 의도 source-of-truth, 사람 review 필요. |
| state file (gitignored) 에만 merged 기록, plan yaml 유지 | picker 가 plan yaml `status`/`merged_pr` 만 읽음. state file 은 picker 입력 아님 → 무효. |

## 3. 영향

### 3.1 plan yaml diff

```diff
   - id: PR-codegen-nssaiavailability-extension
     ...
     depends_on:
       - PR-phase1-wave1-codegen-bootstrap
-    status: not_started
+    status: merged
+    merged_pr: 'https://github.com/jjinri/5gc-impl-kb/pull/110'
```

plan_history 에 `#110 merged` 1 줄 추가.

### 3.2 신규 / 수정 slice 의 scope_files / depends_on

신규 slice 없음. 수정 entry 1 개 — `PR-codegen-nssaiavailability-extension`
의 `status` + `merged_pr` 두 필드 (+ notes 1 줄). scope_files / depends_on
변경 없음.

### 3.3 cascade

- `PR-codegen-nssaiavailability-extension` 을 depends_on 으로 참조하는 slice =
  `PR-phase2-availability-engine` (phase2). phase2 는 아직 진입 전이라 즉시
  영향 없음. 본 sync 후에도 phase2 는 `PR-phase1-wave1-nsselection-handler`
  머지까지 차단 유지.
- verification-matrix.yaml 영향 — 없음.
- engineering-design.md / dependency-decisions.yaml 영향 — 없음 (eng_frozen 무변).
- ADR 신설 필요 — 없음.
- ADR-0004 baseline 영향 — 없음 (codegen model emit, security pattern 무변).

### 3.4 picker 결과 (정합 후 예상)

`statuses[PR-codegen-nssaiavailability-extension] = merged` →
다음 picker 후보 = depends_on 이 모두 merged 인 첫 not_started PR. phase1_wave1
잔여 후보 (depends_on 충족):
- `PR-phase1-wave1-tls-bootstrap` (dep: codegen-bootstrap ✓) — TLS context +
  OAuth2 JWKS + scope check. **보안 민감 (ADR-0004 baseline 준수 영역)**.
- `PR-phase1-wave1-availability-repo` (dep: schema-bootstrap ✓ + codegen-bootstrap ✓).

plan 순서상 picker 는 `PR-phase1-wave1-tls-bootstrap` 를 먼저 반환할 전망.

## 4. Verdict 후 처리

ADR-0005 D5 — plan amendment 는 orchestrator self-merge 미적용. 사람 review/
merge. merge 후 orchestrator 가 `/nf-implement nssf --resume` 로 loop 재개,
picker 가 다음 ready slice 로 advance.
