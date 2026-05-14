from __future__ import annotations

import os
import pathlib
import subprocess

import yaml


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "nf-seed-gen.py"


def _setup(tmp_path: pathlib.Path, openapi: dict) -> pathlib.Path:
    nf = "demo"
    spec = "99.999"
    yaml_name = "TS99999_Demo.yaml"
    (tmp_path / "specs" / spec).mkdir(parents=True)
    (tmp_path / "specs" / spec / yaml_name).write_text(
        yaml.safe_dump(openapi, sort_keys=False), encoding="utf-8"
    )
    docx_name = "99999-aa0.docx"
    (tmp_path / "specs" / spec / docx_name).write_bytes(b"PK\x05\x06" + b"\x00" * 18)
    nf_dir = tmp_path / "design" / nf
    nf_dir.mkdir(parents=True)
    manifest = {
        "nf": nf,
        "primary_spec": spec,
        "primary_files": {"docx": docx_name, "yamls": [yaml_name]},
        "deps": [],
        "status": {"present_count": 0, "in_scope_count": 0, "ready_for_build": True, "missing_priority": []},
        "overrides": {"exclude": [], "add": []},
    }
    (nf_dir / "_manifest.yaml").write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )
    return nf_dir


def _run(tmp_path: pathlib.Path) -> dict:
    env = {**os.environ, "FIVEGC_REPO_ROOT": str(tmp_path)}
    out = subprocess.run(
        [str(REPO / ".venv" / "bin" / "python3"), str(SCRIPT), "demo"],
        capture_output=True, text=True, cwd=tmp_path, timeout=30, env=env,
    )
    assert out.returncode == 0, out.stderr
    seed_path = tmp_path / "design" / "demo" / "_contract_seed.yaml"
    return yaml.safe_load(seed_path.read_text(encoding="utf-8"))


def test_response_ref_does_not_become_schema_topic(tmp_path):
    """OpenAPI `responses:` 의 `$ref: '#/components/responses/400'` 은
    HTTP 상태 코드 응답 참조이지 schema 가 아니다. data-model 토픽으로 들어가면 안 됨."""
    openapi = {
        "openapi": "3.0.0",
        "info": {"title": "demo", "version": "1.0"},
        "paths": {
            "/foo": {
                "get": {
                    "operationId": "GetFoo",
                    "responses": {
                        "200": {
                            "description": "OK",
                            "content": {
                                "application/json": {
                                    "schema": {"$ref": "#/components/schemas/FooOutput"}
                                }
                            },
                        },
                        "400": {"$ref": "#/components/responses/400"},
                        "404": {"$ref": "#/components/responses/404"},
                        "default": {"$ref": "#/components/responses/default"},
                    },
                }
            }
        },
        "components": {
            "schemas": {
                "FooOutput": {"type": "object"},
            },
            "responses": {
                "400": {"description": "Bad Request"},
                "404": {"description": "Not Found"},
                "default": {"description": "Default"},
            },
        },
    }
    _setup(tmp_path, openapi)
    seed = _run(tmp_path)

    topics = set(seed.get("topics", {}).keys())
    assert "data-model/FooOutput" in topics
    assert "api/GetFoo" in topics
    for noise in ("data-model/400", "data-model/404", "data-model/default"):
        assert noise not in topics, f"{noise} 가 schema 토픽으로 잘못 들어갔다"

    op_depends = seed["topics"]["api/GetFoo"]["depends_on"]
    assert op_depends == ["data-model/FooOutput"], op_depends


def test_request_body_schema_ref_kept(tmp_path):
    """`requestBody.content.<media>.schema.$ref` 는 schema 라 토픽 등록 되어야 한다."""
    openapi = {
        "openapi": "3.0.0",
        "info": {"title": "demo", "version": "1.0"},
        "paths": {
            "/bar": {
                "post": {
                    "operationId": "PostBar",
                    "requestBody": {
                        "content": {
                            "application/json": {
                                "schema": {"$ref": "#/components/schemas/BarInput"}
                            }
                        }
                    },
                    "responses": {"200": {"description": "OK"}},
                }
            }
        },
        "components": {"schemas": {"BarInput": {"type": "object"}}},
    }
    _setup(tmp_path, openapi)
    seed = _run(tmp_path)

    assert "data-model/BarInput" in seed["topics"]
    assert seed["topics"]["api/PostBar"]["depends_on"] == ["data-model/BarInput"]
