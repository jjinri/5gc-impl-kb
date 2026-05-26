from __future__ import annotations

import os
import pathlib
import subprocess
import sys

import yaml


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "build-handoff.py"


def _run_build(nf: str, cwd: pathlib.Path) -> pathlib.Path:
    env = {**os.environ, "FIVEGC_REPO_ROOT": str(cwd)}
    out = subprocess.run(
        [sys.executable, str(SCRIPT), nf],
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
        [sys.executable, str(SCRIPT), "demo2"],
        capture_output=True, text=True, cwd=tmp_path, timeout=30, env=env,
    )
    assert out.returncode != 0
    assert "_contract_seed.yaml" in out.stderr


# PR F1.2 — build-handoff.py 가 handoff yaml self-contained 정책 위해 api topic / error-handling
# topic 을 OpenAPI yaml 분석으로 enrich. nf-status 의 api_operation_complete +
# problem_details_matrix_complete check 가 본 enrichment 에 직접 의존. 이전엔 live NSSF 산출
# (`design/nssf/_contract_seed.yaml`, `handoff/nssf/contract.yaml`) 을 직접 읽어 검증했으나, 본
# repo 정책상 두 파일 모두 gitignored generated cache 이므로 fresh checkout 에서 실패. 본 fixture
# 는 tmp_path 안에 enrichment branch 를 트리거할 최소 manifest + minimal OpenAPI yaml + seed 를
# 합성해 deterministic 검증한다.

def _seed_with_yaml(tmp_path: pathlib.Path) -> None:
    """tmp_path 안에 enrichment branch 를 타게 하는 최소 산출 배치.

    추가 산출 (`_seed` 대비).
    - design/demo/_manifest.yaml — primary_spec + primary_files.yamls.
    - specs/X/X.yaml — paths + global security 를 가진 minimal OpenAPI.
    - seed.topics["api/OpA"] 에 method + path 키 (enrichment trigger).
    """
    _seed(tmp_path)
    nf = tmp_path / "design" / "demo"
    (nf / "_manifest.yaml").write_text(
        yaml.safe_dump({
            "primary_spec": "X",
            "primary_files": {"yamls": ["X.yaml"]},
        }),
        encoding="utf-8",
    )
    spec_dir = tmp_path / "specs" / "X"
    spec_dir.mkdir(parents=True)
    (spec_dir / "X.yaml").write_text(
        yaml.safe_dump({
            "openapi": "3.0.0",
            "info": {"title": "Test", "version": "1.0"},
            "security": [{"oauth2": ["read"]}],
            "paths": {
                "/opa/{id}": {
                    "get": {
                        "operationId": "getOpA",
                        "responses": {
                            "200": {
                                "description": "OK",
                                "content": {
                                    "application/json": {
                                        "schema": {"$ref": "#/components/schemas/S"},
                                    },
                                },
                            },
                            "400": {"description": "INVALID_INPUT"},
                            "500": {"description": "INTERNAL_ERROR"},
                        },
                    },
                },
            },
        }),
        encoding="utf-8",
    )
    # seed 의 api/OpA topic 에 method + path 추가 (enrichment trigger).
    seed_path = nf / "_contract_seed.yaml"
    seed = yaml.safe_load(seed_path.read_text(encoding="utf-8"))
    seed["topics"]["api/OpA"]["method"] = "GET"
    seed["topics"]["api/OpA"]["path"] = "/opa/{id}"
    seed_path.write_text(yaml.safe_dump(seed), encoding="utf-8")


def test_build_handoff_v2_api_topic_enrichment_from_yaml(tmp_path: pathlib.Path) -> None:
    """api topic 이 seed 의 method + path 만으로 yaml 에서 responses /
    security_requirements / error_responses / source_refs 4 키를 채워야 함.
    nf-contract-check.py 의 api_operation_complete 6 키 mandate 의 직접 의존.
    """
    _seed_with_yaml(tmp_path)
    out_path = _run_build("demo", tmp_path)
    data = yaml.safe_load(out_path.read_text(encoding="utf-8"))

    api_topics = {tid: t for tid, t in (data.get("topics") or {}).items()
                  if tid.startswith("api/")}
    assert api_topics, "synthetic seed missing api/* topic"
    for tid, t in api_topics.items():
        for key in ("method", "path", "responses", "security_requirements",
                    "error_responses", "source_refs"):
            assert key in t, f"{tid} missing key {key!r} after build-handoff enrichment"
        # 200 은 responses, 400/500 은 error_responses 로 분리됨.
        assert "200" in t["responses"]
        assert set(t["error_responses"]) == {"400", "500"}
        # global security 가 operation-level fallback 으로 적용.
        assert t["security_requirements"] == [{"oauth2": ["read"]}]
        # source_refs 에 yaml 경로 (relative) 가 자동 추가됨.
        assert any("X.yaml" in ref for ref in t["source_refs"]), \
            f"{tid} source_refs missing yaml path"


def test_build_handoff_v2_error_handling_operations_from_yaml(
    tmp_path: pathlib.Path,
) -> None:
    """error-handling topic 이 yaml 의 paths × methods 를 walk 해 operations ×
    causes 매트릭스를 emit. nf-contract-check.py problem_details_matrix_complete check 의
    직접 의존.
    """
    _seed_with_yaml(tmp_path)
    out_path = _run_build("demo", tmp_path)
    data = yaml.safe_load(out_path.read_text(encoding="utf-8"))

    eh = (data.get("topics") or {}).get("error-handling")
    assert eh is not None, "error-handling topic missing"
    ops = eh.get("operations") or {}
    assert ops, "error-handling.operations enrichment empty"
    # operationId 기준 keying — yaml 의 'getOpA' 가 진입해 있어야 함.
    assert "getOpA" in ops
    info = ops["getOpA"]
    assert info["method"] == "GET"
    assert info["path"] == "/opa/{id}"
    statuses = {c["status"] for c in info["causes"]}
    assert statuses == {"400", "500"}, f"unexpected causes: {info['causes']}"
    for c in info["causes"]:
        assert c["description"], "cause description empty"
