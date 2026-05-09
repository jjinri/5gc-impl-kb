#!/usr/bin/env python3
# OpenAPI yaml schema 를 C struct 로 변환 + gcc -fsyntax-only 검증
"""
Usage:
    .venv/bin/python3 scripts/yaml-to-c.py <nf> [--out <path>]

알고리즘.
1. kb/<nf>/_manifest.yaml → primary_spec, primary_files.yamls.
2. 각 primary yaml 의 paths.*.requestBody / responses[2xx].content.*.schema.$ref
   를 root set 으로 수집.
3. root 부터 chain 따라가며 모든 referenced schema 수집 (cross-spec 포함, depth 8).
4. 토폴로지 정렬 후 .c 헤더 emit — typedef/struct/enum.
5. gcc -fsyntax-only -x c <out> 으로 검증. exit code 그대로 반환.

mapping 규칙은 PoC 수준 — README/CLAUDE.md 의 "yaml → C 매핑 규칙" 표 참고.
완전한 C 라이브러리 생성이 아니라 "yaml schema 가 깔끔히 떨어지는지" 의 검증이 목적.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import subprocess
import sys
from typing import Any

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent

C_KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline",
    "int", "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
    "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
    "bool", "true", "false",
}

MAX_DEPTH = 8


# ─── yaml 로딩 + ref 해소 ────────────────────────────────────────────

class SchemaLoader:
    """yaml 파일들을 lazy 로딩 + $ref 해소."""

    def __init__(self, repo: pathlib.Path):
        self.repo = repo
        self._yaml_cache: dict[pathlib.Path, dict] = {}
        # known_yaml_index — basename → 절대경로. specs/*/*.yaml 전체 인덱스
        self.known_yaml: dict[str, pathlib.Path] = {}
        for spec_dir in (repo / "specs").iterdir() if (repo / "specs").is_dir() else []:
            if not spec_dir.is_dir():
                continue
            for y in spec_dir.glob("*.yaml"):
                self.known_yaml[y.name] = y

    def load_yaml(self, path: pathlib.Path) -> dict:
        if path not in self._yaml_cache:
            try:
                self._yaml_cache[path] = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
            except (yaml.YAMLError, FileNotFoundError):
                self._yaml_cache[path] = {}
        return self._yaml_cache[path]

    def resolve_ref(self, ref: str, base_yaml: pathlib.Path) -> tuple[str, dict | None, pathlib.Path]:
        """$ref 문자열 → (schema name, schema dict or None, source yaml path).

        ref 형식.
          '#/components/schemas/X'                — 같은 yaml 내부
          'TS29571_CommonData.yaml#/components/schemas/X' — 다른 yaml
        해소 못 하면 (name, None, base_yaml).
        """
        if "#" in ref:
            file_part, frag = ref.split("#", 1)
        else:
            file_part, frag = "", ref
        # frag 의 마지막 path segment 가 schema name
        name = frag.rsplit("/", 1)[-1]
        if file_part:
            target_path = self.known_yaml.get(file_part.strip())
            if target_path is None:
                return name, None, base_yaml
        else:
            target_path = base_yaml
        ydata = self.load_yaml(target_path)
        schemas = (ydata.get("components") or {}).get("schemas") or {}
        return name, schemas.get(name), target_path


# ─── schema 수집 (chain 추적) ────────────────────────────────────────

def collect_root_schemas(yaml_path: pathlib.Path) -> list[tuple[str, str]]:
    """primary yaml 의 paths 에서 root schema 들의 ref 추출.
    수집 대상.
      - operation.requestBody.content.*.schema.$ref
      - operation.responses[2xx].content.*.schema.$ref
      - operation.parameters[*].content.*.schema.$ref  (query body)
      - operation.parameters[*].schema.$ref            (단순 param)
      - operation.callbacks.*.*.*.requestBody.content.*.schema.$ref (이벤트 알림)
    반환 — [(ref_string, source_yaml_basename)] 중복 제거.
    """
    try:
        d = yaml.safe_load(yaml_path.read_text(encoding="utf-8")) or {}
    except (yaml.YAMLError, FileNotFoundError):
        return []
    out: list[tuple[str, str]] = []
    seen: set[str] = set()

    def add(ref: str | None) -> None:
        if not ref or ref in seen:
            return
        seen.add(ref)
        out.append((ref, yaml_path.name))

    def add_from_schema(s: Any) -> None:
        """schema dict 안의 $ref 또는 oneOf/array.items.$ref 등 surface ref 수집."""
        if not isinstance(s, dict):
            return
        if isinstance(s.get("$ref"), str):
            add(s["$ref"])
            return
        # oneOf/anyOf — 분기 각각의 ref
        for k in ("oneOf", "anyOf", "allOf"):
            for sub in s.get(k) or []:
                add_from_schema(sub)
        # array items
        if isinstance(s.get("items"), dict):
            add_from_schema(s["items"])

    def add_from_content(content: Any) -> None:
        if not isinstance(content, dict):
            return
        for ct, c in content.items():
            if isinstance(c, dict) and isinstance(c.get("schema"), dict):
                add_from_schema(c["schema"])

    def visit_op(op: dict) -> None:
        if not isinstance(op, dict):
            return
        rb = op.get("requestBody") or {}
        if isinstance(rb, dict):
            add_from_content(rb.get("content"))
        for code, resp in (op.get("responses") or {}).items():
            if not str(code).startswith("2") or not isinstance(resp, dict):
                continue
            add_from_content(resp.get("content"))
        for prm in op.get("parameters") or []:
            if not isinstance(prm, dict):
                continue
            add_from_content(prm.get("content"))
            sch = prm.get("schema")
            if isinstance(sch, dict):
                add_from_schema(sch)
        for cbn, cb in (op.get("callbacks") or {}).items():
            if not isinstance(cb, dict):
                continue
            for cpath, cops in cb.items():
                if not isinstance(cops, dict):
                    continue
                for cm, copp in cops.items():
                    visit_op(copp)

    for path, ops in (d.get("paths") or {}).items():
        if not isinstance(ops, dict):
            continue
        for method, op in ops.items():
            visit_op(op)
    return out


def collect_chain(loader: SchemaLoader, roots: list[tuple[str, pathlib.Path]]) -> dict[str, dict]:
    """roots 부터 chain 따라가 모든 schema 수집. 반환 {schema_name: {schema, source_yaml}}.
    같은 이름이 여러 yaml 에 있는 경우 가장 먼저 만난 정의를 채택.
    """
    collected: dict[str, dict] = {}
    queue: list[tuple[str, pathlib.Path, int]] = [(r, b, 0) for r, b in roots]

    def walk_for_refs(node: Any) -> list[str]:
        """schema dict 안에 등장하는 모든 $ref 문자열 수집."""
        refs: list[str] = []
        if isinstance(node, dict):
            for k, v in node.items():
                if k == "$ref" and isinstance(v, str):
                    refs.append(v)
                else:
                    refs.extend(walk_for_refs(v))
        elif isinstance(node, list):
            for v in node:
                refs.extend(walk_for_refs(v))
        return refs

    while queue:
        ref, base_yaml, depth = queue.pop(0)
        if depth > MAX_DEPTH:
            continue
        name, schema, source_yaml = loader.resolve_ref(ref, base_yaml)
        if name in collected:
            continue
        collected[name] = {
            "schema": schema,            # None 이면 unresolved
            "source_yaml": source_yaml,
            "ref": ref,
        }
        if schema is None:
            continue
        for sub_ref in walk_for_refs(schema):
            queue.append((sub_ref, source_yaml, depth + 1))
    return collected


# ─── 의존성 그래프 + 토폴로지 정렬 ──────────────────────────────────

def schema_deps(schema: dict | None) -> list[str]:
    """schema 가 직접 참조하는 다른 schema 이름들 (allOf/oneOf/anyOf/array/object 안 포함)."""
    if not isinstance(schema, dict):
        return []
    out: list[str] = []

    def walk(node: Any) -> None:
        if isinstance(node, dict):
            ref = node.get("$ref")
            if isinstance(ref, str):
                out.append(ref.rsplit("/", 1)[-1])
            for k, v in node.items():
                if k == "$ref":
                    continue
                walk(v)
        elif isinstance(node, list):
            for v in node:
                walk(v)

    walk(schema)
    return out


def topo_sort(collected: dict[str, dict]) -> list[str]:
    """deps 우선 정렬. 순환은 보존 — 후방 typedef forward decl 은 안 쓰고 그대로 둠."""
    visited: set[str] = set()
    on_stack: set[str] = set()
    out: list[str] = []

    def visit(name: str) -> None:
        if name in visited or name in on_stack:
            return
        on_stack.add(name)
        info = collected.get(name)
        if info is not None:
            for dep in schema_deps(info.get("schema")):
                if dep in collected and dep != name:
                    visit(dep)
        on_stack.discard(name)
        visited.add(name)
        out.append(name)

    for n in collected:
        visit(n)
    return out


# ─── name 정규화 ────────────────────────────────────────────────────

def safe_typedef_name(schema_name: str) -> str:
    """schema 이름 → C typedef. SchemaName_t. 충돌 회피."""
    # 비-alnum 제거
    base = re.sub(r"[^A-Za-z0-9_]", "_", schema_name)
    if not base or not (base[0].isalpha() or base[0] == "_"):
        base = "S_" + base
    if base.lower() in C_KEYWORDS:
        base = "S_" + base
    return base + "_t"


def safe_field_name(prop: str) -> str:
    name = re.sub(r"[^A-Za-z0-9_]", "_", prop)
    if not name:
        name = "_unnamed"
    if not (name[0].isalpha() or name[0] == "_"):
        name = "_" + name
    if name in C_KEYWORDS:
        name = name + "_"
    return name


def safe_enum_value(schema_name: str, value: str) -> str:
    base = re.sub(r"[^A-Za-z0-9_]", "_", str(value)).upper()
    if not base:
        base = "EMPTY"
    if base[0].isdigit():
        base = "V_" + base
    prefix = re.sub(r"[^A-Za-z0-9_]", "_", schema_name).upper()
    return f"{prefix}_{base}"


# ─── yaml → C 매핑 ──────────────────────────────────────────────────

class Emitter:
    def __init__(self, collected: dict[str, dict], order: list[str]):
        self.collected = collected
        self.order = order
        self.typedef_for: dict[str, str] = {n: safe_typedef_name(n) for n in collected}
        self.lines: list[str] = []
        # schema name → 보조 array typedef 발급용 카운터 (중복 방지)
        self._inline_counter = 0

    def emit_all(self) -> str:
        self.lines.append("/* Auto-generated by scripts/yaml-to-c.py — do not edit. */")
        self.lines.append("/* PoC: yaml schema → C struct/typedef/enum. */")
        self.lines.append("")
        self.lines.append("#include <stdint.h>")
        self.lines.append("#include <stddef.h>")
        self.lines.append("#include <stdbool.h>")
        self.lines.append("")
        # 1) forward decls — 모든 schema 의 typedef 이름을 미리 struct 태그로 선언
        self.lines.append("/* forward decls */")
        for name in self.order:
            tname = self.typedef_for[name]
            tag = tname[:-2]  # strip _t for struct tag
            self.lines.append(f"typedef struct {tag} {tname};")
        self.lines.append("")
        # 2) 본 정의
        for name in self.order:
            self.emit_schema(name)
            self.lines.append("")
        return "\n".join(self.lines) + "\n"

    # ── schema 정의 emit ───
    def emit_schema(self, name: str) -> None:
        info = self.collected[name]
        schema = info.get("schema")
        tname = self.typedef_for[name]
        tag = tname[:-2]
        if schema is None:
            self.lines.append(f"/* unresolved ref — {info.get('ref')!r} */")
            self.lines.append(f"/* TODO: unresolved cross-spec ref */")
            self.lines.append(f"struct {tag} {{ int _unresolved; }};")
            return

        # allOf — properties 합성 (object 로 취급)
        merged = self._merge_allof(schema)

        # enum 우선 처리 (top-level)
        enum_vals = merged.get("enum")
        if enum_vals and merged.get("type", "string") == "string":
            self.lines.append(f"/* {name} — enum string */")
            self.lines.append(f"enum {tag}_enum {{")
            for i, v in enumerate(enum_vals):
                comma = "," if i < len(enum_vals) - 1 else ""
                self.lines.append(f"    {safe_enum_value(name, v)}{comma}")
            self.lines.append(f"}};")
            # typedef 이미 forward decl 로 struct 로 선언됐으므로 wrapping struct 로 통일
            self.lines.append(f"struct {tag} {{ enum {tag}_enum value; }};")
            return

        # oneOf / anyOf — variant tag + union
        variants = merged.get("oneOf") or merged.get("anyOf")
        if variants:
            self._emit_variant(name, tag, variants)
            return

        # type=array (top-level array schema, 예 PatchDocument)
        if merged.get("type") == "array" or "items" in merged:
            items = merged.get("items") or {}
            item_type = self._render_type(items, hint=name + "Item")
            self.lines.append(f"/* {name} — top-level array */")
            self.lines.append(f"struct {tag} {{")
            self.lines.append(f"    {item_type} *items;")
            self.lines.append(f"    size_t count;")
            self.lines.append(f"}};")
            return

        # type=string/integer/number/boolean — scalar typedef wrapping
        scalar = self._scalar_type(merged)
        if scalar is not None and merged.get("type") != "object" and "properties" not in merged:
            self.lines.append(f"/* {name} — scalar wrapper */")
            self.lines.append(f"struct {tag} {{ {scalar} value; }};")
            return

        # type=object 또는 properties 보유 — struct 로 emit
        self._emit_object(name, tag, merged)

    # ── helpers ───

    def _merge_allof(self, schema: dict) -> dict:
        """allOf 의 properties/required 를 단순 합성 (top-level)."""
        if "allOf" not in schema:
            return schema
        merged: dict = {k: v for k, v in schema.items() if k != "allOf"}
        merged.setdefault("properties", {})
        merged.setdefault("required", [])
        merged["type"] = "object"
        for part in schema["allOf"]:
            if not isinstance(part, dict):
                continue
            if "$ref" in part:
                # ref 의 정의를 끌어와서 inline 합성
                ref_name = part["$ref"].rsplit("/", 1)[-1]
                ref_info = self.collected.get(ref_name)
                if ref_info and isinstance(ref_info.get("schema"), dict):
                    sub = self._merge_allof(ref_info["schema"])
                else:
                    continue
            else:
                sub = self._merge_allof(part)
            for k, v in (sub.get("properties") or {}).items():
                merged["properties"].setdefault(k, v)
            for r in sub.get("required") or []:
                if r not in merged["required"]:
                    merged["required"].append(r)
        return merged

    def _scalar_type(self, schema: dict) -> str | None:
        t = schema.get("type")
        fmt = schema.get("format")
        if t == "integer":
            if fmt == "int64":
                return "int64_t"
            return "int32_t"
        if t == "number":
            if fmt == "float":
                return "float"
            return "double"
        if t == "string":
            return "char *"
        if t == "boolean":
            return "bool"
        return None

    def _render_type(self, schema: dict, hint: str = "") -> str:
        """schema → 멤버 자료형 문자열 (struct 안에서 그대로 사용)."""
        if not isinstance(schema, dict):
            return "void *"
        if "$ref" in schema:
            ref_name = schema["$ref"].rsplit("/", 1)[-1]
            return self.typedef_for.get(ref_name, "void *")
        # allOf 단일 ref 처리
        if "allOf" in schema:
            # allOf inline → 가장 단순하게 첫 ref 의 typedef 사용
            for part in schema["allOf"]:
                if isinstance(part, dict) and "$ref" in part:
                    ref_name = part["$ref"].rsplit("/", 1)[-1]
                    return self.typedef_for.get(ref_name, "void *")
            return "void *"
        if "oneOf" in schema or "anyOf" in schema:
            # inline variant — PoC 단순화: void *
            return "void *"
        t = schema.get("type")
        if t == "array":
            items = schema.get("items") or {}
            inner = self._render_type(items, hint=hint + "Item")
            # inline array struct (anonymous) — 호출측에서 멤버 정의로 사용 어려움
            # 그래서 그냥 inner * 로 반환. count 는 array 멤버에서 별도 처리.
            return inner + " *"
        scalar = self._scalar_type(schema)
        if scalar:
            return scalar
        if t == "object" or "properties" in schema:
            # inline object — PoC 에선 void * 로 처리 (top-level schema 화 권장)
            return "void *"
        return "void *"

    def _emit_object(self, name: str, tag: str, schema: dict) -> None:
        props = schema.get("properties") or {}
        required = set(schema.get("required") or [])
        self.lines.append(f"/* {name} */")
        self.lines.append(f"struct {tag} {{")
        if not props:
            self.lines.append(f"    int _empty;")
        for prop_name, prop_schema in props.items():
            field = safe_field_name(prop_name)
            if not isinstance(prop_schema, dict):
                self.lines.append(f"    void *{field};")
                continue
            is_array = prop_schema.get("type") == "array"
            nullable = prop_schema.get("nullable", False)
            is_required = prop_name in required

            if is_array:
                items = prop_schema.get("items") or {}
                item_t = self._render_type(items, hint=name + "_" + prop_name)
                # array 는 항상 (items*, count) 로 표현
                self.lines.append(f"    {item_t} *{field};")
                self.lines.append(f"    size_t {field}_count;")
                continue

            base_t = self._render_type(prop_schema, hint=name + "_" + prop_name)
            # optional 또는 nullable 은 pointer 로 (단 이미 pointer 인 char* 는 그대로)
            is_pointer_already = base_t.endswith("*")
            if (not is_required) or nullable:
                if is_pointer_already:
                    self.lines.append(f"    {base_t}{field};")
                else:
                    self.lines.append(f"    {base_t} *{field};")
            else:
                if is_pointer_already:
                    self.lines.append(f"    {base_t}{field};")
                else:
                    self.lines.append(f"    {base_t} {field};")
        self.lines.append(f"}};")

    def _emit_variant(self, name: str, tag: str, variants: list) -> None:
        """oneOf / anyOf — variant_tag + union { ... } payload."""
        self.lines.append(f"/* {name} — oneOf/anyOf variant (PoC simplification) */")
        self.lines.append(f"struct {tag} {{")
        self.lines.append(f"    int variant_tag;")
        # variant 가 모두 같은 scalar (예: NssfEventType — string + string-enum) 면
        # union 이 결국 char * 하나로 떨어진다. 그래도 union 으로 나열해서 컴파일은 통과.
        self.lines.append(f"    union {{")
        used_names: set[str] = set()
        for i, v in enumerate(variants):
            if not isinstance(v, dict):
                continue
            t = self._render_type(v, hint=f"{name}_v{i}")
            mname = f"v{i}"
            if mname in used_names:
                continue
            used_names.add(mname)
            if t.endswith("*"):
                self.lines.append(f"        {t}{mname};")
            else:
                self.lines.append(f"        {t} {mname};")
        if not used_names:
            self.lines.append(f"        int _empty;")
        self.lines.append(f"    }} payload;")
        self.lines.append(f"}};")


# ─── 메인 ────────────────────────────────────────────────────────────

def build_for_nf(nf: str, out_path: pathlib.Path) -> tuple[bool, str, int]:
    """returns (compiled_ok, message, schema_count)."""
    nf_dir = REPO / "kb" / nf
    manifest_path = nf_dir / "_manifest.yaml"
    if not manifest_path.is_file():
        return False, f"kb/{nf}/_manifest.yaml 부재", 0
    manifest = yaml.safe_load(manifest_path.read_text(encoding="utf-8")) or {}
    primary_spec = manifest.get("primary_spec")
    primary_yamls = ((manifest.get("primary_files") or {}).get("yamls")) or []
    if not primary_spec or not primary_yamls:
        return False, "manifest 의 primary_spec / primary_files.yamls 없음", 0

    spec_dir = REPO / "specs" / primary_spec
    loader = SchemaLoader(REPO)

    # roots
    roots: list[tuple[str, pathlib.Path]] = []
    seen_refs: set[tuple[str, str]] = set()
    for yname in primary_yamls:
        ypath = spec_dir / yname
        if not ypath.is_file():
            continue
        for ref, src in collect_root_schemas(ypath):
            key = (ref, str(ypath))
            if key in seen_refs:
                continue
            seen_refs.add(key)
            roots.append((ref, ypath))

    if not roots:
        return False, "root schema 0개 (primary yaml 의 paths 에서 ref 찾지 못함)", 0

    collected = collect_chain(loader, roots)
    order = topo_sort(collected)
    emitter = Emitter(collected, order)
    c_text = emitter.emit_all()

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(c_text, encoding="utf-8")

    # gcc -fsyntax-only
    proc = subprocess.run(
        ["gcc", "-fsyntax-only", "-x", "c", str(out_path)],
        capture_output=True, text=True,
    )
    if proc.returncode == 0:
        return True, f"PASS — {len(collected)} schemas, {out_path.stat().st_size}B", len(collected)
    else:
        return False, (
            f"FAIL — gcc 에러 (rc={proc.returncode})\n"
            f"--- stderr ---\n{proc.stderr}"
        ), len(collected)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("nf", help="NF 이름 (예 nssf)")
    parser.add_argument("--out", default=None, help="산출 .c 경로 (기본 /tmp/<nf>_schema.c)")
    args = parser.parse_args()

    nf = args.nf.lower()
    out_path = pathlib.Path(args.out) if args.out else pathlib.Path(f"/tmp/{nf}_schema.c")

    ok, msg, n = build_for_nf(nf, out_path)
    print(f"[yaml-to-c] {nf} — {msg}")
    print(f"[yaml-to-c] 산출 — {out_path}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
