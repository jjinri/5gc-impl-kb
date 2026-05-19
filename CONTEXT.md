# 5gc-impl-kb — NF 개발 파이프라인 컨텍스트

3GPP spec 을 입력으로 받아 AI agent 가 자율적으로 완성된 NF(예 NSSF) 코드를 생성하도록, 단계별 산출물·게이트로 이어지는 지식 베이스. 본 문서는 *용어집* 이며 구현·결정 기록이 아니다.

## Language

**명세-파생 산출 (Spec-Derived Artifact)**:
3GPP spec 으로부터 결정론적으로 도출되어 agent 가 재생성 가능한 산출 — 엔지니어링 의견을 담지 않는다 (contract·architecture, 파이프라인 B~F).
_Avoid_: design output, 설계물 (모호함 — 엔지니어링 결정과 혼동)

**개발 설계 (Engineering Design)**:
spec 으로부터 도출 불가능한, 사람이 소유하는 엔지니어링 결정 (언어·런타임·SBI/HTTP framework·TLS·persistence·telemetry·배포 토폴로지 + 모듈 코드 형태) — 자율 코드 생성 전에 freeze 되어야 한다.
_Avoid_: implementation planning, architecture (둘 다 별개 개념), "dev harness 설계"

**구현 작업 계획 (Implementation Planning)**:
architecture 를 작업 단위로 재분할한, *결정을 연기하는* 작업 목록 — 각 task acceptance 가 "X 를 결정하라" 이며 결정 자체를 담지 않는다 (현 `/nf-impl-plan` = stage G 산출).
_Avoid_: 개발 설계 (G 는 개발 설계가 아니다), dev design

**자율 코드 생성 (Autonomous Code Generation)**:
AI agent 가 freeze 된 개발 설계를 입력으로 실제 NF 코드를 작성해 동작하는 NF 를 빌드하는, 파이프라인 *밖* 의 목표 단계.
_Avoid_: implementation (모호 — 계획과 코드 양쪽 의미)

**게이트 (Gate)**:
한 단계 산출이 다음 단계 입력으로 충분한지 판정하는 PASS/FAIL 조건. *구조 게이트* (모양·섹션·schema 검사) 와 *의미 게이트* (내용 충실성 검사) 는 다른 종류다.
_Avoid_: 두 종류를 "게이트" 로 뭉뚱그리기

**owner: dev**:
한 결정/작업의 해결 주체가 spec-파생 파이프라인이 아니라 dev(코드생성) 단계임을 뜻하는 주석 — schema 미강제, 게이트 무시, 현재 모든 task 에 붙어 변별력 없음.
_Avoid_: owner 를 기계 검사되는 필드로 간주

**연기 레지스터 (Deferral Register)**:
의도적으로 미룬 엔지니어링 결정을 `| 결정 | TBD | 해결자/참조 |` 형태로 모은 표 — 현재 `architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices` 에 7행 실재 (4 `TBD` + 3 `보류`). **개발 설계** 의 결정 인벤토리 원천.
_Avoid_: arch Open choices 와 동일시 (전자는 구조화 표, 후자는 산문)

**개발 설계 Freeze 단계 (Engineering Design Freeze)**:
**구현 작업 계획**(stage G) 과 **자율 코드 생성** 사이를 메우는 단계 — **개발 설계** 의 모든 결정이 `decided` 또는 `explicitly_out_of_scope` 로 잠겼음을 게이트 `eng_frozen` 으로 판정한다. 통과 후에만 자율 코드 생성 진입.
_Avoid_: **구현 작업 계획** 과 동일시 (G 는 결정 연기, 본 단계는 결정 freeze)

**eng_frozen**:
**개발 설계 Freeze 단계** 의 게이트이자 **자율 코드 생성** 의 유일한 GO 신호. *blocking 판정은 결정론적 구조 검사* (slot 별 typed closed-form schema), *의미 sub-judge 는 비차단 advisory* (GO/STOP 미결정). 인벤토리 = engineering-core-slots profile + **연기 레지스터** 행.
_Avoid_: LLM/sub-agent judge 를 blocking 판정자로 둠 (최종 barrier 는 결정론)

## Relationships

- **명세-파생 산출** (B~F) 은 **개발 설계** 의 입력이다 — 담지는 않는다.
- **개발 설계** 는 **자율 코드 생성** 의 입력이며 그 전에 freeze 되어야 한다.
- **구현 작업 계획** (stage G) 은 **개발 설계** 를 *연기* 한다 — 그 사이를 **개발 설계 Freeze 단계**(게이트 `eng_frozen`)가 메운다.
- `handoff_ready` 는 *의미 게이트*, `arch_consistent`·`impl_consistent` 는 *구조 게이트* — 동급 peer 가 아니다.

## Example dialogue

> **사람:** "F 또는 G 에서 실제 코드 작성을 위한 개발 설계를 하려고 했다."
> **Agent:** "빌드된 G(`/nf-impl-plan`)는 **구현 작업 계획** 이다 — task 가 'schema 전략을 *결정하라*' 이고 결정을 담지 않는다. **개발 설계** 를 담는 단계가 파이프라인에 없다."

## Flagged ambiguities

- "개발 설계" 가 (a) stage G impl-plan 과 (b) 엔지니어링 결정 산출 양쪽으로 쓰였다 — 해소: 빌드된 G 는 **구현 작업 계획**(결정 연기), **개발 설계**(결정 freeze)는 현재 어느 단계도 생산하지 않는 별개 개념.
- "design" 이 명세-파생 산출과 엔지니어링 결정 양쪽을 의미했다 — 해소: 전자는 **명세-파생 산출**, 후자는 **개발 설계**.
- "TBD" 가 (1) **연기 레지스터** 의 소유자-명시 정당한 연기 와 (2) `tasks.yaml` acceptance 의 "답 없이 task 통과" 탈출구 양쪽으로 쓰였다 — 해소: 별개 개념. (1)만 canonical TBD, (2)는 거짓-준비신호 누수로 취급.
