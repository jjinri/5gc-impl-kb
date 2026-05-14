# 2026-05-14 — `/nf-init` seed auto-gen + reset scope cleanup summary

Status: complete
Current objective: Phase 1부터 마무리 단계까지 진행한 `/nf-init` 계열 정리 작업과 legacy handoff 제거 작업을 한 문서로 남긴다.

## 한 줄 요약

`/nf-init` 의 핵심 목적을 **legacy `_handoff.yaml` 수동 작성 제거 + `_handoff_seed.yaml` 자동 생성** 으로 다시 고정했고, `--reset` 은 **contract 산출물만 archive** 하도록 좁혔다. 그 결과 legacy fallback 경로는 제거되었고, `handoff/<nf>/contract.yaml` + seed 기반 workflow 가 현재 기준이 되었다.

## Phase 1 — PR #12 머지 확인 및 기준선 정리

- PR #12 머지를 확인했고, 로컬 `main` 을 원격과 동기화했다.
- 이후 작업은 `push/reset-contract-only-legacy-removal-20260514` 브랜치에서 진행했다.
- 기준선:
  - canonical handoff 파일명은 `handoff/<nf>/contract.yaml`
  - legacy `handoff/<nf>/_handoff.yaml` 은 폐기 대상으로 처리

## Phase 2 — legacy `_handoff.yaml` fallback 제거

- `design/scripts/validate-extraction.py`
  - `handoff/<nf>/contract.yaml` 만 읽도록 변경
  - legacy `_handoff.yaml` fallback 제거
- `design/scripts/nf-status.py`
  - handoff 경로를 `contract.yaml` 단일 경로로 고정
  - `handoff_yaml_valid` 검사를 `handoff-v2` 전용으로 정리
- `tests/scripts/test_validate_extraction.py`
  - legacy fallback을 인정하는 회귀 테스트 제거

## Phase 3 — `/nf-init` reset 정책을 contract-only로 축소하고 seed auto-gen 의도를 복구

- 사용자 의도 확인:
  - `nf-init` 의 핵심 목적은 **seed auto-gen** 이다.
  - 사람이 legacy `_handoff.yaml` 을 손으로 쓰던 부분을 없애는 것이 목적이다.
- `.claude/skills/nf-init/SKILL.md`
  - `--reset` 은 contract 산출물만 archive 하도록 재정의
  - `_manifest.yaml`, `_handoff_seed.yaml`, architecture, dev planning 은 보존
  - ready 상태에서는 `nf-seed-gen.py` 로 `_handoff_seed.yaml` 자동 생성/갱신
- `.claude/skills/nf-spec-discover/SKILL.md`
  - canonical wrapper 설명을 seed auto-gen까지 포함하도록 보정

## Phase 4 — 문서와 계획 파일 일관화

- `README.md`
  - spec discovery / reset 행위 설명을 seed auto-gen 기준으로 갱신
- `CLAUDE.md`
  - `/nf-init` 의 목적을 seed auto-gen + legacy handoff 제거로 명시
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
  - reset은 contract-only archive, seed auto-gen 유지로 수정
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`
  - Phase 7 체크포인트에 legacy handoff 제거와 reset scope 축소를 반영

## Phase 5 — 검증

실행한 검증:

- `.venv/bin/python3 design/scripts/nf-seed-gen.py nssf`
  - 기존 scoped seed 보존 확인
- `.venv/bin/python3 design/scripts/build-handoff.py nssf`
  - `handoff/nssf/contract.yaml` 갱신 확인
- `.venv/bin/python3 design/scripts/validate-extraction.py nssf --level basic`
  - `basic 13/13 PASS`
- `.venv/bin/python3 design/scripts/nf-status.py nssf --no-write`
  - `handoff_ready PASS`
  - canonical gate는 기존 상태 그대로 `implementation_guidance_quality` 미등록으로만 막힘
- `pytest tests/scripts`
  - `40 passed`
- `git diff --check`
  - pass

## Phase 6 — 커밋 및 원격 main 직접 반영

- 커밋:
  - `f7a203c legacy handoff 없이 seed 기반 init을 지킨다`
- 사용자 요청대로 PR 없이 원격 `main` 에 직접 반영했다.
- 결과:
  - `origin/main` 이 해당 커밋으로 fast-forward 됨

## Phase 7 — 로컬 정리

- 작업 브랜치 `push/reset-contract-only-legacy-removal-20260514` 삭제
- 로컬 `main` 을 `origin/main` 과 동기화
- 현재 워크트리 clean

## 변경된 핵심 파일

- `.claude/skills/nf-init/SKILL.md`
- `.claude/skills/nf-spec-discover/SKILL.md`
- `design/scripts/nf-seed-gen.py`
- `design/scripts/nf-status.py`
- `design/scripts/validate-extraction.py`
- `tests/scripts/test_validate_extraction.py`
- `README.md`
- `CLAUDE.md`
- `docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`
- `docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`

## 최종 상태

- legacy `_handoff.yaml` fallback 제거 완료
- `/nf-init` 핵심 목적을 seed auto-gen으로 재정렬 완료
- `--reset` 은 contract 산출물만 archive 하도록 축소 완료
- 검증 완료
- 원격 `main` 반영 완료
- 로컬 정리 완료
