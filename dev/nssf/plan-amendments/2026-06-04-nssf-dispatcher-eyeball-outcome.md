---
amendment_id: 2026-06-04-nssf-dispatcher-eyeball-outcome
date: 2026-06-04
author: nf-orchestrator
status: proposed
plan_target: dev/nssf/pr-slicing-plan.yaml
related_adr: docs/adr/ADR-0005-autonomous-implementation-policy.md
three_trigger_escape: false   # ADR 신설 / eng_frozen 변경 / ADR-0004 본문 영향 모두 없음 (baseline 구현이지 개정 아님)
related_slices:
  - PR-phase2-notification-dispatcher
  - PR-phase4-dispatcher-resilience
  - PR-phase2-dispatcher-enqueue-hardening
  - PR-phase2-dispatcher-body-strict-json
---

# NSSF notification-dispatcher eyeball outcome 등록 — #135

## 1. 이유

`PR-phase2-notification-dispatcher` 가 `#135` (f6a4e88) 로 main 에 머지되었다. EYEBALL slice (M4 outbound 첫 consumer — AMF callback POST / oauth2 scope / retry 보안민감) 로 self-merge 금지였고, operator(pane1) + Pane-2(codex) 통합 eyeball 이 autonomous reviewer 가 놓친 blocker 3건을 적발 → rework round 1 후 머지되었다.

plan 갱신 없이는 다음이 깨진다.

- picker 가 plan 의 `PR-phase2-notification-dispatcher.status = not_started` 를 읽어 *이미 머지된* slice 를 next 후보로 계속 반환 (loop advance 불가) — `nf-implement-slice-status.py` 의 `effective_status()` 는 `merged_pr` URL 이 있을 때만 gh 동적 조회를 한다.
- eyeball 에서 *Phase-4 defer 합의* 된 fail-closed FORBIDDEN backoff/dead-letter (F3) + 2건의 non-blocking follow-up 이 어디에도 등록되지 않아 추적 누락.
- G-08 (NotificationDispatcher retry policy deferred) 가 #135 가 도입한 fail-closed `requeue NOW()` hot-loop 위험 + caller-contract 를 반영하지 못함.

## 2. 대안

| 대안 | 거부 이유 |
|---|---|
| status sync 만 하고 follow-up 등록은 생략 | F3/follow-up 이 plan 밖에서 휘발 → 추후 재발견 비용. operator eyeball 합의가 추적 source 를 잃음. |
| follow-up 을 first-class `WI-dispatcher-resilience` 신규 WI 로 codegen + agent-execution + readiness-config 전부 등록 | `design/nssf/readiness-config.yaml` (gaps/phase source) 갱신 + readiness regen 필요 = 본 housekeeping scope (`dev/nssf/** + .gitignore`) 밖. drift validator `phase_wi_aggregate` (agent ↔ readiness) 가 red 됨. → 기존 `WI-notification-dispatcher` 재사용 + slice notes 로 명명, first-class 승격은 별도 readiness regen follow-up. |
| orchestrator 가 본 amendment self-merge | ADR-0005 D5 — plan amendment self-merge 금지. plan = 사람 의도 source-of-truth. operator review 필요. |

## 3. 영향

### 3.1 plan yaml diff (요약)

```diff
   - id: PR-phase2-notification-dispatcher
-    status: not_started
+    status: merged
+    merged_pr: 'https://github.com/jjinri/5gc-impl-kb/pull/135'
+    # EYEBALL — rework round 1 (B1/B2/B3) 후 merge. F3 → Phase-4 resilience.

 plan_history:
+  - '#135 merged — PR-phase2-notification-dispatcher (eyeball, f6a4e88) ...'

+  - id: PR-phase4-dispatcher-resilience          (status: deferred)
+  - id: PR-phase2-dispatcher-enqueue-hardening   (status: deferred)
+  - id: PR-phase2-dispatcher-body-strict-json    (status: deferred)
```

### 3.2 신규 / 수정 slice 의 scope_files / depends_on

- 수정 — `PR-phase2-notification-dispatcher`: `status` not_started→merged + `merged_pr` URL + eyeball/rework notes. scope_files / depends_on 무변.
- 신규 — `PR-phase4-dispatcher-resilience` (G-08 + F3 fold).
  - work_items: `WI-notification-dispatcher` (기존 재사용 — 대안표 참조).
  - scope_files: `src/nssf/dispatcher/notification_dispatcher.{h,c}`, `tests/nssf/**/test_notification_dispatcher_*.c`.
  - depends_on: `PR-phase2-notification-dispatcher`.
  - status: `deferred`. trigger=Phase 4. required_checks: [].
- 신규 — `PR-phase2-dispatcher-enqueue-hardening` (non-blocking follow-up a).
  - 내용: `nssf_retry_store_enqueue` 가 callback_uri NULL/invalid 를 API-level 에서 더 일찍 hard-reject (defense-in-depth). 원 B3 blocker 는 #135 producer not-ready path 로 CLOSED.
  - depends_on: `PR-phase2-notification-dispatcher`. status: `deferred`.
- 신규 — `PR-phase2-dispatcher-body-strict-json` (non-blocking follow-up b).
  - 내용: F7 strict-valid-JSON — invalid non-empty JSON string 을 application/json 으로 보내는 잔여 창 차단. null/empty/corrupt body 는 #135 가 이미 quarantine.
  - depends_on: `PR-phase2-notification-dispatcher`. status: `deferred`.

### 3.3 eyeball blocker / fix 요약 (#135 rework round 1)

- **B1** — outbound URL policy: shared `callback_url_allowed()` (https-only + userinfo`@`/fragment`#`/malformed reject) 를 oauth2 bearer attach *이전* 에 수행. 비-https → token 미부착 + POST 없음 + DROPPED. libcurl 7.81: `CURLOPT_PROTOCOLS`+`CURLPROTO_HTTPS` (`CURLOPT_PROTOCOLS_STR` 는 `#if LIBCURL_VERSION_NUM >= 0x075500` guard).
- **B2** — correlation header injection: `correlation_id_is_safe()` 가 CR/LF/control/over-length reject (inbound + stored), unsafe/absent → safe hex 재생성.
- **B3** — producer ↔ `retry_queue.subscription_id`: nfId 를 subscription_id 에 cast 금지. `change_publish` not-ready/log path. enqueue seam 은 API + in-memory backend 로 보존. silent `(void)` swallow 제거.
- **F7** — valid body 필수 (corrupt/missing → quarantine). **F4** — PG store single-worker/non-thread-safe 계약 + owner-thread guard. **F2** — `.h` false claim 정정 + `_new`/`_new_insecure` ctor split.
- **F3** (deferred) — fail-closed FORBIDDEN backoff/dead-letter → `PR-phase4-dispatcher-resilience`. doc note "caller MUST NOT tight-loop" 추가. #135 polling loop 없음.

### 3.4 carry items 추적 (dedup 후 — 중복 등록 안 함, 빠진 것만)

| carry item | 기존 추적 | 본 amendment 처리 |
|---|---|---|
| schema doc-vs-infra reconcile (`state-persistence.md` rich table vs `infra/nssf/schema.sql` minimal) | 미추적 | 본 md §3.5 에 신규 기록 (reconcile = readiness/architecture 영역, source 변경 = 별도 lifecycle). |
| fan-out → Phase-3 subscription-store | 기추적 — `PR-phase3-subscription-store` depends_on dispatcher + `state-persistence.md` open question | 중복 등록 안 함. |
| main.c wiring (engine publish-seam → dispatcher 등록) | 미추적 (standalone lib) | 본 md §3.5 기록 — 통합 slice 필요 (별도 operator 결정). |
| TLS 1.2→1.3 Phase-4 enforce | 부분 — ADR-0004 baseline | 본 md §3.5 기록 — `PR-phase4-security-tests` verifier gate 에서 확인 권고. |
| `.gitignore` rule `dev/*/_verification_log.yaml` | 미추적 (sibling `_impl_status`/`_readiness_status`/`_implementation_run_state` 는 기등록) | 본 amendment 에서 `.gitignore` 에 추가 (source-of-truth policy — `_`-prefix 산출은 로컬 재생성, 추적 제외). |
| CI postgres service (B3 PG live test 활성, 현재 SKIP-PASS) | 미추적 | 본 md §3.5 기록 — CI infra 영역 (별도 PR). |

### 3.5 미추적 carry item 신규 기록 (tracking only — 본 amendment 가 구현/slice 화 하지 않음)

- **schema doc-vs-infra reconcile**. `design/nssf/architecture/state-persistence.md` 는 rich `nssf_notification_retry_queue` (dedicated `callback_uri`/`correlation_id`/`status`/`locked_by`...) 를 기술하나 shipped `infra/nssf/schema.sql` 는 minimal `retry_queue(id, subscription_id, payload, attempt_count, next_attempt_at, created_at)`. #135 는 shipped schema 에 맞춰 `callbackUri`+`correlationId` 를 payload JSONB 에 담음. Phase-4 hardening (status/last_error/backoff column) 전에 doc↔infra 정합 필요. 둘 다 본 housekeeping scope 밖 (architecture/infra) → operator/readiness 후속.
- **main.c wiring**. dispatcher 는 standalone lib. `nssf_availability_engine_seams_t.publish` 에 `nssf_notification_dispatcher_change_publish` 를 배선하는 integration slice 필요 (단, B3 로 producer 는 Phase-3 subscription resolution 전까지 not-ready path). main.c 는 `PR-phase2-availability-route-registration` scope.
- **TLS 1.3 enforce**. #135 outbound 는 TLS 1.2 floor (`CURLOPT_SSLVERSION=CURL_SSLVERSION_TLSv1_2`). ADR-0004 TLS 1.3 enforce 는 Phase-4 security item — `PR-phase4-security-tests` verifier 가 phase-4 exit 전 확인.
- **CI postgres service**. `test_pg_enqueue_invalid_subscription_fails` 등 PG-gated test 는 `POSTGRES_*` 부재로 SKIP-PASS. live-PG FK error branch 활성화 = CI postgres service container (별도 infra PR).

### 3.6 cascade

- picker 영향 — `PR-phase2-notification-dispatcher = merged` 후 next ready = `PR-phase3-subscription-store` (depends_on schema-bootstrap ✓ + dispatcher ✓, plan-order 상 신규 deferred slice 보다 앞). 신규 3 slice 는 dispatcher 만 depends_on + plan-order 말단 + status=deferred → main sequence/기존 backlog 뒤로만 후보. 즉, 본 amendment merge 후 `--resume` 시 picker 는 정상적으로 phase3 로 advance.
- verification-matrix.yaml 영향 — 없음 (신규 slice required_checks 전부 []).
- codegen-work-items / agent-execution-plan / readiness-config 영향 — 없음 (신규 slice 가 기존 `WI-notification-dispatcher` 재사용, 신규 WI id 없음). drift validator 5 check 무변.
- engineering-design.md / dependency-decisions.yaml 영향 — 없음 (eng_frozen 무변).
- ADR 신설 필요 — 없음. ADR-0004 baseline 영향 — 없음 (#135 는 baseline 구현, 본 amendment 는 plan/doc only).

## 4. Verdict 후 처리

ADR-0005 D5 — plan amendment 는 orchestrator self-merge 미적용. eyeball_stop 유지. operator(pane1) review/merge. merge 후 `/nf-implement nssf --resume` 로 picker 가 `PR-phase3-subscription-store` 로 advance.
