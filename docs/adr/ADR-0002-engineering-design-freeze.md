# ADR-0002 — Engineering Design Freeze 단계와 `eng_frozen` 게이트

Date: 2026-05-19
Status: Proposed

> Proposed → Accepted 전 사람 ratify 필수 4건: (1) `engineering/<nf>/` tracking policy, (2) `engineering-core-slots.yaml` 초기 slot 목록 + version 규약, (3) `engineering-design.md` canonical 섹션 집합, (4) `explicitly_out_of_scope` ratification 필드 최종 schema. 본 4건은 Open choices 표에 대응한다.

## Context

`CONTEXT.md` 가 코드 교차검증으로 확립한 사실 — **개발 설계**(spec 으로 도출 불가능한 사람 소유 엔지니어링 결정: 언어·런타임·SBI/HTTP framework·TLS·persistence·telemetry·배포·모듈 코드형태)를 *현재 어느 파이프라인 단계도 생산하지 않는다*. `/nf-impl-plan`(stage G)은 **구현 작업 계획** 으로 결정을 *연기* 하며(`tasks.yaml` acceptance 가 "X 를 결정하라"), `owner: dev` 는 거의 모든 task 에 붙어 변별력이 없다. 그 결과 **자율 코드 생성** 진입을 통제하는 GO 신호가 없어, 코드 생성 agent 가 언어·DB·TLS 등을 임의로 발명할 수 있다.

연기된 엔지니어링 결정의 인벤토리 원천은 이미 실재한다 — `design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices` 의 **연기 레지스터** 7행 (4 `TBD` + 3 `보류`). 그러나 이 표가 전부 *결정으로 해소* 됐는지 판정하는 단계·게이트가 없다.

## Decision

`/nf-impl-plan`(G)과 **자율 코드 생성** 사이에 **Engineering Design Freeze** 단계를 신설하고, 통과 게이트 `eng_frozen` 을 자율 코드 생성의 *유일한 GO 신호* 로 정의한다.

1. **단계·산출물 위치.** `engineering/<nf>/engineering-design.md`(사람 소유, git 추적) + `engineering/<nf>/_engineering_status.yaml`(validator 생성 보고서, gitignored — `_contract_status.yaml`·`_arch_status.yaml`·`_impl_status.yaml` 와 동일 정책). `dev/<nf>/`(구현 작업 계획)·`design/<nf>/architecture/`(spec-derived) 와 분리된 영역으로 3분리한다.

2. **게이트 의미론.** `eng_frozen` 은 *단일 게이트* 다. *blocking 판정은 결정론적 구조 검사로 한정* — slot 별 typed closed-form schema 충족(예 language slot = `{name, version}`, persistence = named backend). LLM/sub-agent judge 는 *비차단 advisory* 로만 두며 GO/STOP 을 결정하지 않는다. typed schema 가 의미 judge 를 완전 대체하는 것이 아니라, *blocking gate 로 충분할 만큼 의미를 구조화* 한다(잔여 의미 위험 — 빈약한 rationale·오참조 source·미전파 consequence·빌드 불가 stack — 은 advisory 가 표시만).

3. **인벤토리 = 합집합, 하드코딩 금지.** `eng_frozen` 인벤토리 = `design/schemas/engineering-core-slots.yaml`(versioned, 사람/repo-policy 소유, PR review 필수, validator 는 상수 없이 read-only 로 읽음) 의 core slot ∪ **per-NF** 연기 레지스터 행. 후자는 *전역* `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md` 가 아니라 *NF별* `design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices` 다(ADR-0001 동일 번호 두 문서 — 혼동 금지). core 개념 폐기(garbage-in 취약)도, validator 하드코딩(drift·1-NF 과적합)도 하지 않는다.

4. **단계 의존성 (g3).** `eng_frozen` 의 hard 입력 = F architecture + per-NF 연기 레지스터(`design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md`) + engineering-core-slots profile + 사람 결정(`engineering-design.md`). G(impl-plan/tasks/test-matrix/traceability) 는 *soft advisory* — 결정이 task/build 에 전파됐는지 비차단 교차검사. validator 출력은 namespace 분리: `gates.eng_frozen = PASS/FAIL`, `advisory.impl_plan_alignment = PASS/WARN`. G advisory 가 blocking 으로 오염되지 않게 강제한다.

5. **`보류`·`TBD`·`owner: dev` 처리.** `eng_frozen` PASS 는 모든 인벤토리 항목이 `decided` 또는 `explicitly_out_of_scope`. `TBD`·구현영향 없는 `보류`·결정 없는 `owner: dev`·agent 재량 잔존 = FAIL. `explicitly_out_of_scope` 는 *사람 ratify 필수* — agent 단독 선언 금지, 각 행에 `ratified_by`·`date`·`rationale`·`consequence`·`source/reference`. NSSF 의 33.501·38.413·AMF reallocation 보류는 `engineering-design.md` 작성 시 ratify 한다.

6. **검증 skill.** `/nf-eng-design`(산출 생성)·`/nf-eng-status`(측정) + `design/scripts/nf-eng-status.py`. 기존 contract/arch/impl status skill 의 책임분리 패턴(생성 ≠ 측정)을 따른다.

## Considered options

- **단계 신설 vs stage G 확장 vs ADR-only 규율.** G 확장은 "구현 작업 계획 ≠ 개발 설계" glossary 를 한 산출에 뭉개 충돌 재발. ADR-only 는 게이트 부재로 자율 코드 생성 진입 통제 불가. → 신설 채택.
- **게이트 blocking 을 의미 sub-judge 로 (b2) / presence·enum 만 (b3).** b3 은 "decision: 좋은 DB" 류 loophole 잔존. b2 는 flaky LLM judge 를 사람 없는 자율 코드 생성 최종 barrier 에 둬 위험. → blocking 은 결정론 typed schema(b1), 의미는 advisory.
- **단계 의존성 G-hard (g1) / G-완전무관 (g2).** g1 은 결정 연기자(G)를 freeze 필수 입력으로 오해 → glossary 충돌. g2 는 결정의 task/build 전파 검증 기회 상실. → g3 (F-hard + G-soft-advisory).
- **산출물을 `dev/<nf>/` 에.** `dev/<nf>/` 는 이미 구현 작업 계획 위치 → glossary 충돌. → `engineering/<nf>/` 신설.
- **core slot 을 validator 상수로 하드코딩 / deferral-register-only.** 전자는 drift·1-NF 과적합(`nf-arch-status.py` "discover 기반 하드코딩 금지" 원칙 위반), 후자는 작성자 누락 시 garbage-in. → versioned profile + register 합집합.
- **구조/의미 2게이트 분리.** 단일 `eng_frozen` 내부에 `gates.*`(blocking) / `advisory.*`(non-blocking) namespace 를 두면 충분하다. 별도 명명 게이트 2개는 lifecycle 복잡도만 늘리고 `handoff_ready`(단일 게이트·내부 tier) 선례와도 어긋난다. → 단일 게이트 + 내부 namespace.

## Consequences

- **세 번째 SKILL.md ↔ validator drift 쌍 신설.** contract·arch·impl 에 이어 eng-design 산출 ↔ `nf-eng-status.py` 가 drift 위험 쌍이 된다. canonical 섹션 mandate 를 한 source 로 유지해야 한다.
- **신규 top-level 영역 `engineering/<nf>/`.** 온보딩·lifecycle ADR-0001 표 갱신 필요(단계 1행 추가). `engineering/<nf>/` 는 사람 소유 — spec-derived 재생성 대상 아님.
- **tracking/gitignore 정책.** `engineering/<nf>/engineering-design.md` 는 git 추적(사람 소유 결정 산출), `engineering/<nf>/_engineering_status.yaml` 는 generated 라 `.gitignore`(기존 `_*_status.yaml` 정책과 동일). `.gitignore` 갱신 필요.
- **`design/schemas/engineering-core-slots.yaml` = pipeline policy.** profile 은 *모든 NF* 의 `eng_frozen` 의미를 바꾼다. 따라서 profile PR 은 단순 schema 수정이 아니라 *파이프라인 정책 변경* 으로 리뷰하고 본 ADR(또는 후속 ADR)에 기록한다.
- **사람 ratify 의존.** `explicitly_out_of_scope` 는 사람 결정 없이 PASS 불가 — 자율성과 안전의 의도된 트레이드오프(자율 코드 생성 GO 는 사람 ratify 게이트를 통과해야 함).

## Open choices

| 항목 | 상태 | 참조 |
|---|---|---|
| `engineering-design.md` canonical 섹션 집합 | TBD | skill-build 사이클. arch/impl canonical-section 패턴 차용 |
| `advisory.impl_plan_alignment` 산식 (G 전파 교차검사 기준) | TBD | nf-eng-status.py 설계 시 |
| `engineering-core-slots.yaml` 버전 규약 (`v1` 파일명 vs 내부 `version:` 필드) | TBD | profile 도입 PR |
| core slot 초기 목록 확정 (language·runtime·SBI/HTTP fw·schema/codegen·TLS·OAuth2 검증·persistence·telemetry·deploy·module layout·test/build 후보) | TBD | profile 도입 PR + 사람 ratify |

## References

- `CONTEXT.md` — **개발 설계**·**구현 작업 계획**·**연기 레지스터**·**개발 설계 Freeze 단계**·`eng_frozen`·**게이트**(구조 vs 의미) 용어
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md` — lifecycle 단계·canonical skill 명명
- `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices` — 연기 레지스터 7행 (인벤토리 source)
- grill-with-docs 세션 2026-05-19 (Pane 1 + Pane 2 다회 라운드 수렴)
