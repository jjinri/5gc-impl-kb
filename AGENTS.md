# AGENTS.md — General Agent Operating Policy

This file defines general operating policy for AI agents working in this repository. Follow higher-priority system/developer/user instructions first, then this file, then narrower tool or skill instructions.

## Core principles

- Work in small, reversible, verifiable steps.
- Prefer evidence over assumption. Inspect files, run targeted checks, and report concrete results.
- Do not start broad rewrites when a narrow change satisfies the request.
- Keep user-facing updates concise: target result, evidence, blocker or next step.
- Ask only when the next action is destructive, irreversible, credential-gated, externally risky, or materially ambiguous.

## Long-running work and usage-limit resilience

Agents must assume that a session can stop unexpectedly because of usage limits, runtime interruption, network failure, or context loss. Structure work so another agent can resume safely.

### Phase discipline

- Split large work into independently useful phases.
- Each phase must have a clear objective, changed files, validation command, and stop condition.
- Do not begin a new phase when usage/context budget is low.
- Avoid carrying large unverified diffs across phase boundaries.

### Checkpoint before risk points

Create a checkpoint before any of the following:

- broad refactor or directory move,
- destructive or archive/reset operation,
- multi-file generated change,
- dependency/tooling change,
- PR creation, merge, or release-like operation,
- expected long-running test/build,
- signs that usage/context budget is running low.

A checkpoint should record:

- current objective,
- completed work,
- changed files,
- commands already run and results,
- known failures or unverified gaps,
- next recommended command or phase.

### Durable handoff

When a task may outlive the current session, write durable handoff notes in repo-tracked documentation, usually under `docs/` or the active plan document.

Minimum handoff format:

```text
Status: <in_progress|blocked|ready_for_next_phase|complete>
Current objective: <one sentence>
Completed:
- <facts only>
Changed files:
- <path>
Validation:
- <command> → <result>
Open risks / gaps:
- <gap>
Next step:
- <specific command or edit>
```

Runtime state under `.omx/` may be used for session recovery, but `.omx/` is runtime-owned and must not be treated as cleanup scope unless explicitly requested.

### Commit discipline

- Commit only coherent, verified units of work.
- Do not commit merely to hide unfinished or unverified changes.
- Before committing, run at least `git status --short` and `git diff --check`; run targeted tests when behavior changed.
- If a phase cannot be verified, leave an explicit handoff note instead of claiming completion.

## Verification policy

- Define the claim before validating it.
- Run the smallest check that proves the claim.
- Prefer targeted tests for changed behavior, then broader lint/type/build checks when relevant.
- If validation cannot run, state why and provide the next-best evidence.
- Final reports must distinguish verified facts from assumptions or remaining risks.

## Repository hygiene

- Keep generated/runtime/local files out of commits unless they are intentional project artifacts.
- Do not delete or reorganize runtime directories such as `.omx/` without explicit instruction.
- Preserve user changes. Do not reset, checkout, rebase, or overwrite unrelated work.
- When multiple untracked or unrelated changes exist, report them separately from the task result.

## Git workflow policy

This repository follows the git working rules documented in `CLAUDE.md`.

- Use a PR when the change touches main-protected surface area: production/runtime code, public API/schema/contract, migrations, security/auth, deployment/CI/CD, or when review/record-keeping materially helps.
- Use a direct push for small non-functional or local-only changes when review value is low.
- A retro-only document update may use direct push if it is the only change; if code/skill/script changes are bundled, follow the more restrictive path for those changes.
- When in doubt, prefer the safer review path.
- For PR-based work, create a topic branch, push it, open the PR, merge, then sync local state and delete the branch.
- Keep PR descriptions short and evidence-based: summary, reason, validation.
- This repo uses merge commits on `main`; do not mix in squash/rebase unless a fast-forward failure clearly indicates that path.
- After a merge, fast-forward local `main` from `origin/main`; if FF fails, treat it as a squash/rebase signal and reset to `origin/main`.
- After merge, delete merged push branches locally and remotely instead of leaving them around.
