---
name: git-pr-accept
description: Approve, merge, and verify closure of a GitHub pull request after evidence-backed review and verification. Use when the user asks to accept, approve, LGTM, green-light, merge, or land the current PR/branch; examples include "git pr accept", "PR 승인", "approve current PR", "accept this PR". The workflow collects PR metadata/checks/diff, confirms no blocking findings, posts `gh pr review --approve`, merges the PR, and verifies the PR is closed/merged.
---

# git-pr-accept — evidence-backed PR approval workflow

## Purpose

Approve and merge a PR through GitHub only when the current PR state is reviewable and there are no blocking findings. An explicit accept/approve request authorizes the full accept action: post approval, merge, then verify the PR is closed/merged. Do not close an unmerged PR as a substitute for a failed merge.

## Input interpretation

- No argument: target the current branch's PR only when `gh pr view` resolves exactly one PR.
- `<pr-number|pr-url|branch>`: target that PR/branch.
- If target resolution is ambiguous or no current-branch PR is found while multiple open PRs exist, stop before approval/merge, run `bash .codex/skills/git-pr-review/scripts/list-pr-candidates.sh`, and ask the user to rerun with a PR number, URL, or branch.
- `--merge-method <merge|squash|rebase>` or user wording may choose the merge method; default to the repository-safe `gh pr merge --merge` path when no method is specified.
- `--body <text>` may add approval/merge context, but do not let it replace verification evidence.

## Workflow

### 1. Safety/context collection

- Run `git status --short --branch`; do not checkout/reset/rebase/merge or overwrite local changes.
- Collect PR context with the existing review collector when available:

```bash
bash .codex/skills/git-pr-review/scripts/collect-pr-context.sh [target]
```

- Confirm: PR is open, not draft, target is correct, base/head are known, mergeability is not blocked, and checks are not failing.
- If `gh` is unavailable or unauthenticated, stop with `blocked`; approval cannot be posted locally.

#### Multi-PR target policy

- Accept has external side effects (`approve` + `merge` + merged/closed verification), so never auto-pick from multiple candidate PRs.
- If the target is not exactly one PR, output `blocked: ambiguous target`, list candidates with `bash .codex/skills/git-pr-review/scripts/list-pr-candidates.sh`, and stop.
- Continue only after the user specifies a PR number, URL, or branch.

### 2. Review gate

Approve only after one of these is true:

1. A fresh same-turn `git-pr-review` found no blocking findings and verification evidence is still current for the same head SHA.
2. You perform a concise review now: inspect changed files/diff, run the smallest meaningful validation, and find no blocking correctness/security/contract/test issues.

Do **not** approve when:

- PR is draft/closed, checks are failing, mergeability is blocked, or target PR is ambiguous.
- There are High/Medium blocking findings, unresolved security concerns, or required validation could not run and no next-best check is sufficient.
- The user asks to merge but did not explicitly ask to approve; use the narrower requested action.

### 3. Prepare approval body

Body must include evidence, not just "LGTM".

```text
Approved: <PR title or number>

Scope checked:
- <files/components>

Verification:
- <command/check> → <result>

Notes:
- No blocking findings found.
```

### 4. Post approval

Use a temp file for the body.

```bash
gh pr review [target] --approve --body-file <tmp-body>
```

After posting, verify with `gh pr view --json reviewDecision,statusCheckRollup,mergeable,state,url`.

### 5. Merge + close verification gate

Accept means merge after approval, then verify the PR is no longer open.

- Re-check PR state after approval.
- Do not merge with failing checks, draft state, unknown target, merge conflicts, or unresolved blocking discussion unless user explicitly accepts that risk.
- Prefer the user's named merge method; otherwise use merge commit as the conservative default.

```bash
gh pr merge [target] --merge   # or --squash / --rebase when explicitly selected
```

GitHub closes merged PRs automatically. Do not run `gh pr close` for an unmerged PR. After merge, verify closure/merged state:

```bash
gh pr view [target] --json state,mergedAt,mergeCommit,url
```

Success requires `state` to be `MERGED` or otherwise clearly closed by the successful merge result. If merge fails, report `blocked` and leave the PR open.

## Output format

```text
PR Accept: <title or #>
Action: approved+merged+closed | blocked
Evidence:
- <metadata/checks/verification>
Posted: <review URL or gh output summary>
Merge: <completed | blocked + reason>
Close verification: <state/mergedAt/mergeCommit evidence>
Risks / Not verified:
- <gaps, if any>
```
