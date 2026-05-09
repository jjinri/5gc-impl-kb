---
name: nf-build
description: 매니페스트가 준비된 NF 에 대해 7 카테고리 implementation-grade wiki 페이지를 생성·갱신하는 워크플로우. 사용자가 "/nf-build nssf", "NSSF 페이지 만들어", "NRF 빌드", "data-model 만 다시 뽑아", "build nf page" 등을 말하거나 NF 이름을 지정하면 무조건 이 skill 을 사용한다. 동작 — `kb/<nf>/_manifest.yaml` 의 ready_for_build 가 true 인지 확인하고, 7 카테고리 (Interface / API / Data Model / Service Scenarios / Cross-NF Dependencies / Configuration / Error Handling) 를 일괄 또는 부분 빌드한다. 카테고리 인자 (`--data-model`, `--api`, `--interface` 등) 로 부분 빌드 가능 — 가장 자주 쓰이는 시나리오는 "papers/ 에 새 ref 추가 후 Data Model 트리만 재추출". Data Model 은 `scripts/resolve-yaml-refs.py` 가 chain 추적, Service Scenarios 의 mermaid 는 사람이 작성 (도구가 자동 작성하지 않음 — figure 추출은 sprint 후반). 매니페스트 생성·갱신은 sibling `/nf-init`, 완성도 검사는 `/nf-status` 의 책임이며 본 skill 은 페이지 *내용 생성* 에 집중한다. 커밋은 자동 수행 금지.
argument-hint: "<nf> [--<category>]"
allowed-tools: Bash(.venv/bin/python3 scripts/extract.py *) Bash(.venv/bin/python3 scripts/resolve-yaml-refs.py *) Bash(.venv/bin/python3 scripts/render-mermaid.py *) Bash(.venv/bin/python3 scripts/nf-manifest.py *) Bash(mkdir -p *) Bash(ls *) Bash(grep *) Bash(awk *) Bash(find *)
---

# nf-build — 7 카테고리 implementation-grade 페이지 생성·갱신

## 입력
- `<nf>` — NF 이름. `kb/<nf>/_manifest.yaml` 가 이미 존재해야 한다 (없으면 `/nf-init` 먼저).
- `--<category>` — 부분 빌드. 카테고리 명은 다음 중 하나.
  - `--interface`
  - `--api`
  - `--data-model`
  - `--service-scenarios`
  - `--cross-nf`
  - `--configuration`
  - `--error-handling`
- 인자 없으면 7 카테고리 모두 빌드 (full build).

## 책임 분담

| 시나리오 | 사용 skill |
| --- | --- |
| 매니페스트 생성·보강 | `/nf-init` |
| 페이지 빌드·갱신 | `/nf-build` (본 skill) |
| 페이지 완성도 검사 | `/nf-status` |
| Data Model chain 갱신 (priority 부분 빌드) | `/nf-build <nf> --data-model` |

## 절대 규칙
1. CLAUDE.md THE FOUR RULES 를 따른다 — web 금지, specs/ 만 진실, 추출에 없는 사실 추측 금지, chain 미해결 시 명시 leaf.
2. 매니페스트 `ready_for_build = false` 면 빌드 거절하고 `/nf-init` 으로 안내. 단 사용자가 명시적으로 `--force` 를 주면 미완 매니페스트로도 빌드 시도 (Data Model 트리에 missing leaf 가 박힘).
3. 사용자가 *손으로 적은 산문* (Summary 보조 문단, 추가 본문) 은 보존. 본 skill 이 *교체* 하는 영역은 *기계 산출 영역* — Data Model 트리, mermaid SVG, API 매트릭스, Cross-NF 표 등.
4. 카테고리 부분 빌드 시 다른 카테고리는 만지지 않는다.
5. 커밋 자동 수행 금지.

## Workflow

### 1. 입력 검증
- `kb/<nf>/_manifest.yaml` 존재 확인. 없으면 `/nf-init <nf> --primary <spec>` 먼저 안내 후 정지.
- 매니페스트 `status.ready_for_build` 점검. false 면 `missing_priority` 알리고 정지 (또는 `--force`).

### 2. 페이지 파일 결정
- `kb/<nf>/3gpp-{ts|tr}-{n}.md` 가 이미 있으면 *갱신* 모드, 없으면 *신규* 모드.
- 신규 모드 — 7 카테고리 H2 헤더 모두 + frontmatter 만 우선 골격으로 생성.
- 갱신 모드 — 사용자 산문 보존, 기계 산출 영역만 교체.

### 3. 카테고리별 빌드 (full 또는 부분)

각 카테고리는 *어디에서 끌어와* 어떤 형식으로 채울지 명확히 정해진 책임이 있다.

#### 3a. Interface
- 자료원 — primary yaml 의 `info`, `servers`, `paths.*.security`. 본 spec docx §6.x.1 (API URI), §6.x.2 (HTTP), §6.x.9 (Security).
- 출력 — API URI 구조, transport (HTTP/2 RFC 9113), auth (OAuth 2.0 RFC 6749 + TS 33.501), header convention (3gpp-Sbi-* 헤더 등).

#### 3b. API
- 자료원 — primary yaml 의 `paths` 전체.
- 출력 — markdown table. 행 = operation, 열 = (operation, method, path, request body type, response body type, error candidates).
- 행 수 = yaml 의 (path, method) 조합 수와 정확히 일치해야 함 (acceptance criterion).

#### 3c. Data Model
- 자료원 — primary yaml + companion yaml + cross-spec chain.
- 도구 — `scripts/resolve-yaml-refs.py <yaml> <Schema1> <Schema2> ... --depth 8 --external-depth 1`.
- root schema 결정 — primary yaml(s) 의 `paths.*.requestBody.content.*.schema.$ref` 와 `paths.*.responses.*.content.*.schema.$ref` 의 unique 한 schema 이름 집합.
- 출력 — 각 root 마다 `text 코드블록 안 트리. *반드시* 도구 산출 그대로 옮기고 손으로 가공하지 않음.
- 미해결 ref — `[TS XX.YYY] (참조 규격 미등록)` leaf 로 종료.

#### 3d. Service Scenarios
- 자료원 — primary docx 의 §5 (Services offered) + 트리거 절차의 stage 2 spec (예 23.502 §4.x).
- 출력 — Mermaid `sequenceDiagram` 다이어그램 2~4개. autonumber, NF 약어 participant, 메시지 라벨은 HTTP method · 경로 · 자료형. 분기는 alt/else.
- 도구 — `scripts/render-mermaid.py [--clean]` 으로 sibling `_diagrams/<page>-<n>.svg` 산출 (사용자가 검토 후 수동 호출 권장).
- *추측 금지* — 다이어그램의 화살표·자료형은 본 spec 또는 OpenAPI 에 *실제로* 적힌 것만.

#### 3e. Cross-NF Dependencies
- 자료원 — 모든 specs/*/.docx grep — 본 NF 호출하는 곳 / 본 NF 가 호출하는 곳.
- 출력 — 양방향 표 (consumer NF · 호출 operation · 트리거 절차 / producer NF · 호출 operation · 사유).
- 자동 추출 도구가 아직 없을 수 있음 — 그러면 `--cross-nf` 빌드 시 placeholder 안내문 + sprint 후반 도구 신규 후 재빌드.

#### 3f. Configuration
- 자료원 — primary yaml 의 `info.x-gateway-rate-limit-policy`(있다면) + supportedFeatures 표 (docx §6.x.8) + default·timeout (docx 본문).
- 출력 — feature flag 표 (bit, 이름, 의미), 기본 timeout / expiry, 의존하는 OS / 라이브러리 설정.

#### 3g. Error Handling
- 자료원 — primary yaml 의 `responses` + docx §6.x.7 ProblemDetails cause 표.
- 출력 — markdown table. 열 = (HTTP code, ProblemDetails cause, 의미, 권장 복구 동작).

### 4. mermaid SVG 렌더 안내
- mermaid 블록을 새로 추가했으면 결과 보고에 `python3 scripts/render-mermaid.py [--clean]` 실행 권장 한 줄. *자동 호출은 하지 않는다* — 사용자 확인 후 명시 실행.

### 5. 결과 보고 (커밋 X)
- 갱신·신규 파일 목록 (`kb/<nf>/3gpp-*.md`, 매니페스트는 변경 없음).
- 카테고리별 빌드 상태 — *완료 / placeholder / not-built (도구 부재)*.
- 미해결 leaf 목록 (Data Model 의 chain leaf, Cross-NF 의 자동 추출 미가용 등).
- 제안 commit 메시지 — `feat(<nf>): TS NN.NNN 페이지 7-카테고리 빌드 (또는 부분)`.
- 사용자 다음 액션 — `/nf-status <nf>` 로 acceptance gate 확인.

**커밋 자동 수행 금지.**

## 자주 틀리는 지점

- 매니페스트 ready 가 아닌데 빌드 강행했는가 (`--force` 명시 없이).
- Data Model 트리를 손으로 가공해 도구 산출에서 벗어났는가 (acceptance check `data_model_chain_complete` 가 detect 함).
- 카테고리 부분 빌드 시 *다른 카테고리* 본문을 건드렸는가.
- mermaid 추가 후 SVG 미렌더 — `mermaid_renders` check 가 detect.
- 사용자 산문이 새 빌드로 사라졌는가.

## 참고 — 본 skill 안에 다시 적지 말 것

- 매니페스트 schema: `scripts/nf-manifest.py` docstring.
- Data Model chain 알고리즘: `scripts/resolve-yaml-refs.py` docstring.
- mermaid 렌더링: `scripts/render-mermaid.py` docstring.
- 디렉터리·파일명·언어 정책: `CLAUDE.md`.
