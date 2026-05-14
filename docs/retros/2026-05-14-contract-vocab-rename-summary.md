# 2026-05-14 — contract 단계 어휘 rename + 리뷰 후속 정리 summary

Status: complete
Current objective: 직전 git log/docs 리뷰에서 식별한 약한 지점 5개를 한 사이클에 정리하고, ADR-0001 follow-up #1 (seed/status 파일명 비대칭) 까지 한 PR 로 처리한 작업 기록.

## 한 줄 요약

`design/<nf>/_handoff_seed.yaml` → `_contract_seed.yaml`, `_status.yaml` → `_contract_status.yaml` 으로 산출 파일명을 contract 단계 어휘에 맞춰 통일했고, 같은 PR 에 ADR migration notes 분리·plan 종료 마킹·retro PR 정책 예외 사유·`nf-status` 단계 한정 명시까지 묶어 PR #13 으로 머지했다.

## Phase 0 — 리뷰 시작

- 사용자가 `git log` + `docs/` 기준으로 최근 변경 리뷰를 요청.
- 5/13 ~ 5/14 사이 변경 그룹화.
  - lifecycle 어휘 ADR-0001 채택 + canonical wrapper 3 skill 추가.
  - NSSF contract 산출물 `design/nssf/contract/` 하위로 재배치.
  - `/nf-arch-design` + `/nf-impl-plan` skill + 템플릿 + NSSF draft.
  - handoff 파일명 `_handoff.yaml` → `contract.yaml` rename, legacy fallback 제거.
  - `/nf-init` seed auto-gen 의도 복구, `--reset` 을 contract 산출물만 archive 로 축소.
- 일관성은 PASS, 약한 지점 5개 식별.

## Phase 1 — 약한 지점 5개

| 번호 | 항목 | 처리 |
|---|---|---|
| #1 | ADR migration notes 의 fallback 제거 라인이 contract rename 항목에 끼어 있음 | 별도 라인으로 split |
| #2 | seed/status 파일명이 contract 어휘와 비대칭 (`_handoff_seed`, `_status`) | 본 사이클에서 rename 진행 |
| #3 | 직전 retro 가 PR 정책 트리거 영역인데 직접 push 한 사유 없음 | retro 에 정책 예외 사유 한 줄 추가 |
| #4 | lifecycle skill rename plan 이 닫힘 마킹 없음 | header 에 `Status: closed (2026-05-14)` 추가 |
| #5 | `nf-status.py` 가 architecture/dev 단계 status 를 인지하는지 불명 | 조사 결과 contract 단계 한정. 의도된 동작이므로 ADR 에 한 줄 명시 |

## Phase 2 — #5 nf-status 단계 인지 조사

- `design/scripts/nf-status.py` 의 check 와 gate 모두 contract 산출만 본다.
- `architecture/`, `dev/<nf>/` artifact 인식 없음.
- ADR-0001 의 lifecycle 분리 원칙에 부합 — `nf-status` 는 contract validation 한정.
- 결론. 추가 코드 변경 불필요, ADR `Migration notes` 에 명시만 추가한다.

## Phase 3 — #1, #3, #4 문서 정리

- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
  - fallback 제거 라인 별도 항목으로 split.
  - `nf-status` 가 contract 단계 한정임을 명시.
- `docs/retros/2026-05-14-nf-init-seed-reset-summary.md`
  - Phase 6 에 PR 없이 직접 push 한 사유 한 줄 추가 + 다음 사이클부터 PR 복귀 명시.
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
  - header 에 `Status: closed (2026-05-14)` + 후속 작업은 신규 plan/retro 로 분리한다는 한 줄.

## Phase 4 — #2 seed/status 파일명 rename

ADR follow-up #1 처리.

- scripts. `design/scripts/{build-handoff.py, nf-seed-gen.py, nf-status.py}` 의 모든 reference 갱신.
- skills. `.claude/skills/{nf-init, nf-build, nf-status}/SKILL.md` 갱신.
- docs. `CLAUDE.md`, `README.md`, ADR 의 Consequences/Follow-ups/Migration notes 갱신.
- tests. `tests/scripts/test_build_handoff_v2.py` 갱신.
- data. `design/nssf/_handoff_seed.yaml` → `_contract_seed.yaml` git mv. `design/nssf/architecture/overview.md` 의 contract_status 경로 갱신.
- meta. `.gitignore` 의 `design/*/_status.yaml` → `_contract_status.yaml`.

historical 문서 (`docs/superpowers/`, `docs/plan.md`, `_archive/*`, ADR migration notes 의 2026-05-13 라인) 는 시점 정확성을 위해 갱신하지 않았다. 새 migration note 한 줄로 rename 사실만 추가했다.

## Phase 5 — 검증

worktree (`.claude/worktrees/contract-rename-arch-impl-status-20260514`) 에서 실행.

- `.venv/bin/python3 design/scripts/nf-manifest.py nssf --primary 29.531 --write` — manifest 재생성 OK.
- `.venv/bin/python3 design/scripts/build-handoff.py nssf` — `handoff/nssf/contract.yaml` 갱신.
- `.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic` — `basic 13/13 PASS`.
- `.venv/bin/python3 design/scripts/nf-status.py nssf --no-write` — 신규 산출 경로로 보고. worktree fresh manifest 라 `manifest_ready` 만 FAIL, 본 rename 과 무관.
- `pytest tests/scripts` — `40 passed`.

## Phase 6 — PR #13

- 직전 사이클 retro 의 "다음 사이클부터는 PR 로 복귀" 약속을 지켰다.
- branch. `push/contract-vocab-rename-20260514`.
- commit. `1858ca9 contract 단계 어휘를 산출물 이름까지 일관시킨다`.
- 16 files changed, 54 insertions, 49 deletions.
- 사용자가 GitHub 에서 직접 머지. merge commit `cd174bc`.

## Phase 7 — 동기화 + 정리

- `ExitWorktree action=remove` — worktree + 로컬 push 브랜치 삭제.
- `git pull --ff-only origin main` — 이미 up-to-date.
- `git push origin --delete push/contract-vocab-rename-20260514` — 원격 브랜치 삭제.
- 현재 main HEAD — `cd174bc`.

## 변경된 핵심 파일

- `design/scripts/build-handoff.py`
- `design/scripts/nf-seed-gen.py`
- `design/scripts/nf-status.py`
- `.claude/skills/nf-init/SKILL.md`
- `.claude/skills/nf-build/SKILL.md`
- `.claude/skills/nf-status/SKILL.md`
- `tests/scripts/test_build_handoff_v2.py`
- `CLAUDE.md`
- `README.md`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
- `docs/retros/2026-05-14-nf-init-seed-reset-summary.md`
- `design/nssf/architecture/overview.md`
- `.gitignore`
- `design/nssf/_contract_seed.yaml` (rename)
- `handoff/nssf/contract.yaml` (regen)

## 관찰 + 다음 사이클 권고

- ADR follow-up 은 즉시 처리 가능한 작업이면 같은 사이클에 흡수하는 게 비용이 낮다. 본 사이클은 리뷰 약한 지점 4개 + ADR follow-up #1 을 한 PR 로 묶었고 16 파일이지만 의미 단위가 일관해 리뷰 비용이 작았다.
- historical 문서 갱신 정책을 본 사이클에 처음 명시적으로 적용했다 — ADR 의 시점별 migration note 만 추가, 과거 라인은 그대로. 다음 사이클에도 같은 패턴 유지.
- worktree 안에서 NSSF 스크립트를 돌릴 때 `.venv/bin/python3` 가 worktree 에 없어 `nf-manifest.py` 가 internal `subprocess` 호출에서 실패했다. worktree 에 `.venv` symlink 를 만들어 우회했다. 향후 sub-script 의 venv 경로를 절대 경로 또는 `sys.executable` 기준으로 바꾸는 것이 깔끔하다.
- `nf-status.py` 가 contract 단계 한정이라는 점을 ADR 에 명시했다. architecture/dev 단계 status 가 필요해지면 별도 skill (예 `/nf-arch-status`, `/nf-impl-status`) 로 분리하는 게 lifecycle 분리 원칙에 맞는다. 현재는 over-engineering 이므로 사용자 요청 전까지 보류.

## 최종 상태

- ADR-0001 follow-up #1 (seed/status rename) 완료.
- 리뷰 약한 지점 5개 정리 완료 — #5 는 ADR 한 줄 명시로 처리.
- PR #13 머지 + 로컬/원격 정리 완료.
- 검증 완료 — validate-extraction 13/13, pytest 40/40.
