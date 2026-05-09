---
name: nf-init
description: 본 5gc-impl-kb 의 새 NF (Network Function) 작업을 *시작* 하는 워크플로우. 사용자가 "NSSF 시작", "/nf-init nrf", "AMF 매니페스트 만들어", "amf 구현 시작", "create nf manifest", "29.510 으로 NRF 작업 시작" 등을 말하거나 NF 이름 + 주 spec 번호를 지정하면 무조건 이 skill 을 사용한다. 동작 — (1) `scripts/nf-manifest.py <nf> --primary <spec> --write` 호출 (2) 산출 매니페스트 (`kb/<nf>/_manifest.yaml`) 의 status 를 사용자에게 보고 (3) `ready_for_build = true` 가 되려면 어느 spec 을 specs/ 에 추가해야 하는지 priority 순으로 알린다. 본 skill 은 *반복 가능* 하다 — 사용자가 새 spec 을 specs/ 에 cp 한 뒤 다시 호출하면 매니페스트가 보강된다. ready 가 될 때까지 반복하고, 그 다음 `/nf-build` 로 페이지 생성. 신규 NF 의 매니페스트 *생성* 만 한다 — 페이지 빌드는 `/nf-build`, 완성도 검사는 `/nf-status` 의 책임.
argument-hint: "<nf> --primary <spec>"
allowed-tools: Bash(.venv/bin/python3 scripts/nf-manifest.py *) Bash(ls *) Bash(cat *)
---

# nf-init — NF 매니페스트 생성·보강

## 입력
- `<nf>` — `nssf`, `nrf`, `amf` 등 NF 이름 (소문자, kb/ 하위 폴더명).
- `--primary <spec>` — NF 의 주 spec 번호 (점 포함, 예 `29.531`). 필수.
- 인자 없으면 어느 NF 인지·주 spec 이 무엇인지 사용자에게 묻고 정지.

> 다중 primary spec (예 NWDAF = 23.288 + 29.520) 은 sprint 후반에 도구 인자 확장 예정. 현재는 *대표 1개* 로 시작하고 나머지는 매니페스트에서 docx_clause_2 references 로 자동 검출.

## 본 skill 의 책임 범위

본 skill 은 매니페스트 *생성·보강* 에 한정. *페이지 빌드는 `/nf-build`, 완성도 검사는 `/nf-status`* 의 책임.

| 시나리오 | 사용 skill |
| --- | --- |
| 새 NF 작업 처음 시작 | `/nf-init <nf> --primary <spec>` |
| specs/ 에 새 ref spec cp 후 매니페스트 갱신 | `/nf-init <nf> --primary <spec>` (재실행) |
| 매니페스트가 ready 됐고 페이지 생성 차례 | `/nf-build <nf>` |
| 페이지가 implementation 기준 통과하는지 점검 | `/nf-status <nf>` |

## 절대 규칙
1. CLAUDE.md THE FOUR RULES 를 따른다.
2. 매니페스트 자동 검출 결과는 *제안*. 사용자가 `manual_overrides.exclude` / `add` 를 통해 조정 가능. 본 skill 은 사용자 수정을 *덮어쓰지 않는다* (도구가 manual_overrides 를 보존).
3. ready_for_build = true 가 아니면 사용자에게 무엇을 추가해야 하는지 명확히 알린다 — silent 진행 금지.

## Workflow

### 1. 인자 검증
- `<nf>` 가 비어있거나 `--primary` 가 없으면 사용자에게 묻고 정지.
- `<spec>` 의 형식 — `^\d{2}\.\d{3}$` (점 포함). 다른 형식이면 정지.

### 2. specs/ 폴더 실재 확인
- `specs/<spec>/` 디렉터리 존재 + .docx/.pdf/.doc 1개 이상 보유 확인.
- 없으면 정지 — "먼저 `specs/<spec>/` 에 원본 cp 후 재호출".

### 3. 도구 실행
```bash
.venv/bin/python3 scripts/nf-manifest.py <nf> --primary <spec> --write
```
- `--write` 로 `kb/<nf>/_manifest.yaml` 저장.
- 도구 stderr 의 status 라인을 사용자에게 그대로 전달.

### 4. 결과 보고
- 매니페스트 위치 — `kb/<nf>/_manifest.yaml`.
- `manifest_completeness` 비율 (예 `5/11`).
- `ready_for_build` (true/false).
- false 면 `missing_priority` 리스트 + 각각의 의미 (sba_common / data_types / security 등).
- *다음 추천 액션* — (a) priority 순으로 specs/ 에 cp 후 재호출 (b) 또는 manual_overrides.exclude 에 등록 후 재호출.

### 5. 마무리
- 사용자가 ready 시키면 `/nf-build <nf>` 가 다음 단계. 본 skill 은 거기로 *자동 위임하지 않는다* — ready_for_build 결과를 사용자가 확인 후 별도 호출.
- 매니페스트가 이미 ready 였으면 — "이미 빌드 가능. `/nf-build <nf>` 로 진행" 안내.

## 자주 틀리는 지점

- 사용자의 `manual_overrides` 가 도구 재실행 후 사라졌는가. (도구가 보존하므로 사라지면 안 됨 — 사라졌다면 도구 회귀 의심.)
- `<nf>` 와 `<spec>` 의 관계가 맞는가 (29.531 ↔ nssf, 29.510 ↔ nrf, 29.503 ↔ udm 등). 다른 조합은 사용자에게 확인.

## 참고 — 본 skill 안에 다시 적지 말 것

- 매니페스트 schema·자동 검출 알고리즘: `scripts/nf-manifest.py` docstring.
- 카테고리 분류 표 (CATEGORY_TABLE): 같은 도구 안.
- 디렉터리·파일명 규칙: `CLAUDE.md`.
