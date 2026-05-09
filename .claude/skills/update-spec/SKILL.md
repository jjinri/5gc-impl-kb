---
name: update-spec
description: 본 LLM Wiki 의 NF 폴더(`wiki/{nf}/`) 를 *지식 보강* 하는 워크플로우. 사용자가 "NSSF 보강", "nssf 페이지 업데이트", "AMF wiki 채워줘", "데이터 모델만 다시 뽑아", "papers 에 새 ref 추가했으니 반영", "wiki 빈 곳 채워" 등을 말하거나 NF 이름 (`nssf`, `amf`, `smf` 등) 을 지정하면 무조건 이 skill 을 사용한다. 동작 — 지정 NF 의 `wiki/{nf}/*.md` 페이지마다 (1) 완성되지 않은 지식 (placeholder, 비어있는 섹션, "참조 규격 미등록" leaf, 빠진 mermaid, 빠진 Data Model 트리) 을 자동 식별 (2) 그 사이 papers/ 에 새로 들어온 소스 (.docx 신규 버전, .yaml 신규 등록, 이전엔 부재했던 참조 spec 폴더 등장) 가 있는지 비교 (3) 채울 수 있는 것은 채우고, 여전히 missing 인 부분은 "필요 규격 문서 업데이트 필요" 로 정리해 사용자에게 보고한다. 옵션 — `--data-model` 은 Data Model 트리만 따로 재추출·교체 (가장 자주 쓰이는 부분 보강). 신규 spec 시리즈를 처음 등록하는 작업은 sibling `/add-spec` 의 책임이며, 본 skill 은 *이미 존재하는* wiki 페이지의 빈 곳을 채우는 데 집중한다. 커밋은 절대 자동으로 하지 않는다.
argument-hint: "<nf> [--data-model] | --data-model | (empty)"
allowed-tools: Bash(.venv/bin/python3 scripts/extract.py *) Bash(.venv/bin/python3 scripts/resolve-yaml-refs.py *) Bash(.venv/bin/python3 scripts/render-mermaid.py *) Bash(mkdir -p *) Bash(ls *) Bash(find *) Bash(grep *) Bash(awk *) Bash(diff *)
---

# update-spec — NF 단위 wiki 지식 보강

## 입력
- `<nf>` — `nssf`, `amf`, `smf` 등 wiki/{nf}/ 폴더명. 대소문자 무관, 내부적으로 소문자.
- `--data-model` — Data Model 트리만 재추출해 교체. 다른 섹션은 건드리지 않는다.
- 빈 값 + `--data-model` 없음 → wiki/ 의 모든 NF 폴더에 대해 gap 탐지 결과를 사용자에게 표로 보여주고 어느 NF 를 보강할지 묻고 정지.
- 빈 값 + `--data-model` → wiki 전체에서 Data Model 트리만 일괄 재추출 (모든 NF 의 모든 페이지). 비파괴이지만 시간이 들 수 있음.
- `<nf>` + `--data-model` → 그 NF 의 모든 페이지의 Data Model 트리만 재추출.
- `<nf>` 단독 → 그 NF 의 모든 페이지에 대해 *전체 gap 보강* (Data Model + Service Flows + Version History + missing-refs 재평가).

## 본 skill 의 책임 범위

본 skill 은 *이미 존재하는* wiki 페이지의 빈 곳을 채운다. 신규 시리즈 등록은 `/add-spec <spec>` 의 책임. 두 skill 은 다음과 같이 분담.

| 시나리오 | 사용 skill |
| --- | --- |
| 새 spec 시리즈 처음 등록 (`papers/{spec}/` 만 있고 wiki 페이지 없음) | `/add-spec <spec>` |
| 같은 시리즈에 새 release 가 들어왔고 Version History 에 추가하고 싶음 | `/update-spec <nf>` (또는 `/add-spec` 의 명시 파일경로 모드) |
| 본 페이지 만들 때 missing leaf 였던 외부 spec 이 papers/ 에 새로 cp 됨 | `/update-spec <nf> --data-model` |
| 도구 (resolve-yaml-refs.py, render-mermaid.py) 가 발전해 트리 표현이 바뀜 | `/update-spec <nf> --data-model` |
| Data Model 외 다른 섹션도 결락이 있다 (mermaid 미렌더, Service Flows placeholder 등) | `/update-spec <nf>` |

## 절대 규칙
1. CLAUDE.md THE FOUR RULES 를 따른다 (web search 금지, sources/wiki·papers 만 진실).
2. 절차의 진실 출처는 `add-spec/SKILL.md` 의 §4·§5 (sources / wiki 작성) + 두 helper 스크립트 (`scripts/resolve-yaml-refs.py`, `scripts/render-mermaid.py`). 본 skill 은 그 산출을 *교체* 만 하고 새로운 형식을 만들어내지 않는다.
3. 사용자 페이지의 *수동 편집 본문* (Summary / Key Contributions / Methodology and Architecture 등 산문) 은 본 skill 이 임의로 덮어쓰지 않는다. 보강은 *기계 산출 섹션* (Data Model 트리, mermaid SVG, missing-refs 목록, Version History 의 자동 집계 부분) 에 한정.
4. 모호한 보강 (예 "이 placeholder 자리에 무엇을 채울지 모르겠음") 은 사용자에게 묻고 정지. silently 추측해 채우지 않는다.
5. 커밋 자동 수행 금지.

---

## Workflow

### 1. 입력 해석 → 대상 페이지 리스트 확정

- `<nf>` 가 주어지면 `wiki/{nf}/*.md` 의 모든 파일 (단, `_diagrams/` 제외) 을 대상으로.
- `<nf>` 가 비고 `--data-model` 도 없으면 NF 별 gap 요약을 보여주고 묻기.
- `<nf>` 가 비고 `--data-model` 만 있으면 `wiki/**/*.md` 전체를 대상으로 (단 NF 폴더 안만, `_diagrams/` 제외).

### 2. 페이지별 Gap 탐지

각 대상 페이지에 대해 다음을 확인한다.

#### 2a. 메타데이터 변동
- 페이지 frontmatter 의 `version`·`source_filename` 이 `papers/{spec}/` 의 *최신 파일* 과 다른가? 다르면 새 release 가 들어온 것 — Version History 갱신 후보.
- 페이지 frontmatter 의 `related_versions` 와 papers/{spec}/ 의 옛 버전 파일들이 동기화되어 있는가?

#### 2b. Data Model 트리 신선도
- 페이지의 ` ```text ` 코드블록 (Data Model 트리) 들과, 같은 frontmatter 의 spec yaml 을 `resolve-yaml-refs.py` 로 *지금* 다시 추출한 결과를 텍스트 비교 (`diff`).
- 차이가 있으면 두 가지 원인 — (i) papers/ 에 새로 cp 된 ref spec 이 있어 chain 이 더 풀림, (ii) 도구가 발전해 표현이 달라짐. 어느 쪽이든 *교체* 가 답이다.
- 페이지에 Data Model 섹션 자체가 없거나 placeholder 상태면 "Data Model 결락" 으로 마크.

#### 2c. Missing-refs 재평가
- 페이지의 `(참조 규격 미등록)` leaf 들을 grep.
- 각 leaf 의 spec 번호 (예 `TS 29.503`) 가 *지금 papers/{spec}/* 에 존재하는가? 존재하면 chain 이 풀릴 후보 — Data Model 재추출로 자동 해결됨 (2b 에서 처리됨).
- 여전히 부재한 spec 은 보고에 다시 옮긴다.

#### 2d. Service Flows 결락
- ` ```mermaid ` 블록 개수 0 이면 "Service Flows 결락" 으로 마크.
- mermaid 블록은 있는데 sibling `_diagrams/<page>-<n>.svg` 가 그 수보다 적으면 "SVG 미렌더" 로 마크 → 보강 단계에서 `render-mermaid.py` 호출.
- mermaid 본문 자체의 보강 (새 흐름 추가) 은 본 skill 의 자동 책임 *아님* — 사용자에게 "Service Flows 본문 추가가 필요해 보임" 으로 안내만.

#### 2e. 기타 placeholder
- `_(아직 ...)_` 또는 `TODO` / `FIXME` 같은 명시적 placeholder 가 있는지 grep. 있으면 결락 마크 후 사용자에게 보고만.

### 3. 사용자 확인 (빈 인자 또는 `<nf>` 단독 시)

탐지 결과를 페이지·항목별 표로 보여주고 어떤 보강을 적용할지 묻는다.

```
[wiki/nssf/3gpp-ts-29531.md]
  - Data Model 트리 신선도   : 변경 있음 (TS 29.503 chain 추가 풀림)
  - Missing-refs            : (없음)
  - Service Flows           : 3 블록, SVG 3 정상
  - Version History         : 최신 (papers/29.531 의 j60 = v19.6.0 매칭)
  - placeholder             : (없음)
```

`Y` (모두 적용) / `n` (정지) / 항목별 토글 등 옵션. `--data-model` 모드면 자동으로 그 항목만.

### 4. 보강 적용

#### 4a. Data Model 트리 교체

- 페이지 frontmatter 의 `source_path` 또는 `source_filename` 으로 spec 폴더 추정 → `papers/{spec}/*.yaml` 들을 모두 입력으로 `resolve-yaml-refs.py` 호출.
- 출력 ` ```text ` 블록(들) 을 페이지의 `## Data Model` 섹션 안의 같은 위치 블록으로 *교체*.
- 손으로 적은 산문 설명 (트리 위 한두 단락) 은 보존. 트리 코드블록만 교체.

#### 4b. SVG 재렌더

- 변경 페이지 (또는 mermaid 가 있는 페이지) 에 대해 `python3 scripts/render-mermaid.py [--clean]` 실행.

#### 4c. Version History 추가

- papers/{spec}/ 에 새 release 가 발견된 경우 — `/add-spec papers/{spec}/{newer-file}` 의 명시 파일경로 모드를 *내부적으로 위임* 호출. add-spec 은 기존 wiki 페이지가 있으면 frontmatter 의 `version`·`source` 갱신, `related_versions` 에 이전 버전 push, `## Version History` 에 한 줄 추가 동작을 한다 (add-spec/SKILL.md §5 참조).
- 본 skill 자체가 Version History 본문을 손으로 만들지 않는다 — add-spec 위임이 진실 출처.

#### 4d. Missing-refs 목록 갱신

- Data Model 교체로 자동 해결된 항목은 페이지에서 사라진다 (`resolve-yaml-refs.py` 가 새로 풀어낸 트리에 leaf 가 없을 것).
- 여전히 미등록인 항목은 페이지의 "미등록 reference" 절을 갱신.

### 5. 결과 보고 (커밋 X)

페이지별로.
- 적용된 보강 (예 "Data Model 교체: +14 줄, -7 줄. TS 29.503 chain 새로 풀림").
- 적용되지 않은 항목 + 사유 ("Service Flows 본문 추가는 본 skill 책임 아님 — `/add-spec` 으로 페이지 재생성 필요").
- 여전히 missing 인 외부 spec 목록 (사용자가 papers/ 에 cp 하면 다음 호출에서 풀릴 후보).
- 제안 commit 메시지.

**커밋 자동 수행 금지.**

---

## 자주 틀리는 지점 — 사전 점검

- `--data-model` 모드인데 트리 외 다른 섹션을 만지지 않았는가.
- 사용자 산문 (Summary / Key Contributions 등) 을 보존했는가.
- Data Model 교체 시 *코드블록 전체* 만 교체하고 그 위 산문 설명은 그대로 두었는가.
- mermaid SVG 와 mermaid 블록 수가 일치하는가 (`render-mermaid.py --clean` 으로 stale SVG 정리).
- 같은 spec 의 새 release 추가는 본 skill 이 손으로 하지 않고 `/add-spec` 위임으로 했는가.
- 모호한 보강에서 추측해 채우지 않고 사용자에게 물었는가.

## 실패·모호 처리

- frontmatter 가 깨져 있거나 `source_filename` 이 비면 그 페이지는 보강 대상에서 제외, 사용자에게 알린다.
- yaml 이 papers/ 에 있는데 schema 이름을 결정 못 하는 경우 (예 yaml 안에 schema 가 0 개) 도구 산출 실패 — 페이지 그대로 두고 사용자에게 보고만.
- diff 가 너무 큰 경우 (수백 줄 변경) 자동 적용하기 전에 사용자에게 *요약 + 적용 여부* 를 묻는다.

## 참고 — 본 skill 안에 다시 적지 말 것

- sources / wiki 작성 정책: `add-spec/SKILL.md` 의 Workflow §4·§5.
- Data Model 체인 추적: `scripts/resolve-yaml-refs.py` docstring + `--help`.
- Mermaid 렌더링: `scripts/render-mermaid.py` docstring.
- 디렉터리·파일명 규칙: `CLAUDE.md` 의 "Repository Structure"·"File Naming Convention"·"Categories".
