---
amendment_id: 2026-06-04-nssf-sequencing-phase3-first
date: 2026-06-04
author: nf-orchestrator
status: proposed
plan_target: dev/nssf/pr-slicing-plan.yaml
related_adr: docs/adr/ADR-0005-autonomous-implementation-policy.md
three_trigger_escape: false   # ADR 신설 / eng_frozen 변경 / ADR-0004 / readiness-config 본문 변경 모두 없음
related_slices:
  - PR-phase3-fanout-integration
  - PR-phase3-subscription-handlers
  - PR-phase2-oauth2-resilience
  - PR-phase2-config-hardening
  - PR-phase4-dispatcher-resilience
  - PR-phase2-dispatcher-enqueue-hardening
  - PR-phase2-dispatcher-body-strict-json
---

# NSSF sequencing — phase3-first gate + fan-out integration slice

## 1. 이유

`PR-phase3-subscription-store` 머지(#137) 후 status-sync(#138, 1424557) 까지 끝났으나, next-slice picker(topo-sort)가 `PR-phase2-oauth2-resilience` (phase2 M4 backlog) 를 next 로 surface 했다 — 그 depends_on (`PR-phase2-outbound-oauth2` + `PR-phase2-notification-dispatcher`) 가 모두 merged 라 topo 의 D+1 layer 에 올라왔기 때문이다. phase3 의 subscription-handlers 는 D+2 (subscription-store 의존) 라 backlog 뒤로 밀렸다.

operator + Pane-2(codex) 수렴 결정 = **phase3-first**. B3 fan-out / subscription API arc 가 critical path 이고, oauth2-resilience / config-hardening 은 reliability backlog 다. plan 갱신 없이는 picker 가 backlog 를 phase3 앞에서 계속 surface 한다.

## 2. 대안

| 대안 | 거부 이유 |
|---|---|
| M4-first (oauth2-resilience 먼저) — picker 기본값 따름 | B3 fan-out 미완 = subscription 이 실제 outbound 를 못 함. functional critical path (phase3) 가 reliability hardening (M4 #11 backlog) 보다 우선. operator+Pane-2 수렴. |
| backlog 를 `status: deferred` 로만 두고 picker 가 알아서 거르길 기대 | picker 는 `deferred` 를 skip 하지 않는다 (effective_status 가 merged 만 terminal). depends_on gate 만 유효. |
| oauth2-resilience/config-hardening 만 gate (spec 명시 2건) | 불충분 — #136 의 dispatcher follow-up 3건(enqueue-hardening/body-strict-json/dispatcher-resilience)도 D+1 (dispatcher 의존) + ready 라 subscription-handlers(D+2) 앞에 surface. picker next=subscription-handlers 달성하려면 *모든* D+1 deferred backlog 를 gate 해야 함 (검증 요구사항). |

## 3. 영향

### 3.1 신규 slice

- `PR-phase3-fanout-integration` (phase3).
  - work_items: `WI-notification-dispatcher` (기존 codegen WI 재사용 — B3 fan-out 은 dispatcher 책임. first-class WI-fanout 승격 = readiness-config(design/nssf/) regen follow-up, 본 scope 밖).
  - scope_files: `src/nssf/main.c`, `src/nssf/store/subscription_store.{h,c}`, `tests/nssf/integration/test_fanout_integration.c`.
  - depends_on: `PR-phase3-subscription-handlers`.
  - 내용: availability-engine change-event seam (`nssf_availability_change_publish_fn`) → affected subscription match (subscription_store) → dispatcher retry_queue enqueue (실제 subscription UUID + callbackUri) + main.c engine→store→dispatcher publish-seam wiring (#135 B3 not-ready/미배선 path 대체). 기존 seam 소비, 새 outbound 로직 없음.
  - ⚠ worker/polling loop 도입 금지 (call-driven). loop 도입 시 open-gaps G-08 hot-loop blocker 승격 → oauth2-resilience 선행조건.
  - status: not_started. required_checks: [].

### 3.2 수정 slice

- `PR-phase3-subscription-handlers`: scope-boundary notes 추가 — CRUD / Location / callbackUri validation (dispatcher URL policy 재사용) / initial-snapshot dispatch / route wiring 까지만. change-event→match→enqueue + main.c seam 은 fan-out slice (분리). scope_files 무변.
- `PR-phase2-oauth2-resilience`, `PR-phase2-config-hardening`: depends_on += `PR-phase3-fanout-integration` (gate). + reliability 제약 note.
- `PR-phase4-dispatcher-resilience`, `PR-phase2-dispatcher-enqueue-hardening`, `PR-phase2-dispatcher-body-strict-json`: depends_on += `PR-phase3-fanout-integration` (gate — D+1 deferred 가 subscription-handlers 앞에 surface 하던 것 차단).

### 3.3 reliability 제약 (open-gaps 기록)

`open-gaps-and-assumptions.md` G-08 USER addendum 에 추가 — fan-out 이 main.c 배선으로 live outbound 활성화 시 token transient failure 가 single-shot. polling loop 없음 + fail-closed/queue 보존이라 security blocker 아니나, **Phase-4/e2e 전 또는 dispatcher worker loop 도입 전 oauth2-resilience(+config-hardening) 반드시 close** (F3/G-08). 위 depends_on gate 가 이 순서를 plan 으로 강제.

### 3.4 cascade

- picker(--no-gh) next = `PR-phase3-subscription-handlers` (검증 완료 — oauth2-resilience 아님).
- 신규/수정 모두 기존 `WI-notification-dispatcher` 재사용 → codegen-work-items / agent-execution-plan / readiness-config 무변. drift validator 5/5 PASS.
- verification-matrix 영향 없음 (fan-out required_checks []).
- engineering-design / dependency-decisions 영향 없음 (eng_frozen 무변). ADR 신설 없음. ADR-0004 영향 없음.

## 4. Verdict 후 처리

ADR-0005 D5 — plan amendment self-merge 금지. operator(pane1) review/merge. merge 후 `/nf-implement nssf --resume` → picker = `PR-phase3-subscription-handlers`.
