#!/usr/bin/env python3
# /nf-implement 의 next-slice picker. ADR-0005 D4 + plan §1 Q3 의 단일 진실 출처.
# pr-slicing-plan.yaml 의 topo sort + gh pr view 동적 조회로 다음 PR slice 산출.
"""
Usage:
    .venv/bin/python3 design/scripts/nf-implement-slice-status.py <nf> [options]

Options:
    --status            현재 상태 요약 출력 (next slice + open PR + counters).
    --json              JSON 출력 (기본 = YAML/human).
    --use-gh / --no-gh  gh 동적 조회 활성/비활성. 기본 = on. CI 또는 offline
                        검증 시 --no-gh (plan 의 status 필드만 사용).
    --plan PATH         pr-slicing-plan.yaml 경로 override. 기본 =
                        dev/<nf>/pr-slicing-plan.yaml.
    --state PATH        _implementation_run_state.yaml 경로 override. 기본 =
                        dev/<nf>/_implementation_run_state.yaml.

본 script 는 ADR-0005 D4 의 *deterministic* next-slice 산출자다.

알고리즘.
  1. plan.prs[] load.
  2. dependency graph 위상 정렬 (depends_on).
  3. 각 PR 의 *효과적 status* 결정 — gh 동적 조회 (default) 또는 plan 의
     status 필드. gh 우선.
  4. next = depends_on 의 모든 entry 가 `merged` 이고 자신 `status != merged`
     인 첫 PR (topo order).

본 script 는 *read-only*. 어떤 파일도 갱신하지 않는다. 갱신은 orchestrator
agent 의 책임.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import pathlib
import subprocess
import sys
from collections import defaultdict, deque
from typing import Any

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent

GH_TERMINAL_STATES = {"merged"}
GH_OPEN_STATES = {"open"}
GH_CLOSED_NOT_MERGED = {"closed"}

EXIT_OK = 0
EXIT_NO_NEXT = 0   # phase 5 done 또는 모든 slice merged — 정상.
EXIT_BLOCKED = 2   # cycle / 미정의 entry / plan parse fail.


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="next-slice picker for /nf-implement")
    p.add_argument("nf", help="NF 이름 (예 nssf)")
    p.add_argument("--status", action="store_true", help="요약 출력")
    p.add_argument("--json", action="store_true", help="JSON 출력")
    p.add_argument("--use-gh", dest="use_gh", action="store_true", default=True)
    p.add_argument("--no-gh", dest="use_gh", action="store_false")
    p.add_argument("--plan", default=None, help="pr-slicing-plan.yaml override")
    p.add_argument("--state", default=None, help="_implementation_run_state.yaml override")
    return p.parse_args()


def load_yaml(path: pathlib.Path) -> dict[str, Any] | None:
    if not path.exists():
        return None
    with path.open() as f:
        return yaml.safe_load(f) or {}


def gh_pr_state(merged_pr_url: str) -> str | None:
    if not merged_pr_url:
        return None
    pr_num = merged_pr_url.rstrip("/").split("/")[-1]
    if not pr_num.isdigit():
        return None
    try:
        out = subprocess.run(
            ["gh", "pr", "view", pr_num, "--json", "state,mergeStateStatus"],
            check=True,
            capture_output=True,
            text=True,
            timeout=15,
        )
    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
        return None
    try:
        data = json.loads(out.stdout)
    except json.JSONDecodeError:
        return None
    state = (data.get("state") or "").lower()
    if state == "merged":
        return "merged"
    if state == "open":
        return "open"
    if state == "closed":
        return "closed_not_merged"
    return None


def effective_status(pr: dict[str, Any], use_gh: bool) -> str:
    plan_status = (pr.get("status") or "not_started").lower()
    if not use_gh:
        return plan_status
    gh_state = gh_pr_state(pr.get("merged_pr") or "")
    if gh_state == "merged":
        return "merged"
    if gh_state == "open":
        # plan 이 merged 라 했지만 gh 가 open = drift. plan stale.
        return "open"
    if gh_state == "closed_not_merged":
        return "closed_not_merged"
    # gh 결과 없으면 plan 의 status 사용 (예 — merged_pr URL 없는 신규 slice).
    return plan_status


def topo_sort(prs: list[dict[str, Any]]) -> list[str]:
    id_to_pr = {pr["id"]: pr for pr in prs}
    indeg: dict[str, int] = defaultdict(int)
    adj: dict[str, list[str]] = defaultdict(list)
    for pr in prs:
        indeg.setdefault(pr["id"], 0)
        for dep in pr.get("depends_on") or []:
            if dep not in id_to_pr:
                raise ValueError(f"PR {pr['id']} depends on unknown PR {dep}")
            adj[dep].append(pr["id"])
            indeg[pr["id"]] += 1
    q: deque[str] = deque([pid for pid in id_to_pr if indeg[pid] == 0])
    order: list[str] = []
    while q:
        cur = q.popleft()
        order.append(cur)
        for nxt in adj[cur]:
            indeg[nxt] -= 1
            if indeg[nxt] == 0:
                q.append(nxt)
    if len(order) != len(id_to_pr):
        raise ValueError("dependency cycle detected in pr-slicing-plan.yaml")
    return order


def pick_next(prs: list[dict[str, Any]], use_gh: bool) -> tuple[dict[str, Any] | None, dict[str, str]]:
    id_to_pr = {pr["id"]: pr for pr in prs}
    statuses: dict[str, str] = {}
    for pr in prs:
        statuses[pr["id"]] = effective_status(pr, use_gh)
    order = topo_sort(prs)
    for pid in order:
        pr = id_to_pr[pid]
        if statuses[pid] == "merged":
            continue
        deps_ok = all(statuses.get(d) == "merged" for d in pr.get("depends_on") or [])
        if not deps_ok:
            continue
        if statuses[pid] in {"open", "in_progress"}:
            return pr, statuses
        # not_started 또는 closed_not_merged — open 후보.
        return pr, statuses
    return None, statuses


def render_human(payload: dict[str, Any]) -> str:
    return yaml.safe_dump(payload, sort_keys=False, allow_unicode=True)


def _json_safe(value: Any) -> Any:
    if isinstance(value, (dt.datetime, dt.date)):
        return value.isoformat()
    if isinstance(value, dict):
        return {str(k): _json_safe(v) for k, v in value.items()}
    if isinstance(value, list):
        return [_json_safe(v) for v in value]
    if isinstance(value, tuple):
        return [_json_safe(v) for v in value]
    return value


def render_json(payload: dict[str, Any]) -> str:
    return json.dumps(_json_safe(payload), indent=2, sort_keys=False, ensure_ascii=False)


def _display_path(path: pathlib.Path) -> str:
    if path.is_absolute():
        try:
            return str(path.relative_to(REPO))
        except ValueError:
            return str(path)
    return str(path)


def build_payload(nf: str, plan_path: pathlib.Path, state_path: pathlib.Path,
                  next_pr: dict[str, Any] | None, statuses: dict[str, str],
                  state: dict[str, Any] | None) -> dict[str, Any]:
    counters: dict[str, Any] = {}
    if state:
        counters = {
            "current_phase": state.get("current_phase"),
            "completed_phases": state.get("completed_phases"),
            "total_slices_completed": state.get("total_slices_completed"),
            "resume_count": state.get("resume_count"),
            "run_epoch": state.get("run_epoch"),
            "current_slice_attempts": state.get("current_slice_attempts"),
            "stop_requested": state.get("stop_requested"),
            "last_checkpoint_at": state.get("last_checkpoint_at"),
            "blockers": state.get("blockers"),
        }
    return {
        "nf": nf,
        "plan": _display_path(plan_path),
        "state": _display_path(state_path),
        "next_slice": {
            "id": next_pr["id"],
            "title": next_pr.get("title"),
            "phase": next_pr.get("phase"),
            "work_items": next_pr.get("work_items"),
            "scope_files": next_pr.get("scope_files"),
            "depends_on": next_pr.get("depends_on"),
            "required_checks": next_pr.get("required_checks"),
            "estimated_loc": next_pr.get("estimated_loc"),
        } if next_pr else None,
        "statuses": statuses,
        "run_state": counters,
        "recommendation": _recommend(next_pr, state),
    }


def _recommend(next_pr: dict[str, Any] | None, state: dict[str, Any] | None) -> str:
    if state and state.get("stop_requested"):
        return "stop_requested — finish current slice merge, then halt."
    if state and (state.get("resume_count") or 0) >= 20:
        return "resume_count cap 20 reached — halt + report (ADR-0005 D6)."
    if state and (state.get("total_slices_completed") or 0) >= 50:
        return "total_slices_completed cap 50 reached — halt + report."
    if next_pr is None:
        return "no eligible next slice — phase 5 complete or all dependencies blocked."
    return f"dispatch next slice {next_pr['id']} ({next_pr.get('phase')})."


def main() -> int:
    args = parse_args()
    nf = args.nf
    plan_path = pathlib.Path(args.plan) if args.plan else REPO / "dev" / nf / "pr-slicing-plan.yaml"
    state_path = pathlib.Path(args.state) if args.state else REPO / "dev" / nf / "_implementation_run_state.yaml"

    plan = load_yaml(plan_path)
    if plan is None:
        print(f"FAIL: plan file not found: {plan_path}", file=sys.stderr)
        return EXIT_BLOCKED
    prs = plan.get("prs") or []
    if not prs:
        print(f"FAIL: no prs[] in {plan_path}", file=sys.stderr)
        return EXIT_BLOCKED

    state = load_yaml(state_path) or {}

    try:
        next_pr, statuses = pick_next(prs, args.use_gh)
    except ValueError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return EXIT_BLOCKED

    payload = build_payload(nf, plan_path, state_path, next_pr, statuses, state)
    out = render_json(payload) if args.json else render_human(payload)
    print(out)
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
