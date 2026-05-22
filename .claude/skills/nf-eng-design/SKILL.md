---
name: nf-eng-design
description: Canonical lifecycle skill — reviewed NF architecture(+impl-plan)와 contract 를 입력으로 사람 소유 엔지니어링 결정을 `engineering/<nf>/engineering-design.md` 로 freeze 하는 Engineering Design Freeze 단계. 사용자가 "/nf-eng-design nssf", "NSSF engineering design", "엔지니어링 결정 freeze", "tech decision freeze", "eng design 만들어줘" 등을 말하거나 NF 이름 + engineering-design 생성을 지정하면 이 skill 을 사용한다. AI 가 F/G+contract 근거로 13 core slot + per-NF 연기 레지스터 결정을 *초안 자동생성* 하되, engineering decision 은 spec-derived 가 아니라 사람 ratify 전 frozen 아님. `eng_frozen` 은 technology decision freeze 단일 게이트이며 autonomous implementation 의 최종 GO 는 aggregate `readiness_pack_ready` (= contract + arch + impl + eng AND). 본 단계 검증은 `/nf-eng-status`, aggregate 검증은 `design/scripts/nf-readiness-status.py`.
argument-hint: "<nf>"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-eng-status.py *) Bash(cat *) Bash(ls *) Bash(find design/*) Bash(find dev/*) Bash(find engineering/*) Bash(mkdir -p *) Bash(grep *) Bash(sed *)
---

# nf-eng-design

Engineering Design Freeze 단계의 산출 생성 skill 이다 (ADR-0002). `/nf-impl-plan`(G)과 **자율 코드 생성** 사이를 메운다. 검증(측정)은 자매 skill `/nf-eng-status`.

## Inputs

다음만 읽는다.

1. `design/<nf>/architecture/` (F) — 특히 `decisions/ADR-0001-architecture-baseline.md` `## Open choices` (per-NF 연기 레지스터, **F-hard 입력**).
2. `design/schemas/engineering-core-slots.yaml` — 13 core slot + v2 dependency-closure typed shape profile (pipeline policy, read-only).
3. `handoff/<nf>/contract.yaml` + `design/<nf>/contract/**` — data model / API operation / lifecycle (DB schema·backend 후보 도출 근거).
4. `dev/<nf>/` (G) — soft 참조만. 결정의 task 전파 점검용이며 freeze 의 hard 입력 아님.

기존 `engineering/<nf>/engineering-design.md` 갱신 시엔 사람이 ratify 한 행(`ratified_by`/`date`)을 보존한다.

## Preconditions

1. `<nf>` 파싱.
2. `design/<nf>/architecture/` 와 그 `decisions/ADR-0001-architecture-baseline.md` `## Open choices` 가 존재해야 한다 (F-hard). 없으면 `/nf-arch-design`/`/nf-arch-status` 를 먼저 권고하고 정지.

## Output

`engineering/<nf>/engineering-design.md` (사람 소유, git 추적). `engineering/<nf>/_engineering_status.yaml` 는 `/nf-eng-status` 가 생성하는 gitignored 보고서 — 본 skill 이 만들지 않는다.

### Canonical 섹션 (정확히 이 집합·순서)

`## Purpose` · `## Decisions` · `## Out of scope` · `## Open Questions` · `## References`

> **Single source.** 위 집합은 `design/scripts/nf-eng-status.py` 의 `ENG_CANON` 상수와 *1:1* 이다. 한쪽만 바꾸면 drift — 반드시 함께 갱신한다 (ADR-0002 Consequences: 세 번째 SKILL↔validator drift 쌍).

### `## Decisions` 포맷 (기계 계약)

`## Decisions` 아래 *첫 ` ```yaml ` fenced block* 이 `inventory_id: {row}` 매핑이다. validator 는 이 블록만 파싱한다.

- **inventory_id** = (a) 13 core slot 이름 (profile `slots` 키, v2 — `sbi_http_framework`→`sbi_server_stack` rename + `sbi_client_stack` 신설) ∪ (b) per-NF 연기 레지스터 각 choice 의 slug. slug 규칙 = choice 텍스트를 lowercase 후 비-`\w`(unicode) 를 `_` 로 치환, 양끝 `_` strip (한글 보존). 정확한 slug 는 `/nf-eng-status` 실행 시 `eng_inventory_covered` 의 미포함 목록으로 확인 가능.
- **각 row 공통 필드** (profile `common_row_fields.required`): `decision, status, rationale, consequence, source, ratified_by, date`.
- `status ∈ {decided, explicitly_out_of_scope}`.
- **`ratified_by`·`date` 는 모든 frozen row 필수** (decided ∪ explicitly_out_of_scope). 없으면 `eng_frozen` FAIL — AI 초안이 `status:decided` 만 붙여 통과하는 loophole 차단.
- `explicitly_out_of_scope` 는 추가로 `rationale`/`consequence`/`source` 가 실질 내용이어야 한다.
- **core slot row 는 profile v2 typed shape 필드도 보유** (single-source — `design/scripts/nf-eng-status.py` 와 동일 계약). `required_shape` 의 각 필드 spec 은 `{type: scalar}` / `{type: non_empty_list}` / `{shape_ref: dependency_shape}`. `{shape_ref: dependency_shape}` 필드(software dependency — 예 `sbi_server_stack.framework`, `telemetry.logging/metrics/tracing`, `persistence.rdbms.driver_or_orm` 등)는 *nested object* `{dependency_source, version_policy, package?}` 로 채운다 — `dependency_source ∈ {stdlib,third_party,generated,externalized,none}`, `version_policy ∈ {pinned,range,repo_lockfile,runtime_managed,externalized,none}`, `package` 는 `dependency_source ∉ {stdlib,externalized,none}` 일 때 필수. conditional slot(`sbi_client_stack`/`tls_security`/`oauth2_token_validation`/`persistence`)은 discriminant 값으로 variant 선택 후 그 `required_shape` (예 `persistence.backend=rdbms` → `tables,…,driver_or_orm` 필수; `tls_security` `enabled` 이고 `tls_provider=app_library` → `tls_dependency` 추가 필수). `sbi_client_stack.required`(outbound NF 호출 필요여부)의 *진실 source 는 F architecture* — `required=true` 면 `source_arch_ref`(반드시 `design/<nf>/architecture/` 하위 실파일·anchor)·`targets`(non-empty)·`client` dependency 등을, `false` 면 `source_arch_ref`·`rationale` 를 채운다. `explicitly_out_of_scope` 인 slot 은 typed shape 면제.
- 어떤 필드에도 미결정 표현(`TBD`/decide/choose/select/determine) 이나 `owner: dev` 가 남으면 FAIL.

## 핵심 원칙 (이유 포함)

- **AI 초안 ≠ frozen.** AI 는 F/G+contract 를 근거로 13 slot + register 행의 결정을 *초안 자동생성* 할 수 있다 (특히 contract data model/API/lifecycle 로부터 persistence backend 후보와 DB schema 초안 `tables/columns/primary_key/indexes/constraints/migration_policy` 생성). 그러나 engineering decision 은 spec-derived 가 아니라 *사람 소유* — `ratified_by`/`date` 가 채워지기 전엔 frozen 이 아니며 `eng_frozen` 은 FAIL 이다. 사용자에게 초안 제시 후 ratify 를 명시적으로 받는다.
- **decision support 의무 (비개발자 전제) — slot 성격별 이분.** 사용자가 언어·library·version·DB/HTTP/OAuth stack 을 *모를 수 있다*. ratify 전 AI 가 무엇을 제시할지는 slot 이 *선택 가능한가* vs *외부/system requirement 로 고정인가* 에 따라 갈린다.
  - **(1) 선택 가능 decision** — **considered 후보 ≥2**, **selection criteria**, **recommendation(+이유)**, **rejected alternatives(+기각 사유)**, **consequence(구현·운영 영향)** 를 제시해 사용자가 정보에 근거해 선택하도록 돕는다. "알아서 골라줘" 도 추천을 ratify 하는 형태로 처리하되 후보·트레이드오프를 *먼저 보여준다*.
  - **(2) 외부/system requirement 로 고정** (예 시스템 요구로 언어=C 고정) — alternatives 비교는 *생략* (비교 근거가 의미 없음). 대신 **requirement source**(요구 문서·ADR·req ID — 예 `CONTEXT.md`/ADR 의 project constraint), **consequence**, **downstream required decisions**(그 고정이 강제하는 후속 결정 — 예 memory ownership·error handling·build/linking·dependency·sanitizer 정책) 를 기록한다. 비교가 아니라 *requirement trace* 다.
  - 공통 — 안내 산물은 해당 row 의 `rationale`/`consequence` + (선택) `## References` 에 압축 기록. 단순 단일값 통보 금지 (사람 ratify 의 의미를 비움). 고정 제약은 `language/runtime — constraint vs choice` 원칙대로 `CONTEXT.md`/ADR 에 *먼저(또는 동시에)* project constraint 로 기록하고 본 skill 은 그것을 consume 한다.
- **language/runtime — constraint vs choice.** 언어/런타임이 *프로젝트 고정 제약* 이면 `CONTEXT.md`/ADR 에 project constraint 로 *먼저* 기록하고 본 skill 은 그것을 *consume* (재선택 안 함). 선택 여지가 있으면 위 decision support 규칙대로 `language`/`runtime` slot 을 considered/criteria/recommendation/rejected/consequence 로 안내·ratify. F architecture 는 language-neutral 유지 — 선택·ratify 위치는 본 Engineering Design 단계다.
- **DB schema 확정 위치는 본 단계.** F architecture 는 persistent state *필요성만* 식별하고, G impl-plan 은 persistence 결정을 *연기만* 한다. DB schema 를 자율 코드 생성 단계에서 임의 생성하게 두면 이미 GO 신호가 난 뒤라 너무 늦다 — 본 단계에서 초안→ratify 로 확정한다.
- **제외도 결정이다.** 구현 안 하는 항목은 빈칸·`보류` 가 아니라 `explicitly_out_of_scope` + 실질 rationale/consequence/source + 사람 ratify 로 명시한다.
- **다음 단계 자동 호출 안 함.** 산출 정합 후 `/nf-eng-status <nf>` 를 *권고만* 한다.

## Execution

1. Preconditions 검증 (F-hard 존재).
2. profile (`design/schemas/engineering-core-slots.yaml`) 의 13 slot + v2 typed shape, 그리고 per-NF 연기 레지스터 choice 를 읽어 inventory id 집합 확정.
3. F/G+contract 근거로 각 inventory id 의 결정을 *초안* 작성 — core slot 은 profile typed shape 충족, register 행은 공통 필드. persistence/DB schema 는 contract data model 로부터 초안.
4. `engineering/<nf>/engineering-design.md` 를 canonical 5 섹션 + `## Decisions` ```yaml 블록으로 생성/갱신. 기존 ratify 행 보존.
5. **사용자에게 초안 제시 → 결정·`explicitly_out_of_scope` 를 ratify (`ratified_by`/`date`) 받기.** ratify 전 행은 frozen 아님을 명시.
6. `## Open Questions` 는 PASS 시 비어야 한다 — 잔존 질문은 미결정이므로 모두 Decisions 로 해소.
7. (권고) `/nf-eng-status <nf>` 실행해 `eng_frozen` 확인. 본 skill 은 자동 호출하지 않는다.

## Report

- 사용한 입력 (F architecture·profile·contract·연기 레지스터).
- 생성/갱신한 `engineering/<nf>/engineering-design.md`.
- AI 초안 행 vs 사람 ratify 필요 행 구분.
- 미결정·미ratify 로 `eng_frozen` 을 막는 항목.
- 다음 권고 — `/nf-eng-status <nf>`.

## 참고 — 본 skill 안에 다시 적지 말 것
- 검사 규칙·gate·typed shape 알고리즘: `design/scripts/nf-eng-status.py`.
- core slot·typed shape·common_row_fields: `design/schemas/engineering-core-slots.yaml`.
- 단계 의미·트레이드오프: `docs/adr/ADR-0002-engineering-design-freeze.md`. profile v2 dependency closure 정책: `docs/adr/ADR-0003-engineering-dependency-closure.md`.
- 측정: 자매 skill `/nf-eng-status`.
