# Artifact management — lifecycle 산출물과 작업 문서 구분

이 문서는 repo 를 처음 보는 agent 가 파일 위치만 보고 다음을 구분할 수 있게 하는 관리 기준이다.

1. **초기 입력** — 사람이 외부에서 확보해 넣는 원본.
2. **로컬 재생성 산출물** — lifecycle skill/script 가 다시 만들 수 있어 git 에서 제외되는 산출.
3. **검토·ratify 된 lifecycle 산출물** — skill 이 만들거나 갱신하지만 PR 로 보존하는 산출.
4. **작업 계획·회고 문서** — lifecycle 을 완성하기 위해 임시/보조로 작성한 프로젝트 관리 기록.
5. **검증 상태·runtime/cache** — 측정 결과 또는 로컬 실행 상태.

핵심 규칙: **생성 주체가 자동화여도, git 추적 여부와 의미가 다르다.** `dev/<nf>/tasks.yaml` 처럼 skill 이 생성한 파일도 review 된 lifecycle baseline 이면 git 추적 대상이다. 반대로 `_manifest.yaml` 처럼 중요한 파일이어도 도구가 재생성 가능한 중간 산출이면 git 비추적이다.

---

## 1. Artifact class 정의

| class | 정의 | git 정책 | 진실 출처 | 예시 |
|---|---|---|---|---|
| **Source input** | 사람이 확보한 외부 원본 spec/API 파일 | 추적 | 원본 파일 자체 | `specs/29.531/29531-j60.docx`, `specs/29.531/TS29531_*.yaml` |
| **Tool source** | lifecycle skill/script/template/test 자체 | 추적 | repo 코드 | `.claude/skills/*`, `design/scripts/*`, `design/schemas/*`, `tests/*` |
| **Local reproducible artifact** | skill/script 가 원본+도구로 재생성하는 중간 산출 | 비추적 (`.gitignore`) | 생성 script/skill | `design/<nf>/_manifest.yaml`, `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml`, `_*_status.yaml` |
| **Reviewed lifecycle artifact** | lifecycle 단계의 결과로 생성되지만 다음 단계 입력으로 review/ratify 되어 보존되는 산출 | 추적 | 파일 + 해당 단계 skill contract | `design/<nf>/architecture/**`, `design/<nf>/module-decomposition/**`, `dev/<nf>/*`, `engineering/<nf>/engineering-design.md` |
| **Decision/policy artifact** | 프로젝트 공통 결정·용어·운영 원칙 | 추적 | ADR/정책 문서 | `docs/adr/*`, `CONTEXT.md`, `CLAUDE.md`, `ONBOARDING.md`, `AGENTS.md` |
| **Work-management artifact** | 특정 변경 사이클을 진행하기 위한 계획/회고/인수인계 | 추적, 단 완료 후 source of truth 아님 | 관련 PR/retro/ADR | `docs/plans/*`, `docs/retros/*`, `docs/handover.md` |
| **Runtime/local state** | agent/runtime/cache/venv/editor state | 비추적 | 실행 환경 | `.omx/*`, `.venv/`, `.pytest_cache/`, `specs/*/_extracted/` |

---

## 2. Fresh clone + specs-only 상태에서 늘어나는 순서

`specs/` 만 존재한다고 가정하면, lifecycle 은 아래 순서로 파일을 만든다. 각 단계는 **다음 단계 skill 을 자동 호출하지 않는다**. 사람이 다음 단계 진행을 결정한다.

| 단계 | trigger/skill | 입력 | 생성/갱신 산출 | class | git 정책 |
|---|---|---|---|---|---|
| A. Source ingest | 사람 | 3GPP portal 등 | `specs/<spec>/*.{docx,yaml,pdf}` | Source input | 추적 |
| B. Spec discovery | `/nf-spec-discover <nf> --primary <spec>` (`/nf-init`) | `specs/` | `design/<nf>/_manifest.yaml`, `design/<nf>/_contract_seed.yaml` | Local reproducible artifact | 비추적 |
| C. Contract extraction | `/nf-contract-build <nf>` (`/nf-build`) | manifest/seed + specs | `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml` | Local reproducible artifact | 비추적 |
| D. Contract validation | `/nf-contract-check <nf>` (`/nf-status`) | contract 산출 | `design/<nf>/_contract_status.yaml` | Runtime/validation report | 비추적 |
| E. Architecture design | `/nf-arch-design <nf>` | handoff contract + contract docs | `design/<nf>/architecture/**`, `design/<nf>/module-decomposition/**` | Reviewed lifecycle artifact | 추적 |
| F. Architecture validation | `/nf-arch-status <nf>` | architecture 산출 | `design/<nf>/_arch_status.yaml` | Runtime/validation report | 비추적 |
| G. Implementation planning | `/nf-impl-plan <nf>` | architecture + handoff contract | `dev/<nf>/implementation-plan.md`, `tasks.yaml`, `test-matrix.md`, `traceability.md` | Reviewed lifecycle artifact | 추적 |
| H. Impl-plan validation | `/nf-impl-status <nf>` | `dev/<nf>/` | `dev/<nf>/_impl_status.yaml` | Runtime/validation report | 비추적 |
| I. Engineering design freeze | `/nf-eng-design <nf>` | architecture + dev plan + schema + 사람 결정 | `engineering/<nf>/engineering-design.md` | Reviewed lifecycle artifact / 사람 ratify | 추적 |
| J. Eng freeze validation | `/nf-eng-status <nf>` | engineering design + soft advisory inputs | `engineering/<nf>/_engineering_status.yaml` | Runtime/validation report | 비추적 |

`eng_frozen` PASS 이후에만 자율 코드 생성으로 넘어간다. 현재 repo 의 `dev/<nf>/` 는 **코드 디렉터리**가 아니라 implementation-planning 산출 위치다.

---

## 3. 현재 디렉터리별 의미

### `specs/` — 원본 입력 보존

- 역할: 3GPP 원본 문서와 OpenAPI YAML 보관.
- 예: `specs/29.531/29531-j60.docx`, `specs/29.531/TS29531_Nnssf_NSSelection.yaml`.
- git: 원본 파일은 추적. `_extracted/` 같은 split/cache 는 비추적.
- 금지: lifecycle 산출, 요약, 사람이 쓴 해석 문서를 여기에 두지 않는다.

### `design/scripts/`, `design/schemas/` — 도구와 schema

- 역할: lifecycle 산출을 만들거나 검증하는 Python script, schema/profile.
- git: 추적. 변경 시 tests 또는 targeted smoke 필요.
- 예: `nf-manifest.py`, `build-handoff.py`, `nf-arch-status.py`, `nf-eng-status.py`, `engineering-core-slots.yaml`.

### `.claude/skills/`, `.codex/skills/` — agent workflow surface

- `.claude/skills/nf-*`: 이 repo 의 lifecycle skill 정의와 template. 추적.
- `.codex/skills/*`: Codex/PR/pane helper skill. 추적되는 repo-local skill 만 공유 대상.
- 외부 설치 skill symlink/cache 는 `.gitignore` 정책을 따른다.

### `design/<nf>/` — NF별 설계 영역

| 경로 | 의미 | class | git 정책 |
|---|---|---|---|
| `design/<nf>/_manifest.yaml` | spec discovery 결과와 manual override | Local reproducible artifact | 비추적 |
| `design/<nf>/_contract_seed.yaml` | contract extraction seed | Local reproducible artifact | 비추적 |
| `design/<nf>/contract/**` | spec-derived topic contract markdown/json | Local reproducible artifact | 비추적 |
| `design/<nf>/_contract_status.yaml` | contract gate 측정 결과 | validation report | 비추적 |
| `design/<nf>/architecture/**` | review 된 architecture baseline | Reviewed lifecycle artifact | 추적 |
| `design/<nf>/module-decomposition/**` | module 책임·입출력·상태 baseline | Reviewed lifecycle artifact | 추적 |
| `design/<nf>/_arch_status.yaml` | architecture gate 측정 결과 | validation report | 비추적 |
| `design/<nf>/_archive/**` | reset 시 contract 산출 archive | Local reset artifact | 비추적 |

주의: `architecture/**` 는 skill 이 생성/갱신할 수 있지만, 한번 PR 로 ratify 되면 다음 단계의 source of truth 이므로 추적한다.

### `handoff/<nf>/`

- 역할: machine-readable dev contract 전달 영역.
- 현재 canonical: `handoff/<nf>/contract.yaml`.
- class: Local reproducible artifact.
- git: 비추적. fresh clone 후 `/nf-spec-discover` → `/nf-contract-build` 로 복원한다.

### `dev/<nf>/` — implementation planning, 코드 아님

| 파일 | 의미 | 생성/갱신 주체 | git 정책 |
|---|---|---|---|
| `implementation-plan.md` | 구현 phase, scope, risks | `/nf-impl-plan` | 추적 |
| `tasks.yaml` | implementation task graph (`impl-plan-v1`) | `/nf-impl-plan` | 추적 |
| `test-matrix.md` | test inventory/coverage | `/nf-impl-plan` | 추적 |
| `traceability.md` | contract→module→test mapping | `/nf-impl-plan` | 추적 |
| `_impl_status.yaml` | impl planning gate 결과 | `/nf-impl-status` | 비추적 |

`tasks.yaml` 은 자동 validator 산출물이 아니다. `/nf-impl-plan` 이 architecture 를 작업 단위로 분해해 만든 **reviewed lifecycle artifact** 이다. architecture/security baseline 이 바뀌면 `dev/<nf>/` 4개 파일도 다음 정합 PR 에서 갱신해야 한다.

### `engineering/<nf>/`

| 파일 | 의미 | git 정책 |
|---|---|---|
| `engineering-design.md` | spec 으로 자동 도출할 수 없는 개발 설계 결정 freeze. 사람 ratify 필요 | 추적 |
| `_engineering_status.yaml` | `eng_frozen` gate 측정 결과 | 비추적 |

`engineering-design.md` 는 코드 생성 직전의 production-capable dependency/runtime/config/schema 결정을 닫는다. `dev/<nf>/tasks.yaml` 은 이 결정을 대체하지 않고 soft advisory 입력만 제공한다.

### `docs/adr/`

- 역할: 프로젝트/architecture 정책의 source of truth.
- 예: lifecycle vocabulary, engineering freeze, project security baseline.
- git: 추적.
- 원칙: 반복 적용되는 결정이나 downstream 산출을 강제하는 결정은 `docs/plans/` 가 아니라 ADR 로 남긴다.

### `docs/plans/`

- 역할: 특정 변경 사이클을 수행하기 위한 임시/보조 실행 계획.
- git: 추적 가능하나, 완료 후 canonical source of truth 가 아니다.
- 완료 후: 결과는 `docs/retros/` 또는 ADR/README/ONBOARDING/skill 문서로 승격한다.
- 사용 기준: multi-step 작업, 구조 변경, skill/script 변경, PR phase 분할.

### `docs/retros/`

- 역할: 완료된 변경 사이클의 결과·검증·교훈 기록.
- git: 추적.
- 성격: historical record. 현재 정책과 충돌하면 현재 ADR/README/ONBOARDING/skill 이 우선한다.

### `.omx/`, `.venv/`, `.pytest_cache/`

- 역할: runtime/session/cache/local dependency.
- git: 기본적으로 비추적 (`.gitignore` 의 `.omx/*`). 현재 repo 에 과거 추적된 `.omx/audits/*` 가 남아 있으면 historical artifact 로만 취급하고, 새 runtime 파일을 추가 추적하지 않는다.
- 원칙: cleanup scope 가 아니다. 사용자가 명시하지 않는 한 삭제·재구성하지 않는다.

---

## 4. `generated` 라는 말의 두 의미

현재 혼동의 핵심은 “자동 생성”과 “git 비추적”이 같은 뜻이 아니라는 점이다.

| 질문 | 예 | 답 |
|---|---|---|
| 도구가 만들었나? | `dev/nssf/tasks.yaml` | 예 |
| git 에서 제외되나? | `dev/nssf/tasks.yaml` | 아니오 — review 된 lifecycle artifact |
| fresh clone 후 재생성 가능한가? | `handoff/nssf/contract.yaml` | 예 |
| git 에서 제외되나? | `handoff/nssf/contract.yaml` | 예 — local reproducible artifact |
| 검증 결과인가? | `engineering/nssf/_engineering_status.yaml` | 예 |
| 다음 단계 입력 source인가? | `_engineering_status.yaml` | 아니오 — gate report |

판별 기준:

1. **다음 단계가 파일 내용을 source of truth 로 읽는가?** 그렇다면 보통 추적한다.
2. **동일 입력+script 로 결정론적 재생성이 가능한가?** 그렇다면 보통 비추적한다.
3. **사람 ratify/PR review 로 의미가 닫히는가?** 그렇다면 추적한다.
4. **한 번의 작업을 진행하기 위한 계획/회고인가?** `docs/plans/` 또는 `docs/retros/` 로 분리한다.
5. **실행 상태/캐시/로그인가?** `.omx/` 또는 ignore 대상이다.

---

## 5. 현재 NSSF 상태 요약

현재 repo 에서 NSSF 는 다음 baseline 을 이미 보유한다.

- `specs/29.531`, `29.500`, `29.571`, 관련 YAML 등: source input.
- `design/nssf/architecture/**`, `design/nssf/module-decomposition/**`: architecture baseline, git 추적.
- `dev/nssf/{implementation-plan.md,tasks.yaml,test-matrix.md,traceability.md}`: implementation-planning baseline, git 추적.
- `engineering/nssf/engineering-design.md`: engineering freeze 결정 문서, git 추적.
- `docs/adr/ADR-0004-project-security-baseline.md`: 모든 NF 공통 security baseline.

다음 파일들은 현재 작업 중 생성될 수 있지만 git source 로 보지 않는다.

- `design/nssf/_manifest.yaml`
- `design/nssf/_contract_seed.yaml`
- `design/nssf/contract/**`
- `handoff/nssf/contract.yaml`
- `design/nssf/_contract_status.yaml`
- `design/nssf/_arch_status.yaml`
- `dev/nssf/_impl_status.yaml`
- `engineering/nssf/_engineering_status.yaml`

---

## 6. 새 파일 위치 결정표

| 새 파일의 목적 | 위치 |
|---|---|
| 3GPP 원본 docx/pdf/yaml | `specs/<spec>/` |
| spec discovery 결과 | `design/<nf>/_manifest.yaml` |
| spec-derived contract topic | `design/<nf>/contract/<category>/` |
| machine-readable dev contract | `handoff/<nf>/contract.yaml` |
| architecture/module 책임 baseline | `design/<nf>/architecture/`, `design/<nf>/module-decomposition/` |
| implementation planning task/test/trace | `dev/<nf>/` |
| 사람 ratify 가 필요한 engineering decision | `engineering/<nf>/engineering-design.md` |
| lifecycle/보안/게이트 같은 반복 적용 정책 | `docs/adr/` |
| 특정 변경 작업의 실행 계획 | `docs/plans/` |
| 완료된 변경 작업의 결과 기록 | `docs/retros/` |
| agent/runtime/cache/log/local state | `.omx/`, ignored cache dirs |
| repo 도구/검증 script | `design/scripts/`, `tests/` |
| repo workflow skill/template | `.claude/skills/`, `.codex/skills/` |
