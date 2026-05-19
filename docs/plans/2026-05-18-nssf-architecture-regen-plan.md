# NSSF architecture 재생성 — 2026-05-18

Status: in progress (2026-05-18)
Current objective: fresh-full materialization 사이클(PR #24)로 재생성된 NSSF contract 를 입력으로 `/nf-arch-design nssf` 를 실행해 architecture 를 재생성하고, PR #16/#20 으로 머지된 canonical architecture baseline 과 diff 를 *의도적으로* 검토한 뒤 `/nf-arch-status nssf` (gate `arch_consistent`) 로 검증, 별도 PR 로 올린다.

직전 fresh-full materialization 사이클(PR #24 `4967274`, retro+plan-close `a56e333`)의 *의도적 후속* — 그 사이클에서 tooling/contract-generator 작업과 architecture 재생성을 분리하기로 결정됨. 본 plan 은 tooling 변경 없이 NSSF 적용 (E)~(F) 만 수행한다.

## 컨텍스트 — 검증된 사실

worktree `nssf-architecture-regen-20260518` (base origin/main `a56e333`).

1. **canonical architecture 는 git 추적 baseline.** `design/nssf/architecture/*.md` 10개 + `architecture/decisions/ADR-0001-architecture-baseline.md` + `module-decomposition/*.md` 4개 = PR #16/#20 머지 산출. `/nf-arch-design` 은 이를 *재생성/갱신* 하므로 canonical 과 diff 발생. USER 섹션은 보존, AUTO 변동.
2. **contract 는 gitignored, 부재.** `design/nssf/contract/`·`handoff/nssf/contract.yaml`·`_manifest.yaml`·`_contract_seed.yaml` 전부 fresh worktree 에 없음. (E) 입력 = contract → 먼저 재생성.
3. **manual_overrides 복원 필요.** fresh `nf-manifest.py` 는 manual_overrides 비어 33.501·38.413 미충족으로 `ready_for_build=false` 가능. 기존 ratify 된 운영 보류 결정 복원 필요 (33.501 = SBA TLS·OAuth2 외부화 구현깊이 0 / 38.413 = AMF reallocation via RAN 미구현 보류). *기존 사람 결정 복원* 이지 신규 정책 아님 — 복원 사실 사용자 보고 후 진행.
4. **로컬 main `aec753c` 보존.** Pane 2(codex) 미push 직접커밋. 본 worktree 는 origin/main 분기라 무관. 머지 후 로컬 main 동기화는 `git pull --rebase` 로 보존.

## 절차

| 단계 | 동작 | gate / 기대 |
|---|---|---|
| P1 worktree | EnterWorktree + .venv symlink | (완료) |
| P2 plan | 본 파일 | (진행) |
| P3 contract 재생성 | `/nf-spec-discover nssf --primary 29.531` (manifest refresh + manual_overrides 복원 + seed auto-gen) → `materialize-contract.py nssf` → `build-handoff.py nssf` → `validate-extraction.py nssf --level basic` | `ready_for_build=true`, materialize 25/25, validate 13/0 |
| P4 contract-check | `/nf-contract-check nssf` | `handoff_ready` PASS |
| P5 (E) arch 재생성 | `/nf-arch-design nssf` | USER 섹션 보존, AUTO 가 새 contract 반영 |
| P6 diff 검토 | `git diff` canonical(PR#16/#20) 대비 *내용* 분석 | AUTO 변동이 contract 정확 반영, USER 산문 손실 0. 대량/예상외 변동 → 사용자 보고 |
| P7 (F) arch-status | `/nf-arch-status nssf` | `arch_consistent` PASS (canonical 기준 PASS 였음 — 재생성 후 유지) |
| P8 PR | 커밋 분리, gitignored 산출/.venv 제외, push 브랜치 + `gh pr create` | — |
| P9 second-opinion | `/pane-send 2 $git-pr-review <PR#>` → `/pane-compare 2` → 리뷰 반영 → 사용자 머지 지시 대기 | — |

## 명시적 제외

- tooling/script/SKILL 변경 — 본 사이클은 NSSF 적용만. contract-generator·arch-design skill 수정은 별도 사이클.
- contract 내용 자동 격상 우회 — unresolved_refs 비워 격상 금지, root-fix 만. validate 통과 ≠ 산출 정상 → 산출 *내용* 직접 검증.
- 비-NSSF NF — 사용자 결정상 논외.
- 신규 architecture 결정 — 재생성은 새 contract 반영이지 설계 의도 재정의 아님. USER 섹션이 사람 설계 의도 보존.

## 위험 / 함정

- `/nf-arch-design` 산출 *내용* 까지 검증 (chain-tree/AUTO 가 실제 contract 반영). 지표·gate 통과 안주 금지 (직전 사이클 버그 3건이 그렇게 적발됨).
- worktree 운영 마찰 — `git add .` 금지 (.venv symlink·gitignored 산출 혼입). 특정 파일만 add.
- manual_overrides 복원 누락 시 `ready_for_build=false` → P3 중단. 복원 후 재실행.

## Follow-up (별도 사이클)

본 사이클 scope 외 — tooling 보강. (E) 검증에서 fresh-full contract 의 spec-derived 추출 두 곳이 얕음을 확인, Pane 2 second-opinion 3 라운드로 root-fix 범위 확정.

1. **auto_interface all primary YAML service-summary merge** — `materialize-contract.py:205` 의 first-YAML-only (`next((load_yaml(y) for y in yamls if y.is_file()), {})`) 를 `_manifest.yaml` `primary_files.yamls` 전체의 *service-level summary* 병합으로 교체. 적정 범위 = service/API title · version · server URL · `securitySchemes` · global security · OAuth2 scope · transport/common SBI 전제. 과한 범위 (path/method 상세 · request/response schema · operation별 status code · data-model field tree · operation×error cause matrix) 는 api/data-model/error-handling topic 책임이라 제외. 현 generated `interface` output 은 첫 YAML 기반 partial 이며 `interface` topic 의 의도(전체 service-level 표면 대표)와 다름 — first-YAML-only 는 정책·의도 아닌 구현 한계. 명칭 two-service 금지, all-primary-YAML (NF 별 yaml 개수 유동).
2. **auto_error deep extraction** — `materialize-contract.py:222-240` 을 response `description` / `cause` / operation×response 매핑까지 추출하도록 심화. 현재 status code 나열만 (meaning `-`, cause 매핑 없음) 이라 error-propagation cause matrix 가 contract 에 trace 불가.
