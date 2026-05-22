from __future__ import annotations

import os
import pathlib
import subprocess

import yaml


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "build-handoff.py"


def _run_build(nf: str, cwd: pathlib.Path) -> pathlib.Path:
    env = {**os.environ, "FIVEGC_REPO_ROOT": str(cwd)}
    out = subprocess.run(
        [str(REPO / ".venv" / "bin" / "python3"), str(SCRIPT), nf],
        capture_output=True, text=True, cwd=cwd, timeout=120, env=env,
    )
    assert out.returncode == 0, out.stderr
    return cwd / "handoff" / nf / "contract.yaml"


def _seed(tmp_path: pathlib.Path) -> pathlib.Path:
    nf = tmp_path / "design" / "demo"
    (nf / "contract" / "api").mkdir(parents=True)
    (nf / "contract" / "data-model").mkdir(parents=True)
    (nf / "contract" / "interface.md").write_text(
        "---\nid: interface\nstatus: handoff_ready\ngenerated_sections: []\n"
        "user_sections: []\n---\n# Interface\n", encoding="utf-8")
    (nf / "contract" / "error-handling.md").write_text(
        "---\nid: error-handling\nstatus: handoff_ready\ngenerated_sections: []\n"
        "user_sections: []\n---\n# Error\n", encoding="utf-8")
    (nf / "contract" / "api" / "OpA.md").write_text(
        "---\nid: api/OpA\nstatus: handoff_ready\ngenerated_sections: []\n"
        "user_sections: []\n---\n# OpA\n", encoding="utf-8")
    (nf / "contract" / "data-model" / "S.md").write_text(
        "---\nid: data-model/S\nstatus: canonical\ngenerated_sections: []\n"
        "user_sections: []\n---\n# S\n", encoding="utf-8")
    (nf / "contract" / "data-model" / "S.json").write_text("{}", encoding="utf-8")
    seed = nf / "_contract_seed.yaml"
    seed.write_text(
        yaml.safe_dump({
            "nf": "demo",
            "categories": {
                "interface":     {"status": "handoff_ready", "layout": "single-file"},
                "error-handling": {"status": "handoff_ready", "layout": "single-file"},
                "api":           {"status": "handoff_ready", "layout": "directory"},
                "data-model":    {"status": "handoff_ready", "layout": "directory"},
            },
            "topics": {
                "interface":      {"status": "handoff_ready", "spec_refs": ["TS X §1"]},
                "error-handling": {"status": "handoff_ready", "spec_refs": []},
                "api/OpA": {
                    "status": "handoff_ready",
                    "depends_on": ["data-model/S"],
                    "related": ["interface"],
                    "error_refs": [],
                    "spec_refs": ["TS X §2"],
                },
                "data-model/S": {
                    "status": "canonical",
                    "file": "design/demo/contract/data-model/S.md",
                    "machine_file": "design/demo/contract/data-model/S.json",
                    "spec_refs": ["TS X §3"],
                },
            },
            "tasks": {
                "demo-opa": {
                    "phase": "02-api",
                    "goal": "Implement OpA",
                    "read": ["api/OpA", "data-model/S"],
                    "produces": ["<impl>/opa.*"],
                    "blocked_by": ["api/OpA.status not in [canonical, handoff_ready]"],
                    "acceptance": ["handles 200 response"],
                },
            },
            "sources": {"TS X": "specs/X/X.docx"},
        }), encoding="utf-8",
    )
    return seed


def test_build_handoff_v2_emits_full_schema(tmp_path: pathlib.Path) -> None:
    _seed(tmp_path)
    out_path = _run_build("demo", tmp_path)
    data = yaml.safe_load(out_path.read_text(encoding="utf-8"))

    assert data["schema_version"] == "handoff-v2"
    assert data["nf"] == "demo"
    ac = data["agent_contract"]
    assert ac["status_precedence"] == "topic_over_category"
    assert isinstance(ac["default_read_order"], list) and len(ac["default_read_order"]) >= 5
    assert isinstance(ac["must_not"], list) and len(ac["must_not"]) >= 3
    assert isinstance(ac["may_decide"], list) and len(ac["may_decide"]) >= 2
    assert isinstance(ac["must_ask_or_block"], list) and len(ac["must_ask_or_block"]) >= 2

    cats = data["categories"]
    assert cats["api"]["status"] == "handoff_ready"
    assert cats["api"]["layout"] == "directory"

    topics = data["topics"]
    assert topics["api/OpA"]["status"] == "handoff_ready"
    assert topics["api/OpA"]["depends_on"] == ["data-model/S"]
    assert topics["data-model/S"]["machine_file"].endswith("S.json")

    si = data["spec_index"]
    assert "api/OpA" in si["TS X §2"]
    assert "data-model/S" in si["TS X §3"]

    assert data["sources"] == {"TS X": "specs/X/X.docx"}

    tasks = data["tasks"]
    assert tasks["demo-opa"]["read"] == ["api/OpA", "data-model/S"]
    assert tasks["demo-opa"]["blocked_by"][0].startswith("api/OpA.status")


def test_build_handoff_v2_missing_seed_errors(tmp_path: pathlib.Path) -> None:
    nf = tmp_path / "design" / "demo2"
    nf.mkdir(parents=True)
    env = {**os.environ, "FIVEGC_REPO_ROOT": str(tmp_path)}
    out = subprocess.run(
        [str(REPO / ".venv" / "bin" / "python3"), str(SCRIPT), "demo2"],
        capture_output=True, text=True, cwd=tmp_path, timeout=30, env=env,
    )
    assert out.returncode != 0
    assert "_contract_seed.yaml" in out.stderr


# PR F1.2 — build-handoff.py 가 handoff yaml self-contained 정책 위해 api topic / error-handling
# topic 을 OpenAPI yaml 분석으로 enrich. 본 enrichment 가 nf-status 의 api_operation_complete +
# problem_details_matrix_complete check 직접 의존. live NSSF data 로 검증 (seed + manifest +
# primary yaml 풀 셋업이 fixture 로 옮기기 큼).

def test_build_handoff_v2_api_topic_enriched_for_nssf() -> None:
    """NSSF api topic 이 method/path 외 responses/security_requirements/error_responses/
    source_refs 4 키 추가 보유. seed 가 method/path 만 줘도 build-handoff 가 yaml 에서
    추출해 채움. nf-status.py 의 api_operation_complete 6 키 mandate 충족.
    """
    out = subprocess.run(
        [str(REPO / ".venv" / "bin" / "python3"),
         str(REPO / "design" / "scripts" / "build-handoff.py"), "nssf"],
        capture_output=True, text=True, cwd=REPO, timeout=60,
    )
    assert out.returncode == 0, out.stderr
    data = yaml.safe_load((REPO / "handoff" / "nssf" / "contract.yaml").read_text(encoding="utf-8"))
    api_topics = {tid: t for tid, t in (data.get("topics") or {}).items() if tid.startswith("api/")}
    assert api_topics, "NSSF handoff yaml has no api topics"
    for tid, t in api_topics.items():
        for key in ("method", "path", "responses", "security_requirements",
                    "error_responses", "source_refs"):
            assert key in t, f"{tid} missing key {key!r} after build-handoff enrichment"
        # source_refs 가 최소 1개 (yaml 경로 자동 추가).
        assert t["source_refs"], f"{tid} source_refs empty"


def test_build_handoff_v2_error_handling_operations_for_nssf() -> None:
    """NSSF error-handling topic 이 operations × causes 매트릭스 보유.
    nf-status.py problem_details_matrix_complete check 가 operations 필수 검증.
    """
    out = subprocess.run(
        [str(REPO / ".venv" / "bin" / "python3"),
         str(REPO / "design" / "scripts" / "build-handoff.py"), "nssf"],
        capture_output=True, text=True, cwd=REPO, timeout=60,
    )
    assert out.returncode == 0, out.stderr
    data = yaml.safe_load((REPO / "handoff" / "nssf" / "contract.yaml").read_text(encoding="utf-8"))
    eh = (data.get("topics") or {}).get("error-handling")
    assert eh is not None, "NSSF handoff yaml missing error-handling topic"
    ops = eh.get("operations") or {}
    assert ops, "error-handling.operations enrichment empty"
    for op_id, info in ops.items():
        assert isinstance(info, dict)
        assert "causes" in info, f"operation {op_id!r} missing causes list"
        assert isinstance(info["causes"], list)
