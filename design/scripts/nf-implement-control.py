#!/usr/bin/env python3
# /nf-implement 의 외부 control surface. ADR-0005 D6 (--stop signal) + SKILL
# 사람 인터페이스. A′ topology 에서 orchestrator 는 `claude --agent
# nf-orchestrator` 전용 process 이므로, main pane 의 `/nf-implement <nf> --stop`
# 이 state file 의 stop_requested 를 *외부에서* mutate 해야 한다. picker
# (nf-implement-slice-status.py) 는 read-only 계약이라 mutate 하지 못한다.
"""
Usage:
    .venv/bin/python3 design/scripts/nf-implement-control.py <nf> <action> [options]

Actions:
    --stop          state 의 stop_requested = true + stop_requested_at 기록.
                    orchestrator 가 매 loop iteration 시작 시 이 flag 를 확인해
                    현재 slice 머지 후 graceful 종료한다.
    --clear-stop    stop_requested = false 로 되돌림 (--resume 전 재기동용).
    --status        control 관련 state 요약 출력 (read-only). next-slice picker
                    는 nf-implement-slice-status.py 가 담당.

Options:
    --json          JSON 출력 (기본 = YAML/human).
    --state PATH    _implementation_run_state.yaml 경로 override. 기본 =
                    dev/<nf>/_implementation_run_state.yaml.

본 script 는 *stop signal mutation* 의 단일 진입점이다. 그 외 state field 의
진실 출처는 orchestrator agent 다 (state file 의 sole writer). 사람은 본 script
로 stop_requested 만 신호한다.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import pathlib
import sys
from typing import Any

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent

EXIT_OK = 0
EXIT_BLOCKED = 2   # state file 부재 등.

CONTROL_FIELDS = (
    "schema_version",
    "current_phase",
    "completed_phases",
    "current_slice",
    "resume_count",
    "run_epoch",
    "total_slices_completed",
    "stop_requested",
    "stop_requested_at",
    "last_checkpoint_at",
)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="external control surface for /nf-implement")
    p.add_argument("nf", help="NF 이름 (예 nssf)")
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument("--stop", action="store_true", help="stop_requested = true 기록")
    g.add_argument("--clear-stop", dest="clear_stop", action="store_true",
                   help="stop_requested = false 로 복귀")
    g.add_argument("--status", action="store_true", help="control state 요약 (read-only)")
    p.add_argument("--json", action="store_true", help="JSON 출력")
    p.add_argument("--state", default=None, help="_implementation_run_state.yaml override")
    return p.parse_args()


def load_yaml(path: pathlib.Path) -> dict[str, Any] | None:
    if not path.exists():
        return None
    with path.open() as f:
        return yaml.safe_load(f) or {}


def dump_yaml(path: pathlib.Path, data: dict[str, Any]) -> None:
    with path.open("w") as f:
        yaml.safe_dump(data, f, sort_keys=False, allow_unicode=True)


def _now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def _json_safe(value: Any) -> Any:
    if isinstance(value, (dt.datetime, dt.date)):
        return value.isoformat()
    if isinstance(value, dict):
        return {str(k): _json_safe(v) for k, v in value.items()}
    if isinstance(value, list):
        return [_json_safe(v) for v in value]
    return value


def render(payload: dict[str, Any], as_json: bool) -> str:
    safe = _json_safe(payload)
    if as_json:
        return json.dumps(safe, indent=2, sort_keys=False, ensure_ascii=False)
    return yaml.safe_dump(safe, sort_keys=False, allow_unicode=True)


def _summary(nf: str, state_path: pathlib.Path, state: dict[str, Any]) -> dict[str, Any]:
    return {
        "nf": nf,
        "state": str(state_path.relative_to(REPO)) if state_path.is_absolute()
                 and _under_repo(state_path) else str(state_path),
        "control": {k: state.get(k) for k in CONTROL_FIELDS},
        "blockers_count": len(state.get("blockers") or []),
    }


def _under_repo(path: pathlib.Path) -> bool:
    try:
        path.relative_to(REPO)
        return True
    except ValueError:
        return False


def main() -> int:
    args = parse_args()
    nf = args.nf
    state_path = (pathlib.Path(args.state) if args.state
                  else REPO / "dev" / nf / "_implementation_run_state.yaml")

    state = load_yaml(state_path)
    if state is None:
        print(f"FAIL: state file not found: {state_path}", file=sys.stderr)
        print("orchestrator 가 아직 기동 안 함 — `/nf-implement <nf>` 먼저.", file=sys.stderr)
        return EXIT_BLOCKED

    if args.status:
        print(render(_summary(nf, state_path, state), args.json))
        return EXIT_OK

    if args.stop:
        state["stop_requested"] = True
        state["stop_requested_at"] = _now()
        dump_yaml(state_path, state)
        print(render({
            "action": "stop",
            "nf": nf,
            "stop_requested": True,
            "stop_requested_at": state["stop_requested_at"],
            "note": "orchestrator 가 현재 slice 머지 후 graceful 종료한다.",
        }, args.json))
        return EXIT_OK

    if args.clear_stop:
        state["stop_requested"] = False
        state.pop("stop_requested_at", None)
        dump_yaml(state_path, state)
        print(render({
            "action": "clear-stop",
            "nf": nf,
            "stop_requested": False,
            "note": "stop signal 해제 — `/nf-implement <nf> --resume` 으로 재기동 가능.",
        }, args.json))
        return EXIT_OK

    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
