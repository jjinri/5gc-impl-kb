#!/usr/bin/env python3
# 7 카테고리 markdown 페이지를 self-contained handoff yaml 로 변환
"""
Usage:
    .venv/bin/python3 design/scripts/build-handoff.py <nf>

입력:
    design/<nf>/3gpp-ts-*.md      — 7 카테고리 페이지 (진실 출처)
    design/<nf>/_manifest.yaml    — optional (profile 힌트용)
    specs/<spec>/*.yaml           — OpenAPI spec yaml (interface/api/data_model 소스)

출력:
    handoff/<nf>/_handoff.yaml    — schema_version handoff-v1. 외부 $ref 없는 self-contained yaml.

파싱 정책 (D1~D4):
    D1 — interface·api·data_model = spec yaml 직접 파싱;
         service_scenarios·cross_nf·configuration = markdown 파싱;
         error_handling = yaml HTTP 코드 + markdown 표 병합.
    D2 — regex 기반 H2/H3 splitter + table parser. pyyaml 외 신규 의존성 없음.
    D3 — v1: raw mermaid 보존 + participants 정규식 추출만. step list parsing 없음.
    D4 — fail-soft: 도구는 항상 yaml 산출. 누락 카테고리는 coverage.missing_categories 기록.
"""

from __future__ import annotations

import argparse
import datetime
import pathlib
import re
import sys

import yaml

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent

# ── yaml 파일 캐시 ─────────────────────────────────────────────────────────────
_ycache: dict[pathlib.Path, dict] = {}


def _load_yaml(path: pathlib.Path) -> dict:
    if path not in _ycache:
        try:
            _ycache[path] = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
        except Exception:
            _ycache[path] = {}
    return _ycache[path]


# ── $ref → 타입 문자열 (self-contained 보장: 출력 yaml 에 $ref 키 없음) ──────────
def _ref_name(ref: str) -> str:
    return ref.rsplit("/", 1)[-1]


def _prop_type(schema: object) -> str:
    """property schema → 타입 이름 문자열. $ref 를 이름으로 치환."""
    if not isinstance(schema, dict):
        return "any"
    if "$ref" in schema:
        return _ref_name(schema["$ref"])
    t = schema.get("type", "")
    if t == "array":
        items = schema.get("items") or {}
        if isinstance(items, dict) and "$ref" in items:
            return _ref_name(items["$ref"]) + "[]"
        return (items.get("type") if isinstance(items, dict) else None or "any") + "[]"
    if t == "object" and "additionalProperties" in schema:
        ap = schema["additionalProperties"]
        if isinstance(ap, dict) and "$ref" in ap:
            return f"map<string,{_ref_name(ap['$ref'])}>"
        return "map<string,any>"
    for kw in ("anyOf", "oneOf"):
        arms = schema.get(kw)
        if isinstance(arms, list) and arms:
            parts = []
            for a in arms:
                if not isinstance(a, dict):
                    continue
                if "$ref" in a:
                    parts.append(_ref_name(a["$ref"]))
                else:
                    parts.append(a.get("type") or "any")
            return f"{kw}({' | '.join(parts)})"
    return t or "object"


# ── markdown 파싱 헬퍼 ─────────────────────────────────────────────────────────
def _parse_frontmatter(text: str) -> tuple[dict, str]:
    if text.startswith("---"):
        end = text.find("\n---", 3)
        if end > 0:
            fm = yaml.safe_load(text[3:end]) or {}
            return fm, text[end + 4:].lstrip("\n")
    return {}, text


def _split_h2(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    pat = re.compile(r"^## (.+)$", re.MULTILINE)
    ms = list(pat.finditer(text))
    for i, m in enumerate(ms):
        title = m.group(1).strip()
        start = m.end()
        end = ms[i + 1].start() if i + 1 < len(ms) else len(text)
        result[title] = text[start:end].strip()
    return result


def _split_h3(text: str) -> list[tuple[str, str]]:
    result: list[tuple[str, str]] = []
    pat = re.compile(r"^### (.+)$", re.MULTILINE)
    ms = list(pat.finditer(text))
    for i, m in enumerate(ms):
        title = m.group(1).strip()
        start = m.end()
        end = ms[i + 1].start() if i + 1 < len(ms) else len(text)
        result.append((title, text[start:end].strip()))
    return result


def _parse_table(text: str) -> list[dict]:
    """markdown 표 한 개를 파싱. 여러 개 있으면 첫 번째만."""
    lines = [l for l in text.splitlines() if l.strip().startswith("|")]
    if len(lines) < 2:
        return []
    headers = [h.strip() for h in lines[0].split("|")[1:-1]]
    rows: list[dict] = []
    for line in lines[2:]:
        cells = [c.strip() for c in line.split("|")[1:-1]]
        if len(cells) == len(headers) and any(cells):
            rows.append(dict(zip(headers, cells)))
    return rows


def _parse_all_tables(text: str) -> list[dict]:
    """텍스트 안의 모든 markdown 표를 파싱해 row 를 합산."""
    all_rows: list[dict] = []
    lines = text.splitlines()
    headers: list[str] = []
    seen_sep = False
    for line in lines:
        stripped = line.strip()
        if not (stripped.startswith("|") and stripped.endswith("|")):
            if headers:
                headers = []
                seen_sep = False
            continue
        cells = [c.strip() for c in stripped.split("|")[1:-1]]
        if not headers:
            headers = cells
            seen_sep = False
        elif not seen_sep and all(
            re.fullmatch(r"[-:]+", c.replace(" ", "")) for c in cells if c
        ):
            seen_sep = True
        elif seen_sep and len(cells) == len(headers) and any(cells):
            all_rows.append(dict(zip(headers, cells)))
        else:
            # 새 표 시작 감지 (이전 표 종료)
            if seen_sep:
                headers = cells
                seen_sep = False
    return all_rows


# ── spec yaml 파싱 ─────────────────────────────────────────────────────────────
def _interface_from_yaml(yaml_files: list[pathlib.Path]) -> list[dict]:
    result: list[dict] = []
    for yf in yaml_files:
        doc = _load_yaml(yf)
        info = doc.get("info") or {}
        servers = doc.get("servers") or []
        sec = (doc.get("components") or {}).get("securitySchemes") or {}
        result.append({
            "yaml_file": yf.name,
            "title": str(info.get("title", "")),
            "version": str(info.get("version", "")),
            "server_url": servers[0].get("url", "") if servers else "",
            "security_schemes": list(sec.keys()),
        })
    return result


def _api_from_yaml(yaml_files: list[pathlib.Path]) -> list[dict]:
    ops: list[dict] = []
    for yf in yaml_files:
        doc = _load_yaml(yf)
        for path, item in (doc.get("paths") or {}).items():
            if not isinstance(item, dict):
                continue
            for method in ("get", "post", "put", "patch", "delete", "options"):
                op = item.get(method)
                if not isinstance(op, dict):
                    continue
                # request body schema
                rb_schema = ""
                for ct_info in ((op.get("requestBody") or {}).get("content") or {}).values():
                    s = ct_info.get("schema") or {}
                    rb_schema = _ref_name(s["$ref"]) if "$ref" in s else (s.get("type") or "")
                    break
                # 주요 query params (content 타입 포함)
                query_params: list[dict] = []
                for p in (op.get("parameters") or []):
                    if not isinstance(p, dict) or p.get("in") != "query":
                        continue
                    ps: dict = p.get("schema") or {}
                    if not ps:
                        for ct_info in (p.get("content") or {}).values():
                            ps = ct_info.get("schema") or {}
                            break
                    ptype = _ref_name(ps["$ref"]) if "$ref" in ps else (ps.get("type") or "")
                    if ptype:
                        query_params.append({
                            "name": p.get("name", ""),
                            "schema": ptype,
                            "required": bool(p.get("required")),
                        })
                # responses
                resp_info: dict[str, str] = {}
                for code, r in (op.get("responses") or {}).items():
                    if not isinstance(r, dict):
                        resp_info[str(code)] = ""
                        continue
                    for ct_info in (r.get("content") or {}).values():
                        s = ct_info.get("schema") or {}
                        resp_info[str(code)] = (
                            _ref_name(s["$ref"]) if "$ref" in s else (s.get("type") or "")
                        )
                        break
                    else:
                        resp_info[str(code)] = ""
                ops.append({
                    "operationId": op.get("operationId", ""),
                    "method": method.upper(),
                    "path": path,
                    "yaml_file": yf.name,
                    "request_body_schema": rb_schema,
                    "query_params": query_params,
                    "responses": resp_info,
                })
    return ops


def _data_model_from_yaml(yaml_files: list[pathlib.Path]) -> dict:
    schemas: dict[str, dict] = {}
    for yf in yaml_files:
        doc = _load_yaml(yf)
        for name, s in ((doc.get("components") or {}).get("schemas") or {}).items():
            if not isinstance(s, dict):
                continue
            # alias ($ref at top level)
            if "$ref" in s:
                schemas[name] = {
                    "source_yaml": yf.name,
                    "type": "alias",
                    "alias_of": _ref_name(s["$ref"]),
                    "required": [],
                    "properties": {},
                }
                continue
            props: dict[str, str] = {}
            for pn, ps in (s.get("properties") or {}).items():
                props[pn] = _prop_type(ps)
            for arm in (s.get("allOf") or []):
                if isinstance(arm, dict):
                    for pn, ps in (arm.get("properties") or {}).items():
                        props[pn] = _prop_type(ps)
            schemas[name] = {
                "source_yaml": yf.name,
                "type": s.get("type", "object"),
                "description": (s.get("description") or "")[:160],
                "required": s.get("required") or [],
                "properties": props,
            }
    return {"schema_count": len(schemas), "schemas": schemas}


# ── markdown 카테고리 파싱 ─────────────────────────────────────────────────────
def _service_scenarios_from_md(text: str) -> list[dict]:
    scenarios: list[dict] = []
    for title, body in _split_h3(text):
        mermaid_blocks = re.findall(r"```mermaid\n(.*?)```", body, re.DOTALL)
        participants: set[str] = set()
        for blk in mermaid_blocks:
            # "participant ALIAS as LABEL" → LABEL, "participant NAME" → NAME
            for m in re.finditer(
                r"^\s*participant\s+\w+\s+as\s+(.+?)$"
                r"|^\s*participant\s+(\w[\w/\- ]*)$",
                blk, re.MULTILINE,
            ):
                p = (m.group(1) or m.group(2) or "").strip()
                if p:
                    participants.add(p)
        first = re.search(r"```mermaid", body)
        prose = body[:first.start()].strip() if first else body
        slug = re.sub(r"[^a-z0-9]+", "_", title.lower()).strip("_")
        scenarios.append({
            "id": slug,
            "title": title,
            "description": prose,
            "mermaid": [blk.strip() for blk in mermaid_blocks],
            "participants": sorted(participants),
        })
    return scenarios


def _configuration_from_md(text: str) -> dict[str, dict]:
    config: dict[str, dict] = {}
    for h3_title, h3_body in _split_h3(text):
        key = re.sub(r"[^a-z0-9]+", "_", h3_title.lower()).strip("_")
        config[key] = {"title": h3_title, "items": _parse_table(h3_body)}
    return config


def _error_handling(yaml_files: list[pathlib.Path], md_text: str) -> dict:
    # yaml 경로에서 HTTP 응답 코드 수집
    http_codes: set[str] = set()
    for yf in yaml_files:
        doc = _load_yaml(yf)
        for item in (doc.get("paths") or {}).values():
            if not isinstance(item, dict):
                continue
            for method in ("get", "post", "put", "patch", "delete", "options"):
                op = item.get(method)
                if isinstance(op, dict):
                    for code in (op.get("responses") or {}):
                        http_codes.add(str(code))
    # markdown — H3 섹션별 표 수집
    tables: dict[str, list[dict]] = {}
    for h3_title, h3_body in _split_h3(md_text):
        rows = _parse_all_tables(h3_body)
        if rows:
            tables[h3_title] = rows
    if not tables:
        rows = _parse_all_tables(md_text)
        if rows:
            tables["_default"] = rows
    return {"http_codes": sorted(http_codes), "tables": tables}


# ── profile 추론 ───────────────────────────────────────────────────────────────
def _detect_profile(spec_number: str, yaml_files: list) -> str:
    has_yaml = bool(yaml_files)
    if spec_number.startswith("23."):
        return "mixed" if has_yaml else "stage_2_only"
    return "stage_3_only" if has_yaml else "stage_2_only"


# ── main ───────────────────────────────────────────────────────────────────────
def main() -> None:
    parser = argparse.ArgumentParser(
        description="7 카테고리 markdown 페이지 → handoff-v1 yaml"
    )
    parser.add_argument("nf", help="NF 폴더명 (소문자, 예 nssf)")
    args = parser.parse_args()

    nf = args.nf.lower()
    design_dir = REPO_ROOT / "design" / nf
    if not design_dir.is_dir():
        sys.exit(f"[build-handoff] design/{nf}/ 없음")

    # 1. markdown 로드 (3gpp-ts-*.md 중 마지막 파일)
    md_files = sorted(design_dir.glob("3gpp-ts-*.md"))
    if not md_files:
        sys.exit(f"[build-handoff] design/{nf}/3gpp-ts-*.md 없음")
    md_file = md_files[-1]
    text = md_file.read_text(encoding="utf-8")
    fm, body = _parse_frontmatter(text)
    sections = _split_h2(body)

    spec_raw = str(fm.get("spec", ""))
    spec_number = re.sub(r"(?i)^ts\s*", "", spec_raw).strip()  # "TS 29.531" → "29.531"

    # 2. spec yaml 목록
    spec_dir = REPO_ROOT / "specs" / spec_number if spec_number else None
    yaml_files: list[pathlib.Path] = (
        sorted(spec_dir.glob("*.yaml")) if spec_dir and spec_dir.is_dir() else []
    )
    profile = _detect_profile(spec_number, yaml_files)

    # 3. 각 카테고리 파싱 (D4: fail-soft)
    missing: list[str] = []
    warnings: list[str] = []

    interface = _interface_from_yaml(yaml_files)
    if not interface:
        missing.append("interface")

    api = _api_from_yaml(yaml_files)
    if not api:
        missing.append("api")

    data_model = _data_model_from_yaml(yaml_files)
    if not data_model.get("schemas"):
        missing.append("data_model")

    svc_text = sections.get("Service Scenarios", "")
    service_scenarios = _service_scenarios_from_md(svc_text)
    if not service_scenarios:
        missing.append("service_scenarios")

    cross_nf = _parse_table(sections.get("Cross-NF Dependencies", ""))
    if not cross_nf:
        missing.append("cross_nf")

    config_text = sections.get("Configuration", "")
    configuration = _configuration_from_md(config_text)
    if not config_text.strip():
        missing.append("configuration")

    err_text = sections.get("Error Handling", "")
    error_handling = _error_handling(yaml_files, err_text)
    if not error_handling["http_codes"] and not error_handling["tables"]:
        missing.append("error_handling")

    # 4. handoff yaml 조립
    handoff: dict = {
        "schema_version": "handoff-v1",
        "nf": nf,
        "spec": spec_raw,
        "profile": profile,
        "generated_from": str(md_file.relative_to(REPO_ROOT)),
        "generated_at": datetime.datetime.utcnow().isoformat() + "Z",
        "coverage": {
            "status": "complete" if not missing else "incomplete",
            "missing_categories": missing,
            "warnings": warnings,
        },
        "interface": interface,
        "api": api,
        "data_model": data_model,
        "service_scenarios": service_scenarios,
        "cross_nf": cross_nf,
        "configuration": configuration,
        "error_handling": error_handling,
        "persistent_state": {},
        "concurrency": {},
    }

    # 5. 출력
    out_dir = REPO_ROOT / "handoff" / nf
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / "_handoff.yaml"
    out_path.write_text(
        yaml.dump(handoff, allow_unicode=True, default_flow_style=False, sort_keys=False),
        encoding="utf-8",
    )
    print(f"[build-handoff] wrote {out_path.relative_to(REPO_ROOT)}", file=sys.stderr)
    print(f"[build-handoff] coverage={handoff['coverage']['status']}", file=sys.stderr)
    if missing:
        print(f"  missing: {missing}", file=sys.stderr)


if __name__ == "__main__":
    main()
