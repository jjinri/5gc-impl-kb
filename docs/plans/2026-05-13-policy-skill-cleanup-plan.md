# 정책·스킬 구성 정리 계획 — 2026-05-13

## 목적

`CLAUDE.md` 와 skill 구성을 프로젝트 성격에 맞게 축소·정렬한다. 목표는 agent 가 매 turn 읽는 정책 표면을 줄이고, KB 산출물과 개인/운영 도구를 분리하며, stale 정책이 작업 판단을 오염시키지 않게 하는 것이다.

## 범위

정리 대상.

- `CLAUDE.md`
- `README.md` 의 정책/skill 참조 문구
- `.claude/skills/*` 중 프로젝트 NF workflow 와 직접 관련된 skill
- 미추적 `git-pr-*` skill 파일의 보관 위치와 추적 여부
- `.gitignore` 의 로컬 agent/tooling 산출물 규칙

명시적 제외.

- `.omx/` 전체. OMX runtime/state/log/team 작업 디렉터리로 필요하므로 삭제·이동·ignore 정책 변경 대상에서 제외한다.
- `specs/` 원본 문서. KB 의 primary source 이므로 정리 대상이 아니다.
- `design/nssf/`, `handoff/nssf/`, `tests/`, `design/scripts/` 의 기능 동작. 본 계획은 문서·정책·skill 구성 정리만 다룬다.

## 현재 관찰

- `main` 과 `origin/main` 은 동기화되어 있고 tracked diff 는 없다.
- `CLAUDE.md` 는 253 lines 로 정책 문서치고 크다.
- `CLAUDE.md` 안에 README, 설계 문서, script docstring, skill 로 위임 가능한 내용이 섞여 있다.
- `CLAUDE.md` 에 stale 표현이 남아 있다.
  - `build-handoff.py` 설명이 handoff-v1 중심 표현을 포함한다.
  - 제목은 `3 SKILL` 이지만 표에는 선택 skill `/nf-reset` 이 포함된다.
- docs spec 에는 `/nf-reset` 폐기/`nf-init --reset` 통합 방향이 기록되어 있으나, 현재 README/CLAUDE/.claude skill 은 `/nf-reset` 을 활성 workflow 로 유지한다.
- `git-pr-review`, `git-pr-accept`, `git-pr-deny` 는 KB 도메인 산출물이 아니라 운영 보조 도구에 가깝고 현재 untracked 로 남아 있다.

## 정리 원칙

1. `CLAUDE.md` 는 repo-local 정책만 담는다.
   - 프로젝트 소개·setup·browsing 은 `README.md`.
   - 실행 절차는 `.claude/skills/*/SKILL.md`.
   - schema/gate 세부는 `design/scripts/*` docstring 또는 `docs/superpowers/specs/*`.
2. 새 문서를 만들기보다 기존 source of truth 로 위임한다.
3. KB 도메인 workflow 와 운영 보조 workflow 를 분리한다.
4. 삭제가 애매한 것은 먼저 추적 제외/로컬 보관으로 처리하고, 기능 삭제는 별도 PR 에서 다룬다.
5. `.omx/` 는 runtime 필수 영역으로 정리 논의에서 제외한다.

## 단계별 계획

### Phase 1 — `CLAUDE.md` 축소

목표: 253 lines → 80~120 lines 수준.

유지할 내용.

- 언어 정책.
- source-of-truth / no-invent 규칙.
- NF workflow 의 큰 책임 경계.
- PR 생성 정책의 판단 기준.
- 5gc-design ↔ 5gc-dev 책임 경계.
- generated/tracked artifact 정책.

삭제 또는 위임할 내용.

- 상세 repository tree → `README.md` 로 위임.
- 파일명 규칙 상세 → `README.md` 또는 별도 docs 로 위임.
- Categories 상세 tree → `README.md`/`index.md` 로 위임.
- NF profile 표 → `design/scripts/nf-status.py` 로 위임.
- Acceptance gate 상세 표 → `design/scripts/nf-status.py` 와 handoff-v2 spec 으로 위임.
- Source document 세부 규칙 → README 또는 짧은 bullet 만 유지.

Acceptance criteria.

- `CLAUDE.md` 가 정책 문서 역할만 한다.
- stale `handoff-v1` 설명이 없다.
- 3 skill/4 skill 표현이 모순되지 않는다.
- `README.md` 와 역할 분담이 겹치지 않는다.

### Phase 2 — `/nf-reset` 정책 결정 — **(2) 폐기로 진행 (2026-05-13)**

현재 후보.

1. 유지.
   - README/CLAUDE/SKILL 에서 `/nf-reset` 을 공식 4번째 workflow 로 정합화.
   - docs 의 “폐기 예정”은 과거 설계 기록으로 남기되 현재 정책 아님을 명확히 한다.
2. **폐기 — 본 PR (2026-05-13) 에서 진행.**
   - `.claude/skills/nf-reset` 삭제.
   - `/nf-init --reset` 구현·문서화 (`--reset-keep-prose` 는 본 PR 범위 밖, NSSF prose 분류 로직 신설 필요해 별도 사이클).
   - README/CLAUDE 참조 갱신 — "4 SKILL" → "3 SKILL", `/nf-reset` 줄 → `--reset` 옵션 안내.
   - `.gitignore` 의 `design/*/_archive/` 는 이미 보유, tracked 잔재 (`design/nssf/_archive/20260513-112914/`) 만 untrack.

이전 권장 (단기 유지) 은 *외부 분석 + handoff-v2 topic layout 미반영 확인* 으로 (2) 폐기로 전환. `docs/superpowers/specs/2026-05-12-nf-build-restructure-design.md` §8/§10 의 M.3/M.7/M.8 부분을 본 PR 으로 실행.

Acceptance criteria.

- README, CLAUDE, `.claude/skills/nf-init` 설명이 서로 충돌하지 않는다.
- `grep -rn "nf-reset" .claude/skills/ CLAUDE.md README.md` 결과가 비어있다 (또는 *과거 폐기 기록* 컨텍스트만 남는다).
- nf-init SKILL 안에 `--reset` archive workflow 가 명시되어 있다.
- `--reset-keep-prose` 는 별도 사이클로 분리 — 본 PR 에 포함하지 않는다.

### Phase 3 — `git-pr-*` skill 추적 정책

판단.

- `git-pr-*` skill 은 프로젝트 KB 산출물이 아니라 GitHub 운영 보조 도구다.
- 단독 작업자 로컬 workflow 에 유용하지만, repo 의 domain source of truth 는 아니다.

권장 처리.

- repo 에 commit 하지 않는다.
- 필요하면 `~/.codex/skills` 또는 개인 plugin 으로 이동한다.
- repo-local에 남길 경우 `.gitignore` 에 명시적으로 추가한다.

후보 ignore 규칙.

```gitignore
# Local agent/operator skills (not KB deliverables)
.codex/skills/git-pr-*/
.claude/skills/git-pr-*/
.claude/settings.local.json
```

Acceptance criteria.

- `git status --short --untracked-files=all` 에 git-pr 로컬 스킬이 노출되지 않는다.
- NF domain skills (`nf-init`, `nf-build`, `nf-status`, `nf-reset`) 은 계속 tracked 상태로 남는다.

### Phase 4 — README 역할 조정

목표: README 는 소개·setup·browsing·큰 작업 흐름만 유지한다.

수정 후보.

- `CLAUDE.md` 설명을 “정책의 진실 출처”에서 “repo-local agent 정책” 정도로 축소.
- 작업 사이클이 3 또는 4 skill 인지 Phase 2 결정에 맞춰 정리.
- 상세 gate/category/file naming 은 CLAUDE.md 가 아니라 README 또는 scripts 로 위임되도록 링크 문구 정리.

Acceptance criteria.

- 신규 독자는 README 만 보고 프로젝트 성격과 시작 방법을 이해한다.
- agent 는 CLAUDE.md 만 보고 작업 금지/허용 정책을 이해한다.

## 검증 계획

문서 정리 후 실행.

```bash
git diff --check
grep -n "handoff-v1\|3 SKILL\|4 SKILL\|nf-reset" CLAUDE.md README.md .claude/skills/nf-build/SKILL.md .claude/skills/nf-reset/SKILL.md
python - <<'PY'
from pathlib import Path
print('CLAUDE.md lines:', len(Path('CLAUDE.md').read_text(encoding='utf-8').splitlines()))
PY
git status --short --untracked-files=all
```

기능 테스트는 필요 시에만 실행한다. 본 계획의 기본 변경은 문서/ignore 정책이며, `design/scripts/*` 동작을 바꾸지 않는다.

## 예상 산출

- 축소된 `CLAUDE.md`.
- 필요 시 조정된 `README.md`.
- 필요 시 `.gitignore` 에 로컬 운영 스킬 제외 규칙 추가.
- `git-pr-*` skill 은 repo 추적 대상에서 제외 또는 개인 skill 위치로 이동.

## 보류 항목

- `.omx/` 정리. runtime 필수 디렉터리이므로 본 계획에서 제외한다.
- ~~`/nf-reset` 폐기 구현~~ — 2026-05-13 본 PR (`push/nf-init-reset-integrate-20260513`) 으로 처리. Phase 2 의 (2) 폐기 옵션을 채택.
- NF build/status script 동작 변경. 본 계획의 범위를 벗어난다.
- `/nf-init --reset-keep-prose` (NSSF prose 분류 + in-place 보존 로직). NSSF dev 사이클 진입 시점에 별도 spec 으로 분리.
