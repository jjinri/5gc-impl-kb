---
name: nf-eng-status
description: 본 5gc-impl-kb 의 Engineering Design Freeze 산출 (engineering/<nf>/engineering-design.md) 이 `eng_frozen` 게이트를 만족하는지 검사해 `engineering/<nf>/_engineering_status.yaml` 산출하는 워크플로우. 사용자가 "/nf-eng-status nssf", "NSSF eng 검증", "eng_frozen 확인", "engineering design freeze 검사", "자율 코드 생성 가능?" 등을 말하거나 NF 이름 + engineering 검사를 지정하면 이 skill 을 사용한다. 동작 — `design/scripts/nf-eng-status.py <nf>` 호출 → `engineering/<nf>/_engineering_status.yaml` 갱신 + 콘솔에 `gates.eng_frozen` 과 비차단 `advisory.impl_plan_alignment` 보고. contract 검사는 `/nf-contract-check`, architecture 는 `/nf-arch-status`, implementation-planning 은 `/nf-impl-status` 의 책임이며 본 skill 은 Engineering Design Freeze 단계 *측정* 에 집중한다. 산출 생성은 `/nf-eng-design`.
argument-hint: "<nf> [--no-write]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-eng-status.py *) Bash(cat *) Bash(ls *)
---

# nf-eng-status — Engineering Design Freeze 자기 일관성 검사

Engineering Design Freeze 단계 status 검사다 (ADR-0002). contract=`/nf-contract-check`, architecture=`/nf-arch-status`, implementation-planning=`/nf-impl-status` 와 책임이 분리된다. `eng_frozen` 은 **자율 코드 생성의 유일한 GO 신호**다.

## 입력
- `<nf>` — NF 이름.
- `--no-write` — `_engineering_status.yaml` 저장 없이 stdout 만. 일회성 점검.

## 본 skill 의 책임 범위

본 skill 은 *측정* 만 한다. 산출을 *수정* 하지 않는다 — `_engineering_status.yaml` 은 기계가 매번 새로 쓰는 보고서이며 사용자가 직접 편집하지 않는다 (`manual_overrides.pass_anyway` 제외).

| 시나리오 | 사용 skill |
| --- | --- |
| contract 완성도 검사 | `/nf-contract-check` |
| architecture 자기 일관성 | `/nf-arch-status` |
| implementation-planning 자기 일관성 | `/nf-impl-status` |
| Engineering Design 산출 생성·갱신 | `/nf-eng-design` |
| Engineering Design Freeze 검사 | `/nf-eng-status` (본 skill) |

## 동작 원칙 (이유 포함)

- **CLAUDE.md 정책 + ADR-0002 가 우선.**
- **read-only.** 측정과 수정이 같은 skill 에 있으면 측정이 수정에 흔들린다 — 생성·갱신은 `/nf-eng-design` 의 책임.
- **blocking 은 결정론.** `gates.eng_frozen` 은 binary check 의 AND 로만 결정된다. LLM/sub-agent judge 는 blocking 판정에 절대 들어가지 않는다 (ADR-0002 Decision §2). 의미 잔여 위험은 비차단 `advisory` 가 표시만 한다.
- **namespace 분리.** 출력의 `gates.eng_frozen` (PASS/FAIL, 자율 코드 생성 GO) 과 `advisory.impl_plan_alignment` (PASS/WARN/SKIP, 비차단) 는 절대 섞지 않는다. advisory 가 시간이 지나 blocking 처럼 오염되면 안 된다.
- **inventory 하드코딩 금지.** core slot 은 `design/schemas/engineering-core-slots.yaml` profile (pipeline policy) 을 read-only 로 읽고, NF-specific 은 per-NF 연기 레지스터 (`design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices`) 에서 흡수한다. 둘의 합집합이 inventory.
- **deferral register 는 F-hard 입력.** 누락·파싱 실패는 조용한 PASS 가 아니라 `eng_frozen` FAIL 이다.

## Workflow

### 1. 입력 검증
- `<nf>` 비어있으면 묻고 정지.
- `design/<nf>/` 존재 확인 (F architecture 의 연기 레지스터가 입력). 없으면 "F architecture 먼저" 안내 후 정지.

### 2. 실행
```bash
.venv/bin/python3 design/scripts/nf-eng-status.py <nf>
```
- `--no-write` 면 `_engineering_status.yaml` 미저장, stdout 만.
- script 가 `eng_frozen` FAIL 이면 exit 1.
- 검사 항목·gate 정의의 진실 출처는 `design/scripts/nf-eng-status.py` (`GATE_DEFS`, `chk_*`). 본 SKILL 에 재서술하지 않는다.

### 3. 결과 보고
사용자에게 한 화면에 묶어 전달.

- **`gates.eng_frozen`** — PASS 면 *자율 코드 생성 진입 가능*, FAIL 이면 `blocked_by` 의 check.
- **상위 FAIL check 의 to_pass** — 다음에 무엇을 `/nf-eng-design` 으로 고쳐야 하는지.
- **`advisory.impl_plan_alignment`** — PASS / WARN / SKIP. *비차단* 임을 명시 (WARN 이어도 eng_frozen 은 PASS 가능).
- `_engineering_status.yaml` 위치.

### 4. 다음 액션 안내
- `eng_frozen` FAIL → `blocked_by` 의 첫 항목부터 `/nf-eng-design <nf>` 로 보강 후 재실행.
- `eng_frozen` PASS → "자율 코드 생성 진입 가능. advisory 는 비차단 참고."

## 자주 틀리는 지점
- 사용자가 `_engineering_status.yaml` 을 직접 편집 — 금지. 기계 재생성물. `to_pass` 를 따라 *engineering-design.md* 를 고치고 재실행.
- advisory.impl_plan_alignment WARN 을 blocking 으로 오해 — 비차단. eng_frozen 판정과 무관.
- `manual_overrides.pass_anyway` 에 우회 시 `{check, reason, confirmed_by}` 누락 — 셋 다 필수.

## 참고 — 본 skill 안에 다시 적지 말 것
- 검사 항목·gate·typed shape 규칙: `design/scripts/nf-eng-status.py` docstring + 함수.
- core slot profile: `design/schemas/engineering-core-slots.yaml`.
- 단계 정의·트레이드오프: `docs/adr/ADR-0002-engineering-design-freeze.md`.
- 산출 생성: 자매 skill `/nf-eng-design`.
