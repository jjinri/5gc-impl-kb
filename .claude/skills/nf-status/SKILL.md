---
name: nf-status
description: 본 5gc-impl-kb 의 NF 페이지가 implementation-grade 완성도를 만족하는지 검사해 `_status.yaml` 산출하는 워크플로우. 사용자가 "/nf-status nssf", "NSSF 검증", "NRF 페이지 점수 알려줘", "이 NF 빌드 가능?", "implementation 가능 검사", "wiki check" 등을 말하거나 NF 이름을 지정하면 무조건 이 skill 을 사용한다. 동작 — `scripts/nf-status.py <nf>` 호출 → `kb/<nf>/_status.yaml` 갱신 + 콘솔에 acceptance gate 보고. 평가 framework — 가중치 없음·항목별 criterion + to_pass 의무·NF profile 별 NOT_APPLICABLE 처리. Tier 1 (validation, binary), Tier 2 (coverage, threshold), Tier 3 (yaml-to-c viability, NOT_RUN 도구 미존재), Tier 4 (subjective, SKIPPED). 매니페스트 생성은 `/nf-init`, 페이지 빌드는 `/nf-build` 의 책임이며 본 skill 은 *완성도 측정* 에 집중한다.
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

## 동작 원칙 (이유 포함)

- **CLAUDE.md THE FOUR RULES 가 우선.**
- **모든 check 는 `criterion` (PASS 정의) + `to_pass` (FAIL 시 다음 액션) 의무.** 이유 — FAIL 만 보고하면 사용자가 "그래서 뭘 해야 하지?" 를 매번 추측해야 한다. `to_pass` 가 즉시 다음 backlog 항목이 되도록.
- **가중치·총점 산출 안 함, acceptance gate 는 check id 의 AND.** 이유 — 가중치는 사람마다 다르고 시간이 지나면 번복된다. *어떤 check 의 집합이 통과해야 어떤 단계인가* 만 설계하면 의견 다툼이 줄고 추적이 명확.
- **NOT_APPLICABLE 은 PASS 와 동격으로 gate 계산.** 이유 — stage 2 NF 는 yaml 이 없어 `data_model_chain_complete` 자체가 부적용이다. NOT_APPLICABLE 을 FAIL 로 두면 영구 blocker 가 되어 framework 가 정확하지 않게 된다.
- **본 skill 은 *측정만*, 페이지를 수정하지 않는다.** 이유 — 측정과 수정이 같은 skill 에 있으면 측정 결과가 수정에 의해 흔들린다. 다른 skill (`/nf-build`) 의 책임 분리.

## Workflow

### 1. 입력 검증
- `<nf>` 가 비어있으면 사용자에게 묻고 정지.
- `kb/<nf>/` 디렉터리 존재 확인. 없으면 "/nf-init <nf> --primary <spec> 먼저" 안내 후 정지.

### 2. 도구 실행
```bash
.venv/bin/python3 scripts/nf-status.py <nf> [--no-write]
```
- 도구가 `kb/<nf>/_status.yaml` 갱신 + stdout/stderr 로 보고.
- 검사 항목 (도구 docstring 진실 출처) — Tier 1·2 자동, Tier 3 (`yaml_to_c_compiles`) 도 도구가 `scripts/yaml-to-c.py` 자동 호출, Tier 4 (`implementation_guidance_quality`) 는 본 SKILL 의 §2.5 가 처리.

### 2.5 Tier 4 — sub-agent judge (선택)
`implementation_guidance_quality` 가 NOT_RUN 이고 사용자가 production gate 까지 진행을 원하면, main agent 가 sub-agent (general-purpose) 에 페이지 채점을 위임한다.

- 입력 — `kb/<nf>/3gpp-*.md` 본문 + `_manifest.yaml`.
- 채점 기준 — implementation 가이던스 품질 (1-5 점). 5 = 본 페이지만으로 구현 시작 가능, 4 = 약간의 spec 참고로 구현 가능, 3 = 페이지 보강 필요, 2 이하 = 빌드 재진행 필요.
- sub-agent 산출 — `score`, `judged_by` (예 "sub-agent"), `rationale` (한 문단).
- main agent 가 결과를 `kb/<nf>/_manifest.yaml` 의 `manual_overrides.judge_result` 에 등록.
- `/nf-status <nf>` 재실행 → `nf-status.py` 가 그 field 를 보고 PASS/FAIL 판정 (`score >= 4` PASS).

**언제 호출하는가.** 본 단계는 *명시적*. 매 nf-status 호출마다 자동 트리거하지 않는다 — sub-agent 채점은 비용·시간이 들고 페이지가 크게 변하지 않은 이상 결과가 안정적이기 때문. 사용자가 "production 까지 가자" 또는 페이지 큰 갱신 후일 때만 호출.

**대안 — 사람 리뷰.** Claude Code 외 환경에서는 `manual_overrides.judge_result` 에 사람이 직접 등록 (`judged_by: <reviewer>`).

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

## 출력 형식 — 콘솔 보고 템플릿

`_status.yaml` 자체는 도구가 만든다. 본 skill 이 *콘솔* 에 사용자에게 보여줄 형식.

```text
[nf-status] {nf}: PASS {n}, FAIL {m}, NOT_APPLICABLE/NOT_RUN {k}

  gate draft:                {PASS|FAIL [— blocked by ...]}
  gate ready_for_review:     {PASS|FAIL [— blocked by ...]}
  gate implementation_ready: {PASS|FAIL [— blocked by ...]}
  gate production:           {PASS|FAIL [— blocked by ...]}

  상위 FAIL — 다음에 손대야 할 항목 (각 to_pass 첫 줄):
    1. {check_id}: {to_pass[0]}
    2. {check_id}: {to_pass[0]}
    3. {check_id}: {to_pass[0]}

  상세 — kb/{nf}/_status.yaml
```

## 예시

```
사용자: /nf-status nssf
도구:   scripts/nf-status.py nssf
산출:   kb/nssf/_status.yaml
보고:   "[nf-status] nssf: PASS 4, FAIL 5, NOT_APPLICABLE/NOT_RUN 2
         gate draft: PASS
         gate ready_for_review: FAIL — blocked by [sections_complete, manifest_ready]
         gate implementation_ready: FAIL — blocked by [..., data_model_chain_complete, ...]
         gate production: FAIL — ...

         상위 FAIL —
           1. sections_complete: 누락된 H2 추가 — [Interface, API, Cross-NF, Configuration, Error Handling]
           2. manifest_ready: specs/<spec>/ 에 cp 또는 manual_overrides.exclude 등록
           3. data_model_chain_complete: papers/29.503 cp 후 /nf-build nssf --data-model

         상세 — kb/nssf/_status.yaml"
```

## 자주 틀리는 지점

- 사용자가 `_status.yaml` 을 직접 편집했는가 (그러면 안 됨 — 다음 호출에 덮어쓰여짐).
- `manual_overrides.pass_anyway` 에 우회를 넣을 때 사유·confirmed_by 가 없는가 (있어야 함).
- profile 이 NF 성격과 맞는가 (`stage_3_only` 가 기본, NWDAF 같은 mixed NF 는 매니페스트에서 명시 필요).

## 참고 — 본 skill 안에 다시 적지 말 것

- 검사 항목·criterion·to_pass 형식: `scripts/nf-status.py` docstring + 같은 파일의 함수.
- acceptance gate 정의: `scripts/nf-status.py` 의 `GATE_DEFS`.
- NF profile 매트릭스: `scripts/nf-status.py` 의 `applies_to` + CLAUDE.md "NF Profile" 표.
