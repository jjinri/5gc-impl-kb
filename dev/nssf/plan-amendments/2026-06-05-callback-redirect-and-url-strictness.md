# Callback redirect + URL strictness — round-2 plan amendment (2026-06-05)

planner-mode plan-amendment (round-2). operator+Pane2 합의 follow-up 2건 흡수. self-merge 금지 — PR → STOP, operator(pane1)+Pane2 review. `three_trigger_escape=false` (ADR 신설/`eng_frozen`/ADR-0004 본문 무변 — phase4 test acceptance row 확장만). 신규 runtime slice 없음, src/tests 무변.

## 배경

`#150` (phase4-rows absorb, crosswalk #149) 와 `#151` (dispatcher enqueue-hardening) 머지 시 Pane2(codex) 가 approve-with-followup 으로 남긴 2건을 기존 phase4 slice 의 acceptance 로 흡수한다. crosswalk yaml / `_implementation_run_state.yaml` 은 source/snapshot 이라 건드리지 않는다.

## F1 — outbound-callback 3xx no-follow (OPUS-TENSION-001 후속, #150 Pane2)

NotificationDispatcher 의 *outbound callback* POST 에 대한 redirect 처리 행동을 phase4 에서 명시 검증한다.

- **요구**: outbound callback 응답이 307/308/3xx 면 절대 follow 하지 않는다 (libcurl `FOLLOWLOCATION=0`). 그 응답은 retry / dead-letter / fail-closed 로 처리한다. redirect Location host 로 credential/header(특히 `Authorization: Bearer`) 가 새지 않는다.
- **축 구분 (중요)**: 이는 DOCX-GAP-007 의 *server-side* 307/308 (NSSF 가 NSSelection 응답으로 발신하는 redirect) 과 다른 *outbound-callback* 축이다. plan 가시성에서 두 축을 구분 유지한다.
  - GAP-007 (server-side) = NSSF 가 3xx 를 *발신* — `acceptance_docx_rows` (contract/security), "no accidental 3xx OR Location+3gpp-Sbi-Target-Nf-Id".
  - F1 (outbound-callback) = NSSF 가 AMF callback 으로 POST 시 받는 3xx 를 *수신* — `acceptance_round2_rows` (security/e2e), "no follow + fail-closed + no bearer leak".
- **근거**: Opus crosswalk 의 callback 307-retry row 는 미채택 (ADR-0004 no-follow / fail-closed ratification 우선). 현 libcurl 이 `FOLLOWLOCATION=0` 이라 live hole 이 아니다 → 본 row 는 **regression guard** (미래에 누가 follow 를 켜면 phase4 가 잡는다).
- **반영**: `PR-phase4-security-tests` + `PR-phase4-e2e-tests` 의 `acceptance_round2_rows`.

## F2 — callback_url_allowed well-formed strictness (#151 Pane2)

enqueue + dispatch 가 공유하는 URL gate (`callback_url_allowed`, notification_dispatcher.c) 의 well-formed 강도를 phase4 에서 명시 검증한다.

- **요구**: shared gate 가 ASCII control/whitespace (CR/LF/TAB/space) 및 malformed authority/port/IPv6 를 **모든 모드 공통으로** 명시 reject 한다 — "libcurl 가 나중에 reject 할 malformed URL" 을 앞단(enqueue)에서 차단하고 reject test 로 고정한다.
- **⚠ admit 집합 불변 (#151 2-layer 설계 유지)**: F2 는 enqueue 의 admit 집합을 좁히는 게 아니다. enqueue structural gate 는 #151 대로 permissive (ctor-agnostic) — well-formed https + test loopback-http 를 둘 다 admit 한다 (`test_enqueue_accepts_https_and_loopback` 가 loopback-http enqueue 통과를 고정). well-formed **https-only 는 production / inbound / dispatch gate 의 concern** 이다 (dispatch-time/ctor). 따라서 F2 = "enqueue 에서 https 만 admit" 이 *아니라*, 모든 모드 공통으로 control/whitespace/malformed reject 를 *추가* 하는 strictness 다. ("enqueue 에서 https 만 admit" 으로 읽으면 #151 의 ratified 2-layer 동작과 충돌 — Pane2 catch.)
- **비고**: percent-encoded `%40`(@) / `%23`(#) 은 literal userinfo/fragment 가 아니므로 reject 필수가 아니다 (현 정책 OK). row 에 명시해 향후 혼동 방지.
- **반영**: `PR-phase4-security-tests` 의 `acceptance_round2_rows` (+ `PR-phase4-contract-tests` 의 inbound callbackUri 400-reject row).

## 변경 (scope = plan + doc only)

- `dev/nssf/pr-slicing-plan.yaml` — phase4 contract/security/e2e 에 `acceptance_round2_rows` 추가 + plan_history round-2 entry.
- `dev/nssf/open-gaps-and-assumptions.md` — USER summary-body 에 round-2 addendum.
- `dev/nssf/plan-amendments/2026-06-05-callback-redirect-and-url-strictness.md` — 본 문서 (신규).
- (불변) crosswalk yaml / `_implementation_run_state.yaml` 미변경.

## 검증 / exit

- `validate-execution-control-drift.py nssf` 5/5 PASS.
- YAML valid, `nf-readiness-pack-generate --check` PASS (open-gaps USER zone preserved).
- picker next 불변 (phase4 slice deps 무변).
- round-2 plan-amendment PR open (stacked on [A] status-sync #152) → **STOP, operator(pane1)+Pane2 review**. self-merge 금지.
