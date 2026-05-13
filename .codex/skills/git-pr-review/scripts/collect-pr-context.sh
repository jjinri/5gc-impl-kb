#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: collect-pr-context.sh [<pr-number|pr-url|branch>] [--base <branch>] [--no-fetch]

Collect read-only PR/branch context for review. Uses gh when available and falls back to git diff.
USAGE
}

target=""
base=""
fetch=1
while [[ $# -gt 0 ]]; do
  case "$1" in
    --base)
      base="${2:-}"; shift 2 ;;
    --no-fetch)
      fetch=0; shift ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      if [[ -z "$target" ]]; then target="$1"; else echo "Unexpected arg: $1" >&2; usage; exit 2; fi
      shift ;;
  esac
done

section() { printf '\n## %s\n\n' "$1"; }
run() {
  printf '$ %s\n' "$*"
  "$@" 2>&1 || true
  printf '\n'
}

current_branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || true)"
remote="$(git remote 2>/dev/null | sed -n '1p')"

section "Repository"
run git rev-parse --show-toplevel
printf 'current_branch: %s\n' "${current_branch:-unknown}"
printf 'remote: %s\n' "${remote:-none}"

section "Working tree"
run git status --short

if command -v gh >/dev/null 2>&1; then
  section "GitHub PR metadata"
  gh_target=()
  if [[ -n "$target" ]]; then gh_target+=("$target"); fi
  run gh pr view "${gh_target[@]}" --json number,url,title,state,isDraft,author,baseRefName,headRefName,reviewDecision,mergeable,additions,deletions,changedFiles,labels,reviewRequests,assignees,statusCheckRollup

  if [[ -z "$base" ]]; then
    base="$(gh pr view "${gh_target[@]}" --json baseRefName --template '{{.baseRefName}}' 2>/dev/null || true)"
  fi

  section "GitHub PR commits"
  run gh pr view "${gh_target[@]}" --json commits --template '{{range .commits}}{{.oid}} {{.messageHeadline}}{{"\n"}}{{end}}'

  section "GitHub PR files"
  run gh pr view "${gh_target[@]}" --json files --template '{{range .files}}{{.path}} +{{.additions}} -{{.deletions}}{{"\n"}}{{end}}'

  section "GitHub PR diff names"
  run gh pr diff "${gh_target[@]}" --name-only
else
  section "GitHub PR metadata"
  printf 'gh not found; using local git fallback.\n'
fi

if [[ -z "$base" ]]; then
  if git show-ref --verify --quiet refs/remotes/origin/main; then base="origin/main";
  elif git show-ref --verify --quiet refs/remotes/origin/master; then base="origin/master";
  else base="main"; fi
elif [[ "$base" != origin/* ]] && git show-ref --verify --quiet "refs/remotes/origin/$base"; then
  base="origin/$base"
fi

if [[ $fetch -eq 1 && -n "$remote" && "$base" == origin/* ]]; then
  section "Fetch base"
  run git fetch --quiet "$remote" "${base#origin/}"
fi

merge_base="$(git merge-base "$base" HEAD 2>/dev/null || true)"

section "Local comparison"
printf 'base: %s\n' "$base"
printf 'head: %s\n' "${current_branch:-HEAD}"
printf 'merge_base: %s\n' "${merge_base:-unknown}"
run git log --oneline --decorate --no-merges "${base}..HEAD"
run git diff --stat "${base}...HEAD"

section "Changed files"
run git diff --name-status "${base}...HEAD"

section "Review diff"
run git diff --find-renames --find-copies --ignore-submodules "${base}...HEAD"
