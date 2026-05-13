---
name: git-pr-review
description: Git/GitHub pull request inspection and evidence-backed code review workflow for this repository. Use when the user asks to check, inspect, summarize, review, or audit a PR/branch before merge; examples include "PR 확인", "PR 리뷰", "이 PR 봐줘", "review current PR", "check GitHub PR", or requests to produce review comments. The workflow collects PR metadata, checks, commits, files, and diffs via git/gh, verifies local state, reviews for correctness/security/tests/docs, and reports prioritized findings without posting comments unless explicitly requested.
---

# git-pr-review — PR 확인 후 리뷰 워크플로우

## 목적

PR 또는 현재 브랜치를 merge 전에 확인하고, 근거 있는 코드 리뷰를 생성한다. 기본 동작은 **읽기 전용 리뷰**다. GitHub 에 comment/review 를 게시하거나 push/merge 하는 행동은 사용자가 명시적으로 요청했을 때만 한다.

## 입력 해석

- 인자가 없으면 현재 브랜치의 PR 을 대상으로 본다 (`gh pr view` 우선, 실패 시 로컬 diff fallback).
- `<pr-number|pr-url|branch>` 가 있으면 해당 PR/브랜치를 대상으로 본다.
- 대상 PR 이 모호하거나 현재 브랜치 PR 을 특정할 수 없고 여러 open PR 이 있으면 리뷰를 시작하지 말고 후보 목록을 출력한 뒤 번호/URL/브랜치 재지정을 요청한다.
- 후보 목록은 `bash .codex/skills/git-pr-review/scripts/list-pr-candidates.sh` 로 출력한다.
- `--base <branch>` 는 gh PR base 를 찾지 못할 때의 비교 기준이다.
- `--post` 또는 "댓글 달아" 같은 명시 요청이 있을 때만 GitHub review/comment 게시를 고려한다. 그 전에는 게시하지 않는다.

## Workflow

### 1. 사전 안전 확인

- `git status --short` 로 로컬 미커밋 변경을 확인한다.
- 사용자 변경을 덮어쓰는 checkout/reset/rebase/merge 는 하지 않는다.
- 필요하면 `git fetch` 는 허용하되, 네트워크/인증 실패 시 로컬 정보로 fallback 한다.

#### Multi-PR target policy

- `gh pr view [target]` 로 정확한 PR 하나가 확인되면 계속 진행한다.
- target 이 없고 현재 브랜치 PR 이 없거나, 여러 후보 때문에 대상이 모호하면 `blocked: ambiguous target` 로 멈춘다.
- 이때 `bash .codex/skills/git-pr-review/scripts/list-pr-candidates.sh` 를 실행해 `number / title / base...head / state / draft / checks / url` 을 보여주고, 사용자가 PR 번호·URL·브랜치 중 하나를 지정하도록 안내한다.
- 리뷰는 읽기 전용이어도 잘못된 PR 을 리뷰하면 결과가 오염되므로 자동 선택하지 않는다.

### 2. PR 컨텍스트 수집

먼저 bundled script 를 실행해 리뷰 입력을 만든다.

```bash
bash .codex/skills/git-pr-review/scripts/collect-pr-context.sh [target] [--base <branch>]
```

수집 결과에서 반드시 확인한다.

- PR 번호/URL/title/state/draft 여부/base/head
- review decision, mergeability, labels/reviewers/checks
- commit list, changed files, diffstat 또는 changed names
- base...head diff 또는 PR diff
- 로컬 working tree 상태

`gh` 가 없거나 인증 실패하면 script 의 local git fallback 을 사용한다.

### 3. Diff 읽기 순서

1. PR 설명/목표와 changed files 목록으로 의도 파악
2. 큰 파일·생성 파일·삭제 파일·테스트 파일을 먼저 분류
3. 핵심 소스 diff 를 파일 단위로 읽고, 필요한 주변 문맥을 `sed -n` 으로 확인
4. 테스트/문서/스펙 변경이 코드 변경과 맞는지 확인
5. CI/check 실패가 있으면 실패 check 를 review 최상단에 반영

### 4. 리뷰 기준

Finding 은 반드시 재현 가능한 근거를 포함한다. 근거가 없으면 질문/리스크로 분리한다.

우선순위:

1. Correctness — 기능 회귀, edge case, 잘못된 조건/상태/데이터 흐름
2. Safety/Security — credential, destructive action, injection, 권한/검증 누락
3. Contract — public API, schema, handoff/spec/status enum, backward compatibility
4. Tests — 변경 행위에 대한 테스트 누락/불일치, flaky risk
5. Maintainability — 중복, 불필요한 abstraction, repo convention 위반
6. Docs/UX — 사용자-facing 문서, migration note, command help 누락

### 5. 필요 시 검증 실행

리뷰 claim 을 뒷받침할 최소 검증만 실행한다.

- Python/script 변경: 관련 unit test, `python -m py_compile`, targeted CLI smoke
- 문서/skill 변경: frontmatter/markdown 구조, referenced path 존재, 예시 command syntax
- CI 실패가 있으면 같은 명령을 로컬에서 재현 가능한지 확인

검증을 실행하지 못하면 이유와 next-best check 를 명시한다.

### 6. 출력 형식

기본 출력은 한국어로 간결하게 작성한다.

```text
PR Review: <title or branch>
Scope: <files/intent summary>
Checks: <pass/fail/unknown + evidence>
Base/Head: <base>...<head>

Findings:
1. [High] <문제 요약>
   Evidence: <file:line or diff hunk / command output>
   Impact: <왜 merge 전에 고쳐야 하는지>
   Suggested fix: <구체적 수정 방향>

Questions / Assumptions:
- <근거 부족 또는 의사결정 필요한 항목>

Verification run:
- <command> → <result>

Verdict: approve | request changes | comment only | blocked
```

Finding 이 없으면 "No blocking findings" 라고 쓰되, 확인한 범위와 실행한 검증을 반드시 적는다.

## Posting policy

- 기본: GitHub 에 아무것도 게시하지 않는다.
- 사용자가 `--post`, "리뷰 등록", "코멘트 달아"를 명시하면 게시 전 최종 review body 를 보여주고, destructive 하지 않은 `gh pr review --comment|--request-changes|--approve` 중 맞는 명령을 사용한다.
- 승인(`--approve`)은 실제 blocking finding 이 없고 검증 evidence 가 충분할 때만 한다.
