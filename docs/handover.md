# 세션 간 / PC 간 인계 — auto-memory 보존

> **목적.** Claude Code 의 auto-memory (`~/.claude/projects/<path>/memory/`) 는 PC 별 local 이라 git clone 으로 동기화되지 않는다. 본 파일은 그 내용을 git tracked 형태로 보존해 다른 PC 의 첫 세션에서 *행동 일관성* 이 유지되게 한다.

> **다음 세션 시작 시 행동.** Claude (또는 사람) 이 새 PC 에서 본 repo 를 처음 열 때, 본 파일 내용을 읽고 `~/.claude/projects/<현 repo path>/memory/` 디렉터리 부재 시 아래 *5개 메모리 파일 + MEMORY.md 인덱스* 를 그대로 cp 한다. Claude 가 자동 처리해도 OK.

---

## 1. Memory 파일 5개 (verbatim)

### 1.1 `feedback_option_labels.md`

```markdown
---
name: 선택지 라벨에 그리스 문자·특수기호 금지
description: 사용자에게 옵션을 제시할 때 키보드로 직접 타이핑할 수 있는 라벨만 사용
type: feedback
---
선택지를 제시할 때 라벨은 키보드로 직접 입력 가능한 문자만 사용한다. (1)·(2)·(a)·(b) 같은 숫자/알파벳 OK. α·β·γ·δ·ε 같은 그리스 문자, ①·② 같은 enclosed 숫자, 한자, emoji 금지.

**Why:** 사용자가 답할 때 라벨을 다시 타이핑해야 하는 경우가 있는데, 그리스 문자는 키보드에 없어 답변 자체가 불가. 본 세션에서 Q2 답변 시점에 사용자가 직접 지적함.

**How to apply:** AskUserQuestion 의 label 필드, 본문 글머리 (a)/(b)/(1)/(2), grill-me 같이 "추천 답안" 표기에서도 동일. 이미 본문에 포함된 표 등 *사용자가 답하지 않는 영역* 의 그리스 문자 (예 — 수식, 알파/베타 테스트 단계) 는 무관.
```

### 1.2 `feedback_task_list.md`

```markdown
---
name: 작업 리스트 의무화
description: non-trivial 작업은 시작 전 TaskCreate 로 작업 리스트를 만들고, 사용자가 진행 과정을 볼 수 있게 in_progress / completed 갱신을 한다. 5gc-impl-kb 프로젝트에 명시 적용된 정책이지만 일반 행동 규칙으로도 따른다.
type: feedback
---
3개 이상 단계의 작업을 시작할 때 *반드시* TaskCreate 로 작업 리스트를 만든 뒤 진행. 각 작업 시작 시 in_progress, 끝나면 completed.

**Why.** 사용자가 진행 과정을 볼 수 없으면 (a) 어떤 단계가 끝났는지 모르고 (b) 도중에 우선순위 조정이 어렵다. "구체적인 지시사항으로 명시" 라는 사용자 표현 — 모호한 통합 작업이 아니라 *분리된 task* 로 시각화되어야 한다.

**How to apply.**
- non-trivial = 3+ steps, 또는 30초 이상 걸리는 작업, 또는 여러 파일 변경.
- 단순 1-shot 답변·1-파일 1-줄 fix 는 제외.
- 작업 리스트는 *사용자 결정 사항* 도 한 task 로 표시 (예 "frontmatter 4키 vs 9키 결정 받기").
- 매 sub-agent 위임도 task 로.
```

### 1.3 `project_framing.md` (정정본 — 2026-05-10 wrap-up 사용자 지적 반영)

```markdown
---
name: 본 repo 는 design + dev 통합 단일 monorepo (논리적 분리)
description: 본 프로젝트의 정체성·범위·사람 역할 — design ↔ dev 분리 합의 (2026-05-10 grill + wrap-up 정정)
type: project
---
본 repo (`5gc-impl-kb`, 추후 rename 예정) 는 *5gc-design + 5gc-dev 를 한 git repo 안에 통합한 단일 monorepo*. 안에서 *논리적으로* 두 시스템 역할을 분리.
- `design/` — 5gc-design 영역. 3GPP spec 으로부터 LLM agent 자동 파이프라인으로 *NF design deliverable* (`design/<nf>/3gpp-*.md` + `handoff/<nf>/contract.yaml`) 생산.
- `handoff/` — design ↔ dev contract.
- `dev/` — 5gc-dev 영역 (현재 placeholder, 추후 채워질 예정).

**Why:** 2026-05-10 grill-me 세션에서 사용자가 "현재 프로젝트 방향성이 잘못되어 가는 걸로 보여서 바로 잡고 싶다" 고 명시. yaml-to-c.py 가 design/ 책임 안으로 들어와 있고 acceptance gate 이름이 `implementation_ready`·`production` 인 것이 *dev 책임을 design 측이 끌어안는 드리프트* 로 진단됨. 두 시스템은 입력·cadence·검증 방식이 다르므로 *논리적으로* 분리. 솔로 개발자 + 초기 단계라 *물리적* 분리 (별도 repo) 는 과해서 단일 monorepo 유지. 2026-05-10 wrap-up 에서 "이 repo 는 design 과 dev 를 논리적으로 구분했지만 사실 design, dev를 모두 통합한 단일 통합 repo" 라는 사용자 재확인.

**How to apply:**
- `design/` 안에 코드 컴파일러, 빌드 시스템 (CMake), 시뮬레이터 코드, 테스트 코드 추가 금지. 그건 dev/ 책임.
- `yaml-to-c.py` 는 *schema 가 어떤 언어로든 구현 가능한지 증명하는 sanity probe* 로만 잔존 (gate 이름 `schema_implementable`). 본격 codegen 아님.
- 사람 역할은 (1) target NF 이름 결정 (2) 3GPP docx/yaml 을 `specs/<spec>/` 에 cp (3) `/nf-readiness <nf>` 트리거와 결과 리뷰 — 이 3가지로 한정. 내부 spec discovery/contract build/check 단계는 wrapper 가 순서대로 수행하며, markdown·yaml 본문은 사람이 손대지 않음 (손대면 다음 contract build 에 덮어씌워질 수 있음).
- 결정 배치 — *spec 이 글자로 박혀있으면 design, 사용자가 고르면 dev* 가 한 줄 원칙. OS / 언어 / DBMS / 배포 (bare/docker/VM) / HTTP 라이브러리 / threading model = 모두 dev. OpenAPI path·schema, error matrix, service scenario sequence, cross-NF spec, configuration key·default = design.
- 본 repo 가 제공하는 *내부 contract* 는 `handoff/<nf>/contract.yaml` (markdown 의 7 카테고리를 1:1 미러한 self-contained yaml). dev/ 가 markdown 재파싱 없이 이 yaml 만 보면 됨.
```

### 1.4 `project_gate_naming.md`

```markdown
---
name: Acceptance gate 이름 정정 — implementation/production 단어 금지
description: design ↔ dev 분리 후 gate 이름 재정의 (2026-05-10 grill 합의)
type: project
---
본 repo 의 acceptance gate 이름은 `draft / review_ready / handoff_ready / canonical`. 이전의 `implementation_ready`·`production` 은 폐기.

**Why:** "implementation_ready" 는 "NF 가 구현 준비 됨" 으로, "production" 은 "NF 가 production 됐음" 으로 잘못 읽힘. 그건 dev/ 의 책임. design/ 의 acceptance 는 *문서가 어느 단계까지 성숙했는가* 만 표현해야 함 — 단어에 "implementation"·"production" 이 들어가면 즉시 책임 경계 위반.

**How to apply:**
- 신규 명명 의미 —
  - `draft`: 페이지 골격 (frontmatter_valid).
  - `review_ready`: 사람 검토 가능 (+ sections_complete, manifest_ready, wikilinks_resolve, no_korean_colon_end).
  - `handoff_ready`: *dev 가 `contract.yaml` 만으로 NF build planning 을 시작 가능* (+ data_model_chain_complete, api_operation_coverage, service_flow_coverage, handoff_yaml_valid [신규 — C4], handoff_yaml_self_contained [신규 — C4], schema_implementable [yaml_to_c_compiles 에서 rename]).
  - `canonical`: *해당 spec 버전의 design 정본* (+ implementation_guidance_quality — sub-agent judge 4/5 이상).
- check id `yaml_to_c_compiles` → `schema_implementable` 로 rename. 도구 (`yaml-to-c.py`) 는 그대로, 의미만 *언어 무관 구현 가능성 증명* 으로 변경. C 는 *증명의 수단* 일 뿐.
- 본 repo 의 어떤 새 도구·문서·skill 에서도 "implementation"·"production" 이라는 단어로 *design 산출 상태* 를 가리키지 않는다. 두 단어는 dev/ 만의 단어.
```

### 1.5 `project_no_jvm.md`

```markdown
---
name: NF 의 runtime binary + 생성 코드 + linked libs 에서 JVM/Java 금지 (build-time tool 은 허용)
description: 본 5gc-impl-kb 의 NF runtime artifact + 생성 C 코드 + linked third-party 라이브러리에는 JVM 의존 금지. build-time tool (codegen·lint·doc) 의 host 언어는 자유. 사용자 명시 (2026-05-27)
type: project
---
본 repo (5gc-impl-kb) 의 *NF runtime artifact*, *생성된 C 코드*, *C 코드가 link 하는 third-party 라이브러리* 에 JVM/Java 의존 금지. **build-time tool 의 host 언어는 자유** (openapi-generator-cli 같은 JVM 도구도 build step 으로 OK 단 산출은 C).

**Why:** 2026-05-27 `/nf-implement nssf` Phase 1 진입 직전 사용자 명시 → "JVM 의존, java 는 존재해서는 안됨" + 직후 보강 "build 툴은 CMake, tool 언어 무관. C 언어 작성 툴은 언어 무관. 생성되는 코드는 C 이고, C 코드 내에서 호출해서 사용해야 하는 third-party 라이브러리". 추정 배경 — embedded / native deployment 의 footprint 최소화, JVM 의 cold-start / memory overhead 회피, license/distribution 단순화 (단 build 머신은 무관).

**How to apply (allowed):**
- **openapi-generator-cli (Java)** — build-time codegen tool 로 OK. C generator backend 출력 사용. JRE 가 build 머신에 있어야 하지만 NF 산출에는 들어가지 않음.
- **CMake** — host C++ tool, 무관.
- **Python toolchain** (pyyaml, jinja2, pytest 등) — build/test/doc tool, 무관.
- **clang-tidy / clang-analyzer** — C++ tool, 무관 (lint/analysis).

**How to apply (forbidden):**
- **NF binary 가 JVM 을 require** — 절대 금지 (e.g. Cassandra Java client native bridge 등).
- **생성된 코드가 JNI/JVM 호출** — 금지.
- **link 되는 third-party 라이브러리가 JVM 의존** — 금지 (예 일부 Cassandra client native 가 JVM 의존). libcurl / nghttp2 / libpq / libjwt / openssl / cJSON 같은 *순수 C 라이브러리* 만 허용.
- **CI 에서 NF artifact 에 java 포함** — 산출 docker image / binary 에 JRE 가 들어가면 안 됨 (단 build stage 에서 JRE 사용해서 generator 돌리는 것은 OK).

**Validator guidance for AI agents:**
- engineering-design.md 의 `schema_codegen` 가 openapi-generator(C) 같은 build-time JVM tool 을 가리키면 — *정상*. 거부하지 않음.
- engineering-design.md 의 `sbi_server_stack`·`sbi_client_stack`·`persistence` 등 *runtime-linked* slot 이 JVM 의존 라이브러리 가리키면 — *FAIL 로 보고*.
- WI-codegen-bootstrap 등 build step 이 JRE install 을 요구하면 — *정상*. CI workflow 에 `apt-get install default-jre` 추가 OK.

**Override 시 — 본 메모리 즉시 갱신.** 사용자가 "build 머신에서도 JVM 금지" 로 강화 시 반대 방향 update.
```

### 1.6 `MEMORY.md` (인덱스)

```markdown
- [작업 리스트 의무화](feedback_task_list.md) — non-trivial 작업은 TaskCreate 로 시각화 후 in_progress / completed 갱신
- [선택지 라벨 그리스 문자 금지](feedback_option_labels.md) — 옵션 라벨은 키보드 입력 가능한 (1)/(a) 등만, α·β·γ·① 등 금지
- [본 repo 는 design + dev 통합 monorepo](project_framing.md) — 정체성·범위·사람 역할·결정 배치 (design vs dev) 합의
- [Gate 이름 정정 — implementation/production 금지](project_gate_naming.md) — 신규 gate `draft/review_ready/handoff_ready/canonical`
- [JVM/Java runtime 금지 (build-time tool 은 허용)](project_no_jvm.md) — NF runtime binary + 생성 C 코드 + linked libs 에 JVM 금지. build-time codegen/lint tool 의 host 언어는 자유.
```

---

## 2. 새 PC 에서 memory 디렉터리 재생성 — 한 줄 가이드

```bash
# repo 가 /home/<user>/AI/5gc-impl-kb 에 clone 되었다고 가정
DIR=~/.claude/projects/-home-$USER-AI-5gc-impl-kb/memory
mkdir -p "$DIR"
# 본 파일 (docs/handover.md) 의 § 1 의 6개 markdown 블록 (5 memory + MEMORY.md 인덱스) 을 각각 같은 이름 파일로 cp
# (Claude 가 첫 세션에서 자동 처리해도 됨 — "memory 재구성" 요청)
```

---

## 3. 본 wrapping 이 보존하지 않는 것

- 본 plan 파일 (`~/.claude/plans/glittery-beaming-sutherland.md`) — 이후 정리된 lifecycle 문서와 retrospective 에 흡수됨.
- mattpocock skill 13개 — [`docs/setup.md`](./setup.md) §1 의 한 줄 명령으로 재설치.
- statusline·settings.json 등 사용자 환경 — 본 repo 책임 밖. 사용자 자체 dotfiles 로 관리.
