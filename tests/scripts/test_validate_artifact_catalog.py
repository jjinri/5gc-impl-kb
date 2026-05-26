# validate-artifact-catalog.py 의 placeholder 확장·glob 매칭 smoke test
"""Smoke + integration test for validate-artifact-catalog.py.

본 validator 는 docs/lifecycle-artifacts.md §3 ↔ 실제 repo 파일 drift 검사.
hyphen 이름이라 import 안 됨 → subprocess 로 실행 + exit code 검사.
"""

from __future__ import annotations

import pathlib
import subprocess
import sys


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "validate-artifact-catalog.py"


def test_catalog_validator_runs_on_current_repo() -> None:
    """현 repo 상태에서 catalog ↔ files 정합 — exit 0."""
    out = subprocess.run(
        [sys.executable, str(SCRIPT)],
        capture_output=True, text=True, cwd=REPO, timeout=30,
    )
    assert out.returncode == 0, (
        f"validator FAIL (exit {out.returncode}). "
        f"catalog drift detected.\nstderr:\n{out.stderr}"
    )
    assert "catalog ↔ repo consistent" in out.stderr


def test_catalog_validator_summary_format() -> None:
    """PASS/FAIL/SKIP 줄 형식 확인."""
    out = subprocess.run(
        [sys.executable, str(SCRIPT)],
        capture_output=True, text=True, cwd=REPO, timeout=30,
    )
    assert out.returncode == 0
    # 마지막 라인 직전 — PASS N  FAIL 0  SKIP M.
    lines = [l for l in out.stderr.splitlines() if "PASS" in l and "FAIL" in l]
    assert lines, f"summary line 부재. stderr:\n{out.stderr}"
    assert "FAIL 0" in lines[-1]
