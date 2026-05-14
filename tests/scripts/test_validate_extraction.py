from __future__ import annotations

import os
import pathlib
import subprocess

import yaml


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "validate-extraction.py"


def _write_min_nf(tmp_path: pathlib.Path) -> pathlib.Path:
    """Create a minimal valid v2 yaml + topic files. Return repo-like root."""
    root = tmp_path
    nf = root / "design" / "demo"
    (nf / "contract" / "api").mkdir(parents=True)
    (nf / "contract" / "data-model").mkdir(parents=True)
    (nf / "contract" / "interface.md").write_text(
        "---\nid: interface\nstatus: handoff_ready\n"
        "generated_sections: []\nuser_sections: []\n---\n", encoding="utf-8")
    (nf / "contract" / "error-handling.md").write_text(
        "---\nid: error-handling\nstatus: handoff_ready\n"
        "generated_sections: []\nuser_sections: []\n---\n", encoding="utf-8")
    (nf / "contract" / "api" / "OpA.md").write_text(
        "---\nid: api/OpA\nstatus: handoff_ready\n"
        "generated_sections: []\nuser_sections: []\n---\n", encoding="utf-8")
    (nf / "contract" / "data-model" / "S.md").write_text(
        "---\nid: data-model/S\nstatus: canonical\n"
        "generated_sections: []\nuser_sections: []\n---\n", encoding="utf-8")
    (nf / "contract" / "data-model" / "S.json").write_text(
        '{"schema_version":"data-model-v1","nf":"demo","topic_id":"data-model/S",'
        '"status":"canonical","fields":[],"dependencies":[],"unresolved_refs":[]}',
        encoding="utf-8")
    handoff = root / "handoff" / "demo"
    handoff.mkdir(parents=True)
    handoff_yaml = handoff / "contract.yaml"
    handoff_yaml.write_text(yaml.safe_dump({
        "schema_version": "handoff-v2",
        "nf": "demo",
        "agent_contract": {"status_precedence": "topic_over_category",
                           "default_read_order": [], "must_not": [],
                           "may_decide": [], "must_ask_or_block": []},
        "categories": {
            "interface": {"status": "handoff_ready", "layout": "single-file"},
            "error-handling": {"status": "handoff_ready", "layout": "single-file"},
            "api": {"status": "handoff_ready", "layout": "directory"},
            "data-model": {"status": "handoff_ready", "layout": "directory"},
        },
        "topics": {
            "interface": {"status": "handoff_ready"},
            "error-handling": {"status": "handoff_ready"},
            "api/OpA": {
                "status": "handoff_ready",
                "depends_on": ["data-model/S"],
                "related": ["interface"],
                "error_refs": ["error-handling"],
                "spec_refs": [],
            },
            "data-model/S": {
                "status": "canonical",
                "file": "design/demo/contract/data-model/S.md",
                "machine_file": "design/demo/contract/data-model/S.json",
                "spec_refs": [],
            },
        },
        "tasks": {},
        "spec_index": {},
        "sources": {},
    }), encoding="utf-8")
    return root


def _run(root: pathlib.Path, *args: str) -> subprocess.CompletedProcess:
    env = {**os.environ, "FIVEGC_REPO_ROOT": str(root)}
    return subprocess.run(
        [str(REPO / ".venv" / "bin" / "python3"), str(SCRIPT), *args],
        capture_output=True, text=True, cwd=root, timeout=60, env=env,
    )


def test_minimum_yaml_passes(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode == 0, out.stdout + out.stderr
    assert "FAIL" not in out.stdout



def test_rule_1_invalid_schema_version(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "handoff" / "demo" / "contract.yaml"
    data = yaml.safe_load(p.read_text(encoding="utf-8"))
    data["schema_version"] = "handoff-v9"
    p.write_text(yaml.safe_dump(data), encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#1" in out.stdout


def test_rule_2_invalid_status_enum(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "handoff" / "demo" / "contract.yaml"
    data = yaml.safe_load(p.read_text(encoding="utf-8"))
    data["topics"]["api/OpA"]["status"] = "ready_for_review"
    p.write_text(yaml.safe_dump(data), encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#2" in out.stdout


def test_rule_3_missing_topic_file(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    (tmp_path / "design" / "demo" / "contract" / "api" / "OpA.md").unlink()
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#3" in out.stdout


def test_rule_4_dangling_cross_ref(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "handoff" / "demo" / "contract.yaml"
    data = yaml.safe_load(p.read_text(encoding="utf-8"))
    data["topics"]["api/OpA"]["depends_on"] = ["data-model/Ghost"]
    p.write_text(yaml.safe_dump(data), encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#4" in out.stdout


def test_rule_4_anchor_present_passes(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    # Add an explicit anchor inside error-handling.md, then reference it.
    eh = tmp_path / "design" / "demo" / "contract" / "error-handling.md"
    eh.write_text(
        eh.read_text(encoding="utf-8")
        + '\n<a id="op-400"></a>\n## Op 400 — bad request\n',
        encoding="utf-8",
    )
    p = tmp_path / "handoff" / "demo" / "contract.yaml"
    data = yaml.safe_load(p.read_text(encoding="utf-8"))
    data["topics"]["api/OpA"]["error_refs"] = ["error-handling#op-400"]
    p.write_text(yaml.safe_dump(data), encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode == 0, out.stdout + out.stderr


def test_rule_4_missing_anchor_fails(tmp_path: pathlib.Path) -> None:
    # error-handling.md exists but the referenced anchor does not. Without
    # anchor verification, rule_4 used to PASS — that's the bug being fixed.
    _write_min_nf(tmp_path)
    p = tmp_path / "handoff" / "demo" / "contract.yaml"
    data = yaml.safe_load(p.read_text(encoding="utf-8"))
    data["topics"]["api/OpA"]["error_refs"] = ["error-handling#missing-anchor"]
    p.write_text(yaml.safe_dump(data), encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0, (
        f"rule #4 must FAIL on missing anchor (got exit 0).\n"
        f"stdout:\n{out.stdout}\nstderr:\n{out.stderr}"
    )
    assert "#4" in out.stdout
    assert "missing-anchor" in out.stdout


def test_rule_4_task_read_anchor_verified(tmp_path: pathlib.Path) -> None:
    # Anchor verification must also apply to tasks.<id>.read entries.
    _write_min_nf(tmp_path)
    p = tmp_path / "handoff" / "demo" / "contract.yaml"
    data = yaml.safe_load(p.read_text(encoding="utf-8"))
    data["tasks"] = {
        "demo-task": {
            "phase": "01",
            "goal": "demo",
            "read": ["error-handling#missing-anchor"],
            "produces": [],
            "blocked_by": [],
            "acceptance": [],
        }
    }
    p.write_text(yaml.safe_dump(data), encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#4" in out.stdout
    assert "task" in out.stdout and "missing-anchor" in out.stdout


def test_rule_5_category_topic_mismatch(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "handoff" / "demo" / "contract.yaml"
    data = yaml.safe_load(p.read_text(encoding="utf-8"))
    data["topics"]["api/OpA"]["status"] = "draft"
    p.write_text(yaml.safe_dump(data), encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#5" in out.stdout


def test_rule_6_blocked_needs_reason(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "handoff" / "demo" / "contract.yaml"
    data = yaml.safe_load(p.read_text(encoding="utf-8"))
    data["topics"]["api/OpA"]["status"] = "blocked"
    p.write_text(yaml.safe_dump(data), encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#6" in out.stdout


def test_rule_7_duplicate_marker(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "design" / "demo" / "contract" / "api" / "OpA.md"
    p.write_text(
        "---\nid: api/OpA\nstatus: handoff_ready\n"
        "generated_sections: [foo]\nuser_sections: []\n---\n"
        "<!-- AUTO:foo:start -->\nA\n<!-- AUTO:foo:end -->\n"
        "<!-- AUTO:foo:start -->\nB\n<!-- AUTO:foo:end -->\n",
        encoding="utf-8",
    )
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#7" in out.stdout


def test_rule_8_frontmatter_marker_sync(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "design" / "demo" / "contract" / "api" / "OpA.md"
    p.write_text(
        "---\nid: api/OpA\nstatus: handoff_ready\n"
        "generated_sections: [foo]\nuser_sections: []\n---\n"
        "no markers here\n",
        encoding="utf-8",
    )
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#8" in out.stdout


def test_rule_9_missing_machine_file(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    (tmp_path / "design" / "demo" / "contract" / "data-model" / "S.json").unlink()
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#9" in out.stdout


def test_rule_10_invalid_json(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    (tmp_path / "design" / "demo" / "contract" / "data-model" / "S.json").write_text("{ not json", encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#10" in out.stdout


def test_rule_11_topic_id_mismatch(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "design" / "demo" / "contract" / "data-model" / "S.json"
    p.write_text(
        '{"schema_version":"data-model-v1","nf":"demo","topic_id":"data-model/OTHER",'
        '"status":"canonical","fields":[],"dependencies":[],"unresolved_refs":[]}',
        encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#11" in out.stdout


def test_rule_12_unresolved_with_canonical_status(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "design" / "demo" / "contract" / "data-model" / "S.json"
    p.write_text(
        '{"schema_version":"data-model-v1","nf":"demo","topic_id":"data-model/S",'
        '"status":"canonical","fields":[],"dependencies":[],'
        '"unresolved_refs":[{"ref":"#/X","note":"not registered"}]}',
        encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#12" in out.stdout


def test_rule_13_unknown_dependency(tmp_path: pathlib.Path) -> None:
    _write_min_nf(tmp_path)
    p = tmp_path / "design" / "demo" / "contract" / "data-model" / "S.json"
    p.write_text(
        '{"schema_version":"data-model-v1","nf":"demo","topic_id":"data-model/S",'
        '"status":"canonical","fields":[],"dependencies":["data-model/Ghost"],'
        '"unresolved_refs":[]}',
        encoding="utf-8")
    out = _run(tmp_path, "demo", "--level", "basic")
    assert out.returncode != 0
    assert "#13" in out.stdout
