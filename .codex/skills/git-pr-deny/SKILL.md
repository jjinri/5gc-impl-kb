---
name: git-pr-deny
description: Request changes or reject a GitHub pull request with evidence-backed review findings. Use when the user asks to deny, reject, block, request changes on, or not approve the current PR/branch; examples include "git pr deny", "PR 거절", "request changes", "reject this PR", "block merge". The workflow collects PR metadata/checks/diff, writes actionable findings, posts `gh pr review --request-changes` when justified, and never closes a PR unless close/decline is explicitly requested.
---

# git-pr-deny — evidence-backed PR rejection workflow

## Purpose

Block a PR from approval by posting a GitHub `REQUEST_CHANGES` review with concrete, reproducible findings. A deny request authorizes posting a request-changes review, but closing the PR is separate and requires explicit close/decline wording.

## Input interpretation

- No argument: target the current branch's PR only when `gh pr view` resolves exactly one PR.
- `<pr-number|pr-url|branch>`: target that PR/branch.
- If target resolution is ambiguous or no current-branch PR is found while multiple open PRs exist, stop before posting, run `bash .codex/skills/git-pr-review/scripts/list-pr-candidates.sh`, and ask the user to rerun with a PR number, URL, or branch.
- `--comment-only`: post a non-blocking comment instead of request-changes.
- `--close`: allowed only when the user explicitly asked to close/decline the PR.
- `--body <text>` may add user rationale, but evidence-backed findings remain required unless the user supplied a clear policy/business reason.

## Workflow

### 1. Safety/context collection

- Run `git status --short --branch`; do not checkout/reset/rebase/merge or overwrite local changes.
- Collect PR context with the existing review collector when available:

```bash
bash .codex/skills/git-pr-review/scripts/collect-pr-context.sh [target]
```

- Confirm: target PR, current head SHA, title/state/base/head, checks, changed files, and relevant diff.
- If `gh` is unavailable or unauthenticated, produce the review body but report `blocked` for posting.

#### Multi-PR target policy

- Deny/request-changes has external side effects, so never auto-pick from multiple candidate PRs.
- If the target is not exactly one PR, output `blocked: ambiguous target`, list candidates with `bash .codex/skills/git-pr-review/scripts/list-pr-candidates.sh`, and stop.
- Continue only after the user specifies a PR number, URL, or branch.

### 2. Denial gate

Post `--request-changes` only when there is at least one concrete blocker:

- Correctness regression, security/safety issue, broken public contract/schema/API, failing required check, missing test for changed behavior that creates merge risk, or explicit user/policy reason.
- Finding must include file/line or command/check evidence and an actionable fix.

If no blocker is found, do **not** fabricate a denial. Use `comment only` or report `blocked: no evidence-backed blocking finding`.

### 3. Prepare request-changes body

Use concise actionable findings.

```text
Request changes: <PR title or number>

Blocking findings:
1. [High|Medium] <summary>
   Evidence: <file:line / diff hunk / command output>
   Impact: <why this blocks merge>
   Suggested fix: <specific fix>

Verification:
- <command/check> → <result>

Not blocking / notes:
- <optional>
```

### 4. Post denial

Use a temp file for the body.

```bash
gh pr review [target] --request-changes --body-file <tmp-body>
```

If the issue is not merge-blocking or `--comment-only` was requested:

```bash
gh pr review [target] --comment --body-file <tmp-body>
```

After posting, verify with `gh pr view --json reviewDecision,statusCheckRollup,mergeable,url`.

### 5. Optional close gate

Only if the user explicitly requested close/decline:

- Re-check target PR and ensure the close rationale is in the body.
- Closing is stronger than request-changes; do not close for ordinary fixable review issues unless explicitly requested.

```bash
gh pr close [target] --comment-file <tmp-body>
```

## Output format

```text
PR Deny: <title or #>
Action: request-changes | comment-only | closed | blocked
Evidence:
- <metadata/checks/findings>
Posted: <review URL or gh output summary>
Close: <not requested | skipped + reason | completed>
Risks / Not verified:
- <gaps, if any>
```
