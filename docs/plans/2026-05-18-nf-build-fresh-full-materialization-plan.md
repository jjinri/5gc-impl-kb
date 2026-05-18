# nf-build fresh full materialization — 2026-05-18

Status: in_progress
Current objective: fresh 상태 (`design/<nf>/contract/` 부재) 에서 `_contract_seed.yaml` 의 *모든* topic 을 사람 수동 저작 없이 agent/script 주도로 전량 materialize 해, `handoff_ready` gate 까지 도달 가능하게 만든다. 사람 개입은 정책 결정·결과 리뷰·선택적 USER prose 보강으로 최소화.

사용자 결정 — (C) contract 생성은 "전량 신규 저작 + 사람 개입 최소". Pane 2 second-opinion 합의 (옵션 2 재구성: 수동 저작 아님, agent/script 주도 materialization, USER 섹션 TODO placeholder). 본 plan 은 `/nf-build` 의 fresh-full 역량을 명세+구현하는 사이클이며, NSSF 적용 (C)~(F) 는 그 위에서 수행.

## 컨텍스트 — 정확한 gap

검증으로 확인한 사실 (worktree `nssf-lifecycle-AtoF-20260518`, 재생성 seed 기준).

1. `build-handoff.py` 는 `_contract_seed.yaml` *만* 읽어 `handoff/<nf>/contract.yaml` 을 emit한다 (topic `.md` 안 읽음).
2. 그러나 `validate-extraction.py` basic 13 의 **#3 (topic file exists)**, **#9 (data-model machine_file 존재)**, **#4 (xref target 존재)** 는 *실제 topic 파일* 을 요구한다.
3. `nf-status.py` 의 `handoff_ready` gate 는 `validate_extraction_basic` 을 필수 포함한다 (GATE_DEFS). 즉 topic `.md` 부재 → #3/#9 FAIL → `validate_extraction_basic` FAIL → **`handoff_ready` 도달 불가**.
4. `nf-seed-gen.py` 가 만든 fresh seed 는 13 카테고리 전부 `status: draft`. `nf-build` SKILL step2 의 "활성 (`status ≠ draft, ≠ not_applicable`) 카테고리" 규칙상 fresh seed 는 *빌드 대상이 공집합* — 아무것도 materialize 되지 않는다.
5. `nf-build` SKILL.md 는 *fresh full 자동 생성* 을 명세하지 않는다 — step3 표는 카테고리별 AUTO/USER 를 *agent 가 따르는* 절차로 기술하고, data-model JSON 만 스크립트(`resolve-yaml-refs.py --emit-json`) 전담. module-decomposition 등은 "사람이 정한 분해 의도".

결론 — "fresh → seed 전량 topic materialization → handoff_ready 도달" 은 *현 명세·tooling 에 없는 동작*. 본 plan 이 그 역량을 신설한다.

## 설계 결정

| 항목 | 결정 |
|---|---|
| materialize 대상 | seed.topics 의 *모든* entry. 카테고리 draft status 와 무관하게 파일 생성 (validate #3/#4/#9 충족 목적). |
| topic.status 정책 (사용자 결정 + Pane 2 합의) | **spec-derived 자동 격상.** api·data-model·interface·error-handling 토픽은 materialize 후 *구조 검증 통과 시* `draft → handoff_ready` 자동 격상. 격상 조건 = (AUTO 생성 완료) ∧ (validate basic 의 JSON/marker/frontmatter/refs 룰 통과) ∧ (data-model `unresolved_refs` 등 blocking 없음). 非spec-derivable skeleton·USER-TODO-only 토픽은 `draft` 유지. *자동 격상은 spec-derived 구조 검증 통과 범위에 한정* — 이 한정을 plan·retro 에 명시. build-handoff `agent_contract.must_not` (draft 토픽 구현 금지) 와 정합 — draft 로 남는 토픽은 구현 task `read` 필수에서 제외. |
| AUTO 출처 | spec-derivable 카테고리는 스크립트 추출. data-model = `resolve-yaml-refs.py` (md chain-tree/field-table + JSON). api = primary OpenAPI `paths[op]` (method/path/params/requestBody/responses). interface = yaml `info`/`servers`/`security`. error-handling = yaml `responses`/`components`. |
| USER 섹션 | fresh 면 빈 TODO placeholder (marker + frontmatter 만 정확). 사람이 후속 보강. |
| 非spec-derivable 카테고리 | module-decomposition·service-scenarios·behavior-state·failure-policy·configuration·persistence·test-matrix·work-plan·cross-nf — AUTO 최소 skeleton + USER TODO. validate #3/#7/#8 통과하는 marker·frontmatter 구조만 보장, 내용은 명시 TODO. status draft. |
| 사람 최소 경계 | 사람 = spec 원본/범위, manual_overrides 정책, 결과 리뷰, 선택적 USER prose. 그 외 전부 agent/script. |
| 명세 우선 | nf-build SKILL.md 에 "fresh full materialization" 절을 *먼저* 명시 (Pane 2 권고 — 명세가 동작 보장 부족). 구현이 그 명세를 충족. |

## 명시적 제외

- 非spec-derivable 카테고리의 status 자동 격상 — draft 유지 (자동 격상은 spec-derived 4 카테고리 한정).
- USER prose 자동 작성 — fresh 는 TODO. 품질 산문은 사람/별도.
- 3GPP docx 산문 해석 품질 향상 — extract/spec-split 현 수준 사용. AUTO 추출이 불완전하면 USER TODO 가 그 자리.
- nf-seed-gen.py 의 카테고리 status 자동 변경 — seed 정책 별개. 본 plan 은 build 측 materialization 만.
- 비-NSSF NF — 사용자 결정상 NSSF 구현+배포 완료 전까지 다른 NF 논외.

## Phase 분해 (단일 PR 사이클)

### P0 — 본 plan (현 단계)

### P1 — nf-build SKILL.md 명세 보강
- "Fresh full materialization" 절 신설 — fresh/missing topic 시 seed.topics 전량 materialize 규칙, 카테고리별 AUTO 출처·USER TODO, status 정책 (spec-derived 자동 격상 한정), 사람 최소 경계.
- step2 "활성 카테고리" 규칙에 fresh-full 예외 명시 (draft 라도 materialize; spec-derived 는 검증 통과 시 자동 handoff_ready, 그 외 draft 유지).
- 아래 §"카테고리별 marker/frontmatter schema" 표를 SKILL.md 에 박아 구현자 임의 marker id drift 차단.

### 카테고리별 marker/frontmatter schema (구현 계약)

각 topic `.md` frontmatter 필수 — `topic_id`, `category`, `status` (= seed topic status, spec-derived 격상 후 반영), `generated_sections: [<AUTO id…>]`, `user_sections: [<USER id…>]`. 본문 marker `<!-- AUTO:<id>:start/end -->` / `<!-- USER:<id>:start/end -->` 가 frontmatter 와 *1:1*, id 중복 0 (validate #7/#8).

| 카테고리 | AUTO marker id | USER marker id | 격상 |
|---|---|---|---|
| interface (single-file) | `auth-block`, `transport-block` | `implementation-notes` | spec-derived → 자동 |
| api/<op> (directory) | `api-matrix`, `request-schema`, `response-schema` | `implementation-notes` | spec-derived → 자동 |
| data-model/<schema> (directory) | `chain-tree`, `field-table` (+ `<schema>.json` machine_file) | `implementation-notes` | spec-derived → 자동 |
| error-handling (single-file) | `error-matrix` | `recovery-prose`, `implementation-notes` | spec-derived → 자동 |
| module-decomposition/<mod> | `module-graph` | `responsibility-prose`, `implementation-notes` | draft 유지 |
| service-scenarios/<sc> | `scenario-index` | `scenario-prose` | draft 유지 |
| behavior-state/<st> | `state-index` | `state-prose` | draft 유지 |
| failure-policy (single-file) | `failure-index` | `policy-prose` | draft 유지 |
| configuration (single-file) | `config-index` | `policy-notes` | draft 유지 |
| persistence (single-file) | `persistence-index` | `policy-notes` | draft 유지 |
| test-matrix (single-file) | `test-index` | `coverage-notes` | draft 유지 |
| work-plan (single-file) | `work-index` | `plan-notes` | draft 유지 |
| cross-nf (single-file) | `crossnf-index` | `crossnf-notes` | draft 유지 |

draft-유지 카테고리의 AUTO `*-index` = seed 에서 도출 가능한 최소 인덱스 (예 topic 목록·spec_refs), USER = `TODO:` placeholder. spec 추출 불가 내용은 USER TODO 로만.

- **seed 존재 topic 만 materialize. seed 에 없는 non-spec topic/category 를 새로 invent 하지 않는다** (Pane 2 재검토 (ii)).
- `module-graph` 는 seed 만으로 *placeholder/minimal graph (no inferred decomposition)* — 이름이 실제 분해를 함의하지 않음을 SKILL.md 에 명시.
- frontmatter 필수키 (`topic_id`/`category`/`status`) 는 validate basic 이 강제하지 않는다 (validate #8 은 generated/user_sections ↔ marker sync 중심) — *materializer self-check* 로 보장 (Pane 2 재검토 (iii)-4).
- refs 에 `#anchor` 가 있으면 validate 가 실제 anchor/heading 존재를 본다 — skeleton 은 해당 anchor 를 생성하거나 생성 refs 에서 anchor 를 회피 (Pane 2 재검토 (iii)-5).

### 자동격상 알고리즘 (preflight 순서 — Pane 2 재검토 (i))

"validate basic 통과 후 격상" 은 #11 (JSON status ↔ handoff topic status) 순환이다. 격상 조건은 *최종 validate 이전의 validate-basic-equivalent **preflight*** 결과로 산출한다.

1. seed.topics 전량 materialize. 최초 status = seed 기준 `draft`.
2. preflight — 각 topic 의 AUTO marker / frontmatter sync / refs target / data-model JSON parse·dependencies·`unresolved_refs` 를 *검사만* (status 미변경).
3. promotion set 결정 — spec-derived (api·data-model·interface·error-handling) topic 중 preflight 통과한 것만. data-model 은 JSON `unresolved_refs` 가 *비어있을 때만* 격상 (하나라도 있으면 draft 유지 → 최종 #12 FAIL 회피).
4. promotion set 의 status 를 `handoff_ready` 로 갱신 — **seed/handoff source status 와 data-model JSON status 를 *동일* 값으로** 함께 갱신 (#11 정합).
5. category status — *모든 하위 topic 이 격상된 카테고리만* `handoff_ready`. 하나라도 draft 면 category `draft` 유지 (validate #5 정합).
6. `build-handoff.py` → 최종 `validate-extraction.py --level basic` (격상 후 실행).

status 쓰기 위치 — **`materialize-contract.py` 가 `_contract_seed.yaml` 의 topic/category status 를 갱신** (build-handoff.py 가 seed 를 읽으므로 promotion 이 seed 에 반영돼야 handoff yaml 에 전달됨). build-handoff.py 자체는 무변경 (단일책임 유지).

### P2 — script 보강
- 신설 `design/scripts/materialize-contract.py` (단일책임 — build-handoff.py 무변경, Pane 2 합의).
  - seed.topics *존재* entry 만 순회, missing topic 파일 생성 (invent 금지).
  - data-model/* → `resolve-yaml-refs.py --emit-json` + chain-tree/field-table md (기존 로직 재사용).
  - api/* → OpenAPI `paths[op]` 에서 api-matrix/request-schema/response-schema AUTO. USER `implementation-notes` = TODO.
  - interface/error-handling → yaml info·servers·security / responses·components AUTO. USER TODO.
  - 非spec-derivable → §marker schema 표의 marker·frontmatter 정합 skeleton + USER TODO.
  - §"자동격상 알고리즘" 6 단계 구현 — preflight → promotion set → seed/handoff+JSON status 동일 갱신 → category status 규칙 → 그 후 최종 validate.
  - frontmatter 필수키 self-check (validate 비강제분).
  - 모든 산출이 validate basic #3·#4·#5·#7·#8·#9·#10·#11·#12·#13 구조 충족.
- 멱등 — 기존 USER 영역 보존, AUTO 만 재생성 (nf-build marker 계약 준수).

### P3 — NSSF 적용 (C)~(F) + stop-condition

파이프라인 — `materialize-contract.py nssf` → `build-handoff.py nssf` → `validate-extraction.py nssf --level basic` → `nf-status.py nssf --no-write`.

stop-condition (Pane 2 권고 — fresh/synthetic/idempotency 강한 evidence).
- fresh copy — 위 4 단계 무인 실행, validate basic FAIL 0, spec-derived 4 카테고리 handoff_ready 자동 격상 확인, skeleton 카테고리 draft 확인.
- synthetic — (a) marker id mismatch → validate #8 FAIL 재현, (b) data-model JSON 누락 → #9 FAIL, (c) 기존 USER 섹션 보존 (rerun 후 사람 산문 유지), (d) extra rerun 멱등 (AUTO 만 갱신, diff 안정).
- 최종 handoff 의 `tasks.*.read` 가 `draft` topic 을 참조하지 않음 (validate #4 는 target 존재만 보고 draft 를 안 막으므로 stop-condition 으로 별도 확인 — Pane 2 재검토 (iii)-3).
- category status 검증 — `handoff_ready` category 는 산하 topic 전부 격상됐는지 (validate #5 정합).
- (D) `/nf-contract-check nssf` — `handoff_ready` gate 결과. 자동 격상으로 spec-derived PASS, skeleton draft 가 gate 에 미치는 영향 명시 보고.
- (E) `/nf-arch-design nssf` — 기존 tracked architecture 와 diff 검토 (preserve user sections).
- (F) `/nf-arch-status nssf` — `arch_consistent` gate.
- 각 단계 산출 사용자 리뷰.

### P4 — plan→PR→second-opinion→머지→retro
- 단일 PR `push/nf-build-fresh-full-materialization-20260518`.
- gitignored 산출 (contract/·handoff·status) 은 비추적 — PR 은 SKILL.md + 신설 script + 본 plan.
- 머지 전 Pane 2 `$git-pr-review`.

## 성공 기준 (2 분리 — Pane 2 권고)

**A. structural materialization complete.**
- fresh `design/nssf/` (contract/ 없음) 에서 사람 수동 저작 0 으로 25 topic 파일 + data-model JSON materialize.
- `validate-extraction.py nssf --level basic` FAIL 0.
- frontmatter ↔ marker 1:1, id 중복 0 (validate #7/#8).
- 멱등 — rerun 시 USER 보존, AUTO만 갱신, diff 안정.

**B. implementation-ready (정책 범위).**
- spec-derived 4 카테고리 (api·data-model·interface·error-handling) 가 검증 통과로 `handoff_ready` 자동 격상.
- 非spec-derivable skeleton 은 `draft` 유지, 구현 task `read` 필수 제외 (build-handoff `agent_contract.must_not` 정합).
- "자동 격상 = spec-derived 구조 검증 통과 범위 한정" 이 plan·retro 에 명시됨.

**C. 명세·재현.**
- nf-build SKILL.md 가 fresh-full 동작 + 카테고리별 marker schema 를 명세 (gap 0 — `grep "fresh full materialization"`).
- (D)~(F) gate 결과를 사람이 리뷰 가능한 형태로 산출.
- NSSF 로직/이름 하드코딩 0 (seed.topics·OpenAPI 기반 discover).

## 리스크와 완화

- **리스크.** AUTO 추출 (api/error/interface) 가 3GPP docx 산문까지 못 채워 contract 품질이 낮음.
  - 완화. 본 역량 목표는 *구조적 materialization + gate 도달*. 산문 품질은 USER TODO + 사람 후속. 명세에 "AUTO = 추출 가능 범위, 나머지 USER TODO" 명시.
- **리스크.** all-draft 면 build-handoff `agent_contract.must_not` 상 구현 입력 아님 — "전량 생성했는데 구현 불가" 괴리 (memory `feedback-doc-code-accuracy` 동류).
  - 완화. spec-derived 4 카테고리 자동 격상으로 구현 입력 도달. skeleton 은 draft 유지하되 구현 task `read` 필수 제외. 성공 기준 A(structural)/B(implementation-ready) 분리로 "전량 생성=구현 가능" 오해 차단. (D) 보고에 카테고리별 status 명시.
- **리스크.** materialize-contract.py 가 nf-build marker 계약 (#7/#8) 위반 산출.
  - 완화. P3 에서 validate basic 으로 즉시 검출. P2 구현이 frontmatter `generated_sections`/`user_sections` ↔ 본문 marker 정합을 보장.
- **리스크.** 대형 스코프 — script 신설 + 명세 + 적용을 단일 PR.
  - 완화. P1(명세)·P2(script)·P3(적용) 가 한 PR 이되 커밋 분리. P3 는 gitignored 라 PR diff 미포함 — 리뷰 부담은 SKILL.md+script 에 집중.
