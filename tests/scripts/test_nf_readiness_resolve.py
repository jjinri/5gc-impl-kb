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


def _import_resolve_module():
    """module-level decide()/resolve_entry() 를 unit-test 용으로 import."""
    import importlib.util
    spec = importlib.util.spec_from_file_location("nfrr", str(SCRIPT))
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def test_decide_generated_medium_without_manual_is_blocker():
    """PR #45 review Medium fix — generated medium 단독 시 blocker."""
    mod = _import_resolve_module()
    doc = {
        "generated": {"nfs": {"foo": {
            "primary_spec": "99.999",
            "primary_spec_confidence": "medium",
        }}},
        "manual_overrides": {"nfs": {}},
    }
    entry, source = mod.resolve_entry(doc, "foo")
    assert source == "generated"
    decision, reason = mod.decide(entry, source, doc, "foo")
    assert decision == "blocker", (decision, reason)
    assert "medium" in reason.lower()
    assert "manual_overrides" in reason


def test_decide_generated_medium_with_manual_override_proceeds():
    """manual_overrides 가 medium+ 로 같은 NF 를 확정하면 proceed."""
    mod = _import_resolve_module()
    doc = {
        "generated": {"nfs": {"foo": {
            "primary_spec": "99.999",
            "primary_spec_confidence": "medium",
        }}},
        "manual_overrides": {"nfs": {"foo": {
            "primary_spec": "99.999",
            "primary_spec_confidence": "medium",
        }}},
    }
    entry, source = mod.resolve_entry(doc, "foo")
    assert source == "manual"
    decision, reason = mod.decide(entry, source, doc, "foo")
    assert decision == "proceed", (decision, reason)
    assert "manual" in reason.lower()


def test_decide_generated_low_is_blocker():
    """generated low 는 항상 blocker."""
    mod = _import_resolve_module()
    doc = {
        "generated": {"nfs": {"foo": {
            "primary_spec": "99.999",
            "primary_spec_confidence": "low",
        }}},
        "manual_overrides": {"nfs": {}},
    }
    entry, source = mod.resolve_entry(doc, "foo")
    decision, reason = mod.decide(entry, source, doc, "foo")
    assert decision == "blocker"
    assert "low" in reason.lower()


def test_decide_manual_low_is_blocker():
    """manual_override 가 low 면 blocker (high/medium 으로 갱신 요구)."""
    mod = _import_resolve_module()
    doc = {
        "generated": {"nfs": {}},
        "manual_overrides": {"nfs": {"foo": {
            "primary_spec": "99.999",
            "primary_spec_confidence": "low",
        }}},
    }
    entry, source = mod.resolve_entry(doc, "foo")
    assert source == "manual"
    decision, reason = mod.decide(entry, source, doc, "foo")
    assert decision == "blocker"
    assert "low" in reason.lower()
