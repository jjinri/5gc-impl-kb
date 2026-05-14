# NF 개발 lifecycle·프로젝트 구조·skill rename 계획 — 2026-05-13

Status: closed (2026-05-14). 계획 Phase 0~5 + Handoff filename cleanup + Legacy handoff removal/reset contract-only 까지 완료. 이후 후속 변경은 신규 plan 또는 retro 로 분리한다. ADR-0001 `Follow-ups` 의 `_handoff_seed.yaml` / `_status.yaml` rename 은 별도 작업에서 추적한다.

## 목적

현재 NF workflow 는 `nf-init` (`--reset` 포함) → `nf-build` → `nf-status` 로 구성되어 있지만, 이름과 산출물의 개발 단계가 맞지 않는다. 특히 `nf-build` 는 실제 코드 build 가 아니라 3GPP spec 에서 NF contract/design deliverable 을 생성하는 단계다. 이 불일치는 다음 단계인 상세 아키텍처 설계, 구현 계획, 실제 구현과 섞여 agent 판단을 흐린다.

본 계획의 목표는 전체 개발 lifecycle 을 명확히 나누고, 각 단계의 산출물 디렉터리·파일명·skill 이름을 그 역할에 맞게 재정렬하는 것이다.

## 명시적 제외

- `.omx/` 전체. OMX runtime/state/log/team 디렉터리이므로 삭제·이동·ignore 정책 변경 대상에서 제외한다.
- `specs/` 원본 파일의 내용 변경. 원본 spec 은 primary source 이므로 구조 참조만 다룬다.
- 실제 NF 구현 코드 작성. 본 계획은 구조/명명/문서/skill 정렬 계획이며 구현 착수 계획이 아니다.
- 대규모 산출물 재생성. 경로 이동 후 필요한 최소 검증만 수행한다.

## 현재 관찰

### 현재 workflow

| 현재 skill | 실제 역할 | 문제 |
|---|---|---|
| `/nf-init` | NF primary spec 과 의존 spec 을 찾아 `_manifest.yaml` 생성·보강하고, `--reset` 시 기존 산출을 archive 후 manifest 를 재생성 | `init` 이 너무 넓다. 실제로는 spec discovery/manifest 단계이며 reset 은 destructive 옵션으로 분리 인지가 필요하다. |
| `/nf-build` | `_handoff_seed.yaml` 기준으로 spec-derived markdown/json 과 `handoff/<nf>/contract.yaml` 생성 | `build` 라는 이름이 코드 build/compile 과 충돌한다. 실제 단계는 contract extraction/generation 이다. |
| `/nf-status` | contract/design 산출물의 gate 검사 | 무엇의 status 인지 불명확하다. 상세 아키텍처나 구현 status 와 구분되지 않는다. |

### 현재 산출물 구조

현재 NSSF 산출물은 다음처럼 섞여 있다.

```text
design/nssf/
├── _manifest.yaml
├── _handoff_seed.yaml
├── _status.yaml
├── interface.md
├── error-handling.md
├── api/NSSelectionGet.md
├── data-model/*.md
├── data-model/*.json
└── module-decomposition/SelectionEngine.md

handoff/nssf/contract.yaml

dev/README.md
```

문제점.

1. `design/nssf/` 안에 spec-derived contract, 사람이 정의한 seed, 상태 보고서, 초기 모듈 분해가 함께 있다.
2. `module-decomposition` 은 상세 아키텍처의 시작점인데 `nf-build` 산출과 같은 단계처럼 보인다.
3. `handoff/<nf>/contract.yaml` 은 dev contract 이지만 어떤 maturity 의 contract 인지 파일명만으로 알기 어렵다.
4. `dev/` 는 placeholder 이고 상세 아키텍처 설계 산출물 위치가 없다.
5. `README.md` 의 흐름도는 `nf-build` 산출을 “7 카테고리 페이지”로 설명하지만 실제 handoff-v2 는 topic layout 이다.

## 단계 정의

전체 개발 lifecycle 을 다음처럼 분리한다.

| 단계 | 목적 | 입력 | 산출 | 책임 영역 |
|---|---|---|---|---|
| 0. Source ingest | 3GPP 원본 spec 보관 | 외부 docx/pdf/yaml | `specs/<spec>/...` | 사람 |
| 1. Spec discovery | NF primary spec 과 의존 spec 식별 | `specs/` | manifest | design |
| 2. Contract extraction | spec 에서 NF 외부 계약과 요구사항 추출 | manifest, seed, spec text/yaml | markdown/json contract, handoff contract | design |
| 3. Contract validation | contract 가 dev 입력으로 충분한지 검사 | contract 산출물 | status/gate report | design |
| 4. Architecture design | contract 를 구현 가능한 상세 아키텍처로 변환 | handoff contract | module/runtime/state/test architecture | dev-design bridge |
| 5. Implementation planning | 아키텍처를 작업 단위로 분해 | architecture docs | task graph, file plan, acceptance tests | dev |
| 6. Implementation | 코드 작성·빌드 | implementation plan | source code, build artifacts | dev |
| 7. Verification | test/simulation/integration | code + contract | test result, compliance report | dev + design feedback |

핵심 결정.

- 현재 `nf-build` 산출물은 **2. Contract extraction** 단계다.
- `handoff_ready` 는 “상세 아키텍처 없이 바로 코딩 가능”이 아니라 “상세 아키텍처 설계를 시작할 수 있는 contract 입력이 충분함”으로 재정의해야 한다.
- 상세 아키텍처 설계는 별도 4단계로 분리한다.

## 사용자 trigger skill 과 agent 내부 수행 구분

원칙.

- **사용자 trigger skill** 은 사용자가 명시적으로 호출하는 workflow 표면이다. 사용자는 lifecycle 단계만 선택한다.
- **agent 내부 수행** 은 해당 workflow 가 목표 달성을 위해 실행하는 repo tool, sub-skill 성격의 절차, 검증, 파일 갱신이다. 사용자가 매번 직접 호출하지 않는다.
- user-facing skill 이 다른 user-facing skill 을 자동으로 연쇄 호출하지 않는다. 다음 단계는 결과 보고에서 추천한다.
- 예외는 같은 단계 안의 필수 검증이다. 예를 들어 contract build 는 산출 직후 `validate-extraction.py` 를 내부 실행할 수 있지만, `/nf-status` 를 자동 호출하지 않는다.
- destructive reset 은 별도 skill 이 아니라 `/nf-init --reset` 의 explicit option 이다. archive 는 agent 내부 step 으로 처리하고, reset 후 `/nf-build` 는 추천만 한다.

| Lifecycle 단계 | 사용자가 trigger 하는 skill | AI agent 가 내부 수행해야 하는 작업 | 자동 호출하지 않는 다음 skill |
|---|---|---|---|
| 1. Spec discovery | 현재 `/nf-init <nf> --primary <spec>`; 미래 `/nf-spec-discover <nf> --primary <spec>` | `specs/<spec>/` 확인, `nf-manifest.py --write` 실행, `_manifest.yaml` 보고, missing spec/to_pass 출력 | `/nf-build` 또는 `/nf-contract-build` |
| 1R. Reset + rediscovery | 현재 `/nf-init <nf> --primary <spec> --reset`; 미래 `/nf-spec-discover <nf> --primary <spec> --reset` | 기존 contract 산출 archive, manifest/seed reset 정책 적용, `nf-manifest.py --write` 재실행, archive 위치와 재시작 상태 보고 | `/nf-build` 또는 `/nf-contract-build` |
| 2. Contract extraction | 현재 `/nf-build <nf>`; 미래 `/nf-contract-build <nf>` | seed/topic 결정, spec text/yaml 추출, contract markdown/json 갱신, `build-handoff.py`, `validate-extraction.py --level basic` 실행 | `/nf-status` 또는 `/nf-contract-check` |
| 3. Contract validation | 현재 `/nf-status <nf>`; 미래 `/nf-contract-check <nf>` | `nf-status.py` 실행, gate/status report 작성, FAIL 의 `to_pass` 요약 | `/nf-arch-design` |
| 4. Architecture design | 신규 `/nf-arch-design <nf>` | `handoff/<nf>/contract.yaml` 과 contract docs 를 읽고 architecture template/ADR/status 작성, spec-derived fact 와 implementation choice 분리 | `/nf-impl-plan` |
| 5. Implementation planning | 신규 `/nf-impl-plan <nf>` | architecture 를 task graph/test matrix/traceability 로 변환, dev 작업 단위 작성 | 실제 구현 workflow |
| 6. Implementation | 별도 dev workflow. 본 repo 에서는 아직 미정 | 구현 repo 또는 `dev/<nf>/` 의 코드 생성/수정/테스트 | release/deploy |
| 7. Verification | 별도 dev verification workflow. 본 repo 에서는 아직 미정 | contract traceability, scenario/error matrix 기반 검증, 결과 report | 운영 반영 |

이 구분을 README 와 skill 문서에 반영한다. README 는 “사용자가 호출할 skill” 만 보여주고, 각 SKILL.md 는 “agent 내부 수행 절차” 를 상세히 둔다.

## 목표 skill 이름

기존 명령은 당장 삭제하지 않고 compatibility alias 로 남긴다. 새 이름을 canonical 로 문서화한 뒤, 한두 사이클 후 alias 제거 여부를 결정한다.

| 현재 | 새 canonical 후보 | 이유 | alias 정책 |
|---|---|---|---|
| `/nf-init` | `/nf-spec-discover` | primary/ref spec discovery 와 manifest 생성이 핵심. reset 은 별도 skill 이 아니라 `--reset` 옵션 | 기존 `/nf-init` 유지. `--reset` 은 destructive option 으로 명시 |
| `/nf-build` | `/nf-contract-build` 또는 `/nf-contract-generate` | 코드 build 가 아니라 NF contract 생성 | 기존 `/nf-build` 유지, README 흐름도에서는 새 이름 우선 |
| `/nf-status` | `/nf-contract-check` | contract 산출물 gate 검사임을 명시 | 기존 `/nf-status` 유지 |
| 신규 | `/nf-arch-design` | handoff contract → 상세 아키텍처 설계 | 신규 skill |
| 신규 | `/nf-impl-plan` | architecture → 구현 작업 계획 | 신규 skill |

권장 명명.

```text
/nf-spec-discover <nf> --primary <spec>
/nf-contract-build <nf> [--category <name>] [--topic <id>]
/nf-contract-check <nf>
/nf-arch-design <nf>
/nf-impl-plan <nf>
/nf-spec-discover <nf> --primary <spec> --reset   # destructive reset 통합 옵션
```

`build` 라는 단어는 코드 빌드와 혼동될 수 있으므로 장기적으로는 `/nf-contract-generate` 도 후보지만, 현재 도구 이름 `build-handoff.py` 와의 연결성을 고려하면 `/nf-contract-build` 가 이행 비용이 낮다.

## 목표 디렉터리 구조

### Option A — 최소 이동, 의미 보강

현재 구조를 최대한 유지하고 하위 폴더만 정리한다.

```text
design/
└── <nf>/
    ├── _manifest.yaml
    ├── _contract_seed.yaml        # 현재 _handoff_seed.yaml rename
    ├── _contract_status.yaml      # 현재 _status.yaml rename
    ├── contract/                  # 현재 spec-derived 산출물 이동
    │   ├── interface.md
    │   ├── error-handling.md
    │   ├── api/NSSelectionGet.md
    │   └── data-model/*.md|*.json
    └── architecture/              # 신규. 상세 아키텍처 설계
        ├── overview.md
        ├── module-boundaries.md
        ├── runtime-model.md
        ├── state-persistence.md
        ├── error-propagation.md
        ├── observability.md
        └── test-strategy.md

handoff/
└── <nf>/
    ├── contract.yaml
    └── architecture.yaml          # 선택. architecture summary for dev agents

dev/
└── <nf>/
    ├── implementation-plan.md
    └── tasks.yaml
```

장점.

- `design/<nf>` 를 유지하므로 현재 README/CLAUDE/도구 수정 폭이 작다.
- contract 와 architecture 를 하위 폴더로 분리해 단계 혼동을 줄인다.
- Obsidian 탐색도 자연스럽다.

단점.

- `design/<nf>` 아래에 contract 와 architecture 가 함께 있어 design/dev 경계가 여전히 넓다.

### Option B — 단계별 top-level 분리

```text
contracts/
└── <nf>/
    ├── _manifest.yaml
    ├── _seed.yaml
    ├── _status.yaml
    ├── interface.md
    ├── api/
    ├── data-model/
    └── error-handling.md

architecture/
└── <nf>/
    ├── overview.md
    ├── module-boundaries.md
    ├── runtime-model.md
    └── test-strategy.md

handoff/
└── <nf>/contract.yaml

dev/
└── <nf>/implementation-plan.md
```

장점.

- lifecycle 단계가 top-level 에 드러난다.
- `design/` 이라는 넓은 이름의 모호성이 사라진다.

단점.

- 현재 `design/scripts/*`, README, CLAUDE, tests, skill 전반의 경로 변경이 크다.
- 기존 “5gc-design 시스템”이라는 설명과 충돌한다.

### 권장안

단기에는 **Option A** 를 선택한다.

이유.

1. 현재 repo 는 이미 `design/scripts/*` 와 `design/nssf/*` 를 중심으로 테스트와 문서가 형성되어 있다.
2. 목표는 전체 재작성보다 명명과 단계 경계의 명확화다.
3. PR 리스크를 줄이려면 top-level rename 보다 하위 폴더 정리가 안전하다.
4. 나중에 여러 NF 가 쌓인 뒤 `contracts/` top-level 분리 필요성이 더 명확해지면 별도 ADR 로 다룬다.

## 파일명 정책

### Contract 단계

| 현재 | 목표 | 비고 |
|---|---|---|
| `design/<nf>/_manifest.yaml` | 유지 | discovery 산출. 이름이 충분히 명확하다. |
| `design/<nf>/_handoff_seed.yaml` | `design/<nf>/_contract_seed.yaml` | seed 는 handoff 가 아니라 contract 생성 입력이다. |
| `design/<nf>/_status.yaml` | `design/<nf>/_contract_status.yaml` | architecture/implementation status 와 구분한다. |
| legacy `handoff/<nf>/_handoff.yaml` | `handoff/<nf>/contract.yaml` | 파일명이 역할을 직접 표현한다. |
| `design/<nf>/interface.md` | `design/<nf>/contract/interface.md` | contract 산출물임을 명확히 한다. |
| `design/<nf>/api/*` | `design/<nf>/contract/api/*` | 동일. |
| `design/<nf>/data-model/*` | `design/<nf>/contract/data-model/*` | 동일. |
| `design/<nf>/error-handling.md` | `design/<nf>/contract/error-handling.md` | 동일. |
| `design/<nf>/module-decomposition/*` | `design/<nf>/architecture/module-boundaries.md` 또는 topic별 파일 | 상세 아키텍처 단계로 이동. |

### Architecture 단계 신규 파일

```text
design/<nf>/architecture/
├── overview.md
├── module-boundaries.md
├── request-flow.md
├── runtime-model.md
├── state-persistence.md
├── configuration-strategy.md
├── error-propagation.md
├── observability.md
├── test-strategy.md
└── decisions/
    └── ADR-0001-<slug>.md
```

각 파일의 원칙.

- spec 에 직접 적힌 사실은 `contract/` 에 남긴다.
- 구현 선택지가 필요한 판단은 `architecture/decisions/ADR-*` 로 기록한다.
- OS·언어·DBMS·framework 등 dev 선택은 `dev/<nf>/implementation-plan.md` 또는 별도 dev repo 로 넘긴다.
- 단, “선택 기준”과 “contract 가 요구하는 제약”은 architecture 에 기록할 수 있다.

### Implementation planning 단계 신규 파일

```text
dev/<nf>/
├── implementation-plan.md
├── tasks.yaml
├── test-matrix.md
└── traceability.md
```

`traceability.md` 는 task 가 어떤 contract/architecture 항목을 만족하는지 연결한다.

## handoff contract 재정의

기존 `handoff/<nf>/_handoff.yaml` 은 dev contract 로 충분히 중요하지만, 이름이 내부 파일처럼 보였다. 다음처럼 명확히 한다.

```text
handoff/<nf>/contract.yaml
handoff/<nf>/architecture.yaml      # 선택. architecture summary
handoff/<nf>/implementation-plan.yaml # 선택. dev task graph machine-readable form
```

`contract.yaml` 의 의미.

- 3GPP spec-derived truth 만 담는다.
- dev 가 상세 아키텍처를 시작할 수 있는 입력이다.
- runtime/library/deployment 결정은 담지 않는다.

`architecture.yaml` 의 의미.

- `nf-arch-design` 산출의 요약본이다.
- module, request flow, state, timeout/retry, test seam 같은 구현 구조를 담는다.
- 기술 스택이 결정된 경우 그 결정 근거는 ADR 로 링크한다.

## 단계별 실행 계획

### Phase 0 — 기준선 고정

목표: 구조 변경 전 현재 동작과 산출물을 고정한다.

작업.

1. `main` 동기화.
2. tracked diff 없음 확인.
3. 현재 NSSF contract/status 검증.

검증.

```bash
git status --short --branch
.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic
.venv/bin/python3 design/scripts/nf-status.py nssf --no-write
pytest tests/scripts
```

Acceptance criteria.

- 구조 변경 전 실패 목록을 알고 있다.
- 구조 변경 후 비교할 baseline 이 있다.

### Phase 1 — 용어 ADR 작성

목표: `build`, `design`, `handoff_ready` 같은 용어를 재정의한다.

작업.

1. `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md` 작성.
2. lifecycle 단계와 canonical skill 이름을 확정한다.
3. `handoff_ready` 의미를 “architecture start-ready contract” 로 재정의할지 검토한다.

Acceptance criteria.

- README/CLAUDE/skill 에서 참조할 단일 용어 정의가 생긴다.
- `nf-build` 라는 표현이 왜 deprecated 되는지 문서화된다.

### Phase 2 — skill rename compatibility layer

목표: 새 이름을 도입하되 기존 명령을 즉시 깨지 않는다.

작업.

1. `.claude/skills/nf-contract-build/SKILL.md` 신규 작성.
   - 기존 `nf-build` 내용을 옮기되 “contract extraction/generation” 단계로 재서술한다.
2. `.claude/skills/nf-build/SKILL.md` 는 alias/deprecation wrapper 로 축소한다.
3. 같은 방식으로 다음 alias 를 준비한다.
   - `nf-init` → `nf-spec-discover` (`--reset` 옵션 포함)
   - `nf-status` → `nf-contract-check`
   - 별도 `nf-reset` skill 은 만들지 않는다. reset 은 discovery/reset 재시작 의도이므로 `nf-spec-discover --reset` 로 통합한다.
4. Skill description trigger 에서 “build nf page” 같은 모호한 표현을 줄이고, contract/generate/check/reset 표현을 우선한다.

Acceptance criteria.

- 기존 `/nf-build nssf` 호출이 계속 동작한다.
- 새 문서에서는 `/nf-contract-build nssf` 를 canonical 로 안내한다.
- skill 이름만 봐도 산출물 단계가 드러난다.

### Phase 3 — contract 산출물 경로 이동

목표: spec-derived contract 와 architecture 산출물을 분리한다.

작업.

1. `design/<nf>/contract/` 생성.
2. 현재 contract 파일을 `git mv` 로 이동한다.
   - `interface.md` → `contract/interface.md`
   - `error-handling.md` → `contract/error-handling.md`
   - `api/` → `contract/api/`
   - `data-model/` → `contract/data-model/`
3. `_handoff_seed.yaml` rename 검토.
   - 단기 호환이 중요하면 rename 은 Phase 4 로 분리한다.
   - rename 시 `_contract_seed.yaml` 로 이동하고 scripts/tests 를 갱신한다.
4. `_status.yaml` rename 검토.
   - rename 시 `_contract_status.yaml` 로 이동하고 scripts/tests 를 갱신한다.
5. legacy `handoff/<nf>/_handoff.yaml` → `handoff/<nf>/contract.yaml` rename 은 가장 마지막에 수행한다.

수정 대상.

- `design/scripts/build-handoff.py`
- `design/scripts/validate-extraction.py`
- `design/scripts/nf-status.py`
- `design/scripts/resolve-yaml-refs.py`
- `tests/scripts/*`
- `.claude/skills/*/SKILL.md`
- README/CLAUDE

Acceptance criteria.

- `validate-extraction.py nssf --level basic` 가 새 경로에서 통과한다.
- tests/scripts 가 통과한다.
- `grep -R "design/nssf/api\|design/nssf/data-model\|_handoff.yaml"` 결과가 의도된 호환 코드나 과거 docs 로만 남는다.

### Phase 4 — architecture 단계 도입

목표: contract 와 구현 사이의 누락된 상세 아키텍처 설계 단계를 만든다.

작업.

1. `.claude/skills/nf-arch-design/SKILL.md` 작성.
2. `design/<nf>/architecture/` template 작성.
3. 입력은 `handoff/<nf>/contract.yaml` 과 `design/<nf>/contract/*` 로 제한한다.
4. 산출 파일 template.
   - `overview.md`
   - `module-boundaries.md`
   - `request-flow.md`
   - `runtime-model.md`
   - `state-persistence.md`
   - `error-propagation.md`
   - `observability.md`
   - `test-strategy.md`
   - `decisions/ADR-0001-*.md`
5. architecture status/check 는 별도 파일로 둔다.
   - 후보: `design/<nf>/architecture/_architecture_status.yaml`

Acceptance criteria.

- `/nf-arch-design nssf` 가 contract 를 읽고 architecture template 을 생성할 수 있다.
- architecture 문서에는 spec-derived fact 와 implementation choice 가 구분되어 있다.
- architecture 산출 없이 `/nf-impl-plan` 으로 넘어가지 않도록 문서화된다.

### Phase 5 — implementation planning 단계 도입

목표: 상세 아키텍처를 실제 작업 단위로 변환한다.

작업.

1. `.claude/skills/nf-impl-plan/SKILL.md` 작성.
2. `dev/<nf>/implementation-plan.md`, `dev/<nf>/tasks.yaml`, `dev/<nf>/test-matrix.md` template 작성.
3. traceability 규칙 정의.
   - task 는 contract section 또는 architecture decision 을 참조해야 한다.
   - 테스트 항목은 error matrix/service scenario/data model 중 하나 이상과 연결되어야 한다.

Acceptance criteria.

- 구현 작업이 contract/architecture 요구사항과 추적 가능하다.
- `dev/` placeholder 가 실제 계획 산출물 위치로 바뀐다.
- code generation/build 와 문서 generation 이 명확히 분리된다.

### Phase 6 — README/CLAUDE 축소·정렬

목표: 정책 문서와 소개 문서가 새 lifecycle 을 간결하게 설명한다.

작업.

1. README 흐름도를 새 단계로 갱신한다.
2. CLAUDE.md 는 정책만 남기고 단계 세부는 skill/docs 로 위임한다.
3. 기존 “작업 사이클” 표현을 user-triggered lifecycle 표로 교체한다.
4. `build-handoff.py` 가 handoff-v1 로 적힌 stale 설명을 제거한다.
5. `.claude/skills/nf-build` 등 alias 문서에는 deprecated warning 을 넣는다.

Acceptance criteria.

- 신규 독자는 README 만 보고 단계와 산출물 위치를 이해한다.
- agent 는 CLAUDE.md 만 보고 source-of-truth/금지/경계 정책을 이해한다.
- README/CLAUDE/skill 간 명칭 충돌이 없다.

### Phase 7 — `--reset` 정책 정리

목표: destructive reset 을 독립 skill 이 아니라 discovery 재시작 옵션으로 고정하되, 사람이 legacy handoff yaml 을 수동 작성하지 않도록 seed auto-gen 책임은 유지한다.

작업.

1. 별도 `nf-reset` skill 은 폐기 상태로 유지하고 새 canonical skill 을 만들지 않는다.
2. 현재 명령은 `/nf-init <nf> --primary <spec> --reset`, 미래 canonical 명령은 `/nf-spec-discover <nf> --primary <spec> --reset` 로 문서화한다.
3. topic layout 기준 archive 대상을 contract 산출물 allowlist 로 수정한다.
   - `design/<nf>/contract/`
   - `design/<nf>/_status.yaml`
   - `handoff/<nf>/contract.yaml`
4. 기본 reset 은 contract 산출물 archive, manifest refresh, ready-state seed auto-gen 에 한정한다.
   - `design/<nf>/_manifest.yaml` 과 `design/<nf>/_handoff_seed.yaml` 은 보존·갱신한다.
   - architecture 산출물은 기본 보존.
   - implementation plan 은 항상 보존.
   - architecture 까지 버리는 것은 별도 explicit option 이 필요하다. 후보: `--include-architecture`.
5. reset 실행은 user-triggered skill 내부의 agent step 이며, 다른 user-facing skill 을 자동 호출하지 않는다. reset 후 다음 단계 추천만 출력한다.

Acceptance criteria.

- `grep -R "nf-reset" README.md CLAUDE.md .claude/skills` 결과가 비어 있거나 과거 폐기 기록 문맥뿐이다.
- reset 이 contract 산출물만 archive 하고 architecture/implementation plan 을 삭제하지 않는다.
- reset 이 seed auto-gen 목적을 보존하고 legacy `_handoff.yaml` 수동 작성/fallback 을 되살리지 않는다.
- destructive flag 와 archive 대상이 skill 문서에 명확히 출력된다.

### Phase 8 — 검증과 이행 종료

검증 명령.

```bash
git diff --check
pytest tests/scripts
.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic
.venv/bin/python3 design/scripts/nf-status.py nssf --no-write

grep -R "nf-build\|nf-init\|nf-status\|nf-reset\|nf-spec-discover\|nf-contract" -n README.md CLAUDE.md .claude/skills docs/plans | sed -n '1,240p'
grep -R "_handoff.yaml\|_handoff_seed.yaml\|design/nssf/api\|design/nssf/data-model" -n README.md CLAUDE.md .claude/skills design/scripts tests | sed -n '1,200p'
```

Acceptance criteria.

- 새 canonical skill 이름이 문서의 주 표현이다.
- 기존 skill 이름은 alias/deprecated 문맥에서만 등장한다.
- tests/scripts 통과.
- NSSF contract validation 통과.
- `.omx/` 는 변경되지 않는다.

## PR 분할 제안

구조 변경은 한 PR 에 몰지 않는다.

1. PR-A: ADR + README/CLAUDE lifecycle 용어 정리.
2. PR-B: skill rename compatibility layer 추가. 기존 동작 유지.
3. PR-C: contract 디렉터리 이동과 scripts/tests 경로 갱신.
4. PR-D: `nf-arch-design` 신규 skill + architecture templates.
5. PR-E: `nf-impl-plan` 신규 skill + dev plan templates.
6. PR-F: `nf-init --reset` / `nf-spec-discover --reset` 정책 수정.

각 PR 은 독립적으로 revert 가능해야 한다.

## 리스크와 완화

| 리스크 | 영향 | 완화 |
|---|---|---|
| 경로 이동으로 scripts/tests 회귀 | 기존 NSSF 산출 검증 실패 | Phase 0 baseline, PR-C 단독 처리, tests/scripts 필수 |
| skill alias 중복으로 agent routing 혼란 | 잘못된 skill 선택 | canonical/deprecated 문구 명확화, alias 문서는 짧게 유지 |
| architecture 단계가 dev 책임을 과도하게 침범 | design/dev 경계 붕괴 | architecture 문서에 “contract-implied constraints” 와 “implementation choices” 섹션 분리 |
| reset 이 architecture/plan 을 삭제 | 작업 손실 | reset 기본 대상은 contract 로 제한, architecture 삭제는 explicit option 필요 |
| 문서가 다시 비대해짐 | CLAUDE.md 과대화 재발 | 상세는 ADR/skill/template 로 위임, CLAUDE.md 는 정책만 유지 |

## 보류 결정

다음은 본 계획 문서 작성 시점에 확정하지 않는다.

- Option B top-level `contracts/`, `architecture/` 분리 여부.
- `_handoff_seed.yaml` 와 `_status.yaml` rename 의 정확한 시점.
- `handoff/<nf>/architecture.yaml` 의 필수 여부.
- 구현 언어/framework 선택. 이는 dev 단계 결정이다.

## 성공 기준

최종적으로 다음 질문에 파일명만 보고 답할 수 있어야 한다.

1. 이 파일은 spec 원본인가? → `specs/`.
2. 이 파일은 spec 에서 추출한 NF contract 인가? → `design/<nf>/contract/`.
3. 이 파일은 contract 를 dev agent 에 넘기는 machine-readable 계약인가? → `handoff/<nf>/contract.yaml`.
4. 이 파일은 상세 아키텍처 설계인가? → `design/<nf>/architecture/`.
5. 이 파일은 구현 작업 계획인가? → `dev/<nf>/`.
6. 이 skill 은 어느 lifecycle 단계인가? → 이름에 `spec`, `contract`, `arch`, `impl` 중 하나가 드러난다.

## Progress checkpoint — 2026-05-13 Phase 0/1

Status: ready_for_next_phase
Current objective: Phase 0 baseline and Phase 1 lifecycle vocabulary ADR + README/CLAUDE alignment.
Completed:
- Baseline validation before structure changes.
- Added `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`.
- Updated `README.md` to show user-triggered lifecycle skills, future canonical names, functions, and outputs.
- Updated `CLAUDE.md` with concise lifecycle vocabulary and source-of-truth policy.
Changed files:
- `README.md`
- `CLAUDE.md`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
Validation:
- `.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic` → basic 13/13 PASS.
- `.venv/bin/python3 design/scripts/nf-status.py nssf --no-write` → draft/review_ready/handoff_ready PASS, canonical FAIL only by `implementation_guidance_quality` NOT_RUN baseline.
- `pytest tests/scripts` → 37 passed.
Open risks / gaps:
- Canonical skill wrappers are not implemented yet.
- Contract artifact path move is not started.
- Architecture and implementation-plan skills are not created yet.
Next step:
- Phase 2: add canonical skill compatibility wrappers for `/nf-contract-build`, `/nf-contract-check`, and future `/nf-spec-discover` while keeping existing `/nf-init`, `/nf-build`, `/nf-status` behavior intact.

## Progress checkpoint — 2026-05-13 Phase 2

Status: ready_for_next_phase
Current objective: Phase 2 canonical skill compatibility wrappers.
Completed:
- Added `.claude/skills/nf-spec-discover/SKILL.md` as the canonical wrapper for `/nf-init` including `--reset`.
- Added `.claude/skills/nf-contract-build/SKILL.md` as the canonical wrapper for `/nf-build`.
- Added `.claude/skills/nf-contract-check/SKILL.md` as the canonical wrapper for `/nf-status`.
- Added compatibility alias notes to existing `/nf-init`, `/nf-build`, and `/nf-status` skill docs.
- Updated README/CLAUDE/ADR wording from future-only canonical names to available wrapper aliases for the first three lifecycle stages.
Changed files:
- `.claude/skills/nf-spec-discover/SKILL.md`
- `.claude/skills/nf-contract-build/SKILL.md`
- `.claude/skills/nf-contract-check/SKILL.md`
- `.claude/skills/nf-init/SKILL.md`
- `.claude/skills/nf-build/SKILL.md`
- `.claude/skills/nf-status/SKILL.md`
- `README.md`
- `CLAUDE.md`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
Validation:
- Skill frontmatter parse with `yaml.safe_load` → all 6 `.claude/skills/*/SKILL.md` files have `name` and `description`.
- `git diff --check` → pass.
Open risks / gaps:
- Wrappers delegate to existing skill docs, so behavior stays unchanged but depends on future agents following the wrapper instruction.
- Contract artifact path migration is still not started.
Next step:
- Commit/PR Phase 2. Next implementation phase: contract artifact path migration under `design/<nf>/contract/`.

## Progress checkpoint — 2026-05-13 Phase 3

Status: ready_for_review
Current objective: Move spec-derived NSSF contract artifacts under `design/nssf/contract/` while preserving current seed/status/handoff filenames for compatibility.
Completed:
- Moved NSSF `interface.md`, `error-handling.md`, `api/`, and `data-model/` artifacts into `design/nssf/contract/` using `git mv`.
- Updated `design/nssf/_handoff_seed.yaml` paths and regenerated `handoff/nssf/contract.yaml`.
- Updated `path_resolution.py` so contract categories prefer `design/<nf>/contract/` while legacy paths remain readable for compatibility.
- Updated `build-handoff.py` agent read-order guidance to point at contract paths.
- Updated tests to exercise contract-root topic resolution and contract-root fixture paths.
- Updated README/CLAUDE/ADR/skill docs to describe contract-root outputs.
Compatibility deliberately preserved:
- `design/<nf>/_handoff_seed.yaml` not renamed yet.
- `design/<nf>/_status.yaml` not renamed yet.
- legacy `handoff/<nf>/_handoff.yaml` was not renamed yet at this checkpoint.
- `design/<nf>/module-decomposition/` left in place until the architecture-design phase decides its final structure.
Validation:
- `.venv/bin/python3 design/scripts/build-handoff.py nssf` → wrote `handoff/nssf/contract.yaml`, categories=13 topics=6 tasks=1.
- `.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic` → basic 13/13 PASS.
- `.venv/bin/python3 design/scripts/nf-status.py nssf --no-write` → handoff_ready PASS; canonical blocked only by `implementation_guidance_quality` NOT_RUN baseline.
- `pytest tests/scripts` → 40 passed.
Open risks / gaps:
- Handoff filename rename remains pending and should be a separate compatibility-aware phase.
- Seed/status filename rename remains pending.
- `module-decomposition` still needs architecture-phase migration or reinterpretation.
Next step:
- Review/merge Phase 3, then introduce architecture-design skill/templates.

## Progress checkpoint — 2026-05-14 Phase 4

Status: ready_for_review
Current objective: Phase 4 architecture-design skill and initial NSSF architecture templates.
Completed:
- Added `.claude/skills/nf-arch-design/SKILL.md` as the canonical architecture design lifecycle skill.
- Added reusable architecture templates under `.claude/skills/nf-arch-design/templates/architecture/`.
- Created initial NSSF architecture draft documents under `design/nssf/architecture/` from the template set.
- Populated NSSF architecture docs with contract-derived constraints for NSSelectionGet, data model boundaries, error propagation, runtime/state/config/observability/test seams.
- Kept implementation choices explicit as `TBD` and left OS/language/DB/framework/deployment decisions to later dev stages.
- Updated README, CLAUDE, and ADR wording so `/nf-arch-design` is now available rather than planned.
Compatibility deliberately preserved:
- At this checkpoint, legacy `handoff/nssf/_handoff.yaml` still remained the source contract path pending the handoff filename rename phase.
- `design/nssf/_handoff_seed.yaml` and `design/nssf/_status.yaml` remain unchanged.
- `design/nssf/module-decomposition/` remains in place and was not used as the architecture input source.
Changed files:
- `.claude/skills/nf-arch-design/SKILL.md`
- `.claude/skills/nf-arch-design/templates/architecture/*.md`
- `.claude/skills/nf-arch-design/templates/architecture/decisions/ADR-0001-architecture-baseline.md`
- `design/nssf/architecture/*.md`
- `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md`
- `README.md`
- `CLAUDE.md`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
Validation:
- `python3` skill/frontmatter + required architecture file check → pass.
- `git diff --check` → pass.
- `.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic` → basic 13/13 PASS.
- `.venv/bin/python3 design/scripts/nf-status.py nssf --no-write` → handoff_ready PASS; canonical remains blocked only by baseline `implementation_guidance_quality` NOT_RUN.
- `pytest tests/scripts` → 40 passed.
- stale planned-wording grep for `/nf-arch-design` → 0 matches.
Open risks / gaps:
- `/nf-impl-plan` remains planned and not implemented.
- At this checkpoint, handoff filename rename remained pending and was planned as a separate phase.
- Legacy `module-decomposition` migration remains pending.
Next step:
- Commit and open a Phase 4 PR, then implement `/nf-impl-plan` in the next phase.

## Progress checkpoint — 2026-05-14 Phase 5

Status: ready_for_review
Current objective: Phase 5 implementation-planning skill and initial NSSF dev planning artifacts.
Completed:
- Added `.claude/skills/nf-impl-plan/SKILL.md` as the canonical implementation planning lifecycle skill.
- Added reusable dev planning templates under `.claude/skills/nf-impl-plan/templates/dev/`.
- Created initial NSSF implementation planning artifacts under `dev/nssf/`.
- Populated task graph, test matrix, and traceability links from NSSF contract and architecture sources.
- Kept technology choices explicit as `TBD` and did not create source code, dependency files, or build system files.
- Updated README, CLAUDE, ADR, and `dev/README.md` so `/nf-impl-plan` is available rather than planned.
Compatibility deliberately preserved:
- At this checkpoint, legacy `handoff/nssf/_handoff.yaml` still remained the source contract path pending the handoff filename rename phase.
- Contract and architecture paths remain unchanged.
- `dev/nssf/` contains planning artifacts only, not implementation source code.
Changed files:
- `.claude/skills/nf-impl-plan/SKILL.md`
- `.claude/skills/nf-impl-plan/templates/dev/*.md`
- `.claude/skills/nf-impl-plan/templates/dev/tasks.yaml`
- `dev/README.md`
- `dev/nssf/implementation-plan.md`
- `dev/nssf/tasks.yaml`
- `dev/nssf/test-matrix.md`
- `dev/nssf/traceability.md`
- `README.md`
- `CLAUDE.md`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
Validation:
- `python3` skill/frontmatter + required dev planning file + `tasks.yaml` traceability check → 9 traceable tasks OK.
- `git diff --check` → pass.
- `.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic` → basic 13/13 PASS.
- `.venv/bin/python3 design/scripts/nf-status.py nssf --no-write` → handoff_ready PASS; canonical remains blocked only by baseline `implementation_guidance_quality` NOT_RUN.
- `pytest tests/scripts` → 40 passed.
- stale planned-wording grep for `/nf-impl-plan` → 0 matches.
Open risks / gaps:
- At this checkpoint, handoff filename rename remained pending and was planned as a separate phase.
- Seed/status filename rename remains pending.
- Reset archive policy update remains pending Phase 7.
Next step:
- Commit and open a Phase 5 PR, then continue with reset/archive policy cleanup in the next phase.

## Progress checkpoint — 2026-05-14 Handoff filename cleanup

Status: ready_for_review
Current objective: Rename the canonical machine-readable handoff artifact from legacy `_handoff.yaml` to `contract.yaml` with read fallback for compatibility.
Completed:
- Renamed tracked NSSF handoff artifact `handoff/nssf/_handoff.yaml` to `handoff/nssf/contract.yaml`.
- Updated `design/scripts/build-handoff.py` to emit `handoff/<nf>/contract.yaml` and update `agent_contract.default_read_order`.
- Updated `design/scripts/validate-extraction.py` and `design/scripts/nf-status.py` to prefer `contract.yaml` and read legacy `_handoff.yaml` only as fallback.
- Updated `nf-status.py` handoff yaml validity logic to recognize `handoff-v2` directly instead of treating v2 as a v1 false-fail.
- Updated README, CLAUDE, lifecycle ADR, active skills, architecture docs, dev planning docs, and handover memory to use `contract.yaml` as the canonical handoff path.
- Added a regression test proving legacy `_handoff.yaml` still validates when `contract.yaml` is absent.
Decision:
- `_handoff_seed.yaml` and `_status.yaml` are not renamed in this phase. They are still script/skill interface filenames and should move in a separate compatibility PR if needed.
Changed files:
- `handoff/nssf/contract.yaml`
- `design/scripts/build-handoff.py`
- `design/scripts/validate-extraction.py`
- `design/scripts/nf-status.py`
- `tests/scripts/test_build_handoff_v2.py`
- `tests/scripts/test_validate_extraction.py`
- `.claude/skills/*/SKILL.md` touched by handoff path wording
- `README.md`
- `CLAUDE.md`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
- `docs/handover.md`
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
- `design/nssf/architecture/*.md`
- `dev/nssf/*`
Validation:
- `.venv/bin/python3 design/scripts/build-handoff.py nssf` → wrote `handoff/nssf/contract.yaml`, categories=13 topics=6 tasks=1.
- `.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic` → basic 13/13 PASS.
- `.venv/bin/python3 design/scripts/nf-status.py nssf --no-write` → handoff_ready PASS; canonical remains blocked only by baseline `implementation_guidance_quality` NOT_RUN; handoff yaml checks PASS for handoff-v2.
- `pytest tests/scripts` → 41 passed.
- `git diff --check` → pass.
- Current-surface `_handoff.yaml` grep → only explicit legacy fallback/code/test/history mentions remain.
Open risks / gaps:
- Historical docs under `docs/superpowers/`, `docs/retros/`, and older `docs/plan.md` still mention `_handoff.yaml` as historical context.
- Reset/archive policy still needs Phase 7 cleanup.
- `_handoff_seed.yaml` and `_status.yaml` rename remains pending.
Next step:
- Run full validation, commit, push, and open a PR for this filename cleanup.

## Progress checkpoint — 2026-05-14 Legacy handoff removal + reset contract-only policy

Status: ready_for_next_phase
Current objective: Remove runtime fallback to legacy `_handoff.yaml` while preserving `/nf-init` seed auto-generation and narrowing `--reset` to contract artifacts only.
Completed:
- Removed `handoff/<nf>/_handoff.yaml` fallback from `validate-extraction.py` and `nf-status.py`; current checks require `handoff/<nf>/contract.yaml` with `handoff-v2` schema.
- Removed the regression test that accepted legacy `_handoff.yaml` when `contract.yaml` was absent.
- Restored `/nf-init` / `/nf-spec-discover` seed auto-generation as a first-class responsibility via `design/scripts/nf-seed-gen.py`.
- Documented `--reset` as contract-only archive: `design/<nf>/contract/`, `design/<nf>/_status.yaml`, and `handoff/<nf>/contract.yaml`; manifest, seed, architecture, module decomposition, and dev planning are preserved.
- Updated README, CLAUDE, lifecycle ADR, and active skill docs to state that legacy `_handoff.yaml` is retired and is not a workflow input.
Changed files:
- `.claude/skills/nf-init/SKILL.md`
- `.claude/skills/nf-spec-discover/SKILL.md`
- `.claude/skills/nf-arch-design/SKILL.md`
- `.claude/skills/nf-impl-plan/SKILL.md`
- `README.md`
- `CLAUDE.md`
- `design/scripts/nf-seed-gen.py`
- `design/scripts/validate-extraction.py`
- `design/scripts/nf-status.py`
- `tests/scripts/test_validate_extraction.py`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
Validation:
- `.venv/bin/python3 design/scripts/nf-seed-gen.py nssf` → preserved existing scoped seed; categories=13 topics=6 tasks=1.
- `git diff --check` → pass.
- `.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic` → basic 13/13 PASS.
- `.venv/bin/python3 design/scripts/nf-status.py nssf --no-write` → handoff_ready PASS; canonical still blocked only by existing `implementation_guidance_quality` NOT_RUN.
- `pytest tests/scripts` → 40 passed.
Open risks / gaps:
- `nf-seed-gen.py --force` is a conservative generic generator and has not yet been covered by dedicated unit tests.
- `_handoff_seed.yaml` and `_status.yaml` filenames remain compatibility names; rename remains a separate future phase.
- Historical docs outside current workflow surfaces may still mention `_handoff.yaml` as history.
Next step:
- Open a PR later when usage budget allows; this checkpoint intentionally leaves the branch locally committed without creating a GitHub PR.
