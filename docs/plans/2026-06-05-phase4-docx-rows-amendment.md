# Phase4 DOCX-derived acceptance row 흡수 — plan amendment (2026-06-05)

planner-mode plan-amendment. operator 지시. self-merge 금지 — PR → STOP, operator(pane1) review (+Pane2 옵션). `three_trigger_escape=false` (ADR 신설/`eng_frozen`/ADR-0004 본문 무변 — test acceptance row 확장만).

## 목적

`dev/nssf/docx-harness-p2-crosswalk.yaml` (#149, DOCX Harness P2 + Opus crosswalk) 의 DOCX-derived gap row 를 **기존 phase4 test slice 에 흡수**한다. crosswalk 권고 `new_slice_needed_before_phase4=false` 에 따라 신규 runtime slice 는 만들지 않는다.

## source

- `dev/nssf/docx-harness-p2-crosswalk.yaml` (#149 073a43b).
- `docs/kb/nssf-docx-harness-p2-opus-crosswalk.md` (#149).

## 변경 (scope = plan + doc only, src/tests 무변)

### `dev/nssf/pr-slicing-plan.yaml`

기존 phase4 slice 에 `acceptance_docx_rows` 키 추가 (verification-matrix.yaml 가 base matrix 진실 출처, 본 row 는 DOCX-derived 보강).

| slice | 흡수 GAP rows |
|---|---|
| PR-phase4-contract-tests | GAP-002 (SubModifyPatch /event·/filter/event mutation reject + event-IE immutability; empty taiList ONSSAI-gated), GAP-004 (NSSelection registration/PDU/UE-config matrix + PDN/OtherPurpose/InterPLMN supported-or-deferred marker), GAP-007 (307/308 server redirect unsupported 증명 또는 Location+3gpp-Sbi-Target-Nf-Id), GAP-003 negative-claim marker (spec-complete expiry 주장 금지) |
| PR-phase4-security-tests | GAP-002 (mutation fail-closed, persist 전 거부), GAP-007 (no redirect without target NF id), GAP-009 wording-guard (scope = project/deployment policy, not 29.531 resource scope) |
| PR-phase4-e2e-tests | GAP-001 (self-origin AMF suppression regression), GAP-004 (implemented procedure 흐름 + unsupported marker), GAP-005 (NSSRG/NSAG/UDM provide-all supported-or-deferred), GAP-008 (feature-gated notification payload supported-or-501) |

phase5 `PR-phase5-operator-configuration-guide` 에 `deferred_docx_rows` 추가 — GAP-003 (expiry policy), GAP-006 (NSSF discovery/home-NSSF/FQDN), GAP-009 (no spec-defined Nnssf scope 본문).

plan_history += 2026-06-05 amendment entry.

### `dev/nssf/open-gaps-and-assumptions.md`

USER summary-body 영역에 `### DOCX-GAP phase4 흡수` addendum — 흡수/defer 매핑 + OPUS-TENSION-001 명문화.

## 결정 / tension

- **OPUS-TENSION-001 (redirect)**. Opus 의 callback 307 retry row 는 채택하지 않는다. 현 ADR-0004 no-follow / fail-closed ratification 우선 — redirect 미구현/unsupported 선언이 정답. phase4 row 는 "no accidental 3xx" 증명 방향.
- **GAP-003/006/009 defer**. phase5 operator-guide 로 이관. 단 GAP-003 은 phase4 contract negative-claim marker, GAP-009 는 phase4 security wording-guard label 로 phase4 에 흔적을 남긴다 (overclaim 방지).
- **신규 slice 금지**. 미구현 procedure/feature variant 는 phase4 test 의 explicit unsupported/deferred row 로 처리. 실제 구현 확장은 phase4 finding 후 별도 operator 결정 (crosswalk `possible_future_slices_after_phase4_findings`).

## 검증 / exit

- `validate-execution-control-drift.py nssf` 5/5 PASS.
- `nf-readiness-pack-generate.py nssf --check` PASS (open-gaps USER zone preserved).
- YAML valid, picker next 불변 (phase4 slice deps 무변).
- plan-amendment PR open → **STOP, operator review**. self-merge 금지.
