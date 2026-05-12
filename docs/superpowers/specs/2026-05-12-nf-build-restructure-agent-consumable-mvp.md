---
title: nf-build 재설계 — agent-consumable handoff-v2 MVP (NSSelection proof)
date: 2026-05-12
status: design-approved
authors: [yjprak (review·decision), Claude (synthesis)]
scope: 5gc-impl-kb / design + handoff layer (MVP cut)
related:
  - docs/superpowers/specs/2026-05-12-nf-build-restructure-design.md   # direction spec — 본 MVP 의 상위
  - CLAUDE.md
  - .claude/skills/nf-init/SKILL.md
  - .claude/skills/nf-build/SKILL.md
  - .claude/skills/nf-status/SKILL.md
---

# nf-build 재설계 — agent-consumable handoff-v2 MVP

## §0. 본 spec 메타

- **상위 spec** — `2026-05-12-nf-build-restructure-design.md` (방향성). 본 spec 은 그 위의 *첫 구현 사이클* 정의.
- **핵심 가정 변경** — 본 KB 의 dev consumer 는 사람이 아닌 **LLM agent**. 따라서 spec 의 진짜 가치는 *사람이 읽기 좋은 문서* 가 아니라 *agent 가 잘못된 context 를 만들지 않도록 read boundary 를 강제하는 구조*.
- **MVP 범위** — NSSF 의 `NSSelectionGet` 1 API 만. 13 카테고리 전부 아닌, NSSelection 구현에 필요한 최소 토픽만 (api / data-model / module-decomposition / interface / error-handling).
- **MVP 종료 조건** — `handoff/nssf/_handoff.yaml` 단독 입력으로 LLM agent 가 NSSelectionGet 구현 plan 을 안전하게 (`must_not` 위반 없이) 생성. PASS 시 같은 패턴으로 NSSF 나머지 API → 다른 카테고리 → 다른 NF 로 확장 (별도 사이클).
- **본 spec 이 *하지 않는* 일** — 13 카테고리 전체 구현, AMF/SMF/UPF 확장, prose 전수 이주, strict validator, retro 의존 결정. 모두 후속 사이클.

---

## §1. Agent Contract — handoff-v2 top-level block

`handoff/<nf>/_handoff.yaml` 의 단일 entry point 원칙을 깨지 않기 위해 contract 를 yaml top-level 에 박는다. NF 별로 may_decide·must_ask_or_block 이 다를 수 있어 NF 산출에 contract 가 포함되는 게 자연스럽다.

### Schema

```yaml
schema_version: handoff-v2
nf: nssf

agent_contract:
  # 우선 순위 — 토픽 status 가 카테고리 status 보다 우선 (토픽이 실제 작업 단위).
  # 카테고리 status 는 "이 카테고리에 새 토픽을 더 추가할지" 의 메타 정보.
  # 예 — module-decomposition (category=draft) + SelectionEngine (topic=handoff_ready)
  # 는 *MVP scope 한정 의도* 이며 정상. 반대 (category=handoff_ready, topic=draft) 는
  # validator basic #5 가 FAIL 처리.
  status_precedence: topic_over_category

  # Agent 가 자료를 어떤 순서로 읽는다.
  default_read_order:
    - handoff/<nf>/_handoff.yaml                       # 1. 본 yaml — 토픽 ID·status·depends_on
    - handoff/<nf>/_handoff.yaml#categories            # 2. 카테고리 status — 진입 가능 토픽 필터 (참조용, 토픽 status 가 최종)
    - design/<nf>/<topic>/<id>.md (target)             # 3. 작업 대상 토픽
    - design/<nf>/<topic>/<id>.json (target machine)   # 4. data-model 토픽이면 .json 도 (agent/codegen primary)
    - design/<nf>/<topic>/<dep>.md (depends_on)        # 5. 의존 토픽
    - design/<nf>/error-handling.md (error_refs)       # 6. 오류 처리
    - design/<nf>/<topic>/<related>.md (related)       # 7. 관련 토픽 (필요 시)

  # Agent 가 절대 하지 말아야 할 것 — 안전선.
  must_not:
    - status 가 draft 인 토픽으로 구현 시작
    - status 가 blocked 인 토픽으로 구현 시작
    - status 가 not_applicable 인 토픽을 *생성*
    - spec_refs / Implementation Notes 에 근거 없는 행동·자료형·정책 invent
    - agent_contract 외부 (design/<nf>/_archive/) 의 자료를 현행 contract 로 인용

  # Agent 의 재량 영역 — 효율선.
  may_decide:
    - 내부 패키지/모듈 이름 (design 산출이 라이브러리 비종속)
    - 프레임워크 종속 handler 구조 (예 router 등록 방식)
    - 테스트 프레임워크 매핑 (Test Matrix 의 케이스 → 실제 test runner)
    - 로깅 라이브러리 선택 (Configuration 의 관측 키 충족 한)

  # Agent 가 멈추고 사람/도구 escalation — escalation 선.
  must_ask_or_block:
    - 필수 정책 값 부재 (timeout/retry/idempotency 미정)
    - OpenAPI chain leaf 가 `(참조 규격 미등록)` 인데 구현이 필요
    - 역방향 status 불일치 (category=handoff_ready 인데 산하 topic=draft) — basic #5 영역. 정방향 (category=draft, topic=handoff_ready + scope) 은 MVP 한정 의도이므로 정상
    - depends_on 의 target 토픽이 yaml 에 부재
    - Cross-NF 호출의 상대 NF op 가 아직 미정의

# (생략 — categories / topics / spec_index / version 등 v2 기본 schema)
```

### Agent contract 위치 결정 — A 안 (handoff-v2 top-level)

| 옵션 | 결정 |
|---|---|
| **A. handoff-v2 yaml top-level** | **채택** — single entry point 원칙 유지, 산출 = contract 의 진실 출처, NF 별 분기 가능 |
| B. 별도 `agent-contract.yaml` | 거부 — agent 가 2 파일 로드. single entry 원칙 깨짐 |
| C. spec 본문 (constitutional doc) | 거부 — agent 가 spec parsing 필요. 산출 외 의존 추가 |

### Status enum + 행동 매핑 (재인용, 상위 spec §4 와 일치)

| status | agent 행동 |
|---|---|
| `canonical` | 그대로 사용. 변경 금지 |
| `handoff_ready` | 구현 가능. plan 진입 |
| `draft` | 구현 금지. `must_ask_or_block` 발동 |
| `blocked` | 구현 금지. 의존 spec/tool 수정 필요 |
| `not_applicable` | 생성 금지. NF profile 상 부적용 |

---

## §2. Marker 정책 — generated vs user

자연어 원칙 ("사용자 산문 보존") 만으로는 agent 가 재빌드 시 산문을 손실시킨다. marker 기반 *기계 계약* 으로 격상.

### Markdown 블록 marker

```markdown
<!-- AUTO:api-matrix:start -->
| path | method | idempotent | timeout | scope |
| ... | ... | ... | ... | ... |
<!-- AUTO:api-matrix:end -->

<!-- USER:implementation-notes:start -->
### Implementation Notes

NSSF 의 selection 알고리즘에서 ... (사람이 작성한 산문)
<!-- USER:implementation-notes:end -->
```

### 규칙

1. **`AUTO:<id>:start` ~ `AUTO:<id>:end`** — `build-handoff.py` / `/nf-build` 가 *덮어쓰기 허용*. 사람이 수정해도 다음 빌드에서 재생성.
2. **`USER:<id>:start` ~ `USER:<id>:end`** — 도구가 *건드리지 않는다*. 사람만 수정. 도구는 *위치 보존* (전후 컨텍스트 재정렬 시에도 같은 자리에 남긴다).
3. **marker 외부** — 도구는 *추가만 가능, 기존 텍스트 변경 금지*. (전이기간 안전선)
4. **marker ID 충돌** — 같은 파일 안에 같은 `<id>` 가 2번 나오면 validator FAIL.
5. **frontmatter manifest** — 토픽 파일 frontmatter 에 `generated_sections` 리스트 명시:

```yaml
---
id: api-nsselection-get
status: handoff_ready
generated_sections:
  - api-matrix
  - errors-table
user_sections:
  - implementation-notes
---
```

이 manifest 가 *진실 출처* — marker 가 manifest 와 mismatch 면 validator FAIL.

### MVP 범위에서 marker 적용 위치

| 카테고리 | AUTO 섹션 | USER 섹션 |
|---|---|---|
| api | `api-matrix`, `request-schema`, `response-schema` | `implementation-notes` |
| data-model | `chain-tree`, `field-table` | `implementation-notes` |
| module-decomposition | `module-graph` (mermaid) | `responsibility-prose`, `implementation-notes` |
| interface | `auth-block`, `transport-block` | `implementation-notes` |
| error-handling | `error-matrix` | `recovery-prose`, `implementation-notes` |

### Data Model JSON — marker 대상 아님 (별도 정책)

`data-model/<id>.json` 은 markdown marker 정책 대상 *아니다*. 완전 **AUTO machine artifact** — `/nf-build --data-model` (내부 `resolve-yaml-refs.py --emit-json`) 가 매 빌드 전체 재생성. 사람 편집 금지.

- 정책 — markdown 은 trace/prose, JSON 은 agent/codegen contract. 두 산출 모두 같은 yaml chain 해석의 결과.
- 사람 산문은 markdown 의 `Implementation Notes` (USER section) 에만. JSON 에는 prose field 없음.
- JSON 안에는 agent 행동 계약 (`may_codegen` 등) 을 *박지 않는다*. 행동 계약은 handoff-v2 top-level `agent_contract` 가 단일 진실 출처.

---

## §3. Work Plan = Agent Task Graph

상위 spec 의 카테고리 12 (Work Plan) 를 *사람용 WBS* 가 아닌 *기계 실행 가능 task graph* 로 재정의. MVP 에서는 Work Plan 카테고리 자체가 yaml 산출 (markdown 보조).

### Task schema

```yaml
# handoff/<nf>/_handoff.yaml#tasks (또는 별도 work-plan.yaml — MVP 는 같은 yaml 안)
tasks:
  nssf-api-nsselection-get:
    phase: 02-api-implementation
    goal: Implement NSSelectionGet handler
    read:
      - api/NSSelectionGet
      - data-model/SliceInfoForRegistration.machine_file        # JSON normalized schema (agent/codegen primary)
      - data-model/SliceInfoForRegistration                     # markdown (trace/Implementation Notes)
      - data-model/AuthorizedNetworkSliceInfo.machine_file
      - data-model/AuthorizedNetworkSliceInfo
      - module-decomposition/SelectionEngine
      - interface
      - error-handling#nsselection
    produces:
      - <impl-path>/api/nsselection.*       # 구체 path 는 dev agent 결정 (may_decide)
      - <test-path>/test_nsselection.*
    blocked_by:
      - api/NSSelectionGet.status not in [canonical, handoff_ready]
      - data-model/SliceInfoForRegistration.status not in [canonical, handoff_ready]
      - data-model/AuthorizedNetworkSliceInfo.status not in [canonical, handoff_ready]
    acceptance:
      - handles 200/400/403/404 responses (per error-handling matrix)
      - preserves idempotency policy from api-matrix
      - negative test exists for each error cause
      - request/response schema match yaml chain
```

### 필드 의미

| 필드 | 의미 | agent 사용법 |
|---|---|---|
| `phase` | 사람이 정한 순서 그룹 | 동 phase 내 task 는 병렬 가능, phase 간은 dependency |
| `goal` | 한 줄 자연어 의도 | agent 의 plan summary 입력 |
| `read` | context bundle | agent 가 *이것만* 읽는다 — 외부 토픽 invent 금지 |
| `produces` | 산출 path glob | dev 코드의 *어디에* 가 나오는지. 구체 이름은 `may_decide` |
| `blocked_by` | 진입 차단 조건식 | 한 줄이라도 참이면 task 진입 금지 (`must_ask_or_block`) |
| `acceptance` | 완료 self-judge 기준 | agent 가 자기 plan 의 done 판정 |

### 사람 자리 vs 도구 자리

| 결정 | 누구 |
|---|---|
| phase 묶음·순서·dependency 의도 | **사람** |
| read/produces/blocked_by/acceptance 의 *컴파일* | **도구** (build-handoff.py v2 가 카테고리·status·depends_on 으로부터 자동 생성) |
| goal 한 줄 | **사람** (Implementation Notes 와 같은 톤) |

---

## §4. Validator MVP — basic hard gate + strict report-only

상위 spec 의 `validate-extraction.py` 를 MVP 에서는 **basic only** 로 한정. strict 룰은 report 만 (FAIL 으로 gate 막지 않음).

### Basic (hard gate, FAIL → handoff_ready 통과 차단)

| # | 룰 | 검사 방법 |
|---|---|---|
| 1 | `handoff-v2` schema valid | yaml 구조 검증. MVP 는 *경량 Python validator* (`design/scripts/validate-extraction.py` 안 dict 검사). jsonschema 외부 의존성 추가하지 않음 — 후속 사이클에서 dependency 추가 여부 별도 결정 |
| 2 | status enum valid | 값이 `[canonical, handoff_ready, draft, blocked, not_applicable]` 안 |
| 3 | topic file exists | 모든 토픽 ID 가 path resolution 규칙 (§4.5) 으로 실존 파일에 매핑 |
| 4 | cross-reference target exists | 각 토픽의 `depends_on` · `related` · `error_refs` 와 `tasks[*].read` 가 모두 yaml 안 다른 토픽 ID (또는 토픽#anchor) 와 일치. depends_on 만이 아닌 *모든 cross-ref* 검증 |
| 5 | category/topic consistency | category status 가 `handoff_ready` 면 산하 모든 topic status 도 `handoff_ready` 또는 `canonical` |
| 6 | blocked/not_applicable semantics | `blocked` 는 `blocked_reason` 필수, `not_applicable` 은 `na_reason` 필수 + 본문 비어있어야 함 |
| 7 | **marker ID unique** | 같은 파일 안에 `AUTO:<id>` 또는 `USER:<id>` 가 2번 이상 등장하면 FAIL (§2 의 marker 원칙) |
| 8 | **frontmatter ↔ marker sync** | 토픽 파일 frontmatter 의 `generated_sections` · `user_sections` 가 본문의 marker ID 집합과 정확히 일치. marker 정책은 MVP 의 *핵심 안전장치* 이므로 report-only 가 아닌 hard gate |
| 9 | **data-model machine_file 존재** | 카테고리 `data-model` 의 모든 토픽은 handoff yaml 에 `machine_file` 키 + 해당 path (`design/<nf>/data-model/<id>.json`) 실존 파일 둘 다 필수 |
| 10 | **machine_file JSON parse valid** | `json.load` 가능. `schema_version`·`topic_id`·`status`·`fields`·`dependencies`·`unresolved_refs` 필드 존재 |
| 11 | **JSON ↔ handoff topic 정합** | JSON 의 `topic_id` 가 handoff topic ID 와 일치. JSON 의 `status` 가 handoff topic `status` 와 일치 (둘이 다르면 어느 쪽이 진실인지 불명) |
| 12 | **JSON unresolved_refs ↔ status** | JSON `unresolved_refs` 가 비어있지 *않으면* topic status 는 `blocked` 또는 `draft` 여야 함. `canonical` / `handoff_ready` 상태인데 unresolved 가 남아있으면 FAIL |
| 13 | **JSON dependencies target exists** | JSON `dependencies` 는 *handoff topic ID 만* 담는다 (transitive non-handoff schema 는 `_inlined_from` 으로 inline). 등록된 모든 ID 가 handoff topics 안에 존재해야 함. cross-ref 검증의 JSON 변형 (basic #4 의 markdown 측면과 paired) |

### 4.5 Path resolution 규칙 (basic #3 의 정의)

| layout | path | 예 |
|---|---|---|
| `directory` 카테고리 | `design/<nf>/<category>/<topic-id>.md` | `api/NSSelectionGet` → `design/nssf/api/NSSelectionGet.md` |
| `single-file` 카테고리 | `design/<nf>/<category>.md`. 토픽 ID == 카테고리 ID | `interface` → `design/nssf/interface.md` |
| 토픽#anchor | `<topic-path>` 안 `<a id="anchor">` 또는 `## <heading>` 슬러그 일치 | `error-handling#nsselection-400` → `design/nssf/error-handling.md` 안 `nsselection-400` anchor |

### Strict (report only, MVP 단계에선 차단 안 함)

- service flow participant 가 spec 의 actor 어절과 정확히 일치
- docx § anchor exact match
- error cause 전수 매칭 (yaml responses × ProblemDetails cause)
- mermaid op semantic validation (sequenceDiagram 화살표 자료형 = yaml schema 와 일치)

### 출력 형식

```text
[validate-extraction] nssf --level basic
  basic: PASS 12, FAIL 1
    FAIL #4 cross-reference target: api/NSSelectionGet → data-model/SliceInfoForRegistration_v2 (yaml 에 없음)
  strict (report): WARN 3
    WARN service flow participant: SD-1 에 "OAuth Server" — spec 어절 "NRF" 와 mismatch
```

### `/nf-status` 와의 관계

`/nf-status` 의 `handoff_ready` gate 는 본 validator 의 *basic* 모두 PASS 를 포함. strict 는 `canonical` gate 에서도 권고 (hard 아님) — MVP 후속에서 결정.

---

## §5. MVP 범위 — NSSelection 1 API

### 대상 토픽 (NSSelection 구현 최소 집합)

```
design/nssf/
├── _manifest.yaml                # 기존 유지
├── interface.md                  # 단일 파일 — auth/transport
├── error-handling.md             # 단일 파일 — error matrix
├── api/
│   └── NSSelectionGet.md         # 디렉터리·토픽 — 1 API
├── data-model/
│   ├── SliceInfoForRegistration.md         # trace / Implementation Notes
│   ├── SliceInfoForRegistration.json       # AUTO machine artifact (agent/codegen primary)
│   ├── AuthorizedNetworkSliceInfo.md
│   └── AuthorizedNetworkSliceInfo.json
└── module-decomposition/
    └── SelectionEngine.md
```

총 6 토픽 + 2 machine_file (.json). 13 카테고리 중 5 카테고리만 MVP. **MVP 외 카테고리** — Service Scenarios, Behavior & State, Failure Policy, Test Matrix, Work Plan (graph 형태로는 yaml 안에만), Configuration, Persistence Design, Cross-NF Calls — 본 MVP 에서는 *생성하지 않거나 빈 placeholder + status=draft*.

### 산출 — handoff-v2 yaml MVP 예시 (요약)

```yaml
schema_version: handoff-v2
nf: nssf

agent_contract:
  # §1 참조

categories:
  api:
    status: handoff_ready
    layout: directory
  data-model:
    status: handoff_ready
    layout: directory
  module-decomposition:
    status: draft        # MVP 안 — SelectionEngine 1 모듈만 작성
    layout: directory
  interface:
    status: handoff_ready
    layout: single-file
  error-handling:
    status: handoff_ready
    layout: single-file
  # MVP 범위 밖
  service-scenarios:    { status: draft, layout: directory }
  behavior-state:       { status: draft, layout: directory }
  failure-policy:       { status: draft, layout: single-file }
  configuration:        { status: draft, layout: single-file }
  persistence:          { status: draft, layout: single-file, deferred_reason: "NSSelectionGet MVP 범위 밖. stateless 여부는 구현 정책 결정 — agent_contract.may_decide 가 아닌 별도 decision record 로 후속 사이클에서 처리" }
  test-matrix:          { status: draft, layout: single-file }
  work-plan:            { status: draft, layout: single-file }
  cross-nf:             { status: draft, layout: single-file }

topics:
  api/NSSelectionGet:
    status: handoff_ready
    depends_on:
      - data-model/SliceInfoForRegistration
      - data-model/AuthorizedNetworkSliceInfo
    related: [interface]
    error_refs: [error-handling#nsselection-400, error-handling#nsselection-403]
    spec_refs: [TS 29.531 §5.2.2.2.1]
  data-model/SliceInfoForRegistration:
    status: canonical
    file: design/nssf/data-model/SliceInfoForRegistration.md           # trace / Implementation Notes (markdown)
    machine_file: design/nssf/data-model/SliceInfoForRegistration.json  # AUTO normalized schema (agent/codegen primary)
    spec_refs: [TS 29.531 §6.1.6.2.4]
  data-model/AuthorizedNetworkSliceInfo:
    status: canonical
    file: design/nssf/data-model/AuthorizedNetworkSliceInfo.md
    machine_file: design/nssf/data-model/AuthorizedNetworkSliceInfo.json
    spec_refs: [TS 29.531 §6.1.6.2.5]
  module-decomposition/SelectionEngine:
    status: handoff_ready
    scope: "NSSelectionGet MVP only"   # 카테고리 module-decomposition 은 draft 지만, 본 토픽만 MVP 범위에서 handoff_ready (다른 모듈은 후속 사이클)
    file: design/nssf/module-decomposition/SelectionEngine.md
    spec_refs: []                       # spec 기반 아닌 *사람이 정한 분해 의도*. Implementation Notes 가 진실
  interface:
    status: handoff_ready
    spec_refs: [TS 29.531 §6.1.1, §6.1.5]
  error-handling:
    status: handoff_ready
    spec_refs: [TS 29.531 §6.1.7]

tasks:
  nssf-api-nsselection-get:
    # §3 참조

# spec ref → topic ID 역방향 lookup (상위 spec §4 와 일치).
# agent 가 "TS 29.531 §X 를 구현하라" 요청 시 어떤 토픽을 읽어야 하는지 즉시 결정.
spec_index:
  "TS 29.531 §5.2.2.2.1":  [api/NSSelectionGet]
  "TS 29.531 §6.1.6.2.4":  [data-model/SliceInfoForRegistration]
  "TS 29.531 §6.1.6.2.5":  [data-model/AuthorizedNetworkSliceInfo]
  "TS 29.531 §6.1.1":      [interface]
  "TS 29.531 §6.1.5":      [interface]
  "TS 29.531 §6.1.7":      [error-handling]

# spec ref → source file (별도 블록 — spec_index 와 의미 분리).
sources:
  "TS 29.531": specs/29.531/29531-i40.docx
```

### MVP 에서 *수동* 으로 이주할 prose (M.5 등가)

기존 단일 `design/nssf/3gpp-ts-29531.md` 에서 위 6 토픽에 해당하는 부분만 분리. *전체 이주가 아니다* — MVP 외 카테고리의 prose 는 archive 로 보내고 후속 사이클에서 다룬다.

### Data Model JSON — 산출 형식 + 도구 자리

**도구 자리** — `design/scripts/resolve-yaml-refs.py` 를 *확장* (`--emit-json` 옵션 또는 markdown 산출과 동시 emit). 별도 신설 도구 추가하지 않음 — 이미 chain resolve 책임이 같은 도구에 있음.

**Schema 해석 정책 — transitive 는 inline-resolved**

JSON 안의 sub-schema 는 두 가지로 처리:

- **Handoff topic 인 schema** — `{ "topic": "data-model/<id>" }` 로 *참조만*. `dependencies` 리스트에도 등록. agent 는 해당 토픽의 `.json` 을 따로 read.
- **Handoff topic 이 아닌 transitive schema** (`AllowedSnssai` 등) — **inline 으로 펼친다**. OpenAPI ref 출처는 `_inlined_from` 메타로 추적. `dependencies` 에 등록하지 *않음*.

이 정책의 의도 — `dependencies` 가 *handoff topic 간 의존* 만 표현. validator basic #13 ("JSON dependencies target exists in handoff topics") 의 의미를 명확히 하고, MVP 의 작은 토픽 집합으로도 정합한 JSON 산출 가능.

**산출 형식 (예시 — `AuthorizedNetworkSliceInfo.json`)**

MVP 에서 AuthorizedNetworkSliceInfo 는 `AllowedSnssai` 같은 transitive 만 의존 (handoff topic 의존 없음). 따라서 `dependencies: []`, AllowedSnssai 는 inline.

```json
{
  "schema_version": "data-model-v1",
  "nf": "nssf",
  "topic_id": "data-model/AuthorizedNetworkSliceInfo",
  "status": "canonical",
  "source": {
    "spec_refs": ["TS 29.531 §6.1.6.2.5"],
    "openapi_refs": ["#/components/schemas/AuthorizedNetworkSliceInfo"],
    "source_yaml": "specs/29.531/TS29531_Nnssf_NSSelection.yaml"
  },
  "root_schema": "AuthorizedNetworkSliceInfo",
  "fields": [
    {
      "name": "authorizedNssai",
      "required": true,
      "type": "array",
      "items": {
        "_inlined_from": "#/components/schemas/AllowedSnssai",
        "type": "object",
        "properties": [
          { "name": "allowedSnssai", "required": true, "type": "object", "_inlined_from": "#/components/schemas/Snssai" },
          { "name": "nsiInformationList", "required": false, "type": "array", "items": { "_inlined_from": "#/components/schemas/NsiInformation" } }
        ]
      },
      "nullable": false
    }
  ],
  "dependencies": [],
  "unresolved_refs": []
}
```

**JSON 안에 두지 않는 것** —

- `agent_contract` (`may_codegen` 등) — handoff-v2 top-level 의 단일 진실. JSON 안 중복 금지.
- 사람 산문 — markdown 의 `Implementation Notes` (USER section) 에만.
- spec 어절 인용 prose — markdown 본문에만 (JSON 은 `spec_refs` 메타로만 추적).

---

## §6. Agent Proof — MVP acceptance

본 MVP 가 PASS 인지 판정하는 *단일 receipt*.

### 절차

1. NSSF MVP yaml + 6 토픽 파일 commit.
2. fresh Claude Code agent 세션 시작 (본 spec / 직전 대화 context 없음).
3. 초기 entry point — `handoff/nssf/_handoff.yaml` 하나만 제공. 이후 agent 가 contract 의 `default_read_order` 에 따라 topic 파일을 *읽는 것은 허용 (요구됨)*.
4. 요청 — "이 yaml 을 초기 entry point 로 NSSelectionGet API 구현 plan 을 세워라."
5. agent 산출 — plan 마크다운 1개.

### PASS 기준 (4개 — AND)

| # | 기준 | 판정 |
|---|---|---|
| 1 | agent 가 `default_read_order` 를 따라 토픽을 읽었는가 (plan 의 *Read* 절에 명시) | agent 산출에 read 목록 존재 + 순서가 contract 와 일치 |
| 2 | `status=draft` 또는 `status=blocked` 토픽을 *구현 대상에 포함하지 않았는가* (기존 토픽의 잘못된 implement) | plan 의 implement 절에 persistence·service-scenarios·behavior-state·failure-policy·configuration·test-matrix·work-plan·cross-nf (MVP 외 카테고리, status=draft) 의 implement 산출 없음. SelectionEngine 은 topic=handoff_ready 이므로 implement 대상에 포함 *가능* (위반 아님) |
| 3 | yaml 에 *없는* 토픽·산출을 *invent 하지 않았는가* (새 artifact 의 무근거 추가) | plan 의 `produces` 가 yaml 의 tasks[*].produces 와 일치하고, yaml 외 토픽 ID (예 `data-model/Foo`) 가 등장하지 않음 |
| 4 | plan 에 `read` / `produces` / `acceptance` 가 있고 yaml 의 task schema 와 isomorphic 한가 | plan 의 구조가 §3 task schema 와 1:1 매핑 |

### FAIL 시 행동

- 기준 1·4 FAIL → `agent_contract.default_read_order` 또는 `tasks` schema 가 너무 모호. spec 정련.
- 기준 2 FAIL → agent 가 draft/blocked 토픽을 *implement 하려* 했다. `must_not` 어절이 약함. 더 명확한 금지 표현 + status enum 행동 표 강화.
- 기준 3 FAIL → agent 가 yaml 외 토픽·산출을 *invent* 했다. `must_not` 의 "spec_refs / Implementation Notes 에 근거 없는 행동 invent" 항목 강화.
- 다음 사이클 진입 전 PASS 필수.

### MVP 종료 후 next

- PASS → NSSF 의 나머지 API (NSSelectionPost 등) 로 동일 패턴 확장. *별도 사이클*.
- PASS → 다른 카테고리 (Service Scenarios, Failure Policy 등) 토픽화 *별도 사이클*.
- PASS → AMF/SMF 등 다른 NF *별도 사이클*.

본 spec 은 *이 한 PASS 까지* 의 범위에서 종결.

---

## §7. Acceptance Criteria — 본 MVP spec 자체

- [ ] §1 agent_contract 의 4 block (default_read_order / must_not / may_decide / must_ask_or_block) 정의
- [ ] §1 agent_contract 위치 결정 (A/B/C) 와 이유 명시
- [ ] §1 status enum × agent 행동 매핑 표 명시
- [ ] §1 `status_precedence: topic_over_category` 명시 (카테고리·토픽 status 우선 순위)
- [ ] §2 AUTO / USER marker 어휘 + frontmatter `generated_sections` manifest 정의
- [ ] §2 MVP 범위의 marker 적용 위치 표 명시
- [ ] §2 Data Model JSON 은 marker 대상 아님 (완전 AUTO machine artifact) 명시
- [ ] §3 task schema 6 필드 (phase·goal·read·produces·blocked_by·acceptance) 정의
- [ ] §3 사람 vs 도구 자리 분리 표 명시
- [ ] §4 basic 13 룰 (hard gate, marker 안전장치 + data-model JSON 5 룰 포함) + strict (report-only) 분리 명시
- [ ] §4.5 path resolution 규칙 (directory / single-file / topic#anchor) 명시
- [ ] §4 basic #1 의 schema 검증 방식 (경량 Python validator, jsonschema 의존성 없음) 명시
- [ ] §5 NSSF MVP 토픽 6 개 + machine_file 2 개 (data-model JSON) 목록 + handoff-v2 yaml 예시 명시
- [ ] §5 Data Model JSON 산출 형식 + 도구 자리 (`resolve-yaml-refs.py --emit-json`) 명시
- [ ] §5 JSON schema 해석 정책 — handoff topic 은 `{ "topic": ... }` 참조 + `dependencies` 등록, transitive non-handoff schema 는 `_inlined_from` 으로 inline (`dependencies` 등록 X) 명시
- [ ] §5 module-decomposition/SelectionEngine 은 카테고리=draft 인데 토픽=handoff_ready (`scope: "NSSelectionGet MVP only"`) — MVP 한정 의도 명시
- [ ] §5 MVP 외 카테고리는 `status=draft` (deferred_reason 포함) 로 처리. `not_applicable` 은 MVP 에서 사용하지 않음 (구현 정책 결정은 별도 decision record)
- [ ] §5 category slug 가 상위 spec 과 일관 (특히 `persistence` — `persistence-design` 아님)
- [ ] §5 `spec_index` 가 spec ref → topic 역방향 lookup, source file 매핑은 별도 `sources:` 블록
- [ ] §6 Agent Proof 절차 (초기 entry point 만 제공, topic 파일 read 허용) + 4 PASS 기준 + FAIL 시 행동 정의
- [ ] §6 MVP 종료 후 next (NSSF 나머지 API / 다른 카테고리 / 다른 NF) 가 *별도 사이클* 임을 명시

---

## §8. 위험 — MVP 범위 안

| # | 위험 | 임계 / 재평가 시점 |
|---|---|---|
| 1 | NSSelection 1 API 가 너무 작아 agent contract 의 한계가 드러나지 않음 | Agent Proof PASS 후에도 다음 사이클 진입 전 NSSelectionPost 같은 *복잡한* API 로 한 번 더 검증 |
| 2 | basic 13 룰이 너무 느슨해 strict 룰 일부가 hard gate 가 됐어야 함이 사후 드러남 | Agent Proof FAIL 의 원인이 strict 룰 영역이었는지 분석 — 해당 룰을 basic 으로 격상 |
| 3 | AUTO/USER marker 의 frontmatter manifest 가 사람이 수동 동기화하기 부담 | MVP 종료 시점에 도구 자동 sync 가 필요한지 (`generated_sections` 자동 산출) 판정 |
| 4 | agent_contract 가 yaml top-level 에 박혀 NF 추가 시 contract 중복 | NSSF 의 contract 와 다른 NF 의 contract 가 *실제로* 다른지 확인 — 같다면 별도 `agent-contract.yaml` (옵션 B) 재검토 |
| 5 | Agent Proof 가 본 KB 외 환경 (다른 IDE·다른 모델) 으로 generalize 안 됨 | MVP 종료 후 Claude Code 외 1 환경 (예 codex CLI) 에서 동일 yaml 로 proof 재실행 |
| 6 | MVP 외 카테고리를 `draft` 로 두면 `handoff_ready` gate 통과 못 함 → `/nf-status` 가 거짓 FAIL | gate 정의에서 `draft` 카테고리는 *해당 카테고리만 FAIL* 로 보고. NF 전체 gate 와는 별도. nf-status.py 갱신 |
| 7 | data-model JSON 과 markdown 두 산출이 schema 내용 차원에서 분기 (yaml chain 재해석 차이로) | basic #11 은 `topic_id`/`status` 정합만 검증 — schema 내용 분기는 *검출하지 않음*. divergence 방지는 `resolve-yaml-refs.py` 의 *한 번 resolve* 결과에서 markdown + JSON *동시 emit* 으로 보장. 후속 사이클에서 JSON `source.resolve_hash` 또는 `source.source_hash` (yaml chain 의 정규화 해시) 필드로 검증 가능성 격상 |
| 8 | `resolve-yaml-refs.py` 확장 (`--emit-json`) 이 markdown emit 동작을 깨뜨림 | MVP 작업 시 markdown emit 회귀 테스트 (NSSF 2 토픽 markdown 출력 비교) 를 step 진입 조건으로 |
| 9 | MVP 외 카테고리 (`module-decomposition` 등) 가 카테고리 status=draft 인데 *한 토픽만* handoff_ready (SelectionEngine `scope: "NSSelectionGet MVP only"`) — agent 가 카테고리 status 와 토픽 status 어느 쪽 따를지 혼동 | agent_contract `default_read_order` 에서 *토픽 status 가 우선* 임을 명시. 카테고리 status 는 *해당 카테고리에 새 토픽을 더 추가할지* 의 메타 정보. handoff yaml 의 토픽 `scope` 필드가 사람·agent 모두에게 MVP 한정 의도를 알린다 |

---

## §9. 참고 — 본 spec 안에 다시 적지 말 것

- 13 카테고리 정의·5 Tier 그룹화·하이브리드 분할: `2026-05-12-nf-build-restructure-design.md` §2~§3.
- handoff-v2 base schema (`schema_version`, `categories`, `topics`, `spec_index`, `version`): 동 §4.
- 도구 7종 (4 신규 + 3 확장) 의 입력·산출·책임: 동 §6.
- SKILL 3종 (`/nf-init`, `/nf-build`, `/nf-status`) 의 갱신 항목: 동 §7.
- THE FOUR RULES · NF profile · acceptance gate 의미: `CLAUDE.md`.
