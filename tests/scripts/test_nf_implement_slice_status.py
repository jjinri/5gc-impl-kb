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
