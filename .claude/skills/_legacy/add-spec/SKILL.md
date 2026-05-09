---
name: add-spec
description: 3GPP 5GC spec 한 *시리즈* 를 본 wiki 에 등록·완성하는 통합 워크플로우. 사용자가 "TS NN.NNN 추가", "29.531 wiki 만들어줘", "NSSF spec 정리해줘", "spec 추가", "wiki 에 등록", "add this spec" 등을 말하거나 spec 번호 (`29.531`) / spec 폴더 경로 (`specs/29.531`) 를 직접 지정하면 무조건 이 skill 을 사용한다. 동작 — specs/{spec}/ 의 가장 최신 docx/pdf/doc 와 같이 들어 있는 OpenAPI yaml(있으면) 까지 모두 끌어와 (1) 텍스트 추출 (2) sources 한국어 요약 (3) wiki 본 페이지 (Service Flows mermaid + Data Model chain 트리 등) 를 한 번에 생성한다. Data Model 의 `$ref` 체인은 specs/ 안의 *다른 spec yaml 까지* 끝까지 추적해 자료형 트리를 가능한 만큼 완성한다. 추적 도중 specs/ 에 없는 외부 spec 을 만나면 자동으로 leaf 로 표기하고 결과 보고에 "필요 규격 문서 업데이트 필요" 목록으로 정리해 사용자가 어느 spec 을 cp 하면 트리가 더 풀릴지 알 수 있게 한다. 단순 텍스트 추출(`extract.py` 직접) 과는 다르며 본 skill 은 sources·wiki·index.md 동기화 + 가능한 모든 외부 chain 자동 보강 + missing-refs 보고를 한 번에 한다. 커밋은 사용자 검토 후 별도. *이미 존재하는* wiki 페이지의 빈 곳 채우기·신규 ref 반영·Data Model 만 재추출하는 작업은 sibling `/update-spec` 의 책임이다.
argument-hint: "<spec> | specs/{spec}[/{file}] | (empty)"
allowed-tools: Bash(.venv/bin/python3 scripts/extract.py *) Bash(.venv/bin/python3 scripts/resolve-yaml-refs.py *) Bash(.venv/bin/python3 scripts/render-mermaid.py *) Bash(mkdir -p *) Bash(ls *) Bash(find *) Bash(grep *)
---

# add-spec — 3GPP spec 문서를 wiki 에 등록

## 입력 형태

본 skill 의 *입력 단위는 spec 시리즈* 이다. 파일 한 건이 아니라 시리즈 폴더(`specs/{spec}/`) 가 단위다. `$ARGUMENTS` 는 다음 셋 중 하나로 들어온다.

- **(a) spec 번호** — `29.531`, `23.501` 같은 점 포함 표기. 가장 자주 쓰이는 형태.
- **(b) spec 폴더 경로** — `specs/29.531` 또는 `specs/29.531/29531-j60.docx` 같은 명시적 경로. (a) 와 등가이지만 같은 spec 시리즈에 여러 버전이 공존할 때 *특정 버전을 강제* 하고 싶으면 (c)-style 전체 파일 경로를 쓴다.
- **(c) 빈 값** — `specs/` 안의 spec 폴더 중 wiki 에 *아직 페이지가 없는* 시리즈를 보여주고 사용자에게 어느 시리즈를 등록할지 묻는다.

본 skill 은 항상 시리즈 한 건만 등록한다. 여러 시리즈를 한꺼번에 정리하거나, 이미 만든 wiki 페이지의 비어있는 부분을 사후에 채우는 작업은 sibling `/update-spec` 의 책임이다.

## 프로젝트 환경 (전제)
- 프로젝트 루트는 `~/AI/llm-wiki`. 모든 경로는 그 기준 상대 경로로 다룬다.
- 의존성과 디렉터리 골격은 `bash scripts/setup.sh` 가 이미 처리했다. `.venv/bin/python3` 를 사용한다.
- `CLAUDE.md` 의 "Repository Structure"·"File Naming Convention"·"Categories"·"Adding a New Document" 섹션이 진실의 출처다. 본 skill 은 그 정책을 *집행* 하는 자동화일 뿐이며, 정책 자체를 여기에 다시 적지 않는다.

## 절대 규칙 — CLAUDE.md THE FOUR RULES
1. WebSearch / WebFetch 금지. 본 skill 은 외부 인터넷에서 정보를 가져오지 않는다.
2. wiki 본문의 근거는 `specs/<spec>/<file>` 의 추출 텍스트뿐이다. 추출되지 않은 내용을 추측·창작해 채우지 않는다.
3. 추출 텍스트가 부족해 핵심 섹션이 비면 `--max-chars` 를 늘려 재추출한다. 그래도 부족하면 사용자에게 어느 부분이 더 필요한지 묻는다.
4. spec type / release / version / NF 중 하나라도 자신 없으면 추측해 진행하지 말고 사용자에게 묻고 정지한다. 한 번 더 묻는 비용이 잘못 만든 페이지를 고치는 비용보다 항상 낮다.

## 언어 정책 (Override 포함)
- wiki 본문(prose)·`digests/*.md` 본문은 **한국어**.
- frontmatter 키·값, 섹션 헤더 (`## Summary` 등), 3GPP 약어·메시지명·필드명 (NSSF, AMF, S-NSSAI, SUCI 등) 은 **영어 원문 유지**.
- 스펙 정의 문구를 옮길 때는 영어 원문 인용 + 한국어 해설을 병기한다.
- 한국어 문장은 `.`/`?`/`!` 로 끝낸다. 콜론(`:`) 으로 한국어 문장을 끝내지 않는다. 콜론은 코드·key-value·라벨 안에서만.

---

## Workflow

### 1. 입력 해석 — `$ARGUMENTS` 를 단일 `(spec, file, yamls[])` 로 정규화

본 skill 의 입력 단위는 *spec 시리즈* 다. 시리즈 안의 파일 선택은 skill 이 책임진다.

**(a) spec 번호** — `29.531`, `23.501` 같은 점 포함 표기.
1. `specs/<spec>/` 디렉터리 실재 확인. 없으면 정지.
2. 그 폴더의 `.pdf|.docx|.doc` 중 *가장 최신 버전* 을 자동 선택. 우선순위 — 파일명 끝의 letter 매핑 (j>i>h>g>f) → letter 가 같으면 NM 큰 쪽 (`j60` > `j50`). 동일 버전·다른 포맷이면 `.docx` > `.pdf` > `.doc`.
3. 같은 폴더의 `.yaml` 들을 companion OpenAPI 정의로 모아둔다. (있으면 §5 Data Model chain 추적의 출발점이 된다.)
4. *이미 등록된* 시리즈 (`kb/{nf}/3gpp-{ts|tr}-{n}.md` 가 존재) 면 사용자에게 "재생성 vs Version History 갱신 vs 정지" 중 하나를 묻는다. 본 skill 의 기본 모드는 *최초 등록* 이며, 사후 보강은 `/update-spec` 의 책임이다.

**(b) spec 폴더 경로** — `specs/29.531` / `specs/29.531/` 또는 `specs/29.531/29531-i40.docx` 같은 명시적 경로.
1. 폴더 형태면 (a) 와 동일.
2. 파일 형태면 *그 파일을 강제 사용* — 같은 시리즈의 다른 버전이 있어도 사용자가 지정한 것을 따른다 (예 과거 버전을 명시 등록하고 싶을 때).
3. companion yaml 은 (a) 와 같은 방식으로 같은 폴더에서 모은다.

**(c) 빈 값** — 후보 제시 후 정지.
1. `specs/*/` 폴더 목록과, 각 폴더에 대응하는 `kb/{nf}/3gpp-{ts|tr}-{n}.md` 의 존재 여부를 비교.
2. 아직 wiki 페이지가 없는 spec 시리즈만 표로 보여주고 사용자가 어느 것을 등록할지 묻고 정지. 후보가 0개면 "모든 spec 이 wiki 에 있음"으로 보고하고 정지.

해석 결과는 `(spec, file, yamls[])` 삼 요소. 이를 §2 이후 입력으로 넘긴다.

> 입력이 *시리즈* 단위이므로 같은 시리즈의 여러 버전 모두 등록하려면 (b) 의 명시적 파일 경로 형태로 한 번씩 호출하거나, 일괄 보강은 `/update-spec`.

### 2. 메타데이터 추론

파일명·폴더명에서 다음을 추출한다.

- **Spec series**: 폴더명. 예 `specs/29.531/...` → `29.531`.
- **3GPP 버전 인코딩** (파일명 끝의 `-{letter}{N}{M}` 패턴).
  - 자릿수 의미: letter = release, N = minor (=중간 자리), M = patch (=마지막 자리). version 표기는 `<release>.<N>.<M>`.
  - 예: `j60` → Rel-19, v19.6.0. `i40` → Rel-18, v18.4.0. `h60` → Rel-17, v17.6.0.
  - letter 매핑: `f`=15, `g`=16, `h`=17, `i`=18, `j`=19, `k`=20.
- **Spec type (TS|TR)**: 파일 본문 첫 페이지의 표기 ("Technical Specification" / "Technical Report") 로 판정한다. 추출 텍스트에 둘 다 안 보이면 사용자에게 묻는다.
- **NF category**: 추출된 문서 제목·범위로 판정한다.
  - 빠른 힌트 (확정은 본문에서 한 번 더 검증). `29.531` → `nssf`, `29.503` → `udm`, `29.502` → `smf`, `29.510` → `nrf`, `29.518` → `amf`, `29.509` → `ausf`, `23.501`/`23.502` → `architecture`, `33.501` → `security`. 본 표는 힌트일 뿐 확정 매핑이 아니다.
  - 모호하면 사용자에게 확인한다.
- **Stem (정규형)**:
  - source 단위: `3gpp-{ts|tr}-{n-no-dot}-v{x.y.z}` (예: `3gpp-ts-29531-v19.6.0`).
  - wiki 시리즈 단위: `3gpp-{ts|tr}-{n-no-dot}` (예: `3gpp-ts-29531`).

### 3. 텍스트 추출

기본 호출.

```bash
.venv/bin/python3 scripts/extract.py <path> --max-chars 60000
```

핵심 섹션(Scope / Definitions / Procedures / References) 이 잘려 보이면 `--max-chars 120000` 으로 재호출한다. 그래도 부족하면 어느 페이지·섹션이 더 필요한지 사용자에게 묻는다.

### 4. `digests/{source-stem}.md` 작성

경로: `digests/3gpp-{ts|tr}-{n}-v{x.y.z}.md`. 같은 경로 파일이 이미 있으면 덮어쓸지 사용자에게 확인한다.

템플릿 (영문 frontmatter, 한국어 prose).

```yaml
---
title: "<영문 원제 그대로>"
authors: 3GPP <WG, 예: CT4, SA3>
spec: TS 29.531
release: 19
version: 19.6.0
year: <문서 표지·footer 의 발행 연>
category: <nssf|amf|smf|nrf|...|architecture|security|slicing|interfaces|concepts|other>
source_path: specs/{spec}/{filename}
source_filename: {filename}
source_format: {pdf|docx|doc}
source_collection: 3gpp
---

## One-line Summary
[한국어 한 줄. 콜론 종결 금지.]

## 1. Document Information
- 발행: 3GPP, <year>
- 시리즈: TS|TR <NN.NNN>, Release <N>, v<x.y.z>
- 담당 WG: <CT4 등>
- 페이지 수: <원본 페이지 수, 추출 가능하면>

## 2. Key Contributions
[문서가 정의·도입하는 핵심 기능·메시지·절차 3~7개 bullet. 한국어 prose, 약어·메시지명 영어.]

## 3. Methodology and Architecture
[아키텍처 위치(어떤 NF·인터페이스가 관여하는지), 메시지 흐름의 큰 그림. 길어지면 wiki 페이지로 미루고 여기서는 요약만.]

## 4. Key Procedures and Messages
[주요 service operation·메시지·status code·필드의 핵심. 표 사용 OK.]

## 5. Limitations and Open Issues
[문서 자체가 명시한 limitation·FFS·editor's note 만 옮긴다. 추출 범위 밖 내용을 추정해 채우지 않는다.]

## 6. Related Specs
[Reference 섹션에서 본 핵심 의존 spec. 영어 spec 번호·제목만.]

## 7. Glossary
[추출 텍스트에서 본 핵심 약어 한·영 병기.]
```

### 5. `kb/{nf}/{wiki-stem}.md` 작성/갱신

경로: `kb/{nf}/3gpp-{ts|tr}-{n}.md`.

- `kb/{nf}/` 폴더가 없으면 `mkdir -p kb/{nf}` 로 생성하고, "새 NF 폴더를 만들었다" 사실을 결과 보고에 명시한다 (CLAUDE.md "NF 폴더는 첫 문서가 들어갈 때 생성").
- 파일이 이미 있으면 *같은 spec 시리즈의 다른 버전 추가* 케이스다. 본문은 그대로 두고, frontmatter 의 `version`·`source` 를 새 버전으로 갱신, `related_versions` 에 이전 버전을 추가, `## Version History` 섹션에 한 줄 추가한다.

신규 파일 템플릿.

```yaml
---
title: "<영문 원제>"
authors: 3GPP <WG>
spec: TS 29.531
release: 19
version: 19.6.0
year: <year>
source: 3gpp-ts-29531-v19.6.0.md
category: <nssf|...>
source_path: specs/{spec}/{filename}
source_filename: {filename}
source_format: {pdf|docx|doc}
source_collection: 3gpp
related_versions: []
tags: [<nf>, sbi, <topic1>, <topic2>]
---

## Summary
[한국어 4~10줄. 이 spec 이 5GC 안에서 무슨 역할을 하는지·왜 존재하는지·어떤 문제를 푸는지. 약어·메시지명·필드명은 영어 원문.]

## Key Contributions
[bullet 3~7개. sources 의 §2 보다 wiki 독자(나중에 검색해 들어오는 자기 자신) 친화적 표현으로.]

## Methodology and Architecture
[그림 대신 prose. 어느 NF 가 어떤 인터페이스로 호출하는지, 어떤 데이터/모델이 흐르는지. API URI 진입점은 코드블록으로.]

## Service Flows
[**필수**. Mermaid `sequenceDiagram` 으로 대표 흐름 2~4개. 각 다이어그램 직전에 어느 stage 2 절차에 매핑되는지 한 줄 캡션. 아래 "Service Flows 작성 규칙" 참고.]

## Key Procedures
[Service Flows 다이어그램에서 다 담지 못하는 운용 정보 — operation 매트릭스 표(input/output 핵심 IE), 트리거되는 stage 2 절차 목록, 응답 코드. 다이어그램과 본 섹션은 보완 관계지 중복이 아니다.]

## Data Model
[**필수**. 본 spec 의 OpenAPI yaml 을 출발점으로 `$ref` 체인을 끝까지 추적해 자료형 트리를 만든다. 외부 spec 의 yaml 이 `specs/` 안에 있으면 거기까지, 그래도 부재하면 `[TS XX.YYY] (참조 규격 미등록)` leaf 로 종료한다. 자세한 알고리즘과 도구는 아래 "Data Model 작성 규칙" 참고.]

## Version History
- v<x.y.z> (Rel-<N>, <year>) — <변경점 또는 "초기 등록">

## Related Pages
- [[architecture/3gpp-ts-23501]] — 5GC 전체 아키텍처 맥락 (이 페이지가 wiki 에 *실제로* 있을 때만 링크)
- [[slicing/...]] — 관련 cross-cutting (실재할 때만)
```

#### Service Flows 작성 규칙

- 표기 형식은 Mermaid `sequenceDiagram`. Markdown 코드블록 `` ```mermaid `` 안에 둔다. Obsidian·GitHub 가 native 로 렌더하고, 정적 SVG 가 필요하면 `scripts/render-mermaid.py` 가 sibling `_diagrams/<page-stem>-<n>.svg` 로 생성한다 (호출은 사용자 수동, 또는 본 skill 의 §7 결과 보고에서 안내만 한다 — 자동 호출 금지).
- `autonumber` 를 켠다. participant 이름은 NF 약어 그대로 (`AMF`, `NSSF`, `V-NSSF`, `H-NSSF` 등).
- 메시지 라벨은 `HTTP method · 경로 · 본문 자료형` 순. 한 줄에 다 안 들어가면 `<br/>` 로 줄바꿈한다. mmdc v11 기준 별도 quoting 없이 `<br/>` 줄바꿈이 정상 SVG `<tspan>` 으로 변환됨이 확인됨.
- 입력 query/IE 가 많으면 `Note over X,Y` 또는 `Note right of NSSF` 로 펼친다. 한국어 prose 도 가능하다.
- 분기 (성공·실패·redirect) 는 `alt`/`else` 로 나누고 응답 코드와 자료형을 함께 적는다.
- 콜론(`:`) 은 Mermaid 의 메시지 구분자로 한 번 쓰이는 것이 자연스럽다. 라벨 내부에 또 콜론이 필요하면 `=` 또는 `·` 로 대체해 파서 충돌을 줄인다.
- 다이어그램 안에서 추측 금지. 화살표·자료형은 본 spec 본문이나 OpenAPI 에 *실제로* 적힌 것만.
- 다이어그램 직전에 출처 절 한 줄 캡션을 단다 (예: "Get during Registration (TS 23.502 §4.2.2.2.2)").
- 어떤 흐름을 고를지 — 본 spec 이 정의한 service operation 별로 가장 자주 쓰일 시나리오 1개씩 + roaming/cross-NF 체이닝이 있다면 따로 1개. 보통 2~4개로 충분.
- 같은 페이지를 다시 작성·갱신할 때는 결과 보고에 "SVG 갱신이 필요하면 `.venv/bin/python3 scripts/render-mermaid.py [--clean]` 실행" 한 줄 안내. `--clean` 은 블록 수가 줄었을 때 옛 SVG 를 정리한다.

#### Data Model 작성 규칙

본 wiki 의 가장 중요한 정보 자산이다. 본 spec 의 OpenAPI yaml 을 출발점으로 *체인 끝까지* 끌고 가야 한다.

**도구**: `scripts/resolve-yaml-refs.py` 가 OpenAPI yaml 의 schema 를 입력받아 `$ref` 체인을 자동 추적해 ` ```text ` 트리를 만든다.

```bash
.venv/bin/python3 scripts/resolve-yaml-refs.py \
  specs/{spec}/{TSnnnnn_xxx.yaml} \
  <SchemaName1> <SchemaName2> ... \
  --depth 8 --external-depth 1
```

본문에는 stdout 출력을 그대로 ` ```text ... ``` ` 코드블록으로 옮기고, 절대 손으로 다시 그리지 않는다 (사람이 그리면 빠짐·오타가 생긴다). 도구가 적용한 표기 약속을 페이지 상단에 한 번 설명해 둔다 — `*required`, `[TS NN.NNN]`, `[…]`, `(allOf)`, `(extensible)`, `map<key, T>` 등.

**`$ref` 해결 우선순위** (도구도 이 순서를 따른다):

1. 같은 yaml 파일 안의 `#/components/schemas/X` (local).
2. 같은 spec 폴더 안의 다른 yaml — 같은 spec 에 여러 service yaml 이 함께 들어있는 경우 (예 `TS29531_Nnssf_NSSelection.yaml` 과 `TS29531_Nnssf_NSSAIAvailability.yaml`).
3. 다른 spec 폴더의 yaml — 파일명 패턴 `TSNNNNN_*.yaml` → `specs/NN.NNN/...` 로 매핑.
4. yaml 부재 시 같은 spec 폴더의 `*.docx` Annex A 텍스트에서 schema 정의 발췌 (도구의 `--no-docx-fallback` 으로 비활성화 가능).
5. 모두 부재 → `[TS XX.YYY] (참조 규격 미등록)` leaf 로 chain 종료. 추측해서 펼치지 않는다.

**진입 자료형** 으로 어느 schema 를 트리화할지 — 각 service 의 *최상위 요청·응답 자료형* 만 root 로. 예 Nnssf_NSSelection → `AuthorizedNetworkSliceInfo`(응답) + `SliceInfoForXxx`(요청). Nnssf_NSSAIAvailability → `NssaiAvailabilityInfo`, `AuthorizedNssaiAvailabilityInfo`, `NssfEventSubscriptionCreateData`, `NssfEventSubscriptionCreatedData`, `NssfEventNotification`. 하위 자료형은 root 트리 안에 자연히 펼쳐지므로 별도 root 로 또 그릴 필요 없다.

**Data Model 섹션 끝**에 두 가지 짧은 목록을 둔다.
- *미등록 reference* — 체인이 종료된 지점. 어느 외부 spec / 자료형이 `specs/` 에 추가되면 트리가 더 풀릴지 명시.
- *체인이 끝까지 풀린 외부 spec* — 트리 안에서 `[TS XX.YYY]` 로 등장한 spec 들의 정리 목록.

**도구를 우회해 손으로 트리를 적지 않는다.** 정확성이 의심되거나 도구가 못 푸는 부분 (예 `additionalProperties` 의 복잡한 조합) 이 있으면 도구를 고치는 쪽이 항상 옳다 — 한 페이지에서만 정확하게 만드는 것보다, 다음 호출에서도 같은 품질이 나오도록 도구를 발전시키는 편이 누적 가치가 크다.

**docx 만 있는 spec** (예 TS 23.501 같은 stage 2 spec) 은 OpenAPI 정의가 없거나 prose 위주라 본 도구의 자동 추적 대상이 아니다. 그런 spec 에 대해 wiki 페이지를 만들 때는 Data Model 섹션을 비우거나, 산문으로 핵심 정보 모델 (예 stage 2 의 reference point ID·attribute) 만 요약한다.

**Related Pages 의 wikilink 는 wiki 에 실재하는 파일만** 적는다. 존재하지 않는 페이지를 가짜로 링크하지 않는다 (THE FOUR RULES #4). 후보 wikilink 는 `ls kb/{architecture,slicing,interfaces,security,concepts,overviews}/` 로 실재 여부를 확인한 뒤 적는다.

### 6. `index.md` 갱신

- 해당 NF 섹션의 "_(아직 페이지 없음 ...)_" placeholder 가 있으면 한 줄 항목으로 *교체* 한다.
- 이미 항목이 있으면 spec-number 오름차순으로 한 줄 *추가* 한다.
- 형식: `- [[{nf}/{wiki-stem}]] — <한국어 한 줄 설명>`.
- `kb/{nf}/` 가 *새로* 만들어진 NF 라면 `## 다른 NF (예정)` 섹션의 안내에서 그 NF 를 빼고, 기존 NSSF 섹션처럼 별도 섹션을 만들어 항목을 넣는다.

### 7. 결과 보고 (커밋 X)

사용자에게 한 화면에 묶어 보고한다.

- **만들어진/수정된 파일 목록** (`digests/...`, `kb/{nf}/...`, `index.md`, `kb/{nf}/_diagrams/*.svg`).
- 각 파일의 핵심 변경점 1~3줄 요약.
- **체인 보강 결과** — Data Model chain 이 어디까지 풀렸는지. `resolve-yaml-refs.py` 출력에서 발견된 외부 spec 마커 `[TS XX.YYY]` 를 spec-단위로 집계해 "체인이 풀린 외부 spec" 목록.
- **필요 규격 문서 업데이트 필요** — chain 추적 도중 `specs/` 에 *없어서* leaf 로 종료된 외부 spec 들. 한 줄 한 줄 spec 번호 + 사용된 자료형 + 어느 페이지의 어느 트리에서 막혔는지를 보여줘 사용자가 무엇을 cp 하면 트리가 더 풀릴지 알 수 있게 한다. 사용자가 그 spec 을 specs/ 에 추가한 뒤 `/update-spec <nf> --data-model` 로 부분 보강 가능.
- **알려진 한계 그 외** — 추출 truncate 된 섹션, 추정한 NF/release, 본문에서 끝까지 확인 못 한 항목, mermaid 렌더 실패 (있으면) 등.
- **제안 commit 메시지 한 줄** (`feat({nf}): TS NN.NNN v{x.y.z} wiki 페이지 추가`).
- **다음 단계 hint** — 산출 검토 후 `python3 scripts/render-mermaid.py` 로 SVG 갱신, 그리고 사용자가 만족하면 별도 커밋.

**커밋은 절대 자동으로 하지 않는다.** 사용자가 검토 후 명시적으로 "커밋해" 라고 지시할 때만 진행한다.

---

## 자주 틀리는 지점 — 사전 점검

마무리 직전 다음을 한 번 훑어 본다.

- 한국어 문장을 콜론으로 끝내고 있지 않은가. (불릿 다음 문장도 마침표.)
- frontmatter 의 `source_path` 가 절대 경로 (`...`) 인가. 상대 경로 금지.
- `specs/` 안의 파일이 cp 가 아니라 symlink 가 아닌가 (`ls -l` 로 확인 가능).
- `## Related Pages` 의 wikilink 가 모두 wiki 에 실재하는 파일을 가리키는가.
- 추출 텍스트에 없는 사실을 wiki 본문에 옮기지 않았는가.
- spec type / release / version / NF 추론을 한 번 사용자에게 확인받았는가 (자신 있을 때는 보고만으로 OK).
- `## Service Flows` 에 Mermaid `sequenceDiagram` 이 적어도 1개 이상 있는가. 이름뿐인 placeholder 금지.
- `## Data Model` 의 트리가 `scripts/resolve-yaml-refs.py` 산출 그대로인가. 손으로 가공하지 않았는가.
- `specs/` 안에 yaml 이 있는 외부 spec 자료형은 자동 펼침되었는가, 미등록 ref 는 `[TS XX.YYY] (참조 규격 미등록)` leaf 로 명시되었는가.

## 실패·모호 처리

- spec type / release / version / NF 중 하나라도 자신 없으면 사용자에게 묻고 정지한다.
- 추출 텍스트가 너무 짧거나 (<500 chars) 핵심 섹션이 안 보이면 `--max-chars` 를 키워 재추출한다.
- `specs/{spec}/` 폴더가 비어있거나 인자 파일이 없으면 정지한다.
- 같은 stem 의 sources / wiki 파일이 이미 있으면 덮어쓰기 전에 사용자에게 묻는다.

## 참고 — 본 skill 안에 다시 적지 말 것 (중복 금지)

다음은 `CLAUDE.md` 또는 helper 스크립트 가 이미 정의했다. 본 skill 은 거기에 따르고, 같은 내용을 복제하지 않는다.

- 디렉터리 매핑·파일명 규칙: `CLAUDE.md` 의 "Repository Structure"·"File Naming Convention"·"Categories".
- 추출 도구의 옵션·폴백: `scripts/extract.py --help` 와 그 docstring.
- Data Model 체인 추적 알고리즘: `scripts/resolve-yaml-refs.py` 의 docstring 과 `--help`.
- Mermaid → SVG 렌더링 동작: `scripts/render-mermaid.py` 의 docstring.
- 환경 부트스트랩: `scripts/setup.sh`.
