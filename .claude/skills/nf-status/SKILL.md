---
name: nf-status
description: 본 5gc-impl-kb 의 NF 페이지가 implementation-grade 완성도를 만족하는지 검사해 `_status.yaml` 산출하는 워크플로우. 사용자가 "/nf-status nssf", "NSSF 검증", "NRF 페이지 점수 알려줘", "이 NF 빌드 가능?", "implementation 가능 검사", "wiki check" 등을 말하거나 NF 이름을 지정하면 무조건 이 skill 을 사용한다. 동작 — `scripts/nf-status.py <nf>` 호출 → `kb/<nf>/_status.yaml` 갱신 + 콘솔에 acceptance gate 보고. 평가 framework 는 ADR 0001 D7 — 가중치 없음·항목별 criterion + to_pass 의무·NF profile 별 NOT_APPLICABLE 처리. Tier 1 (validation, binary), Tier 2 (coverage, threshold), Tier 3 (yaml-to-c viability, NOT_RUN 도구 미존재), Tier 4 (subjective, SKIPPED). 매니페스트 생성은 `/nf-init`, 페이지 빌드는 `/nf-build` 의 책임이며 본 skill 은 *완성도 측정* 에 집중한다.
argument-hint: "<nf> [--no-write]"
allowed-tools: Bash(.venv/bin/python3 scripts/nf-status.py *) Bash(cat *) Bash(ls *)
---

# nf-status — implementation-grade 완성도 검사

## 입력
- `<nf>` — NF 이름.
- `--no-write` — `_status.yaml` 저장 없이 stdout 만. 일회성 점검에 유용.

## 본 skill 의 책임 범위

본 skill 은 *측정* 만 한다. 어떤 작업도 *수정* 하지 않는다 — 산출 `_status.yaml` 은 기계가 매번 새로 쓰는 보고서이며 사용자가 직접 편집하지 않는다.

| 시나리오 | 사용 skill |
| --- | --- |
| 매니페스트 생성·보강 | `/nf-init` |
| 페이지 빌드·갱신 | `/nf-build` |
| 페이지 완성도 검사 | `/nf-status` (본 skill) |

## 절대 규칙
1. CLAUDE.md THE FOUR RULES 를 따른다.
2. `_status.yaml` 의 모든 check 는 *항목별 criterion + to_pass* 보유. 본 skill 은 그 결과를 그대로 사용자에게 전달.
3. FAIL check 의 `to_pass` 는 *구체 액션 리스트* — silent FAIL 금지.
4. 가중치·총점 산출하지 않는다. acceptance gate 는 check id 의 AND.
5. NOT_APPLICABLE 은 자동 만족 (gate 계산에서 PASS 와 동격).

## Workflow

### 1. 입력 검증
- `<nf>` 가 비어있으면 사용자에게 묻고 정지.
- `kb/<nf>/` 디렉터리 존재 확인. 없으면 "/nf-init <nf> --primary <spec> 먼저" 안내 후 정지.

### 2. 도구 실행
```bash
.venv/bin/python3 scripts/nf-status.py <nf> [--no-write]
```
- 도구가 `kb/<nf>/_status.yaml` 갱신 + stdout/stderr 로 보고.
- 검사 항목 (도구 docstring 진실 출처) — Tier 1 6건, Tier 2 3건, Tier 3·4 placeholder.

### 3. 결과 보고
사용자에게 다음을 한 화면에 묶어 전달.

- **gate 상태 한 줄** — draft / ready_for_review / implementation_ready / production 각각 PASS / FAIL.
- **FAIL gate 의 blocked_by** — 어느 check 가 막고 있는지.
- **Tier 별 PASS/FAIL/NOT_APPLICABLE 카운트**.
- **상위 3건 FAIL 항목 의 to_pass** — 실제로 다음에 무엇을 해야 하는지.
- **`_status.yaml` 위치** — 사용자가 직접 보고 싶을 때.

### 4. 다음 액션 안내
gate 상태에 따라.
- 모든 gate FAIL → blocked_by 의 첫 항목부터 손대도록 안내.
- ready_for_review PASS 만 → "draft 통과. implementation_ready 까지 5건 남음" 같이 진행도 표시.
- implementation_ready PASS → "구현 가능 수준 달성. production 은 Tier 3·4 도구 갖춰지면 자동 평가".

## 자주 틀리는 지점

- 사용자가 `_status.yaml` 을 직접 편집했는가 (그러면 안 됨 — 다음 호출에 덮어쓰여짐).
- `manual_overrides.pass_anyway` 에 우회를 넣을 때 사유·confirmed_by 가 없는가 (있어야 함).
- profile 이 NF 성격과 맞는가 (`stage_3_only` 가 기본, NWDAF 같은 mixed NF 는 매니페스트에서 명시 필요).

## 참고 — 본 skill 안에 다시 적지 말 것

- 검사 항목·criterion·to_pass 형식: `scripts/nf-status.py` docstring + 같은 파일의 함수.
- acceptance gate 정의: `scripts/nf-status.py` 의 `GATE_DEFS`.
- NF profile 매트릭스: ADR 0001 D7 + `scripts/nf-status.py` 의 `applies_to`.
