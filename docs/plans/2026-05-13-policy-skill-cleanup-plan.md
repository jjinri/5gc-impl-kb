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

- `specs/` 원본 문서. KB 의 primary source 이므로 정리 대상이 아니다.
- `design/nssf/`, `handoff/nssf/`, `tests/`, `design/scripts/` 의 기능 동작. 본 계획은 문서·정책·skill 구성 정리만 다룬다.

## 현재 관찰 (2026-05-13 시작 시점)

- `CLAUDE.md` 는 253 lines 로 정책 문서치고 크다.
- `CLAUDE.md` 안에 README, 설계 문서, script docstring, skill 로 위임 가능한 내용이 섞여 있다.
- `CLAUDE.md` 에 stale 표현이 남아 있다 (`handoff-v1`, 폐기된 nf-reset 등).

> **본 PR (push/claude-md-shrink-20260513) 적용 후** — CLAUDE.md 113 lines, README 52 lines, stale grep empty.

## 정리 원칙

1. `CLAUDE.md` 는 repo-local 정책만 담는다.
   - 프로젝트 소개·setup·browsing 은 `README.md`.
   - 실행 절차는 `.claude/skills/*/SKILL.md`.
   - schema/gate 세부는 `design/scripts/*` docstring 또는 `docs/superpowers/specs/*`.
2. 새 문서를 만들기보다 기존 source of truth 로 위임한다.
3. KB 도메인 workflow 와 운영 보조 workflow 를 분리한다.
4. 삭제가 애매한 것은 먼저 추적 제외/로컬 보관으로 처리하고, 기능 삭제는 별도 PR 에서 다룬다.
5. `.omx/` 는 runtime 필수 영역으로 정리 논의에서 제외한다.
6. Obsidian 관련 문구 내용은 제거한다.
7. 매핑 — 2-tier (Karpathy 1:1 에서의 의도적 분기) Karpathy 규칙은 메모리에서도 삭제하고, 폐기 시켜야 한다.

## 단계별 계획

### Phase 1 — `CLAUDE.md` 축소 — **완료 (2026-05-13, PR push/claude-md-shrink-20260513)**

목표: 253 lines → 80~120 lines 수준. **결과: 113 lines.**

유지된 내용.

- 언어 정책.
- source-of-truth / no-invent 규칙 (THE FOUR RULES).
- NF workflow 의 큰 책임 경계 (3 SKILL 표).
- PR 생성 정책의 판단 기준.
- 5gc-design ↔ 5gc-dev 책임 경계.
- Knowledge depth 원칙.

완전 위임된 내용 (CLAUDE.md 에 링크 줄도 안 남김).

- ~~Repository Structure~~ → 실제 디렉터리.
- ~~File Naming Convention~~ → `design/scripts/*` docstring.
- ~~Categories 상세 tree~~ → 실제 디렉터리.
- ~~NF Profile 표~~ → `design/scripts/nf-status.py` 의 `applies_to`.
- ~~Acceptance Gates 상세 표~~ → `design/scripts/nf-status.py` + `docs/superpowers/specs/2026-05-12-*`.
- ~~Source Document Management Rules~~ → `design/scripts/extract.py` docstring + 실제 운영.
- ~~매핑 — 2-tier (Karpathy 1:1)~~ → 폐기 (원칙 7).

Acceptance criteria — *통과*.

- `CLAUDE.md` 가 정책 문서 역할만 한다.
- stale `handoff-v1` 설명이 없다 (`grep handoff-v1 CLAUDE.md` empty).
- `README.md` 와 역할 분담이 겹치지 않는다.

### Phase 2 — README 역할 조정 — **완료 (2026-05-13, 같은 PR)**

목표: README 는 소개·setup·큰 작업 흐름만 유지. Obsidian 섹션은 원칙 6 에 따라 *제거*.

적용된 수정.

- `CLAUDE.md` 설명을 "정책의 진실 출처" → "repo-local agent 정책" 으로 축소.
- 작업 사이클 3 SKILL 표제·항목은 이미 PR #5 으로 정합화.
- 디렉터리·파일 명명·NF profile·acceptance gate 세부는 각 진실 출처 위임 라인 추가.
- ~~Browsing with Obsidian~~ 섹션 제거 (원칙 6).

Acceptance criteria — *통과*.

- 신규 독자는 README 만 보고 프로젝트 성격과 시작 방법을 이해한다.
- agent 는 CLAUDE.md 만 보고 작업 금지/허용 정책을 이해한다.

## 검증 계획

문서 정리 후 실행.

```bash
git diff --check
grep -rn "handoff-v1\|nf-reset\|Obsidian\|Karpathy\|2-tier" CLAUDE.md README.md .claude/skills/
wc -l CLAUDE.md README.md
git status --short --untracked-files=all
```

기능 테스트는 필요 시에만 실행한다. 본 계획의 기본 변경은 문서/ignore 정책이며, `design/scripts/*` 동작을 바꾸지 않는다.

## 예상 산출

- 축소된 `CLAUDE.md` (113 lines).
- 조정된 `README.md` (52 lines, Obsidian 섹션 제거).
- `.gitignore` 갱신은 본 PR 범위 밖 (이미 PR #5 으로 `design/*/_archive/` 처리).
