# nf-arch-status / nf-impl-status validator skill 신설 — 2026-05-18

Status: in_progress
Current objective: architecture·implementation-planning 산출의 *자기 일관성* 을 측정하는 read-only validator skill 2 개를 신설한다. contract 단계의 `/nf-status` 와 동격이되 책임이 분리된 arch/dev 단계 status 검사다.

retro `docs/retros/2026-05-18-templates-canonical-summary.md` "다음 사이클 후보" (3), retro 2026-05-14 후보 (3) + L110, ADR-0001 L54 가 예고한 follow-up. 사용자 결정 — (3) 진행. Pane 2 비교에서 우선순위 (3)→(7)→(1) 타당 + 과적합 완화책 합의.

## 컨텍스트

- PR #20 으로 architecture 6 / module-decomposition 7 / dev 표준 섹션이 skill 계약 + exact-match validation snippet 으로 영구화됨. 본 사이클은 그 snippet 을 *독립 실행 가능한 validator skill* 로 승격한다.
- `nf-status.py` (contract 단계) 가 진실 출처 형태 모델 — `check_*() → dict`, `compute_gates`, `render_yaml`, `--no-write`, stderr 요약. 본 skill 들은 같은 shape 을 따른다.
- ADR-0001 L54 — "architecture and implementation-planning stage status are out of scope and would belong to a separate future skill." 본 plan 이 그 future skill.

## Pane 2 과적합 완화책 (scope 강제)

- **read-only.** 어떤 산출도 수정 안 함. `_arch_status.yaml`·`_impl_status.yaml` 만 기계가 매번 새로 씀 (`/nf-status` 와 동일 원칙).
- **discover 기반, 하드코딩 금지.** NSSF module 명·operation 수·test id 직접 박지 않음. module list 는 `design/<nf>/module-decomposition/*.md` glob 으로 discover. task/test/traceability id 는 파일에서 읽어 *관계만* 검사.
- **semantic judge 제외.** 3GPP 해석 품질·설계 적절성·구현 기술 선택 평가는 범위 밖 (NSSF 1 표본 과적합 차단). 구조·참조 정합성까지만.
- **Phase 분할.**
  - Phase 1 (본 사이클) — exact headings + required files + frontmatter + YAML schema + obvious cross-ref(advisory WARNING, non-gate).
  - Phase 2 (별도 사이클) — traceability/table id relation 무결성을 gate 化.
  - 보류 — coverage quality, spec completeness, runtime design adequacy.

## 산출

- `design/scripts/nf-arch-status.py` → `design/<nf>/_arch_status.yaml`
- `design/scripts/nf-impl-status.py` → `dev/<nf>/_impl_status.yaml`
- `.claude/skills/nf-arch-status/SKILL.md`, `.claude/skills/nf-impl-status/SKILL.md`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md` Follow-ups 항목 (L54 resolve)
- `.gitignore` — `design/*/_arch_status.yaml`, `dev/*/_impl_status.yaml` (재생성물, `_contract_status.yaml` 와 동격)

## Check 설계 (Phase 1)

### nf-arch-status — gate `arch_consistent` (binary check AND)

| id | tier | PASS 정의 |
|---|---|---|
| `arch_files_present` | 1 | `design/<nf>/architecture/` 의 10 표준 파일 + `decisions/ADR-0001-architecture-baseline.md` 존재 |
| `arch_sections_exact` | 1 | 각 arch `.md` 의 `## ` 집합이 canonical 6 와 순서까지 일치, ADR 은 ADR 5 와 일치 |
| `module_decomposition_present` | 1 | `design/<nf>/module-decomposition/*.md` ≥ 1 |
| `module_sections_exact` | 1 | 각 module `.md` 의 `## ` 집합이 canonical 7 와 순서까지 일치 |
| `arch_frontmatter_valid` | 1 | arch+module 파일 frontmatter 가 valid YAML + 필수 키 (`nf, stage, status, source_contract, generated_date`) |
| `module_boundaries_xref` | 2 | (advisory WARNING, non-gate) `module-boundaries.md` 의 module 표 ↔ `module-decomposition/*.md` 파일 집합 정합 |

### nf-impl-status — gate `impl_consistent` (binary check AND)

| id | tier | PASS 정의 |
|---|---|---|
| `impl_files_present` | 1 | `dev/<nf>/` 의 `implementation-plan.md, tasks.yaml, test-matrix.md, traceability.md` 존재 |
| `impl_sections_exact` | 1 | implementation-plan 5 / test-matrix 5 / traceability 4 canonical 섹션 순서까지 일치 |
| `tasks_schema_valid` | 1 | `tasks.yaml` `schema_version: impl-plan-v1`, 각 task `id` + `trace_to` 비어있지 않음 |
| `impl_frontmatter_valid` | 1 | 3 `.md` frontmatter valid + 필수 키 (`nf, stage, status, source_architecture, source_contract, generated_date`) |
| `traceability_xref` | 2 | (advisory WARNING, non-gate) `tasks.yaml` id ↔ `traceability.md`/`test-matrix.md` 참조 존재. 깊은 id-relation 은 Phase 2 |

canonical 섹션 정의는 PR #20 으로 머지된 SKILL.md 의 mandate 와 *동일 source*. 본 script 는 그 목록을 상수로 보유 (SKILL.md mandate 와 1:1).

## 명시적 제외

- semantic/품질 judge (sub-agent) — 본 사이클 범위 밖.
- Phase 2 deep traceability id-relation gate 化 — 별도 사이클.
- 기존 `nf-status.py` 변경 — contract 단계 책임, 본 skill 과 분리 유지.
- canonical wrapper 명명 (`/nf-arch-check` 류) — ADR-0001 후속 논의, 본 사이클은 lifecycle 이름 그대로.

## Phase 분해 (단일 PR)

- P0 — 본 plan (현 단계).
- P1 — `nf-arch-status.py` (task #8).
- P2 — `nf-impl-status.py` (task #9).
- P3 — 2 SKILL.md + ADR-0001 follow-up + .gitignore (task #10).
- P4 — smoke (canonical NSSF PASS, synthetic 깨짐 FAIL) + PR (task #11).

PR 단위 — 전체 단일 PR `push/arch-impl-status-20260518`. 머지 전 Pane 2 second-opinion.

## 성공 기준

- 두 script 가 canonical NSSF (`design/nssf`, `dev/nssf`) 에서 gate PASS.
- synthetic 깨짐 (extra heading / 파일 누락 / `trace_to` 빈 task) 에서 해당 check FAIL + gate FAIL.
- 두 SKILL.md 가 measure-only·read-only 명시, `/nf-status` 와 책임 분리표 포함.
- ADR-0001 Follow-ups 에 본 skill 신설 기록 (L54 resolve).
- NSSF 하드코딩 0 (`grep -n 'nssf\|SelectionEngine\|NSSelectionGet' design/scripts/nf-arch-status.py design/scripts/nf-impl-status.py` = 0).

## 리스크와 완화

- **리스크.** canonical 섹션 목록을 script 상수로 복제 → SKILL.md mandate 와 drift 가능.
  - 완화. plan·SKILL.md·script 가 *동일 source* (PR #20 머지 산출) 임을 주석 명시. drift 검사는 Phase 2 또는 단일 source 추출 별도 사이클 후보.
- **리스크.** discover glob 이 `_MODULE.md` 같은 template 잔재나 비-module 파일을 module 로 오인.
  - 완화. `design/<nf>/module-decomposition/*.md` 만 대상 (template 은 `.claude/skills/.../templates/` 라 분리). `_` prefix 파일 제외 규칙 적용.
- **리스크.** exact-match 가 너무 엄격해 사용자 추가 섹션을 막아 운영 마찰.
  - 완화. 이는 *의도* (PR #20 영구화 목적 = 구조 고정). 진짜 우회는 `_*_status.yaml` 의 `manual_overrides.pass_anyway` (nf-status 와 동일 패턴) 로 사용자 명시 결정.
