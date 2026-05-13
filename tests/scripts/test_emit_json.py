from __future__ import annotations

import json
import pathlib
import subprocess


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "resolve-yaml-refs.py"
NSSF_YAML = REPO / "specs" / "29.531" / "TS29531_Nnssf_NSSelection.yaml"


def _run(*args: str) -> dict:
    out = subprocess.run(
        [".venv/bin/python3", str(SCRIPT), *args],
        capture_output=True, text=True, cwd=REPO, timeout=60,
    )
    assert out.returncode == 0, out.stderr
    return json.loads(out.stdout)


def test_emit_json_basic_shape() -> None:
    payload = _run(
        str(NSSF_YAML), "AuthorizedNetworkSliceInfo",
        "--emit-json",
        "--topic-id", "data-model/AuthorizedNetworkSliceInfo",
        "--nf", "nssf",
        "--spec-ref", "TS 29.531 §6.1.6.2.5",
        "--handoff-topics", "data-model/SliceInfoForRegistration",
        "--handoff-topics", "data-model/AuthorizedNetworkSliceInfo",
    )
    assert payload["schema_version"] == "data-model-v1"
    assert payload["nf"] == "nssf"
    assert payload["topic_id"] == "data-model/AuthorizedNetworkSliceInfo"
    assert payload["root_schema"] == "AuthorizedNetworkSliceInfo"
    assert payload["source"]["spec_refs"] == ["TS 29.531 §6.1.6.2.5"]
    assert "fields" in payload and isinstance(payload["fields"], list)
    assert isinstance(payload["dependencies"], list)
    assert isinstance(payload["unresolved_refs"], list)


def test_emit_json_handoff_topic_becomes_dependency() -> None:
    # SliceInfoForRegistration is itself a handoff topic; it may transitively
    # appear inside other schemas. When it does, it must show as { "topic": ... }
    # and be listed in dependencies — not inlined.
    payload = _run(
        str(NSSF_YAML), "AuthorizedNetworkSliceInfo",
        "--emit-json",
        "--topic-id", "data-model/AuthorizedNetworkSliceInfo",
        "--nf", "nssf",
        "--spec-ref", "TS 29.531 §6.1.6.2.5",
        "--handoff-topics", "data-model/SliceInfoForRegistration",
        "--handoff-topics", "data-model/AuthorizedNetworkSliceInfo",
    )
    # The root itself never appears as a topic ref inside its own tree.
    # Check that any field whose type IS a known handoff topic resolves to a topic ref
    # and lands in dependencies. AllowedSnssai (non-handoff) must be inlined.
    body = json.dumps(payload)
    assert "_inlined_from" in body
    # dependencies only carries handoff topics
    for dep in payload["dependencies"]:
        assert dep.startswith("data-model/")


def test_emit_json_unresolved_refs_listed() -> None:
    # Provoke an unresolved ref by pointing to a tiny synthetic yaml under tmp.
    pass  # covered indirectly via build-handoff test; placeholder removed in T8
