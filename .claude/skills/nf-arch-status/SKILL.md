---
name: nf-arch-status
description: 본 5gc-impl-kb 의 architecture 산출 (design/<nf>/architecture + module-decomposition) 이 canonical 섹션 계약과 자기 일관성을 만족하는지 검사해 `_arch_status.yaml` 산출하는 워크플로우. 사용자가 "/nf-arch-status nssf", "NSSF architecture 검증", "arch 일관성 검사", "architecture status", "이 NF arch 구현계획 가능?" 등을 말하거나 NF 이름 + architecture 검사를 지정하면 이 skill 을 사용한다. 동작 — `design/scripts/nf-arch-status.py <nf>` 호출 → `design/<nf>/_arch_status.yaml` 갱신 + 콘솔에 acceptance gate (`arch_consistent`) 보고. contract 단계 검사는 `/nf-contract-check`, implementation-planning 단계는 `/nf-impl-status` 의 책임이며 본 skill 은 architecture 단계 *측정* 에 집중한다.
argument-hint: "<nf> [--no-write]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-arch-status.py *) Bash(cat *) Bash(ls *)
---

# nf-arch-status — architecture 자기 일관성 검사

architecture 단계 status 검사다. ADR-0001 § Migration notes (2026-05-14) 가 예고한 "separate future skill" — contract 단계 `/nf-contract-check` 와 책임이 분리된다.

## 입력
- `<nf>` — NF 이름.
- `--no-write` — `_arch_status.yaml` 저장 없이 stdout 만. 일회성 점검에 유용.

## 본 skill 의 책임 범위

본 skill 은 *측정* 만 한다. 어떤 산출도 *수정* 하지 않는다 — `_arch_status.yaml` 은 기계가 매번 새로 쓰는 보고서이며 사용자가 직접 편집하지 않는다.

| 시나리오 | 사용 skill |
| --- | --- |
| contract 완성도 검사 | `/nf-contract-check` |
| architecture 자기 일관성 검사 | `/nf-arch-status` (본 skill) |
| implementation-planning 자기 일관성 검사 | `/nf-impl-status` |
| architecture 산출 생성·갱신 | `/nf-arch-design` |

## 동작 원칙 (이유 포함)

- **CLAUDE.md 정책 + ADR-0001 lifecycle vocabulary 가 우선.**
- **read-only.** architecture 산출을 수정하지 않는다. 측정과 수정이 같은 skill 에 있으면 측정이 수정에 흔들린다 — 생성·갱신은 `/nf-arch-design` 의 책임.
- **discover 기반, 하드코딩 금지.** module 목록은 `design/<nf>/module-decomposition/*.md` glob 으로 발견 (`_` prefix 제외). 특정 NF 의 module 명·operation 수를 박지 않는다 — 1 NF 표본 과적합 차단.
- **semantic judge 제외.** 3GPP 해석 품질·설계 적절성·구현 기술 선택은 평가하지 않는다. 구조·참조 정합성까지만. 품질 판단은 사람/별도 사이클.
- **gate 는 binary check 의 AND, WARN 은 advisory.** `arch_consistent` = Tier 1 check 전부 PASS. `module_boundaries_xref` 는 Tier 2 advisory WARN — gate 비포함 (Phase 1). 깊은 id-relation 무결성은 Phase 2 별도 사이클.
- **모든 check 는 criterion + to_pass 의무.** FAIL 만 보고하면 사용자가 다음 액션을 추측해야 한다. `to_pass` 가 즉시 backlog 항목이 되도록.
- **canonical 섹션은 SKILL.md mandate 와 동일 source.** script 의 `ARCH_CANON`/`ADR_CANON`/`MODULE_CANON` 상수는 `nf-arch-design/SKILL.md` 의 mandate (PR #20 영구화) 와 1:1. 한쪽만 바꾸면 drift — 함께 갱신.

## 검사 항목 (Phase 1)

| id | tier | PASS 정의 |
| --- | --- | --- |
| `arch_files_present` | 1 | architecture 표준 10 파일 + `decisions/ADR-0001-architecture-baseline.md` 존재 |
| `arch_sections_exact` | 1 | 각 arch `.md` `## ` 집합이 canonical 6 와 순서까지 일치, ADR 은 ADR 5 |
| `module_decomposition_present` | 1 | `module-decomposition/*.md` (`_` prefix 제외) ≥ 1 |
| `module_sections_exact` | 1 | 각 module `.md` `## ` 집합이 canonical 7 와 순서까지 일치 |
| `arch_frontmatter_valid` | 1 | arch+module frontmatter valid YAML + 필수 키 (`nf, stage, status, source_contract, generated_date`) |
| `module_boundaries_xref` | 2 | advisory WARN (gate 비포함) — `module-boundaries.md` 가 각 module 파일 stem 언급 |

gate — `arch_consistent` = 위 Tier 1 5개 AND.

## Workflow

### 1. 입력 검증
- `<nf>` 가 비어있으면 사용자에게 묻고 정지.
- `design/<nf>/` 디렉터리 존재 확인. 없으면 "/nf-arch-design <nf> 먼저" 안내 후 정지.

### 2. 실행
```bash
.venv/bin/python3 design/scripts/nf-arch-status.py <nf>
```
- `--no-write` 면 `_arch_status.yaml` 미저장, stdout 만.
- script 가 gate FAIL 이면 exit 1.

### 3. 보고
- `arch_consistent` gate PASS/FAIL.
- FAIL check 와 그 `to_pass` 액션.
- WARN (advisory) 는 비차단으로 별도 표시.
- 다음 추천 — gate PASS 면 `/nf-impl-plan <nf>` (architecture 검토 후), FAIL 이면 `/nf-arch-design <nf>` 로 to_pass 반영.

## 자주 틀리는 지점
- `_arch_status.yaml` 을 손으로 편집 — 금지. 기계 재생성물. to_pass 를 따라 *산출* 을 고치고 재실행.
- 사용자 추가 섹션이 `arch_sections_exact` FAIL 을 냄 — 이는 *의도* (PR #20 구조 영구화). 진짜 우회는 `_arch_status.yaml` 의 `manual_overrides.pass_anyway` 에 사유 + 사용자 명시 결정.

## 참고 — 본 skill 안에 다시 적지 말 것
- lifecycle vocabulary: `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`.
- canonical 섹션 정의: `nf-arch-design/SKILL.md` mandate.
- implementation-planning 검사: 자매 skill `/nf-impl-status`.
