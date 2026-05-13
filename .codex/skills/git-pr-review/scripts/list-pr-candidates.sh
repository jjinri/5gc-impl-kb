#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: list-pr-candidates.sh [--limit N] [--head <branch>]

List open GitHub PR candidates in a compact table so the user can rerun
review/accept/deny with an explicit PR number, URL, or branch.
USAGE
}

limit=30
head_branch=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --limit)
      limit="${2:-30}"; shift 2 ;;
    --head)
      head_branch="${2:-}"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "Unexpected arg: $1" >&2; usage; exit 2 ;;
  esac
done

if ! command -v gh >/dev/null 2>&1; then
  echo "gh not found; cannot list GitHub PR candidates." >&2
  exit 1
fi

if [[ -z "$head_branch" ]]; then
  head_branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || true)"
fi

json_fields='number,title,state,isDraft,baseRefName,headRefName,mergeable,statusCheckRollup,url'

print_rows() {
  python3 - <<'PY'
import json, sys
prs = json.load(sys.stdin)
if not prs:
    sys.exit(0)
print("number\ttitle\tbase...head\tstate\tdraft\tchecks\turl")
for pr in prs:
    checks = pr.get("statusCheckRollup") or []
    if not checks:
        checks_summary = "none"
    else:
        states = {}
        for c in checks:
            state = c.get("conclusion") or c.get("status") or c.get("state") or "unknown"
            states[state] = states.get(state, 0) + 1
        checks_summary = ",".join(f"{k}:{v}" for k, v in sorted(states.items()))
    print("\t".join([
        str(pr.get("number", "")),
        (pr.get("title") or "").replace("\t", " "),
        f"{pr.get('baseRefName','')}...{pr.get('headRefName','')}",
        pr.get("state") or "",
        str(pr.get("isDraft", "")),
        checks_summary,
        pr.get("url") or "",
    ]))
PY
}

if [[ -n "$head_branch" && "$head_branch" != "HEAD" ]]; then
  echo "## Current-branch candidates: $head_branch"
  gh pr list --state open --head "$head_branch" --limit "$limit" --json "$json_fields" | print_rows || true
  echo
fi

echo "## Open PR candidates"
gh pr list --state open --limit "$limit" --json "$json_fields" | print_rows
