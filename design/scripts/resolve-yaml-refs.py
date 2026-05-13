#!/usr/bin/env python3
# OpenAPI yaml 의 $ref 체인을 끝까지 추적해 wiki Data Model 섹션용 트리를 출력
"""
Usage:
    .venv/bin/python3 design/scripts/resolve-yaml-refs.py <yaml-path> [schema-name ...] [options]

옵션:
    --depth N            최대 재귀 깊이 (기본 8)
    --external-depth N   외부 spec 자료형의 추가 펼침 깊이 (기본 2)
    --no-docx-fallback   yaml 부재 시 docx Annex A 추출 fallback 비활성화

동작:
    1) input yaml 안의 schema 를 시작점으로 트리 구성.
    2) 각 $ref 해결 우선순위 — 동일 파일 → 동일 spec 폴더의 다른 yaml → 다른 spec 폴더의 yaml → 그 spec 의 docx Annex A → 미등록 leaf.
    3) 깊이/visited set 으로 cycle 방지.
    4) design/{nf}/*.md 의 Data Model 섹션에 그대로 붙일 수 있는 ```text 트리를 stdout 에 출력.

예:
    .venv/bin/python3 design/scripts/resolve-yaml-refs.py \\
      specs/29.531/TS29531_Nnssf_NSSelection.yaml AuthorizedNetworkSliceInfo
"""

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib
import re
import shutil
import subprocess
import sys
from typing import Any

import yaml


REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
EXTRACT_PY = REPO_ROOT / "design" / "scripts" / "extract.py"

# 파일명 패턴 → spec 폴더 (TS29571_CommonData.yaml → 29.571)
TS_FILENAME_RE = re.compile(r"^TS(\d{2})(\d{3})_")


@dataclasses.dataclass
class ResolvedRef:
    spec: str | None       # "29.571" or None for unresolved
    file: pathlib.Path | None
    schema_name: str
    schema: dict | None    # parsed schema dict, or None if unresolved
    note: str = ""         # additional info (e.g., "via docx Annex A")


def spec_dir_for_yaml(filename: str) -> tuple[str, pathlib.Path] | None:
    """TS29571_CommonData.yaml → ('29.571', specs/29.571/TS29571_CommonData.yaml)."""
    m = TS_FILENAME_RE.match(filename)
    if not m:
        return None
    spec = f"{m.group(1)}.{m.group(2)}"
    return spec, REPO_ROOT / "specs" / spec / filename


def load_yaml(path: pathlib.Path) -> dict:
    return yaml.safe_load(path.read_text(encoding="utf-8")) or {}


_yaml_cache: dict[pathlib.Path, dict] = {}


def cached_yaml(path: pathlib.Path) -> dict:
    if path not in _yaml_cache:
        _yaml_cache[path] = load_yaml(path)
    return _yaml_cache[path]


def find_other_yamls_in_spec_dir(spec_dir: pathlib.Path) -> list[pathlib.Path]:
    return sorted(spec_dir.glob("*.yaml"))


def extract_schema_from_docx(spec_dir: pathlib.Path, schema_name: str) -> str | None:
    """docx Annex A 의 OpenAPI yaml 텍스트에서 schema 정의 블록을 발췌. 본문은 raw 문자열로 반환."""
    docx = next(spec_dir.glob("*.docx"), None)
    if not docx:
        return None
    try:
        out = subprocess.run(
            [".venv/bin/python3", str(EXTRACT_PY), str(docx), "--max-chars", "200000"],
            capture_output=True, text=True, timeout=120, cwd=REPO_ROOT,
        )
    except Exception:
        return None
    if out.returncode != 0:
        return None
    text = out.stdout
    # OpenAPI yaml 안에서 `    SchemaName:` 으로 시작하는 블록을 찾는다 (4-space indent 가정)
    pat = re.compile(rf"(?m)^[ \t]{{4}}{re.escape(schema_name)}:\s*$")
    m = pat.search(text)
    if not m:
        return None
    # 같은 들여쓰기(4 space) 의 다음 schema 또는 더 낮은 들여쓰기까지가 본문
    start = m.start()
    rest = text[m.end():]
    nxt = re.search(r"(?m)^[ \t]{0,4}[A-Za-z_][A-Za-z0-9_]*:\s*$", rest)
    end = m.end() + (nxt.start() if nxt else len(rest))
    return text[start:end]


def resolve_ref(
    ref: str,
    current_file: pathlib.Path,
    no_docx_fallback: bool = False,
) -> ResolvedRef:
    """
    $ref 문자열을 해석해 ResolvedRef 반환.
    - "#/components/schemas/Foo"           → 같은 파일
    - "Other.yaml#/components/schemas/Foo" → 다른 파일
    """
    if ref.startswith("#/"):
        path_in_doc = ref[1:]   # "/components/schemas/Foo"
        doc = cached_yaml(current_file)
        node = doc
        for part in [p for p in path_in_doc.split("/") if p]:
            node = (node or {}).get(part)
            if node is None:
                break
        spec_match = TS_FILENAME_RE.match(current_file.name)
        spec = f"{spec_match.group(1)}.{spec_match.group(2)}" if spec_match else None
        return ResolvedRef(
            spec=spec, file=current_file,
            schema_name=path_in_doc.rsplit("/", 1)[-1],
            schema=node,
        )

    if "#" not in ref:
        return ResolvedRef(spec=None, file=None, schema_name=ref, schema=None,
                           note="malformed ref (no #)")
    file_part, frag = ref.split("#", 1)
    schema_name = frag.rsplit("/", 1)[-1]

    # 같은 spec 폴더 안에 file_part 가 있는지 먼저 (cross-yaml within spec)
    same_dir = current_file.parent / file_part
    if same_dir.is_file():
        doc = cached_yaml(same_dir)
        node = doc
        for part in [p for p in frag.split("/") if p]:
            node = (node or {}).get(part)
            if node is None:
                break
        spec_match = TS_FILENAME_RE.match(file_part)
        spec = f"{spec_match.group(1)}.{spec_match.group(2)}" if spec_match else None
        return ResolvedRef(spec=spec, file=same_dir, schema_name=schema_name, schema=node)

    # 다른 spec 폴더 매핑
    mapped = spec_dir_for_yaml(file_part)
    if mapped:
        spec, target = mapped
        if target.is_file():
            doc = cached_yaml(target)
            node = doc
            for part in [p for p in frag.split("/") if p]:
                node = (node or {}).get(part)
                if node is None:
                    break
            return ResolvedRef(spec=spec, file=target, schema_name=schema_name, schema=node)
        # yaml 없으면 같은 spec 폴더의 docx Annex A fallback
        spec_dir = target.parent
        if not no_docx_fallback and spec_dir.is_dir():
            block = extract_schema_from_docx(spec_dir, schema_name)
            if block:
                # docx 발췌 본문을 raw 로 yaml 시도. 시작 들여쓰기 4-space 를 0 으로 정규화.
                lines = block.splitlines()
                if lines:
                    # 첫 줄 "    SchemaName:" 을 "SchemaName:" 으로
                    norm = []
                    for l in lines:
                        if l.startswith("    "):
                            norm.append(l[4:])
                        else:
                            norm.append(l)
                    try:
                        parsed = yaml.safe_load("\n".join(norm))
                        if isinstance(parsed, dict) and schema_name in parsed:
                            return ResolvedRef(
                                spec=spec, file=None,
                                schema_name=schema_name,
                                schema=parsed[schema_name],
                                note="via docx Annex A",
                            )
                    except yaml.YAMLError:
                        pass
        return ResolvedRef(spec=spec, file=None, schema_name=schema_name,
                           schema=None, note="참조 규격 미등록")

    return ResolvedRef(spec=None, file=None, schema_name=schema_name,
                       schema=None, note="알 수 없는 ref 패턴")


def fmt_inline_type(schema: dict | None) -> str:
    """단순 leaf 타입을 한 줄로 요약."""
    if schema is None:
        return "?"
    if "$ref" in schema:
        return ""  # 호출 측에서 ref 해석
    t = schema.get("type")
    fmt = schema.get("format")
    enum = schema.get("enum")
    pat = schema.get("pattern")
    # 3GPP extensible enum: anyOf [{type:T, enum:[...]}, {type:T}] 패턴
    if not t:
        arms = schema.get("anyOf")
        if isinstance(arms, list) and len(arms) == 2:
            a, b = arms
            if (isinstance(a, dict) and isinstance(b, dict)
                and "$ref" not in a and "$ref" not in b
                and a.get("type") == b.get("type") and a.get("type")
                and a.get("enum") and not b.get("enum")):
                return f"{a['type']}, enum=[{' | '.join(map(str, a['enum']))}] (extensible)"
        # 단일 arm anyOf/oneOf — 그대로 펼침
        for kw in ("anyOf", "oneOf"):
            arms = schema.get(kw)
            if isinstance(arms, list) and len(arms) == 1 and isinstance(arms[0], dict) and "$ref" not in arms[0]:
                return fmt_inline_type(arms[0])
        # 다중 arm — 합집합 표기
        for kw in ("anyOf", "oneOf"):
            arms = schema.get(kw)
            if isinstance(arms, list) and arms:
                parts = []
                for sub in arms:
                    if not isinstance(sub, dict):
                        continue
                    if "$ref" in sub:
                        parts.append(sub["$ref"].rsplit("/", 1)[-1])
                    else:
                        parts.append(fmt_inline_type(sub) or "?")
                return f"{kw}(" + " | ".join(parts) + ")"
    parts = []
    if t == "array":
        items = schema.get("items", {}) or {}
        if isinstance(items, dict) and "$ref" in items:
            return f"{items['$ref'].rsplit('/', 1)[-1]}[]"
        sub = fmt_inline_type(items) or "?"
        return f"{sub}[]"
    if t:
        parts.append(t)
    if fmt:
        parts.append(f"format={fmt}")
    if pat:
        parts.append(f"pattern='{pat}'")
    if enum:
        parts.append("enum=[" + " | ".join(map(str, enum)) + "]")
    if not parts:
        if "properties" in schema:
            return "object"
        return "?"
    return ", ".join(parts)


def render(
    name: str,
    schema: dict | None,
    current_file: pathlib.Path,
    depth: int,
    max_depth: int,
    external_depth: int,
    visited: set[tuple[str, str]],
    prefix: str = "",
    is_last: bool = True,
    no_docx_fallback: bool = False,
    is_external: bool = False,
    out: list[str] | None = None,
    label_override: str | None = None,
) -> list[str]:
    if out is None:
        out = []
    branch = "└── " if is_last else "├── "
    base_label = label_override or name
    spec_match = TS_FILENAME_RE.match(current_file.name) if current_file else None
    cur_spec = f"{spec_match.group(1)}.{spec_match.group(2)}" if spec_match else "?"

    # leaf inline summary
    inline = fmt_inline_type(schema) if schema else ""
    will_render_children = bool(schema and (
        schema.get("properties")
        or any(isinstance(schema.get(k), list) and any(("$ref" in s) or ("properties" in s) for s in schema[k] if isinstance(s, dict)) for k in ("allOf", "oneOf", "anyOf"))
    ))
    line = f"{prefix}{branch}{base_label}"
    if inline and "$ref" not in (schema or {}) and not (will_render_children and inline == "object"):
        line += f": {inline}"
    out.append(line)

    if schema is None:
        return out

    # Visited check
    key = (str(current_file) if current_file else "?", name)
    if key in visited:
        out.append(f"{prefix}{'    ' if is_last else '│   '}└── (cycle: see above)")
        return out
    visited = visited | {key}

    if depth >= max_depth:
        out.append(f"{prefix}{'    ' if is_last else '│   '}└── […]")
        return out

    child_prefix = prefix + ("    " if is_last else "│   ")

    # Composition support — allOf / oneOf / anyOf
    for kw in ("allOf", "oneOf", "anyOf"):
        if kw in schema:
            for i, sub in enumerate(schema[kw]):
                if "$ref" in sub:
                    rr = resolve_ref(sub["$ref"], current_file, no_docx_fallback)
                    label = f"({kw}) {rr.schema_name}"
                    if rr.spec:
                        label += f"  [TS {rr.spec}]"
                    if rr.note:
                        label += f"  ({rr.note})"
                    if rr.schema is None:
                        out.append(f"{child_prefix}└── {label}")
                    else:
                        next_external = (rr.spec is not None and rr.file is not None and rr.file != current_file)
                        nd = max_depth if not next_external else min(max_depth, depth + 1 + external_depth)
                        render(rr.schema_name, rr.schema, rr.file or current_file,
                               depth + 1, nd, external_depth, visited, child_prefix,
                               True, no_docx_fallback, next_external or is_external, out,
                               label_override=label)

    props = schema.get("properties", {})
    required = set(schema.get("required") or [])
    items = list(props.items())

    for i, (fname, fschema) in enumerate(items):
        last = i == len(items) - 1
        marker = " *required" if fname in required else ""
        if "$ref" in fschema:
            rr = resolve_ref(fschema["$ref"], current_file, no_docx_fallback)
            label = f"{fname}: {rr.schema_name}"
            if rr.spec and rr.spec != cur_spec:
                label += f"  [TS {rr.spec}]"
            if rr.note:
                label += f"  ({rr.note})"
            label += marker
            if rr.schema is None:
                # leaf — unresolved
                out.append(f"{child_prefix}{'└── ' if last else '├── '}{label}")
            else:
                # 재귀
                next_external = (rr.spec is not None and rr.file is not None and rr.file != current_file)
                # external 진입 시 깊이 제한 추가
                nd = max_depth if not next_external else min(max_depth, depth + 1 + external_depth)
                render(rr.schema_name, rr.schema, rr.file or current_file,
                       depth + 1, nd, external_depth, visited, child_prefix,
                       last, no_docx_fallback, next_external or is_external, out,
                       label_override=label)
            continue

        if fschema.get("type") == "array":
            items_schema = fschema.get("items", {}) or {}
            if "$ref" in items_schema:
                rr = resolve_ref(items_schema["$ref"], current_file, no_docx_fallback)
                label = f"{fname}: {rr.schema_name}[]"
                if rr.spec and rr.spec != cur_spec:
                    label += f"  [TS {rr.spec}]"
                if rr.note:
                    label += f"  ({rr.note})"
                label += marker
                if rr.schema is None:
                    out.append(f"{child_prefix}{'└── ' if last else '├── '}{label}")
                else:
                    next_external = (rr.spec is not None and rr.file is not None and rr.file != current_file)
                    nd = max_depth if not next_external else min(max_depth, depth + 1 + external_depth)
                    render(rr.schema_name, rr.schema, rr.file or current_file,
                           depth + 1, nd, external_depth, visited, child_prefix,
                           last, no_docx_fallback, next_external or is_external, out,
                           label_override=label)
                continue
            inline_t = fmt_inline_type(fschema)
            out.append(f"{child_prefix}{'└── ' if last else '├── '}{fname}: {inline_t}{marker}")
            continue

        if fschema.get("type") == "object" and "additionalProperties" in fschema:
            ap = fschema["additionalProperties"]
            if isinstance(ap, dict):
                # case A: map<key, $refType>
                if "$ref" in ap:
                    rr = resolve_ref(ap["$ref"], current_file, no_docx_fallback)
                    label = f"{fname}: map<key, {rr.schema_name}>"
                    if rr.spec and rr.spec != cur_spec:
                        label += f"  [TS {rr.spec}]"
                    if rr.note:
                        label += f"  ({rr.note})"
                    out.append(f"{child_prefix}{'└── ' if last else '├── '}{label}{marker}")
                    continue
                # case B: map<key, $refType[]>
                if ap.get("type") == "array":
                    items = ap.get("items") or {}
                    if isinstance(items, dict) and "$ref" in items:
                        rr = resolve_ref(items["$ref"], current_file, no_docx_fallback)
                        label = f"{fname}: map<key, {rr.schema_name}[]>"
                        if rr.spec and rr.spec != cur_spec:
                            label += f"  [TS {rr.spec}]"
                        if rr.note:
                            label += f"  ({rr.note})"
                        out.append(f"{child_prefix}{'└── ' if last else '├── '}{label}{marker}")
                        continue
            inline_t = fmt_inline_type(ap if isinstance(ap, dict) else {"type": "any"})
            out.append(f"{child_prefix}{'└── ' if last else '├── '}{fname}: map<key, {inline_t}>{marker}")
            continue

        # nested object inline
        if fschema.get("type") == "object" and "properties" in fschema:
            out.append(f"{child_prefix}{'└── ' if last else '├── '}{fname}: object{marker}")
            render(fname, fschema, current_file,
                   depth + 1, max_depth, external_depth, visited,
                   child_prefix, last, no_docx_fallback, is_external, out,
                   label_override="(inline object)")
            continue

        inline_t = fmt_inline_type(fschema)
        out.append(f"{child_prefix}{'└── ' if last else '├── '}{fname}: {inline_t}{marker}")

    return out


def _resolve_to_node(
    schema: dict,
    current_file: pathlib.Path,
    handoff_topic_index: dict[str, str],
    visited: set[tuple[str, str]],
    no_docx_fallback: bool,
    unresolved: list[dict],
    inlined_from: str | None = None,
) -> dict:
    """schema dict → JSON node. handoff topic 은 { "topic": "<id>" }, 그 외는 inline."""
    if schema is None:
        return {"type": "unknown"}
    if "$ref" in schema:
        rr = resolve_ref(schema["$ref"], current_file, no_docx_fallback)
        ref_name = rr.schema_name
        topic_id = handoff_topic_index.get(ref_name)
        if topic_id:
            return {"topic": topic_id}
        if rr.schema is None:
            unresolved.append({"ref": schema["$ref"], "note": rr.note or ""})
            return {
                "type": "unknown",
                "_inlined_from": schema["$ref"],
                "_unresolved": True,
            }
        key = (str(rr.file or current_file), ref_name)
        if key in visited:
            return {"type": "object", "_cycle": ref_name}
        visited2 = visited | {key}
        node = _schema_node(
            rr.schema, rr.file or current_file, handoff_topic_index,
            visited2, no_docx_fallback, unresolved,
            inlined_from=schema["$ref"],
        )
        return node

    t = schema.get("type")
    if t == "array":
        items = schema.get("items") or {}
        return {
            "type": "array",
            "items": _resolve_to_node(
                items, current_file, handoff_topic_index, visited,
                no_docx_fallback, unresolved,
            ),
        }
    if t == "object" or "properties" in schema or "additionalProperties" in schema:
        return _schema_node(
            schema, current_file, handoff_topic_index, visited,
            no_docx_fallback, unresolved, inlined_from=inlined_from,
        )
    out: dict = {}
    if t:
        out["type"] = t
    for k in ("format", "pattern", "enum", "nullable"):
        if k in schema:
            out[k] = schema[k]
    if not out:
        out["type"] = "object"
    if inlined_from:
        out["_inlined_from"] = inlined_from
    return out


def _schema_node(
    schema: dict,
    current_file: pathlib.Path,
    handoff_topic_index: dict[str, str],
    visited: set[tuple[str, str]],
    no_docx_fallback: bool,
    unresolved: list[dict],
    inlined_from: str | None = None,
) -> dict:
    required = set(schema.get("required") or [])
    properties = []
    for pname, pschema in (schema.get("properties") or {}).items():
        node = _resolve_to_node(
            pschema, current_file, handoff_topic_index, visited,
            no_docx_fallback, unresolved,
        )
        entry = {
            "name": pname,
            "required": pname in required,
        }
        entry.update(node)
        properties.append(entry)
    for arm in (schema.get("allOf") or []):
        if isinstance(arm, dict) and (arm.get("properties") or arm.get("required")):
            sub = _schema_node(
                arm, current_file, handoff_topic_index, visited,
                no_docx_fallback, unresolved,
            )
            properties.extend(sub.get("properties", []))
    out = {"type": "object", "properties": properties}
    ap = schema.get("additionalProperties")
    if ap is True:
        out["additional_properties"] = {"type": "any"}
    elif isinstance(ap, dict):
        out["additional_properties"] = _resolve_to_node(
            ap, current_file, handoff_topic_index, visited,
            no_docx_fallback, unresolved,
        )
    if inlined_from:
        out["_inlined_from"] = inlined_from
    return out


def emit_json(
    *,
    yaml_path: pathlib.Path,
    root_schema_name: str,
    topic_id: str,
    nf: str,
    spec_refs: list[str],
    status: str,
    handoff_topics: list[str],
    no_docx_fallback: bool = False,
) -> dict:
    doc = cached_yaml(yaml_path)
    schemas = (doc.get("components") or {}).get("schemas") or {}
    root_schema = schemas.get(root_schema_name)
    if root_schema is None:
        raise SystemExit(f"[emit-json] root schema {root_schema_name!r} not in {yaml_path.name}")

    # handoff_topic_index — schema name → topic id (for those schemas that ARE
    # handoff topics). Root itself is excluded — its own tree is what we emit.
    index: dict[str, str] = {}
    for tid in handoff_topics:
        if "/" in tid:
            name = tid.split("/", 1)[1]
            if name != root_schema_name:
                index[name] = tid

    unresolved: list[dict] = []
    visited: set[tuple[str, str]] = {(str(yaml_path), root_schema_name)}
    body = _schema_node(
        root_schema, yaml_path, index, visited, no_docx_fallback, unresolved,
    )

    dependencies: set[str] = set()

    def _walk(node: object) -> None:
        if isinstance(node, dict):
            if "topic" in node and isinstance(node["topic"], str):
                dependencies.add(node["topic"])
            for v in node.values():
                _walk(v)
        elif isinstance(node, list):
            for v in node:
                _walk(v)

    _walk(body)

    return {
        "schema_version": "data-model-v1",
        "nf": nf,
        "topic_id": topic_id,
        "status": status,
        "source": {
            "spec_refs": spec_refs,
            "openapi_refs": [f"#/components/schemas/{root_schema_name}"],
            "source_yaml": str(yaml_path.relative_to(REPO_ROOT))
                if yaml_path.is_absolute() and yaml_path.is_relative_to(REPO_ROOT)
                else str(yaml_path),
        },
        "root_schema": root_schema_name,
        "fields": body.get("properties", []),
        "dependencies": sorted(dependencies),
        "unresolved_refs": unresolved,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("yaml_path", type=pathlib.Path)
    parser.add_argument("schemas", nargs="*",
                        help="해석할 schema 이름들. 비우면 components/schemas 의 모든 항목.")
    parser.add_argument("--depth", type=int, default=8)
    parser.add_argument("--external-depth", type=int, default=2)
    parser.add_argument("--no-docx-fallback", action="store_true")
    parser.add_argument("--emit-json", action="store_true",
                        help="JSON machine artifact 으로 stdout 출력 (markdown 트리 대신)")
    parser.add_argument("--topic-id", default="")
    parser.add_argument("--nf", default="")
    parser.add_argument("--spec-ref", action="append", default=[])
    parser.add_argument("--status", default="canonical")
    parser.add_argument("--handoff-topics", action="append", default=[])
    args = parser.parse_args()

    yaml_path = args.yaml_path.resolve()
    if not yaml_path.is_file():
        sys.exit(f"[resolve-yaml-refs] 파일 없음: {yaml_path}")

    doc = cached_yaml(yaml_path)
    schemas = (doc.get("components") or {}).get("schemas") or {}

    if args.schemas:
        targets = [(name, schemas.get(name)) for name in args.schemas]
    else:
        targets = list(schemas.items())

    spec_match = TS_FILENAME_RE.match(yaml_path.name)
    cur_spec = f"{spec_match.group(1)}.{spec_match.group(2)}" if spec_match else "?"

    if args.emit_json:
        if not args.schemas or not args.topic_id or not args.nf:
            sys.exit("[emit-json] requires <schema>, --topic-id, --nf")
        payload = emit_json(
            yaml_path=yaml_path,
            root_schema_name=args.schemas[0],
            topic_id=args.topic_id,
            nf=args.nf,
            spec_refs=args.spec_ref,
            status=args.status,
            handoff_topics=args.handoff_topics,
            no_docx_fallback=args.no_docx_fallback,
        )
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return

    print("```text")
    print(f"# {yaml_path.name}  [TS {cur_spec}]")
    for name, schema in targets:
        if schema is None:
            print(f"\n{name}  (스키마 없음)")
            continue
        print()
        rendered = render(name, schema, yaml_path, depth=0,
                          max_depth=args.depth, external_depth=args.external_depth,
                          visited=set(), prefix="", is_last=True,
                          no_docx_fallback=args.no_docx_fallback)
        # remove the leading "└── " from root for cleaner header
        if rendered:
            rendered[0] = rendered[0].lstrip("└─ ").lstrip()
        print("\n".join(rendered))
    print("```")


if __name__ == "__main__":
    main()
