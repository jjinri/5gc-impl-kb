#!/usr/bin/env python3
"""Generate the initial handoff seed from an NF manifest and primary OpenAPI YAML.

This intentionally replaces the old manual `_handoff.yaml` authoring step:
`/nf-spec-discover` discovers specs, then this script creates `design/<nf>/_contract_seed.yaml`
so `/nf-contract-build` can assemble `handoff/<nf>/contract.yaml`.

Default behavior is conservative: if a seed already exists, preserve it unless
`--force` is passed. Existing seeds may contain scoped MVP decisions and manual
status choices that a blind generator must not erase.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import sys
from typing import Any

import yaml


REPO_ROOT = pathlib.Path(
    os.environ.get("FIVEGC_REPO_ROOT")
    or pathlib.Path(__file__).resolve().parent.parent.parent
).resolve()


def _load_yaml(path: pathlib.Path) -> dict[str, Any]:
    return yaml.safe_load(path.read_text(encoding="utf-8")) or {}


def _ref_name(ref: str) -> str:
    return ref.rsplit("/", 1)[-1]


_SCHEMA_REF_RE = "/components/schemas/"


def _schema_refs(obj: Any) -> list[str]:
    refs: list[str] = []
    if isinstance(obj, dict):
        ref = obj.get("$ref")
        if isinstance(ref, str) and _SCHEMA_REF_RE in ref:
            refs.append(_ref_name(ref))
        for value in obj.values():
            refs.extend(_schema_refs(value))
    elif isinstance(obj, list):
        for item in obj:
            refs.extend(_schema_refs(item))
    return refs


def _operation_schemas(op: dict[str, Any]) -> list[str]:
    refs: list[str] = []
    refs.extend(_schema_refs(op.get("requestBody")))
    refs.extend(_schema_refs(op.get("responses")))
    refs.extend(_schema_refs(op.get("parameters")))
    # stable de-dup preserving first discovery order
    return list(dict.fromkeys(refs))


def _iter_operations(yaml_path: pathlib.Path):
    doc = _load_yaml(yaml_path)
    for path, item in (doc.get("paths") or {}).items():
        if not isinstance(item, dict):
            continue
        for method, op in item.items():
            if method.lower() not in {"get", "post", "put", "patch", "delete", "options"}:
                continue
            if not isinstance(op, dict):
                continue
            op_id = op.get("operationId")
            if isinstance(op_id, str) and op_id:
                yield path, method.lower(), op_id, _operation_schemas(op)


def _primary_yaml_paths(manifest: dict[str, Any]) -> list[pathlib.Path]:
    spec = str(manifest.get("primary_spec") or "")
    yamls = ((manifest.get("primary_files") or {}).get("yamls") or [])
    out: list[pathlib.Path] = []
    for name in yamls:
        path = REPO_ROOT / "specs" / spec / str(name)
        if path.is_file():
            out.append(path)
    return out


def _build_seed(nf: str, manifest: dict[str, Any], yaml_paths: list[pathlib.Path]) -> dict[str, Any]:
    categories = {
        "interface": {"status": "draft", "layout": "single-file"},
        "error-handling": {"status": "draft", "layout": "single-file"},
        "api": {"status": "draft", "layout": "directory"},
        "data-model": {"status": "draft", "layout": "directory"},
        "module-decomposition": {"status": "draft", "layout": "directory"},
        "service-scenarios": {"status": "draft", "layout": "directory"},
        "behavior-state": {"status": "draft", "layout": "directory"},
        "failure-policy": {"status": "draft", "layout": "single-file"},
        "configuration": {"status": "draft", "layout": "single-file"},
        "persistence": {"status": "draft", "layout": "single-file"},
        "test-matrix": {"status": "draft", "layout": "single-file"},
        "work-plan": {"status": "draft", "layout": "single-file"},
        "cross-nf": {"status": "draft", "layout": "single-file"},
    }
    topics: dict[str, Any] = {
        "interface": {"status": "draft", "file": f"design/{nf}/contract/interface.md"},
        "error-handling": {"status": "draft", "file": f"design/{nf}/contract/error-handling.md"},
    }
    tasks: dict[str, Any] = {}

    discovered_models: dict[str, None] = {}
    for yaml_path in yaml_paths:
        for path, method, op_id, schemas in _iter_operations(yaml_path):
            api_topic = f"api/{op_id}"
            model_topics = [f"data-model/{name}" for name in schemas]
            topics[api_topic] = {
                "status": "draft",
                "file": f"design/{nf}/contract/api/{op_id}.md",
                "method": method.upper(),
                "path": path,
                "depends_on": model_topics,
                "related": ["interface"],
            }
            for name in schemas:
                discovered_models.setdefault(name, None)
            task_id = f"{nf}-api-{op_id.replace('_', '-').lower()}"
            tasks[task_id] = {
                "phase": "02-api-implementation",
                "goal": f"Implement {op_id} handler",
                "read": [api_topic, *model_topics, "interface", "error-handling"],
                "produces": [f"<impl>/api/{op_id}.*", f"<test>/test_{op_id}.*"],
                "blocked_by": [f"{api_topic}.status not in [canonical, handoff_ready]"],
                "acceptance": ["contract markdown/json reviewed before implementation"],
            }

    for name in discovered_models:
        topics[f"data-model/{name}"] = {
            "status": "draft",
            "file": f"design/{nf}/contract/data-model/{name}.md",
            "machine_file": f"design/{nf}/contract/data-model/{name}.json",
        }

    primary_spec = str(manifest.get("primary_spec") or "")
    sources = {}
    if primary_spec:
        docx = ((manifest.get("primary_files") or {}).get("docx"))
        if docx:
            sources[f"TS {primary_spec}"] = f"specs/{primary_spec}/{docx}"

    return {"nf": nf, "categories": categories, "topics": topics, "tasks": tasks, "sources": sources}


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate design/<nf>/_contract_seed.yaml")
    parser.add_argument("nf", help="NF folder name, e.g. nssf")
    parser.add_argument("--force", action="store_true", help="regenerate even when a seed already exists")
    args = parser.parse_args()

    nf = args.nf.lower()
    design_dir = REPO_ROOT / "design" / nf
    manifest_path = design_dir / "_manifest.yaml"
    seed_path = design_dir / "_contract_seed.yaml"
    if not manifest_path.is_file():
        sys.exit(f"[nf-seed-gen] {manifest_path.relative_to(REPO_ROOT)} 없음")

    if seed_path.is_file() and not args.force:
        seed = _load_yaml(seed_path)
        print(
            f"[nf-seed-gen] preserved existing {seed_path.relative_to(REPO_ROOT)} "
            f"(categories={len(seed.get('categories') or {})} "
            f"topics={len(seed.get('topics') or {})} tasks={len(seed.get('tasks') or {})}; "
            f"use --force to regenerate)",
            file=sys.stderr,
        )
        return

    manifest = _load_yaml(manifest_path)
    if not (((manifest.get("status") or {}).get("ready_for_build")) is True):
        sys.exit("[nf-seed-gen] manifest status.ready_for_build != true")
    yaml_paths = _primary_yaml_paths(manifest)
    if not yaml_paths:
        sys.exit("[nf-seed-gen] primary OpenAPI yaml 없음")

    seed = _build_seed(nf, manifest, yaml_paths)
    design_dir.mkdir(parents=True, exist_ok=True)
    seed_path.write_text(
        yaml.dump(seed, allow_unicode=True, default_flow_style=False, sort_keys=False),
        encoding="utf-8",
    )
    print(
        f"[nf-seed-gen] wrote {seed_path.relative_to(REPO_ROOT)} "
        f"(categories={len(seed['categories'])} topics={len(seed['topics'])} tasks={len(seed['tasks'])})",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
