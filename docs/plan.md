# 5gc-design 재정렬 plan

> **status.** 진행 중. 다음 commit = **C1**.
> **source.** 2026-05-10 grill-me 세션 (`/grill-me 이프로젝트의 목적을 이해해서 분석해줘`).
> **memory.** `~/.claude/projects/-home-jjinri-AI-5gc-impl-kb/memory/project_framing.md`·`project_gate_naming.md` 가 본 plan 의 *원칙* 측면 진실 출처. 본 파일은 *실행 sequence* 만.

---

## 0. 한 줄 요약

본 repo (`5gc-impl-kb`) 는 **5gc-design 시스템** — 3GPP spec → LLM agent 자동 파이프라인 → NF design deliverable (`design/<nf>/3gpp-*.md` + `handoff/<nf>/_handoff.yaml`). NF 빌드·시뮬·배포는 별도 시스템 5gc-dev 의 책임이며 본 repo 의 영역이 아님. 현 자산 일부 (yaml-to-c.py 의 본격 codegen, gate 이름 `implementation_ready`/`production`) 가 dev 책임을 끌어안는 *드리프트* 를 보이므로 *책임 경계 회복* + *handoff contract 1급 승격* 을 한다.

---

## 1. Grill 결정 요약 (Q1 ~ Q8)

| Q | 결정 |
|---|---|
| Q1 — 1차 사용자 | LLM agent (5gc-dev 측 codegen) |
| Q2 — NF acceptance bar | (4) Configuration·Error·Scenarios 모두 통과. 단 그건 5gc-dev 책임 |
| Q3 — "모든 기능 통과" 정의 | Service Scenarios + Error matrix + Config 변동 시뮬 |
| Q4 — SDLC | 기획→설계 흡수 (a) |
| Q4-후 — 시스템 분리 | 5gc-design (본 repo) ↔ 5gc-dev (별도 책임). *논리적* 항상 분리, *물리적* 단일 monorepo (디렉터리 분리) |
| Q5 — handoff contract | (2) markdown + sidecar yaml |
| Q6 — 결정 배치 | spec 직접 = design / 사용자 선택 = dev. OS·언어·DBMS·배포·HTTP lib·threading = dev. OpenAPI path·schema, error matrix, scenario seq, cross-NF, config key·default = design |
| Q7 — handoff yaml | 7 카테고리 1:1 미러 outline / source-of-truth = markdown / yaml-to-c.py = sanity probe 잔존 |
| Q8 — gate 재정의 | `draft / review_ready / handoff_ready / canonical`. check rename `yaml_to_c_compiles` → `schema_implementable`. 신규 check 2개 |
| Q8-후 — 사람 역할 | (1) target NF 이름 (2) 3GPP docx/yaml cp (3) 명령 트리거 — 그것만. markdown·yaml 본문 손대지 않음 |

---

## 2. 산출 목표 (plan 종료 후 도달 상태)

```
5gc-impl-kb/                    # repo 이름은 후속 결정 (TBD §4)
├── design/                     # ← 현 kb/ 이전
│   ├── nssf/3gpp-ts-29531.md, _manifest.yaml, _status.yaml, _archive/
│   ├── architecture/, interfaces/, security/, slicing/, concepts/, overviews/, other/
│   └── scripts/                # ← 현 scripts/ 이전 (build-handoff.py 신규 포함)
├── handoff/                    # 신규
│   └── nssf/_handoff.yaml      # 5gc-dev contract — markdown 미러
├── dev/                        # 5gc-dev placeholder (README.md 1장만)
├── specs/                      # 그대로
├── docs/                       # 본 plan 등
├── .claude/skills/             # nf-init·nf-build·nf-status·nf-reset SKILL 본문 갱신
├── CLAUDE.md, README.md        # framing 재작성
└── ...
```

acceptance — `/nf-status nssf` → 4 gate (`draft/review_ready/handoff_ready/canonical`) 모두 PASS.

---

## 3. 체크리스트 — C1 ~ C8

각 commit 은 *독립적으로 reversible*. 한 commit 이 깨지면 그것만 revert 하고 다음의 가정만 손보면 됨.

### [ ] C1 — 디렉터리 재구성 (rename only, 동작 변경 0)

- **배경.** 본 repo 가 *5gc-design* 임을 디렉터리로 가시화. 현 평면 구조 (`kb/`·`scripts/`) 가 design 과 dev 의 책임 경계를 흐림.
- **관련 파일.**
  - `kb/` (전체)
  - `scripts/` (전체)
  - `index.md` (kb/ 링크 다수)
  - `.claude/skills/{nf-init,nf-build,nf-status,nf-reset}/SKILL.md` (kb/·scripts/ 언급)
  - `CLAUDE.md` (Repository Structure 섹션)
- **예상 변경.**
  ```bash
  git mv kb design
  git mv scripts design/scripts
  mkdir -p handoff dev docs
  # design/scripts/*.py 안 'kb/' 하드코딩 → 'design/' 일괄 치환
  # index.md 안 kb/ → design/
  # SKILL.md 4개 안 kb/·scripts/ → design/·design/scripts/
  # dev/README.md 1줄 — "5gc-dev 가 채울 placeholder. 본 repo 는 5gc-design 책임만"
  ```
  (CLAUDE.md/README.md 본문 재작성은 C7 으로 분리.)
- **검증.**
  - `/nf-status nssf` → PASS 변동 없음 (gate 이름은 옛 그대로).
  - `grep -r "kb/" design/scripts/ .claude/ index.md` → 0건.
  - `git status` → tracked rename 만, untracked = `handoff/`·`dev/`·`docs/` 의 신규 파일 (placeholder 만).
- **commit msg.** `refactor: kb/ → design/, scripts/ → design/scripts/, handoff/·dev/·docs/ 신설`

---

### [ ] C2 — `nf-status.py` GATE_DEFS rename + check id rename

- **배경.** Q8 합의 — design repo 의 어떤 단어도 "implementation"·"production" 을 *문서 상태* 로 쓰지 않음. yaml_to_c_compiles 도 *언어 무관 구현 가능성 증명* 임을 이름이 가려서 변경.
- **관련 파일.**
  - `design/scripts/nf-status.py` (GATE_DEFS, CHECK_DEFS, applies_to)
  - `.claude/skills/nf-status/SKILL.md` (gate 이름 언급) — 본 commit 에서 같이 손댐 (의미 단위 동일)
  - `design/<nf>/_status.yaml` (재실행으로 자동 갱신)
- **예상 변경.**
  - gate id `implementation_ready` → `handoff_ready`
  - gate id `production` → `canonical`
  - check id `yaml_to_c_compiles` → `schema_implementable` (도구 호출은 그대로 `yaml-to-c.py`, name·docstring 만 *언어 무관* 으로)
  - SKILL.md 내 gate·check 이름 언급 일괄 치환
- **검증.**
  - `/nf-status nssf` → 새 gate 이름으로 4개 모두 PASS.
  - `grep -r "implementation_ready\|production" design/scripts/ .claude/skills/nf-status/` → 0건 (gate 의미로).
  - `cat design/nssf/_status.yaml | grep -E 'id: (handoff_ready|canonical|schema_implementable)'` → 3건 매치.
- **commit msg.** `refactor(nf-status): gate rename — implementation_ready→handoff_ready, production→canonical, check rename — yaml_to_c_compiles→schema_implementable`

---

### [ ] C3 — 신규 도구 `design/scripts/build-handoff.py`

- **배경.** Q5·Q7 — handoff contract (`handoff/<nf>/_handoff.yaml`) 를 1급 산출로 승격. 5gc-dev 가 markdown 재파싱 없이 yaml 만 보면 됨.
- **관련 파일.**
  - 신규 `design/scripts/build-handoff.py`
  - 입력 — `design/<nf>/3gpp-*.md` + `design/<nf>/_manifest.yaml` + `specs/<spec>/*.yaml`
  - 출력 — `handoff/<nf>/_handoff.yaml`
  - 재사용 — `design/scripts/resolve-yaml-refs.py` 의 chain 추적
- **예상 변경.**
  - 첫 줄 한국어 헤더 (전역 §6) — `# 7 카테고리 markdown 페이지를 self-contained handoff yaml 로 변환`
  - schema_version `handoff-v1`. 키 outline (project_framing.md 합의) — `nf, spec, profile, schema_version, generated_from, generated_at, interface, api, data_model[fully resolved], service_scenarios, cross_nf, configuration, error_handling, persistent_state, concurrency`
  - mermaid → `service_scenarios.steps` 변환은 *최선 노력* — 안정 변환 안되면 step list 비워두고 C4 의 `service_flow_coverage` 가 잡도록 함
- **검증.**
  - `python3 design/scripts/build-handoff.py nssf` → exit 0
  - `python3 -c "import yaml; yaml.safe_load(open('handoff/nssf/_handoff.yaml'))"` → exit 0
  - 결과 yaml 의 top-level key — `nf, spec, api, data_model` 4개 이상 존재
- **commit msg.** `feat(handoff): build-handoff.py — markdown 7카테고리 → handoff-v1 yaml 변환 도구`

---

### [ ] C4 — `nf-status.py` 신규 check 2개 추가

- **배경.** handoff yaml 의 *유효성* 과 *self-contained* 를 design 측이 자체 검증. C3 의 산출이 contract 로 받아들여질 수 있는지의 gate.
- **관련 파일.**
  - `design/scripts/nf-status.py` (CHECK_DEFS 에 2개 추가, GATE_DEFS.handoff_ready.requires_pass 에 등록)
  - 산출 갱신 대상 — `design/nssf/_status.yaml`
- **예상 변경.**
  ```yaml
  - id: handoff_yaml_valid
    tier: 1
    applies_to: ['stage_3_only', 'mixed']
    criterion: |
      handoff/<nf>/_handoff.yaml 존재 + yaml.safe_load 통과 + schema_version == handoff-v1 +
      필수 top-level key 4개 (nf, spec, api, data_model) 보유.
    to_pass: |
      .venv/bin/python3 design/scripts/build-handoff.py <nf> 실행, 에러 메시지 따라 수정.

  - id: handoff_yaml_self_contained
    tier: 2
    applies_to: ['stage_3_only', 'mixed']
    criterion: |
      handoff/<nf>/_handoff.yaml 안 모든 $ref 가 같은 파일 내 anchor 로 resolve.
      외부 spec yaml 또는 다른 NF handoff 로의 ref 0건.
    to_pass: |
      Data Model 카테고리에서 (참조 규격 미등록) leaf 를 spec cp 또는 manual_overrides 로 닫고
      build-handoff.py 재실행.
  ```
  - `handoff_ready` gate 의 requires_pass 에 위 두 id 추가.
- **검증.**
  - `/nf-status nssf` → 두 신규 check PASS, gate `handoff_ready` PASS 유지.
- **commit msg.** `feat(nf-status): handoff_yaml_valid·handoff_yaml_self_contained 신설 — handoff_ready gate 강화`

---

### [ ] C5 — NSSF 의 첫 handoff yaml 생성 + commit

- **배경.** C3·C4 가 완성된 직후 NSSF 의 첫 contract 산출. 본 commit 으로 `handoff/nssf/_handoff.yaml` 이 처음 git 에 들어감.
- **관련 파일.**
  - 신규 `handoff/nssf/_handoff.yaml`
- **예상 변경.**
  ```bash
  .venv/bin/python3 design/scripts/build-handoff.py nssf
  git add handoff/nssf/_handoff.yaml
  ```
- **검증.**
  - `/nf-status nssf` → 4 gate 모두 PASS, 신규 check 2개 PASS.
  - `wc -l handoff/nssf/_handoff.yaml` → 합리적 크기 (수백 줄 이상 예상 — data_model resolve 결과).
- **commit msg.** `feat(nssf): handoff/nssf/_handoff.yaml 초판 생성 — 5gc-dev contract 1차`

---

### [ ] C6 — SKILL.md 4개 일괄 갱신

- **배경.** SKILL.md 가 사용자가 `/nf-*` 명령을 호출했을 때 agent 가 따르는 절차의 진실 출처. C1~C5 의 변경 후 절차가 현실과 어긋남.
- **관련 파일.**
  - `.claude/skills/nf-init/SKILL.md`
  - `.claude/skills/nf-build/SKILL.md`
  - `.claude/skills/nf-status/SKILL.md`
  - `.claude/skills/nf-reset/SKILL.md`
- **예상 변경.**
  - 모든 `kb/` 언급 → `design/` (C1 에서 일부 했지만 누락된 곳 점검)
  - gate 이름 `implementation_ready`·`production` 모두 `handoff_ready`·`canonical` 로 (C2 에서 nf-status SKILL 만 했고, 나머지 3개도 점검)
  - **nf-build SKILL** — *"build-handoff.py 자동 호출"* 단계 추가. markdown 갱신 후 yaml 도 자동 갱신
  - **nf-reset SKILL** — 백업 대상에 `handoff/<nf>/_handoff.yaml` 추가
  - **nf-status SKILL** — Tier 3 의 도구 이름·check id 갱신 (C2 에서 일부 했으면 본 commit 은 nop)
- **검증.**
  - `grep -rEn "kb/|implementation_ready|production" .claude/skills/` → 0건 (production 단어가 정당한 다른 맥락 — 예 README — 은 무관)
  - `/nf-build nssf` 더미 실행 시뮬 — SKILL.md 의 step 이 build-handoff.py 호출 step 을 포함하는지 확인 (실행 안 함, 본 plan 은 docs only)
- **commit msg.** `docs(skills): design/ 디렉터리·신규 gate 이름·build-handoff 자동 호출 반영`

---

### [ ] C7 — `CLAUDE.md` / `README.md` 재작성

- **배경.** 본 repo 의 정체성 문서가 변경된 framing 을 반영해야 함. *implementation* 단어가 사라진 design repo.
- **관련 파일.**
  - `CLAUDE.md`
  - `README.md`
- **예상 변경.**
  - **CLAUDE.md.**
    - "implementation 정의 — C struct 로 떨어질 수 있어야" 등의 단어 제거
    - "Repository Structure" 새 layout (design/·handoff/·dev/) 반영
    - "Acceptance Gates" 표 갱신 — 새 4 gate 이름·의미
    - 새 섹션 *"5gc-design ↔ 5gc-dev 책임 경계"* — Q6 결정 배치 표 그대로
    - "NF Profile" 표·"Knowledge depth, not breadth" 섹션은 유지
  - **README.md.**
    - 첫 문단 — *"3GPP spec 으로부터 NF design deliverable 을 자동 생산하는 5gc-design 시스템"*
    - "Quick start" 의 명령은 그대로 (인터페이스 동일)
    - "4 SKILL 사이클" 그림에 `build-handoff` 가 `nf-build` 안에 흡수됨을 명시
- **검증.**
  - `grep -E "implementation_ready|production" CLAUDE.md README.md` → 0건 (gate 의미로)
  - 새 디렉터리 layout 그림이 실제 디렉터리와 일치
- **commit msg.** `docs(framing): CLAUDE.md·README.md 재작성 — 5gc-design 시스템으로 재정렬`

---

### [ ] C8 — push

- **배경.** 모든 commit 이 origin 에 반영되어 다른 환경 (또는 다음 세션) 에서 즉시 pull 가능 상태.
- **관련 파일.** 없음 (git 작업만).
- **예상 변경.** `git push origin main`.
- **검증.**
  - `git ls-remote origin HEAD` 가 local HEAD 와 일치
  - `/nf-status nssf` → 4 gate 모두 PASS
  - `git log --oneline -7` → C1~C7 의 의미 단위 commit 7 개 (C8 자체는 commit 없음)
- **commit msg.** 없음 (push only).

---

## 4. 비실행 (out of scope)

- repo 이름 변경 (`5gc-impl-kb` → `5gc-stack` 등) — TBD, 사용자 확인 필요. 디렉터리 재구성만으로 framing 회복 가능.
- `dev/` 별도 repo 분리 — 지금은 monorepo 유지. dev 가 실제로 채워지고 무거워졌을 때 결정.
- `yaml-to-c.py` 다언어 polyglot 확장 — C-only 유지. 도구 이름·docstring 만 변경.
- 회고 (retrospective) 단계 정형화 — 본 grill 흐름이 이미 회고의 한 형태. 추후 별도 plan.
- 5gc-dev 자체 설계 — 본 repo 에서 다루지 않음. `dev/README.md` 1줄 placeholder 만.

## 5. 다음 plan 의 후보 (본 plan 종료 후)

- 5gc-dev placeholder 의 *minimal consumer* (yaml 만 읽어 operation 카운트 출력) 작성 → contract 가 *실제로 실행 가능* 한지 1회 검증.
- NRF 시작 — `/nf-init nrf --primary 29.510`. 두 번째 NF 로 framework 일반화 검증.
- repo 이름 변경 결정.
