# Engineering Design Freeze — skill-build 사이클 — 2026-05-19

Status: in progress (2026-05-19)
Current objective: ADR-0002(Proposed)를 구현 — `engineering-core-slots.yaml` profile + `nf-eng-status.py` validator script + `/nf-eng-design`·`/nf-eng-status` 2 skills + 정책 문서(.gitignore·ONBOARDING·lifecycle ADR-0001) 갱신. Open choices 4건을 사람 ratify 로 잠근 뒤 ADR-0002 Status Proposed→Accepted.

직전 사이클: grill-with-docs 2026-05-19 (PR #27 머지 `032ab80`, ADR-0002+CONTEXT.md). 본 사이클은 그 의도된 후속(ADR Consequences/Open choices 구현).

## 컨텍스트 — spec 출처

`docs/adr/ADR-0002-engineering-design-freeze.md` Decision 6항이 spec. `CONTEXT.md` 가 glossary 기반. 변경 없이 따른다.

## Open choices ratify 추천안 (사용자 승인 게이트 — 빌드 전 잠금)

ADR-0002 는 Accepted 전 사람 ratify 4건 요구. 추천 ratify 값:

| # | 항목 | 추천 ratify 값 | 근거 |
|---|---|---|---|
| R1 | `engineering/<nf>/` tracking policy | `engineering-design.md`=git 추적, `engineering/<nf>/_engineering_status.yaml`=`.gitignore`. `engineering/` 신규 top-level. | 기존 `_*_status.yaml` 전부 gitignored 선례와 동일. ADR Consequences 명시. |
| R2 | core slot 초기 목록 + version 규약 | slot **12**: `language`·`runtime`·`sbi_http_framework`·`schema_codegen`·`tls_security`·`oauth2_token_validation`·`persistence`·`telemetry`·`deployment_topology`·`module_source_layout`·`test_build_tooling`·`configuration_management`. 파일=`design/schemas/engineering-core-slots.yaml`, 내부 `version: 1` 필드(파일명 무버전 — rename churn 회피). | ADR Open choices 후보 + Pane 2 보정 — `configuration_management`(env/config/secret injection/defaults/override precedence) 미포함 시 agent 가 발명, deployment/tls 에 묻으면 runtime config 계약 흐려짐. slot 은 `none/disabled/out-of-scope+rationale` 값 허용 → NF-invariant 유지. 내부 version 이 `v1.yaml` 보다 git·참조 안정. |
| R3 | `engineering-design.md` canonical 섹션 집합 | `## Purpose` · `## Decisions` · `## Out of scope` · `## Open Questions` · `## References` (5). 각 slot/register 행은 `## Decisions` 의 typed 표. **PASS 규칙**: `## Open Questions` 가 비어있어야 PASS — 내용 있는 Open Question = `eng_frozen` FAIL. `## Inputs`/`## Inventory` 섹션 불요(validator 가 profile ∪ deferral register 로 inventory 재구성). | arch/impl canonical-section 패턴 차용. Pane 2 보정 — 잔존 Open Question 은 미결정이므로 freeze 와 모순. |
| R4 | slot별 typed decided schema **+** out-of-scope ratify schema | **(a) profile 에 slot별 required typed shape** — 예 `language={name, version}`, `configuration_management={source, secret_strategy, override_order}`. **(a′) conditional/discriminated typed schema** — discriminant 값에 따라 required shape 분기. 예 `persistence`: `backend=rdbms` → `{backend, tables, columns, primary_key, indexes, constraints, migration_tool, migration_policy}` 필수 / `backend∈{kv,file,in_memory}` → 해당 backend storage contract / `backend=none` → rationale. `tls_security`·`oauth2_token_validation` 도 mode/enabled discriminant 로 동형. (별도 `db_table_schema` core slot 은 과적합 — 모든 NF 가 RDBMS 아님, NF-invariant 위반. 단 rdbms 선택 시 table schema 없이 PASS = 위험한 자율코드생성 GO 라 conditional 로 강제.) **(b) engineering-design.md row 는 generic 문자열 1개가 아니라 slot별 typed value** 로 validator 가 검사. **(c)** 공통 row 필드 `{decision, status, rationale, consequence, source, ratified_by, date}`, `status ∈ {decided, explicitly_out_of_scope}`. **`ratified_by`·`date` 는 *모든 frozen row*(`decided` ∪ `explicitly_out_of_scope`)에 필수 — 없으면 FAIL** (AI-draft 가 `status:decided` 만 붙여 PASS 하는 loophole 차단). `explicitly_out_of_scope` 는 추가로 `rationale`/`consequence`/`source` 가 실질 내용이어야 한다. | ADR Decision §2 "slot별 typed closed-form schema" + §5 + Pane 2 차단급 보정(+conditional schema) — row metadata 만으로는 §2 미충족. discriminated typed shape 가 blocking 결정론의 핵심이며 NF-invariant 유지. |
| R5 | `advisory.impl_plan_alignment` 산식 | WARN if: `dev/<nf>/tasks.yaml` 의 acceptance/open-risk 에 *미결정 표현*(`TBD`·decide·choose·select·determine 류)이 있고 그 항목이 engineering-design.md `## Decisions` 와 매칭 안 됨. `tasks.yaml` 부재 = `SKIP`(WARN 아님). **절대 FAIL 아님, blocking 아님.** | ADR Decision §4 soft-advisory + Pane 2 보정 — `owner: dev` 전체 기준은 거의 모든 task 가 WARN 후보라 noise. 미결정 표현+Decisions 미매칭으로 좁힘. |

이 5건은 *엔지니어링/정책 결정* — 사용자 ratify 후 빌드. 변경 원하면 plan 승인 시 지시.

## 빌드 절차 (R1~R5 승인 후)

| P | 산출 | 게이트/검증 |
|---|---|---|
| P1 plan | 본 파일 + Pane2 second-opinion | 사용자 승인 |
| P2 profile | `design/schemas/engineering-core-slots.yaml` (R2 12 slot) — **각 slot 의 required typed shape 포함**(R4-a). P3 의 선행 의존: validator 가 읽을 typed schema 가 여기서 먼저 확정돼야 함 | yaml lint, version 필드, 12 slot typed shape 존재 |
| P3 validator | `design/scripts/nf-eng-status.py` — `engineering/<nf>/_engineering_status.yaml` emit, `gates.eng_frozen` + `advisory.impl_plan_alignment` namespace 분리, inventory=profile ∪ per-NF 연기 레지스터, blocking=R4 slot별 typed value 검사. **deferral register parse fail/missing = `eng_frozen` FAIL**(F-hard 입력). **`persistence.backend=rdbms` ∧ `tables/columns/PK/indexes/constraints/migration_policy` 부재 = FAIL**(conditional schema, R4-a′). **AI-draft 행이라도 `ratified_by`/`date` 없으면 frozen 아님 → FAIL**(engineering decision 은 사람 ratify 전 미freeze) | NSSF 더미 engineering-design.md 로 FAIL→PASS 경로 sanity. nf-arch-status.py 패턴 차용 |
| P4 2 skill + 1 script | `.claude/skills/nf-eng-design/SKILL.md`(생성, R3 canonical 섹션 mandate) · `.claude/skills/nf-eng-status/SKILL.md`(측정 wrapper, nf-eng-status.py 호출). (script = P3 의 nf-eng-status.py — "triplet" 명칭 폐기, 실제 2 skill + 1 script). **`/nf-eng-design` mandate** = F architecture + G impl-plan + contract(data model/API op/lifecycle) 를 근거로 12 slot 결정(persistence backend 후보·DB schema 초안 tables/columns/PK/indexes/constraints/migration_policy 포함) 을 **AI 가 초안 자동생성**, 단 *engineering decision 은 spec-derived 아님 → 사람 ratify(`ratified_by`/`date`) 전 frozen 아님*. F 는 persistent state *필요성만* 식별, G 는 persistence 결정 *연기만* 드러냄 — DB schema 확정 위치는 F/G 아닌 `/nf-eng-design`. | write-a-skill 패턴. 생성≠측정 책임분리. SKILL mandate ↔ validator 상수 single-source. *AI-draft → 사람 ratify → eng_frozen* 흐름이 자율코드생성서 schema 임의생성(너무 늦음)을 차단 |
| P5 정책 문서 | `.gitignore`(R1, `engineering/*/_engineering_status.yaml` 으로 *협소* — 광역 패턴 금지) + `ONBOARDING.md` + `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md` 단계 1행 + ADR-0002 Status Proposed→Accepted + Open choices→Decided | 문서 정합. **ADR Accepted 전 검증**: ① Open choices 전부 Decided ② ONBOARDING/lifecycle ADR-0001 이 새 단계와 무충돌 ③ `.gitignore` 협소 ④ 잔존 Open/`TBD` 없음(있으면 ADR 자기모순) |
| P6 PR | 커밋 분리(profile=pipeline-policy 별 커밋), gitignored/.venv 제외, push 브랜치+gh pr create | — |
| P7 Pane2 | `$git-pr-review` → 반영 → 사용자 머지 지시 → sync → 정리 → retro+plan-close | — |

## 명시적 제외

- **NSSF engineering-design.md 실작성** — 본 사이클은 *메커니즘*(profile·validator·skill·정책) 만. NSSF 의 실제 12 slot + 7 register 행 결정·ratify 는 별 사이클(`/nf-eng-design nssf` 첫 실행). 33.501/38.413/AMF ratify 도 그때. (Pane 2: 메커니즘 PR ≠ NSSF 첫실행 PR — 책임경계 선명.)
- **자율 코드 생성** — 파이프라인 밖. eng_frozen 은 그 GO 신호 정의만.
- **기존 contract/arch/impl skill·validator 변경** — 본 사이클 무관.
- ADR-0002 Decision 재설계 — 구현이지 재설계 아님. grill 수렴 그대로.

## 위험 / 함정

- **세 번째 SKILL↔validator drift** (ADR Consequences) — R3 canonical 섹션을 SKILL mandate 와 nf-eng-status.py 상수가 single-source 로 공유(nf-arch-status.py 의 ARCH_CANON↔SKILL 패턴 차용). 한쪽만 바꾸면 drift.
- **profile = pipeline policy** — P2 는 단순 schema 아님(모든 NF eng_frozen 의미 변경). 별 커밋 + PR 본문 명시.
- **deferral register parse fail/missing** — F-hard 입력이므로 누락·파싱 실패는 `eng_frozen` FAIL(조용한 PASS 금지). P3 validator 에 명시 처리.
- **ADR Accepted 자기모순** — Open choices/`TBD` 잔존 상태로 Accepted 격상 시 ADR 자체 모순. P5 검증에 잔존 0 확인 포함.
- validator blocking 이 결정론 유지 — LLM judge 절대 blocking 금지(ADR Decision §2). advisory namespace 격리.
- worktree `git add .` 금지 (.venv symlink·gitignored 혼입). 특정 파일만.
- 산출 *내용* 검증 — validator 가 실제 FAIL→PASS 를 정확히 판정하는지 더미 입력으로 직접 확인(지표·존재만으로 안주 금지, 직전 사이클 교훈).
