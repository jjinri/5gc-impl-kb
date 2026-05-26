---
name: nf-readiness
description: Public lifecycle wrapper for `/nf-readiness <nf>` in 5gc-impl-kb. Use when the user requests readiness pack generation for an NF (e.g. "/nf-readiness nssf", "NSSF readiness", "readiness pack 만들어줘", "/nf-readiness amf --continue-on-fail"). Internally orchestrates the 11-step lifecycle chain (registry resolve → spec-discover → contract-build → contract-check → arch-design → arch-status → impl-plan → impl-status → eng-design → eng-status → readiness-status) and stops at the first blocking gate by default. Internal lifecycle skills (`/nf-spec-discover`, `/nf-contract-build`, `/nf-contract-check`, `/nf-arch-design`, `/nf-arch-status`, `/nf-impl-plan`, `/nf-impl-status`, `/nf-eng-design`, `/nf-eng-status`) remain available for *new-contract* situations but are not the canonical public UX.
argument-hint: "<nf> [--continue-on-fail] [--no-bootstrap-write]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-readiness-resolve.py *) Bash(.venv/bin/python3 design/scripts/nf-registry-bootstrap.py *) Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(.venv/bin/python3 design/scripts/nf-seed-gen.py *) Bash(.venv/bin/python3 design/scripts/build-handoff.py *) Bash(.venv/bin/python3 design/scripts/resolve-yaml-refs.py *) Bash(.venv/bin/python3 design/scripts/validate-extraction.py *) Bash(.venv/bin/python3 design/scripts/nf-contract-check.py *) Bash(.venv/bin/python3 design/scripts/nf-arch-status.py *) Bash(.venv/bin/python3 design/scripts/nf-impl-status.py *) Bash(.venv/bin/python3 design/scripts/nf-eng-status.py *) Bash(.venv/bin/python3 design/scripts/nf-readiness-status.py *) Bash(cat *) Bash(ls *) Bash(mkdir -p *) Bash(find design/* *) Bash(find dev/* *) Bash(find engineering/* *) Bash(find handoff/* *)
---

# nf-readiness — public wrapper for autonomous implementation readiness

본 skill 은 plan §3 (workflow upgrade, 2026-05-21) 의 public workflow 3 행위 중 두 번째 — `/nf-readiness <nf>` 를 구현한다. 사람은 specs/ 에 3GPP 원본을 투입한 뒤 본 wrapper 만 호출하면 lifecycle 전체가 자동 실행되어 *Implementation Readiness Pack* 이 산출되고 aggregate gate `readiness_pack_ready` 가 평가된다.

본 wrapper 가 끝나면 추천 다음 행위는 `/nf-implement <nf>` (PR E3) 다. 본 wrapper 는 implementation code 를 만들지 *않는다*.

## 입력

- `<nf>` — NF 이름. registry resolve 의 키.
- `--continue-on-fail` (옵션) — first blocking gate 에서 멈추지 않고 *diagnostic sweep* 으로 모든 step 을 끝까지 실행. final verdict 는 여전히 첫 blocker 기준.
- `--no-bootstrap-write` (옵션) — registry bootstrap 의 `--write` 차단 (dry-run 만). registry 가 stale 이어도 파일 갱신하지 않는다.

## 책임 범위

본 wrapper 는 *orchestration + reporting* 이다. 각 lifecycle skill 의 책임을 흡수하지 않는다.

- registry resolve 와 confidence policy 적용 (Phase 0).
- lifecycle subroutine 의 *순차 실행* 과 *first-blocker short-circuit*.
- progress table + final readiness verdict 의 통합 보고.

본 wrapper 는 사람의 직접 lifecycle skill 호출을 *대체* 하지 않는다 — third-party library 교체 / DBMS 교체 / security policy 변경 / spec 추가·제외 / public contract 변경 등 "새 계약" 시에는 lifecycle skill 을 직접 호출한다.

## Phase 0 — registry resolve

```bash
.venv/bin/python3 design/scripts/nf-readiness-resolve.py <nf> [--no-bootstrap-write]
```

본 helper 가 수행한다.

1. `design/nf-registry.yaml` load. 부재 또는 `<nf>` entry 부재 시 `nf-registry-bootstrap.py` 자동 실행. dry-run 후 *semantic diff* (generated_at 제외 동일 비교) 있을 때만 `--write` — noisy commit 차단.
2. effective entry resolve — `manual_overrides.nfs.<nf>` 우선 → `generated.nfs.<nf>` 다음.
3. confidence policy.
   - **high (generated)** → proceed.
   - **medium (generated)** → blocker. `manual_overrides.nfs.<nf>` 에 `primary_spec/confidence(>=medium)/rationale` 을 추가해 사람이 확정해야 한다 (generated 단독으로는 insufficient).
   - **low (generated)** → blocker. `manual_overrides.nfs.<nf>` 등록 필요.
   - **manual_override (high/medium)** → 항상 proceed.
   - **manual_override (low)** → blocker (manual 을 high/medium 으로 갱신 필요).
4. effective primary_spec 의 `specs/<spec>/` 존재 확인. 없으면 blocker.

Phase 0 결과는 YAML report 로 stdout. exit 0=proceed, exit 1=blocker.

Phase 0 blocker 시 wrapper 는 step 1 이후를 *시작하지 않는다*. 사용자에게 blocker 메시지를 그대로 보고하고 종료.

## Phase 1~10 — lifecycle chain

Phase 0 proceed 시 다음 순서로 실행. 각 step 마다 PASS/FAIL/SKIP 확인 후 다음.

| step | command | 산출 / gate |
|---|---|---|
| 1 | `.claude/skills/nf-spec-discover` 로직 (manifest + seed) — `.venv/bin/python3 design/scripts/nf-manifest.py <nf> --primary <spec> --write` 후 ready 면 `nf-seed-gen.py <nf>` | `design/<nf>/_manifest.yaml`, `_contract_seed.yaml` |
| 2 | `nf-contract-build` 로직 — topic/category 별 페이지 + `build-handoff.py` + `validate-extraction.py` | `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml` |
| 3 | `.venv/bin/python3 design/scripts/nf-contract-check.py <nf>` | `design/<nf>/_contract_status.yaml` · gates `handoff_ready`, `contract_implementable` |
| 4 | `nf-arch-design` 로직 (skill 내 생성) | `design/<nf>/architecture/**` |
| 5 | `.venv/bin/python3 design/scripts/nf-arch-status.py <nf>` | `design/<nf>/_arch_status.yaml` · gate `arch_consistent` |
| 6 | `nf-impl-plan` 로직 (9-file readiness pack) | `dev/<nf>/{implementation-plan.md, tasks.yaml, test-matrix.md, traceability.md, api-implementation-matrix.md, data-model-implementation-map.md, codegen-work-items.yaml, team-execution-plan.md, verification-plan.md, implementation-readiness-review.md, design-adequacy-checklist.md, spec-to-design-coverage.md, open-gaps-and-assumptions.md}` |
| 7 | `.venv/bin/python3 design/scripts/nf-impl-status.py <nf>` | `dev/<nf>/_impl_status.yaml` · gates `impl_consistent`, `impl_ready_for_codegen` |
| 8 | `nf-eng-design` 로직 (사람 ratify 필요한 행이 있으면 wrapper 가 사용자에게 ratify 안내 + stop) | `engineering/<nf>/engineering-design.md` |
| 9 | `.venv/bin/python3 design/scripts/nf-eng-status.py <nf>` | `engineering/<nf>/_engineering_status.yaml` · gate `eng_frozen` |
| 10 | `.venv/bin/python3 design/scripts/nf-readiness-status.py <nf>` | `dev/<nf>/_readiness_status.yaml` · aggregate gate `readiness_pack_ready` |

각 step 의 generator (1/2/4/6/8) 는 해당 lifecycle skill 의 SKILL.md 워크플로우를 *그대로* 따른다. wrapper 는 시작/끝만 추적한다.

각 step 의 status (3/5/7/9/10) 는 결정론 script 다. wrapper 는 exit code + 산출 YAML 의 `gates[*]` 만 본다.

## Short-circuit policy

default — *first blocking gate* 에서 stop.

step result 분류.

- **PASS** — script exit 0 + 모든 gate PASS. 다음 step 진행.
- **FAIL** — script exit ≠ 0, 또는 gate.status=FAIL. wrapper 가 stop (default). `--continue-on-fail` 이면 다음 step 진행하되 final verdict 는 FAIL.
- **SKIP** — 앞 step 의 hard 산출이 없어 본 step 실행이 의미 없음 (예 step 1 SKIP → step 2 SKIP). progress 표에 표시.

stop 시 `next_to_pass` 추출 우선순위.

1. 해당 step 의 status YAML 의 `gates[<gate>].blocked_by[]` 의 첫 check.
2. 해당 check 의 `to_pass` (string 또는 list 의 첫 원소).
3. 둘 다 없으면 command stderr 의 마지막 비공백 줄.

## Progress table

wrapper 종료 시 사람에게 다음 컬럼 표를 보여준다 (markdown 또는 console table).

```
| step | command            | result | artifact                          | gate                       | blocked_by         | next_to_pass                |
| 0    | nf-readiness-resolve | PASS | (registry)                        | —                          | —                  | (proceed)                   |
| 1    | nf-spec-discover    | PASS  | design/<nf>/_manifest.yaml         | manifest_ready             | —                  | —                           |
| ...  | ...                  | ...   | ...                                | ...                        | ...                | ...                         |
```

`gate` 컬럼은 step 이 평가한 gate 이름. generator step 은 `—`.

## Final report

step 10 이 PASS 면 wrapper 가 마지막에 다음을 보여준다.

- `dev/<nf>/_readiness_status.yaml` 경로.
- aggregate gate `readiness_pack_ready` PASS 또는 FAIL.
- 추천 다음 행위 — PASS 면 `/nf-implement <nf>` (PR E3, autonomous implementation). FAIL 이면 `blocked_by` 와 `next_to_pass` 만 우선 안내, `/nf-implement` 언급 보류.

## Workflow

### 1. 입력 검증
- `<nf>` 비어있으면 묻고 정지.
- `--continue-on-fail` / `--no-bootstrap-write` 외 알 수 없는 옵션이면 정지.

### 2. Phase 0 — registry resolve

```bash
.venv/bin/python3 design/scripts/nf-readiness-resolve.py <nf> [--no-bootstrap-write]
```

stdout YAML 의 `decision` 이 `blocker` 면 즉시 progress 표에 step 0 FAIL + reason 표시 후 종료.

`proceed` 면 effective_primary_spec / effective_source / effective_confidence 를 step 0 PASS row 에 기록 후 step 1 진행.

### 3. Step 1~10 실행

각 step 마다.

1. 해당 step 의 command 실행 (위 표 참조).
2. exit code 와 산출 YAML 의 `gates[*]` 확인.
3. PASS 면 다음 step. FAIL 이면 short-circuit (default).
4. 각 step 의 결과를 progress 표에 누적.

### 4. 최종 보고

step 10 도달 후 (또는 short-circuit 시):

- progress table 전체.
- 마지막 step 의 status YAML 경로.
- final verdict — `readiness_pack_ready` PASS / FAIL.
- 추천 다음 행위.

## 자주 틀리는 지점

- Phase 0 blocker 메시지를 "registry 갱신" 으로만 안내 — 실제 원인이 confidence=low 또는 specs missing 일 수 있다. helper 의 `reason` 필드를 그대로 인용.
- wrapper 가 `/nf-implement` 를 *자동 호출* — 금지. PR E3 skeleton 에도 wrapper-to-wrapper 자동 호출은 없다.
- wrapper 가 manual_overrides 를 *수정* — 금지. wrapper 는 *읽기만*. 수정은 사람.
- `--continue-on-fail` 을 default 로 오해 — diagnostic sweep 옵션. default 는 first blocker stop.

## 참고 — 본 skill 안에 다시 적지 말 것

- registry 정의: `design/nf-registry.yaml` header + `nf-registry-bootstrap.py` docstring.
- aggregate gate 정의: `design/scripts/nf-readiness-status.py` docstring + `docs/adr/ADR-0002-engineering-design-freeze.md`.
- lifecycle subroutine 상세: 각 `nf-*` SKILL.md.
- plan: `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` §3 / §5.1.
