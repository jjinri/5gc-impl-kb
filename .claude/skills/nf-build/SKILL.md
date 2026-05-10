---
name: nf-build
description: 매니페스트가 준비된 NF 에 대해 7 카테고리 design wiki 페이지 (handoff_ready·canonical 목표)를 생성·갱신하는 워크플로우. 사용자가 "/nf-build nssf", "NSSF 페이지 만들어", "NRF 빌드", "data-model 만 다시 뽑아", "build nf page" 등을 말하거나 NF 이름을 지정하면 무조건 이 skill 을 사용한다. 동작 — `design/<nf>/_manifest.yaml` 의 ready_for_build 가 true 인지 확인하고, 7 카테고리 (Interface / API / Data Model / Service Scenarios / Cross-NF Dependencies / Configuration / Error Handling) 를 일괄 또는 부분 빌드한다. 카테고리 인자 (`--data-model`, `--api`, `--interface` 등) 로 부분 빌드 가능 — 가장 자주 쓰이는 시나리오는 "papers/ 에 새 ref 추가 후 Data Model 트리만 재추출". Data Model 은 `design/scripts/resolve-yaml-refs.py` 가 chain 추적, Service Scenarios 의 mermaid 는 사람이 작성 (도구가 자동 작성하지 않음 — figure 추출은 sprint 후반). 매니페스트 생성·갱신은 sibling `/nf-init`, 완성도 검사는 `/nf-status` 의 책임이며 본 skill 은 페이지 *내용 생성* 에 집중한다. 커밋은 자동 수행 금지.
argument-hint: "<nf> [--<category>]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/extract.py *) Bash(.venv/bin/python3 design/scripts/spec-split.py *) Bash(.venv/bin/python3 design/scripts/resolve-yaml-refs.py *) Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(mkdir -p *) Bash(ls *) Bash(grep *) Bash(awk *) Bash(find *)
---

# nf-build — 7 카테고리 design 페이지 생성·갱신

## 입력
- `<nf>` — NF 이름. `design/<nf>/_manifest.yaml` 가 이미 존재해야 한다 (없으면 `/nf-init` 먼저).
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

## 동작 원칙 (이유 포함)

- **CLAUDE.md THE FOUR RULES 가 우선.** 추출 텍스트에 없는 사실을 본문에 끼워넣지 않는다 — wiki 의 신뢰성이 그것에 달려있다. yaml chain 이 끝까지 안 풀리면 `[TS XX.YYY] (참조 규격 미등록)` leaf 로 표시해 사용자에게 무엇을 cp 하면 풀릴지 보여준다.
- **`ready_for_build = false` 면 기본 거절, `--force` 시 시도.** 거절이 default 인 이유 — missing dependency 로 빌드하면 Data Model 트리가 leaf 투성이라 다음 단계 (코드 생성·검토) 가 의미 없어진다. `--force` 는 사용자가 알면서 일부 결과만 원할 때의 escape hatch.
- **사용자 산문 보존, 기계 산출만 교체.** Summary·Methodology 같은 prose 는 사용자가 손으로 다듬은 자산. 본 skill 이 매번 새로 쓰면 그 노력이 매 빌드마다 사라진다. 기계 산출 영역 — Data Model 트리, API 매트릭스, Cross-NF 표 — 은 도구가 진실 출처라 안전하게 교체.
- **카테고리 부분 빌드 시 다른 카테고리 불간섭.** `--data-model` 호출은 *Data Model 만* 손댄다. Service Scenarios 의 mermaid 를 함께 갱신하지 않는다. 부분 빌드의 가치가 *예측 가능한 변경 범위* 에 있기 때문.
- **mermaid 화살표·자료형은 추출 텍스트에 적힌 것만.** 다이어그램에서 추측하면 잘못된 시퀀스가 wiki 에 박혀 구현자를 오도한다. spec 본문이나 OpenAPI 에 없는 흐름은 *적지 않는다* 가 안전.
- **커밋은 자동 수행 안 함.** wiki 변경은 사용자가 검토 + 의미 있는 단위로 묶고 싶어하는 영역. 자동 커밋이 그 흐름을 방해한다.

## Workflow

### 1. 입력 검증
- `design/<nf>/_manifest.yaml` 존재 확인. 없으면 `/nf-init <nf> --primary <spec>` 먼저 안내 후 정지.
- 매니페스트 `status.ready_for_build` 점검. false 면 `missing_priority` 알리고 정지 (또는 `--force`).

### 2. 페이지 파일 결정
- `design/<nf>/3gpp-{ts|tr}-{n}.md` 가 이미 있으면 *갱신* 모드, 없으면 *신규* 모드.
- 신규 모드 — 7 카테고리 H2 헤더 모두 + frontmatter 만 우선 골격으로 생성.
- 갱신 모드 — 사용자 산문 보존, 기계 산출 영역만 교체.

**frontmatter 4키 (강제).** `nf-status` 의 `frontmatter_valid` check 가 다음 4키만 요구한다 — 그 외는 manifest·docx·폴더명이 진실 출처라 *중복*. 더 늘리지 마라.
```yaml
---
nf: <nf>           # 폴더명과 일치 (예 nssf)
spec: TS NN.NNN    # 본 페이지가 다루는 spec (예 TS 29.531)
version: <ver>     # docx version 또는 yaml info.version (둘이 다르면 둘 다 표기 가능)
status: draft|ready_for_review|handoff_ready|canonical  # 사람이 갱신
---
```

### 3. 카테고리별 빌드 (full 또는 부분)

**docx 자료원은 `_extracted/` 캐시를 우선 사용.** 매 빌드마다 600KB docx 를 재추출 + grep 하지 말고 미리 split 된 § 단위 파일을 Read 한다.

```bash
# specs/<spec>/_extracted/ 가 부재하거나 docx mtime > cache 면 재생성
.venv/bin/python3 design/scripts/spec-split.py <spec>
```

산출 — `specs/<spec>/_extracted/<NN-slug>.md` + `_index.md` 인벤토리. cache fresh 면 즉시 종료, stale 또는 부재면 자동 재생성. **빌드 전에 한 번 호출** 후 카테고리별 작업은 _extracted/ 안의 § 파일을 Read.

| 카테고리 | _extracted 안 자료 | yaml 자료 |
|---|---|---|
| Summary | `04-overview.md` | — |
| Interface | `06-X-<service>.md` (§6.x.1 URI · §6.x.2 HTTP · §6.x.9 Security) | yaml `info`/`servers`/`security` |
| API | `06-X-<service>.md` (§6.x.3·§6.x.4) | **primary** — yaml `paths` |
| Data Model | `annex-a-...md` (보조) | **primary** — yaml + `resolve-yaml-refs.py` |
| Service Scenarios | `05-services-...md` (§5) | — |
| Cross-NF | `05-services-...md` + `06-X-...md` (호출 흐름) | yaml oAuth scope (보조) |
| Configuration | `06-X-<service>.md` (§6.x.8 supportedFeatures) | — |
| Error Handling | `06-X-<service>.md` (§6.x.7 ProblemDetails) | yaml `responses` |

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
- 도구 — `design/scripts/resolve-yaml-refs.py <yaml> <Schema1> <Schema2> ... --depth 8 --external-depth 1`.
- root schema 결정 — primary yaml(s) 의 `paths.*.requestBody.content.*.schema.$ref` 와 `paths.*.responses.*.content.*.schema.$ref` 의 unique 한 schema 이름 집합.
- 출력 — 각 root 마다 ` ```text ` 코드블록 안 트리. 도구 산출을 그대로 옮긴다 — 손으로 가공하면 도구 발전 시 페이지 정확성이 흐려지고, `/nf-status` 의 `data_model_chain_complete` 검사도 표면적 통과를 잃는다.
- 미해결 ref — `[TS XX.YYY] (참조 규격 미등록)` leaf 로 종료.

#### 3d. Service Scenarios
- 자료원 — primary docx 의 §5 (Services offered) + 트리거 절차의 stage 2 spec (예 23.502 §4.x).
- 출력 — Mermaid `sequenceDiagram` 다이어그램 2~4개. autonumber, NF 약어 participant, 메시지 라벨은 HTTP method · 경로 · 자료형. 분기는 alt/else.
- mermaid 블록은 .md 안에 그대로 둔다 — Obsidian·GitHub·VS Code 가 네이티브 렌더한다. 별도 SVG 산출은 하지 않는다.
- 다이어그램의 화살표·자료형은 본 spec 또는 OpenAPI 에 *실제로* 적힌 것만 — 추측한 흐름이 wiki 에 박히면 구현자를 오도하기 때문.

#### 3e. Cross-NF Dependencies
- **본 섹션의 책임 = 본 NF docx 명시 분 단편 표.** 자급자족 — 다른 NF 빌드 의존 없음. 본 NF docx 의 §5 (Services), §6.x (operation 별 호출 흐름) 에 적힌 cross-NF 호출만 표로.
- 자료원 — 본 NF 의 docx + manifest 의 cross-nf 카테고리 + yaml 의 oAuth scope (호출자 추정 보조).
- 출력 — 표. 열 — `상대 NF | 방향 | 트리거 | 출처 (docx 절·yaml)`.
- 자동 추출 도구는 부재 (sprint 후반). 그러나 본 NF docx 만으로 표가 채워지는 경우가 대부분 — *본 섹션은 placeholder 가 아니라 단편 표로 완성*.
- **여러 NF 합성 (cross-NF 호출 그래프 mermaid + 절차 매핑) 은 본 섹션 책임 아님.** 그건 `design/overviews/cross-nf-graph.md` 같은 합성 페이지에서. NSSF·AMF·SMF 등 여러 NF 의 본 섹션이 모인 *후* 별도 산출물로 작성.

| 산출 | 어디 | 무엇 | 의존 |
|---|---|---|---|
| 본 NF Cross-NF 단편 | `design/<nf>/3gpp-*.md` §Cross-NF | 본 NF docx 의 호출 표 | 본 NF docx 만 |
| 합성 호출 그래프 | `design/overviews/cross-nf-graph.md` (가칭) | 여러 NF 합성 + mermaid | 여러 NF 페이지 |

#### 3f. Configuration
- 자료원 — primary yaml 의 `info.x-gateway-rate-limit-policy`(있다면) + supportedFeatures 표 (docx §6.x.8) + default·timeout (docx 본문).
- 출력 — feature flag 표 (bit, 이름, 의미), 기본 timeout / expiry, 의존하는 OS / 라이브러리 설정.

#### 3g. Error Handling
- 자료원 — primary yaml 의 `responses` + docx §6.x.7 ProblemDetails cause 표.
- 출력 — markdown table. 열 = (HTTP code, ProblemDetails cause, 의미, 권장 복구 동작).

### 4. `index.md` 갱신
- root `index.md` 의 해당 NF 섹션을 본 페이지 항목으로 갱신.
- 신규 페이지 — `_(아직 페이지 없음)_` placeholder 또는 `## 다른 NF (예정)` 안내문에서 본 NF 를 빼내고 한 줄 항목으로 *교체*. NF 섹션 자체가 없으면 새로 추가 (Karpathy 트리 순서 유지).
- 갱신 페이지 — 항목이 이미 있으면 *내용 줄* 만 갱신 (예 version, 한 줄 설명).
- 형식 — `- [[{nf}/{wiki-stem}]] — TS NN.NNN v<x.y.z>, <한국어 한 줄 설명>`.

### 5. 결과 보고 (커밋 X)
- 갱신·신규 파일 목록 (`design/<nf>/3gpp-*.md`, 매니페스트는 변경 없음).
- 카테고리별 빌드 상태 — *완료 / placeholder / not-built (도구 부재)*.
- 미해결 leaf 목록 (Data Model 의 chain leaf, Cross-NF 의 자동 추출 미가용 등).
- 제안 commit 메시지 — `feat(<nf>): TS NN.NNN 페이지 7-카테고리 빌드 (또는 부분)`.
- 사용자 다음 액션 — `/nf-status <nf>` 로 acceptance gate 확인.

**커밋은 사용자 검토 후 별도 단계로** — 자동 수행하지 않는다.

## 예시

**호출 — full build (신규 NF).**

```
사용자: /nf-build nrf
조건:   design/nrf/_manifest.yaml.status.ready_for_build == true
산출:
  - design/nrf/3gpp-ts-29510.md (7 카테고리 골격 + 채움)
  - index.md 의 "## NRF" 섹션에 한 줄 항목 추가
보고: "Interface ✓, API ✓ 표 작성, Data Model ✓ chain leaf 0건,
       Service Scenarios — mermaid 3개 (다이어그램 4 권장 — 사용자가 추가),
       Cross-NF — 도구 부재로 placeholder, Configuration ✓, Error Handling ✓.
       다음 — `/nf-status nrf` 로 acceptance gate 확인."
```

**호출 — Data Model 만 부분 재빌드.**

```
사용자: specs/29.503/ 갱신 (예 i40 → j60). /nf-build nssf --data-model.
도구: resolve-yaml-refs.py 재호출, design/nssf/3gpp-ts-29531.md 의 ```text
       Data Model 트리만 *교체*. 다른 H2 섹션 (Service Scenarios 등) 은
       건드리지 않음.
보고: "Data Model 만 갱신. 1 leaf 해결 (RecurTime), trees 산출 +14/-7 줄."
```

## 자주 틀리는 지점

- 매니페스트 ready 가 아닌데 빌드 강행했는가 (`--force` 명시 없이).
- Data Model 트리를 손으로 가공해 도구 산출에서 벗어났는가 (acceptance check `data_model_chain_complete` 가 detect 함).
- 카테고리 부분 빌드 시 *다른 카테고리* 본문을 건드렸는가.
- 사용자 산문이 새 빌드로 사라졌는가.

## 참고 — 본 skill 안에 다시 적지 말 것

- 매니페스트 schema: `design/scripts/nf-manifest.py` docstring.
- Data Model chain 알고리즘: `design/scripts/resolve-yaml-refs.py` docstring.
- 디렉터리·파일명·언어 정책: `CLAUDE.md`.
