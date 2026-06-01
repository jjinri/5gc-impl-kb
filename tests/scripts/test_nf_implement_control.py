from __future__ import annotations

import json
import pathlib
import subprocess
import sys
import textwrap

import yaml

REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "nf-implement-control.py"


def _state(tmp_path: pathlib.Path) -> pathlib.Path:
    state = tmp_path / "_implementation_run_state.yaml"
    state.write_text(
        textwrap.dedent(
            """
            schema_version: implementation-run-state-v2
            current_phase: 2
            completed_phases: [1]
            resume_count: 1
            run_epoch: 1
            total_slices_completed: 3
            stop_requested: false
            blockers: []
            """
        ),
        encoding="utf-8",
    )
    return state


def _run(args: list[str], check: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        cwd=REPO,
        check=check,
        capture_output=True,
        text=True,
    )


def test_stop_mutates_state(tmp_path: pathlib.Path) -> None:
    state = _state(tmp_path)
    out = _run(["demo", "--stop", "--json", "--state", str(state)])
    payload = json.loads(out.stdout)
    assert payload["stop_requested"] is True
    assert payload["stop_requested_at"]

    persisted = yaml.safe_load(state.read_text())
    assert persisted["stop_requested"] is True
    assert persisted["stop_requested_at"]
    # 다른 field 보존.
    assert persisted["resume_count"] == 1
    assert persisted["total_slices_completed"] == 3


def test_clear_stop_resets(tmp_path: pathlib.Path) -> None:
    state = _state(tmp_path)
    _run(["demo", "--stop", "--state", str(state)])
    _run(["demo", "--clear-stop", "--state", str(state)])
    persisted = yaml.safe_load(state.read_text())
    assert persisted["stop_requested"] is False
    assert "stop_requested_at" not in persisted


def test_status_is_read_only(tmp_path: pathlib.Path) -> None:
    state = _state(tmp_path)
    before = state.read_text()
    out = _run(["demo", "--status", "--json", "--state", str(state)])
    payload = json.loads(out.stdout)
    assert payload["control"]["stop_requested"] is False
    assert payload["control"]["resume_count"] == 1
    assert payload["blockers_count"] == 0
    # mutate 안 함.
    assert state.read_text() == before


def test_missing_state_file_blocks(tmp_path: pathlib.Path) -> None:
    missing = tmp_path / "nope.yaml"
    out = _run(["demo", "--stop", "--state", str(missing)], check=False)
    assert out.returncode == 2
