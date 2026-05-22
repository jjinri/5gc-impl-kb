from __future__ import annotations

import os
import pathlib
import subprocess

import yaml


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "nf-readiness-resolve.py"
PY = REPO / ".venv" / "bin" / "python3"


def _run(args: list[str]) -> subprocess.CompletedProcess:
    env = os.environ.copy()
    return subprocess.run(
        [str(PY), str(SCRIPT), *args],
        cwd=str(REPO),
        env=env,
        capture_output=True,
        text=True,
        check=False,
    )


def test_resolve_nssf_proceed():
    """현재 registry 에 NSSF=high 이므로 proceed."""
    r = _run(["nssf"])
    assert r.returncode == 0, r.stderr
    doc = yaml.safe_load(r.stdout)
    assert doc["decision"] == "proceed", doc
    assert doc["effective_primary_spec"] == "29.531"
    assert doc["effective_confidence"] == "high"
    assert doc["effective_source"] == "generated"


def test_resolve_unknown_nf_blocker():
    """등록되지 않은 NF 는 blocker."""
    r = _run(["definitely_unknown_nf"])
    assert r.returncode == 1, r.stderr
    doc = yaml.safe_load(r.stdout)
    assert doc["decision"] == "blocker", doc
    assert "effective entry 없음" in doc["reason"]


def test_resolve_no_bootstrap_write_flag():
    """--no-bootstrap-write 옵션이 수용되고 정상 exit."""
    r = _run(["nssf", "--no-bootstrap-write"])
    assert r.returncode == 0, r.stderr
    doc = yaml.safe_load(r.stdout)
    assert doc["decision"] == "proceed"
    # bootstrap_action 은 dry-run 형태여야 함 (또는 up-to-date)
    assert "bootstrap_action" in doc
