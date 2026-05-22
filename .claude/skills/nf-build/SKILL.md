---
name: nf-build
description: 매니페스트가 준비된 NF 에 대해 토픽 디렉터리 layout (handoff-v2) 으로 design 페이지를 생성·갱신하는 워크플로우. 사용자가 "/nf-build nssf", "NSSF 페이지 만들어", "NRF 빌드", "data-model 만 다시 뽑아", "build nf page" 등을 말하거나 NF 이름을 지정하면 무조건 이 skill 을 사용한다. 동작 — `design/<nf>/_manifest.yaml` 의 ready_for_build 가 true 인지 확인하고, `design/<nf>/_contract_seed.yaml` 의 categories/topics 정의에 따라 토픽 파일 (`design/<nf>/contract/<category>/<topic>.md` 또는 single-file `design/<nf>/contract/<category>.md`) 을 생성·갱신하고, data-model 토픽은 `resolve-yaml-refs.py --emit-json` 으로 `<topic>.json` 도 함께 emit, 마지막에 `build-handoff.py` 로 `handoff/<nf>/contract.yaml` (handoff-v2) 와 `validate-extraction.py` (basic 13) 를 한 사이클에서 호출한다. AUTO/USER marker 가 사람 산문 보존의 기계 계약 — frontmatter `generated_sections`·`user_sections` manifest 와 정확히 일치해야 한다. 매니페스트 생성·갱신은 sibling `/nf-init`, 완성도 검사는 `/nf-status` 의 책임이며 본 skill 은 페이지 *내용 생성* 에 집중한다. 커밋은 자동 수행 금지.
argument-hint: "<nf> [--<category>] [--topic <topic-id>]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/extract.py *) Bash(.venv/bin/python3 design/scripts/spec-split.py *) Bash(.venv/bin/python3 design/scripts/resolve-yaml-refs.py *) Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(.venv/bin/python3 design/scripts/materialize-contract.py *) Bash(.venv/bin/python3 design/scripts/build-handoff.py *) Bash(.venv/bin/python3 design/scripts/validate-extraction.py *) Bash(mkdir -p *) Bash(ls *) Bash(grep *) Bash(find *)
---

# nf-build — 토픽 디렉터리 layout (handoff-v2)

> Compatibility alias. Canonical user-facing wrapper is `/nf-contract-build`. This workflow generates spec-derived NF contract artifacts, not implementation code build artifacts.

## 입력
- `<nf>` — NF 이름. `design/<nf>/_manifest.yaml` + `design/<nf>/_contract_seed.yaml` 가 이미 존재해야 한다.
- `--<category>` — 부분 빌드. 카테고리 이름 (api, data-model, interface, error-handling, module-decomposition 등). seed.categories 에 등록된 것만.
- `--topic <topic-id>` — 단일 토픽만 (예 `--topic data-model/SliceInfoForRegistration`).
- 인자 없으면 seed 의 모든 활성 (`status ≠ draft, ≠ not_applicable`) 카테고리 빌드.

## 책임 분담

| 시나리오 | 사용 skill |
| --- | --- |
| 매니페스트 생성·보강 | `/nf-init` |
| 페이지·JSON 빌드·갱신 + handoff yaml emit | `/nf-build` (본 skill) |
| 페이지 완성도 검사 (gate) | `/nf-status` |
| 백업·재시작 | `/nf-init <nf> --primary <spec> --reset` |

## 동작 원칙 (이유 포함)

- **CLAUDE.md THE FOUR RULES 가 우선.** 추출 텍스트에 없는 사실을 본문에 끼워넣지 않는다.
- **`ready_for_build = false` 면 기본 거절, `--force` 시 시도.**
- **AUTO/USER marker 가 사람 산문 보존의 *기계 계약*.** 사람이 쓴 산문은 `USER:<id>:start/end` 안에만. 도구는 AUTO 영역만 매 빌드 새로 쓴다. frontmatter `generated_sections`·`user_sections` 가 진실 출처 — 본문 marker ID 집합과 정확히 일치해야 한다 (validator basic #7·#8).
- **Data Model = markdown(trace) + JSON(agent/codegen contract) 페어.** 두 산출 모두 `resolve-yaml-refs.py` 의 한 번 resolve 결과에서 emit — schema divergence 방지.
- **`_contract_seed.yaml` 가 토픽·카테고리·tasks 정의의 단일 입력.** seed 는 사람·본 SKILL 이 함께 편집. `build-handoff.py` 는 seed → `contract.yaml` (handoff-v2) 만 한다 — *해석* 은 seed 에 모이고 *조립* 은 도구에 모인다.
- **빌드 직후 validate-extraction.py 호출 의무.** 빌드 산출이 basic 13 룰을 통과하지 못하면 그 자리에 출력해서 사용자에게 알린다 — 다음 단계 (`/nf-status`) 까지 끌고 가지 않는다.
- **커밋은 자동 수행 안 함.**

## Workflow

### 1. 입력 검증
- `design/<nf>/_manifest.yaml` + `design/<nf>/_contract_seed.yaml` 존재 확인. 어느 하나라도 없으면 정지.
- seed 의 `categories` / `topics` 무결성 확인 (categories 의 layout ∈ {directory, single-file}, 토픽 ID 가 `<category>/<id>` 또는 `<category>` 형식).

### 2. 카테고리·토픽 결정
- 인자 없음 → seed 의 *활성* (`status ≠ draft, ≠ not_applicable`) 카테고리·토픽 전체.
- **Fresh full materialization 예외** — `design/<nf>/contract/` 가 부재하거나 seed.topics 중 파일 없는 topic 이 있으면, 활성 필터를 적용하지 않고 *seed.topics 전량* 을 materialize 한다 (draft 라도). 상세는 §"Fresh full materialization".
- `--<category>` → 해당 카테고리 산하 토픽만.
- `--topic <id>` → 단일 토픽만.

### 3. 카테고리별 빌드

**docx 자료원은 `_extracted/` 캐시 우선 사용.** `spec-split.py` 가 cache 보장.

| 카테고리 | layout | 자료원 | AUTO 섹션 | USER 섹션 |
|---|---|---|---|---|
| interface | single-file | yaml `info`/`servers`/`security` + docx §6.x.1·6.x.2·6.x.9 | `auth-block`, `transport-block` | `implementation-notes` |
| api | directory | yaml `paths.<op>` + docx §6.x.3·6.x.4 | `api-matrix`, `request-schema`, `response-schema` | `implementation-notes` |
| data-model | directory | yaml + `resolve-yaml-refs.py` | `chain-tree`, `field-table` | `implementation-notes` |
| module-decomposition | directory | 사람이 정한 분해 의도 | `module-graph` (mermaid) | `responsibility-prose`, `implementation-notes` |
| error-handling | single-file | yaml `responses` + docx §6.x.7 | `error-matrix` | `recovery-prose`, `implementation-notes` |
| (MVP 외) service-scenarios / behavior-state / failure-policy / configuration / persistence / test-matrix / work-plan / cross-nf | (미정) | 후속 사이클 | — | — |

#### 3a-3e: AUTO 영역 갱신
- 본 SKILL 이 marker `<!-- AUTO:<id>:start --> ... <!-- AUTO:<id>:end -->` 안만 덮어쓴다.
- 사람 산문이 USER 영역에 남아있으면 *위치 보존* (전후 컨텍스트 재정렬 시에도 같은 자리에).
- frontmatter `generated_sections`·`user_sections` 도 본 SKILL 이 갱신 — manifest 와 본문 marker 가 어긋나면 사용자에게 즉시 보고 (validator basic #8 의 사전 검출).

#### 3c (특수): Data Model 토픽
- 각 토픽에 대해 두 산출 동시 emit.
  - markdown — `design/<nf>/contract/data-model/<id>.md` (AUTO `chain-tree` = `resolve-yaml-refs.py` text 트리, AUTO `field-table` = 표, USER `implementation-notes` 보존).
  - JSON — `design/<nf>/contract/data-model/<id>.json` = `resolve-yaml-refs.py --emit-json --topic-id data-model/<id> --nf <nf> --handoff-topics <list-of-data-model-topics>` 산출 그대로 (완전 AUTO).
- handoff-topics 인자에는 seed 의 모든 data-model 토픽 ID 를 전달 — 그래야 transitive 가 inline 으로 펼쳐지지 않고 `{ "topic": ... }` 참조로 남는다.

### 4. seed 갱신
- 새 토픽이 추가됐다면 seed 의 `topics` 항목에도 추가. spec_refs / depends_on / related / error_refs 가 사람이 정의.
- AUTO 갱신 시 status 가 자동으로 바뀌지 *않는다* — status 는 사람이 의도로 결정 (draft → handoff_ready 격상은 명시적 의도).

### 5. fresh full materialize + handoff yaml emit + validate
```bash
.venv/bin/python3 design/scripts/materialize-contract.py <nf>
.venv/bin/python3 design/scripts/build-handoff.py <nf>
.venv/bin/python3 design/scripts/validate-extraction.py <nf> --level basic
```

- 첫 번째 — §"Fresh full materialization" 의 구현. seed.topics 전량을 `design/<nf>/contract/<category>/<topic>.{md,json}` 으로 emit 하고 spec-derived topic 의 status 를 promotion 한다. fresh checkout · `--reset` 후 design contract dir 부재 상태에서 *반드시* 선행해야 §3 의 topic file 들이 실제 디스크에 존재 — `build-handoff.py` 가 seed→yaml 변환만 하므로 본 step 이 빠지면 validate basic #3·#9 가 false FAIL. 멱등 (AUTO 만 재생성, USER marker 산문 보존).
- 두 번째 — `handoff/<nf>/contract.yaml` 갱신 (handoff-v2, agent_contract 포함).
- 세 번째 — basic 13 룰 검사. FAIL 가 1개라도 있으면 사용자에게 그 자리에 보고 (`/nf-status` 까지 끌고 가지 않음).

### 6. 결과 보고 (커밋 X)
- 신규·갱신 파일 목록.
- 카테고리별 빌드 상태 + validate-extraction 결과 (PASS X, FAIL Y).
- 미해결 leaf (Data Model JSON 의 `unresolved_refs` 등).
- 제안 commit 메시지.
- 사용자 다음 액션 — `/nf-status <nf>` 또는 사용자 prose 보강 위치.

## Fresh full materialization

> §5 의 첫 step `materialize-contract.py <nf>` 의 동작 명세. fresh checkout · `--reset` 후 design contract dir 부재 상태에서 사람 수동 저작 없이 `handoff_ready` 까지 도달 가능하게 하는 메커니즘.

`design/<nf>/contract/` 부재 (fresh checkout, `--reset` 후 등) 또는 seed.topics 중 파일 없는 topic 존재 시, *사람 수동 저작 없이* seed.topics 전량을 materialize 해 `handoff_ready` 까지 도달 가능하게 한다. 사람 = spec 원본·범위·`manual_overrides` 정책·결과 리뷰·선택적 USER prose. 그 외 전부 script/agent.

근거 — `validate-extraction.py` basic #3 (topic file exists)·#9 (data-model machine_file)·#4 (xref target) 는 *실제 topic 파일* 을 요구한다. fresh 는 파일 부재라 `validate_extraction_basic` FAIL → `handoff_ready` 도달 불가. 본 모드가 그 파일들을 전량 생성한다.

### 카테고리별 marker/frontmatter schema (구현 계약)

각 topic `.md` frontmatter 필수 — `topic_id`, `category`, `status`, `generated_sections: [<AUTO id…>]`, `user_sections: [<USER id…>]`. 본문 marker `<!-- AUTO:<id>:start/end -->` / `<!-- USER:<id>:start/end -->` 가 frontmatter 와 *1:1*, id 중복 0 (validate #7/#8). 필수키 (`topic_id`/`category`/`status`) 는 validate 가 강제하지 않으므로 *materializer self-check* 로 보장.

| 카테고리 | layout | AUTO marker id | USER marker id | 격상 |
|---|---|---|---|---|
| interface | single-file | `auth-block`, `transport-block` | `implementation-notes` | spec-derived → 자동 |
| api/&lt;op&gt; | directory | `api-matrix`, `request-schema`, `response-schema` | `implementation-notes` | spec-derived → 자동 |
| data-model/&lt;schema&gt; | directory | `chain-tree`, `field-table` (+ `<schema>.json` machine_file) | `implementation-notes` | spec-derived → 자동 (JSON `unresolved_refs` 빈 경우만) |
| error-handling | single-file | `error-matrix` | `recovery-prose`, `implementation-notes` | spec-derived → 자동 |
| module-decomposition/&lt;mod&gt; | directory | `module-graph` (placeholder/minimal — no inferred decomposition) | `responsibility-prose`, `implementation-notes` | draft 유지 |
| service-scenarios/&lt;sc&gt; | directory | `scenario-index` | `scenario-prose` | draft 유지 |
| behavior-state/&lt;st&gt; | directory | `state-index` | `state-prose` | draft 유지 |
| failure-policy | single-file | `failure-index` | `policy-prose` | draft 유지 |
| configuration | single-file | `config-index` | `policy-notes` | draft 유지 |
| persistence | single-file | `persistence-index` | `policy-notes` | draft 유지 |
| test-matrix | single-file | `test-index` | `coverage-notes` | draft 유지 |
| work-plan | single-file | `work-index` | `plan-notes` | draft 유지 |
| cross-nf | single-file | `crossnf-index` | `crossnf-notes` | draft 유지 |

- **seed.topics 에 존재하는 topic 만 materialize. seed 에 없는 topic/category 를 invent 하지 않는다.**
- draft-유지 카테고리 AUTO `*-index` = seed 도출 최소 인덱스 (topic 목록·spec_refs). USER = `TODO:` placeholder. spec 추출 불가 내용은 USER TODO 로만.
- refs 에 `#anchor` 포함 시 validate 가 anchor/heading 실재를 본다 — skeleton 은 해당 anchor 를 생성하거나 생성 refs 에서 anchor 회피.

### 자동격상 알고리즘 (preflight 순서)

"validate basic 통과 후 격상" 은 #11 (JSON status ↔ handoff topic status) 순환이다. 격상 조건은 *최종 validate 이전의 validate-basic-equivalent **preflight*** 로 산출한다.

1. seed.topics 전량 materialize. 최초 status = seed 기준 `draft`.
2. preflight — 각 topic 의 AUTO marker / frontmatter sync / refs target / data-model JSON parse·dependencies·`unresolved_refs` 를 *검사만* (status 미변경).
3. promotion set — spec-derived (api·data-model·interface·error-handling) 중 preflight 통과한 것만. data-model 은 JSON `unresolved_refs` 가 *비어있을 때만* 격상.
4. promotion set status 를 `handoff_ready` 로 갱신 — **seed/handoff source status 와 data-model JSON status 를 *동일* 값으로 함께** (#11 정합).
5. category status — *모든 하위 topic 이 격상된 카테고리만* `handoff_ready`. 하나라도 draft 면 category `draft` 유지 (#5 정합).
6. `build-handoff.py` → 최종 `validate-extraction.py --level basic` (격상 후 실행).

- status 쓰기 위치 — materializer 가 `_contract_seed.yaml` 의 topic/category status 를 갱신 (build-handoff.py 가 seed 를 읽으므로 promotion 이 seed 에 반영돼야 handoff yaml 에 전달). `build-handoff.py` 자체는 무변경.
- `draft` 로 남는 topic 은 최종 handoff 의 `tasks.*.read` 필수 참조에서 제외 (validate #4 는 target 존재만 보고 draft 를 막지 않으므로 materializer 가 보장).

## 자주 틀리는 지점
- `_contract_seed.yaml` 없이 도구를 직접 호출했는가 — build-handoff.py 가 SystemExit.
- 사용자 산문이 AUTO 영역에 들어갔는가 — 다음 빌드에 덮어쓰여 사라진다. USER 영역으로 옮긴 뒤 frontmatter 갱신.
- frontmatter `generated_sections` / `user_sections` 가 본문 marker 와 sync 안 됨 — validator basic #8 FAIL.
- data-model JSON 의 `unresolved_refs` 가 비어있지 않은데 topic.status 가 canonical/handoff_ready — validator basic #12 FAIL.

## 참고 — 본 skill 안에 다시 적지 말 것
- handoff-v2 schema, agent_contract 내용, marker 정책 어휘 — spec `2026-05-12-nf-build-restructure-agent-consumable-mvp.md` §1, §2.
- 매니페스트 schema — `design/scripts/nf-manifest.py` docstring.
- Data Model chain·JSON emit 알고리즘 — `design/scripts/resolve-yaml-refs.py`.
- 13 basic 룰 정의 — `design/scripts/validate-extraction.py` docstring + spec §4.
- 디렉터리·파일명·언어 정책 — `CLAUDE.md`.
