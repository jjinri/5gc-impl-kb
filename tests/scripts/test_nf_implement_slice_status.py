from __future__ import annotations

import json
import pathlib
import subprocess
import sys
import textwrap

REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "nf-implement-slice-status.py"


def test_json_output_serializes_yaml_timestamps(tmp_path: pathlib.Path) -> None:
    plan = tmp_path / "pr-slicing-plan.yaml"
    state = tmp_path / "_implementation_run_state.yaml"
    plan.write_text(
        textwrap.dedent(
            """
            prs:
              - id: PR-base
                title: base
                status: merged
              - id: PR-next
                title: next
                status: not_started
                depends_on: [PR-base]
                phase: phase1_wave1
                scope_files: [src/demo/generated/model/**]
                required_checks: [pytest_full_suite]
            """
        ),
        encoding="utf-8",
    )
    # PyYAML safe_load turns this unquoted value into datetime.datetime.
    state.write_text(
        textwrap.dedent(
            """
            schema_version: implementation-run-state-v1
            current_phase: 1
            completed_phases: [1]
            last_checkpoint_at: 2026-05-27T09:51:00Z
            blockers: []
            """
        ),
        encoding="utf-8",
    )

    out = subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            "demo",
            "--no-gh",
            "--json",
            "--plan",
            str(plan),
            "--state",
            str(state),
        ],
        cwd=REPO,
        check=True,
        capture_output=True,
        text=True,
    )

    payload = json.loads(out.stdout)
    assert payload["next_slice"]["id"] == "PR-next"
    assert payload["run_state"]["last_checkpoint_at"] == "2026-05-27T09:51:00+00:00"


def _run(script_args: list[str], cwd: pathlib.Path) -> dict:
    out = subprocess.run(
        [sys.executable, str(SCRIPT), *script_args],
        cwd=cwd,
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(out.stdout)


def _write_plan(plan: pathlib.Path) -> None:
    plan.write_text(
        textwrap.dedent(
            """
            prs:
              - id: PR-base
                title: base
                status: merged
              - id: PR-next
                title: next
                status: not_started
                depends_on: [PR-base]
                phase: phase1_wave1
            """
        ),
        encoding="utf-8",
    )


def test_resume_count_cap_recommendation(tmp_path: pathlib.Path) -> None:
    # ADR-0005 D6 — resume_count (이전 chain_depth) cap 20 도달 시 halt 권고.
    plan = tmp_path / "pr-slicing-plan.yaml"
    state = tmp_path / "_implementation_run_state.yaml"
    _write_plan(plan)
    state.write_text(
        textwrap.dedent(
            """
            schema_version: implementation-run-state-v2
            current_phase: 1
            resume_count: 20
            run_epoch: 20
            total_slices_completed: 3
            """
        ),
        encoding="utf-8",
    )

    payload = _run(
        ["demo", "--no-gh", "--json", "--plan", str(plan), "--state", str(state)],
        cwd=REPO,
    )
    assert payload["run_state"]["resume_count"] == 20
    assert payload["run_state"]["run_epoch"] == 20
    assert "resume_count cap 20" in payload["recommendation"]
    assert "chain_depth" not in payload["run_state"]


def test_progress_dashboard(tmp_path: pathlib.Path) -> None:
    # --progress = pr-slicing-plan 전체 진행 dashboard (phase별 + 전체 %).
    plan = tmp_path / "pr-slicing-plan.yaml"
    _write_plan(plan)
    out = subprocess.run(
        [sys.executable, str(SCRIPT), "demo", "--no-gh", "--progress", "--plan", str(plan)],
        cwd=REPO,
        check=True,
        capture_output=True,
        text=True,
    )
    text = out.stdout
    assert "pr-slicing-plan progress" in text
    assert "overall: 1/2 merged" in text          # PR-base merged, PR-next not_started
    assert "50%" in text
    assert "[phase1_wave1] 0/1" in text            # PR-next phase, 0 merged
    assert "<- NEXT" in text                       # PR-next is eligible next
    assert "PR-next" in text
