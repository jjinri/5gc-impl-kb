#!/usr/bin/env python3
# fresh full materialization — seed.topics 전량을 사람 수동 저작 없이 topic .md/.json 으로 생성
"""
Usage:
    .venv/bin/python3 design/scripts/materialize-contract.py <nf> [--force]

옵션:
    --force   기존 topic 파일이 있어도 AUTO 영역 재생성 (USER 영역은 항상 보존)

동작 (nf-build SKILL.md §"Fresh full materialization" 의 구현).
  1. seed.topics *존재* entry 만 순회 — seed 에 없는 topic/category invent 금지.
  2. 카테고리별 AUTO 섹션 생성 (data-model=resolve-yaml-refs, api/interface/error=OpenAPI yaml),
     USER 섹션은 기존 보존 또는 TODO placeholder.
  3. preflight (status 미변경) → promotion set (spec-derived, preflight 통과, data-model 은
     JSON unresolved_refs 빈 경우만) → seed/JSON status 동일 갱신 → category status.
  build-handoff.py 는 무변경. 본 script 가 _contract_seed.yaml status 를 갱신한다.

멱등 — AUTO 만 재생성, USER marker 안 사람 산문은 위치·내용 보존.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent
RESOLVE = REPO / "design" / "scripts" / "resolve-yaml-refs.py"
PYEXE = sys.executable

SPEC_DERIVED = {"api", "data-model", "interface", "error-handling"}

# 카테고리 → (AUTO id 목록, USER id 목록). nf-build SKILL.md marker schema 와 1:1.
SCHEMA = {
    "interface": (["auth-block", "transport-block"], ["implementation-notes"]),
    "api": (["api-matrix", "request-schema", "response-schema"], ["implementation-notes"]),
    "data-model": (["chain-tree", "field-table"], ["implementation-notes"]),
    "error-handling": (["error-matrix"], ["recovery-prose", "implementation-notes"]),
    "module-decomposition": (["module-graph"], ["responsibility-prose", "implementation-notes"]),
    "service-scenarios": (["scenario-index"], ["scenario-prose"]),
    "behavior-state": (["state-index"], ["state-prose"]),
    "failure-policy": (["failure-index"], ["policy-prose"]),
    "configuration": (["config-index"], ["policy-notes"]),
    "persistence": (["persistence-index"], ["policy-notes"]),
    "test-matrix": (["test-index"], ["coverage-notes"]),
    "work-plan": (["work-index"], ["plan-notes"]),
    "cross-nf": (["crossnf-index"], ["crossnf-notes"]),
}


def load_yaml(p: pathlib.Path) -> dict:
    return yaml.safe_load(p.read_text(encoding="utf-8")) or {}


def primary_yamls(nf: str, manifest: dict) -> list[pathlib.Path]:
    spec = str(manifest.get("primary_spec") or "")
    ys = ((manifest.get("primary_files") or {}).get("yamls") or [])
    return [REPO / "specs" / spec / y for y in ys]


def ref_name(node) -> str | None:
    if isinstance(node, dict) and "$ref" in node:
        return str(node["$ref"]).split("/")[-1]
    return None


# ── USER 영역 보존 ─────────────────────────────────────────────────

def extract_user_blocks(path: pathlib.Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    txt = path.read_text(encoding="utf-8")
    out = {}
    for m in re.finditer(
        r"<!-- USER:([\w-]+):start -->\n(.*?)\n<!-- USER:\1:end -->",
        txt, re.DOTALL):
        out[m.group(1)] = m.group(2)
    return out


def user_block(uid: str, preserved: dict[str, str]) -> str:
    body = preserved.get(uid)
    if body is None or not body.strip():
        body = f"TODO: {uid} — 사람이 보강 (fresh placeholder)."
    return f"<!-- USER:{uid}:start -->\n{body}\n<!-- USER:{uid}:end -->"


def auto_block(aid: str, body: str) -> str:
    return f"<!-- AUTO:{aid}:start -->\n{body}\n<!-- AUTO:{aid}:end -->"


def frontmatter(topic_id: str, category: str, status: str,
                gen: list[str], usr: list[str]) -> str:
    return (
        "---\n"
        f"topic_id: {topic_id}\n"
        f"category: {category}\n"
        f"status: {status}\n"
        "generated_sections:\n" + "".join(f"  - {g}\n" for g in gen) +
        "user_sections:\n" + "".join(f"  - {u}\n" for u in usr) +
        "---\n"
    )


def assemble(topic_id, category, status, auto: dict[str, str],
             preserved: dict[str, str]) -> str:
    gen, usr = SCHEMA[category]
    parts = [frontmatter(topic_id, category, status, gen, usr), ""]
    parts.append(f"# {topic_id}\n")
    for aid in gen:
        parts.append(auto_block(aid, auto.get(aid, f"TODO(AUTO {aid})")))
        parts.append("")
    for uid in usr:
        parts.append(user_block(uid, preserved))
        parts.append("")
    return "\n".join(parts).rstrip() + "\n"


# ── 카테고리별 AUTO 생성 ───────────────────────────────────────────

def yaml_for_path(yamls: list[pathlib.Path], path: str, method: str):
    for yp in yamls:
        if not yp.is_file():
            continue
        d = load_yaml(yp)
        op = (d.get("paths") or {}).get(path, {})
        if method.lower() in {k.lower() for k in op}:
            return yp, d
    return None, None


_SCHEMA_YAML_CACHE: dict[str, pathlib.Path] = {}


def yaml_for_schema(yamls: list[pathlib.Path], schema: str):
    # 1) NSSF primary yaml 우선.
    for yp in yamls:
        if not yp.is_file():
            continue
        d = load_yaml(yp)
        if schema in ((d.get("components") or {}).get("schemas") or {}):
            return yp, d
    # 2) 외부 spec yaml (29.571 CommonData·29.510 등) 탐색 — bucket(i)
    #    resolver-입력-선택. yaml_for_schema 가 NSSF primary 만 보면 공통
    #    자료형(Snssai/PlmnId/NfInstanceId/SupportedFeatures@29.571,
    #    NFType@29.510)이 resolve 실패해 false unresolved 가 된다.
    if not _SCHEMA_YAML_CACHE:
        for yp in sorted((REPO / "specs").glob("*/*.yaml")):
            try:
                d = load_yaml(yp)
            except yaml.YAMLError:
                continue
            for name in ((d.get("components") or {}).get("schemas") or {}):
                _SCHEMA_YAML_CACHE.setdefault(name, yp)
    ext = _SCHEMA_YAML_CACHE.get(schema)
    if ext is not None:
        return ext, load_yaml(ext)
    return (yamls[0] if yamls else None), None


def auto_api(topic: dict, yamls) -> dict[str, str]:
    path, method = topic.get("path", ""), str(topic.get("method", "")).lower()
    yp, d = yaml_for_path(yamls, path, method)
    if d is None:
        return {"api-matrix": f"TODO(AUTO api-matrix): path {path} {method} 미발견",
                "request-schema": "TODO(AUTO request-schema)",
                "response-schema": "TODO(AUTO response-schema)"}
    op = (d["paths"][path])[method]
    sec = op.get("security") or d.get("security") or []
    mat = ["| field | value |", "|---|---|",
           f"| operationId | {op.get('operationId','')} |",
           f"| method | {method.upper()} |",
           f"| path | {path} |",
           f"| summary | {op.get('summary','').strip()} |",
           f"| security | {sec} |",
           f"| depends_on | {topic.get('depends_on', [])} |",
           f"| related | {topic.get('related', [])} |",
           f"| source_yaml | specs 내 {yp.name} |"]
    rb = op.get("requestBody") or {}
    rname = None
    for ct in (rb.get("content") or {}).values():
        rname = ref_name(ct.get("schema")) or ref_name((ct.get("schema") or {}).get("items"))
        if rname:
            break
    req = f"requestBody schema — `{rname}`" if rname else "requestBody 없음 또는 inline"
    rows = ["| status | schema |", "|---|---|"]
    for code, r in (op.get("responses") or {}).items():
        sch = None
        for ct in ((r or {}).get("content") or {}).values():
            sch = ref_name(ct.get("schema")) or ref_name((ct.get("schema") or {}).get("items"))
            if sch:
                break
        rows.append(f"| {code} | {sch or (r or {}).get('description','').strip() or '-'} |")
    return {"api-matrix": "\n".join(mat),
            "request-schema": req,
            "response-schema": "\n".join(rows)}


def auto_interface(yamls) -> dict[str, str]:
    d = next((load_yaml(y) for y in yamls if y.is_file()), {})
    info, servers = d.get("info", {}), d.get("servers", [])
    schemes = (d.get("components") or {}).get("securitySchemes") or {}
    auth = ["| field | value |", "|---|---|",
            f"| api | {info.get('title','')} |",
            f"| version | {info.get('version','')} |",
            f"| securitySchemes | {list(schemes)} |",
            f"| global security | {d.get('security', [])} |"]
    for n, s in schemes.items():
        auth.append(f"| scheme {n} | {s.get('type','')} / "
                    f"{list((s.get('flows') or {}))} |")
    trans = ["| field | value |", "|---|---|", "| transport | HTTP/2 (SBI, 3GPP TS 29.500) |"]
    for sv in servers:
        trans.append(f"| server url | {sv.get('url','')} |")
    return {"auth-block": "\n".join(auth), "transport-block": "\n".join(trans)}


def auto_error(yamls, seed: dict) -> dict[str, str]:
    codes: dict[str, str] = {}
    for yp in yamls:
        if not yp.is_file():
            continue
        d = load_yaml(yp)
        for path, ops in (d.get("paths") or {}).items():
            for method, op in ops.items():
                if not isinstance(op, dict):
                    continue
                for code, r in (op.get("responses") or {}).items():
                    if str(code)[0] in {"4", "5"}:
                        desc = (r or {}).get("description", "").strip()
                        codes.setdefault(str(code), desc)
    rows = ["| status | meaning (spec response description) |", "|---|---|"]
    for c in sorted(codes):
        rows.append(f"| {c} | {codes[c] or '-'} |")
    rows.append("| ProblemDetails | 3GPP TS 29.571 ProblemDetails — cause/title/status |")
    return {"error-matrix": "\n".join(rows)}


def auto_index(category: str, topic_id: str, seed: dict) -> dict[str, str]:
    # 非spec-derivable — seed 도출 최소 인덱스. invent 금지.
    aid = SCHEMA[category][0][0]
    sib = [t for t in seed.get("topics", {}) if t.split("/")[0] == category]
    body = [f"seed 도출 최소 인덱스 (placeholder/minimal — no inferred content).",
            f"category: {category}", f"topics: {sib or [topic_id]}"]
    return {aid: "\n".join(body)}


def materialize_datamodel(topic_id: str, topic: dict, nf: str,
                          yamls, all_dm: list[str],
                          status: str) -> tuple[dict[str, str], list]:
    schema = topic_id.split("/", 1)[1]
    yp, _ = yaml_for_schema(yamls, schema)
    mf = REPO / topic["machine_file"]
    mf.parent.mkdir(parents=True, exist_ok=True)
    cmd = [PYEXE, str(RESOLVE), str(yp), schema, "--emit-json",
           "--topic-id", topic_id, "--nf", nf, "--status", status]
    for t in all_dm:
        cmd += ["--handoff-topics", t]
    unresolved = []
    try:
        js = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        if js.returncode == 0 and js.stdout.strip():
            mf.write_text(js.stdout, encoding="utf-8")
            unresolved = (json.loads(js.stdout) or {}).get("unresolved_refs", [])
        else:
            mf.write_text(json.dumps({
                "schema_version": "datamodel-v1", "topic_id": topic_id,
                "status": status, "fields": [], "dependencies": [],
                "unresolved_refs": [{"ref": schema, "note": "resolve failed"}],
            }, ensure_ascii=False, indent=2), encoding="utf-8")
            unresolved = [{"ref": schema, "note": "resolve failed"}]
    except subprocess.TimeoutExpired:
        unresolved = [{"ref": schema, "note": "timeout"}]
    # chain-tree (text 모드)
    tree = "TODO(AUTO chain-tree)"
    try:
        tx = subprocess.run([PYEXE, str(RESOLVE), str(yp), schema, "--depth", "6"],
                            capture_output=True, text=True, timeout=120)
        if tx.returncode == 0 and tx.stdout.strip():
            tree = tx.stdout.strip()
    except subprocess.TimeoutExpired:
        pass
    # field-table (JSON top-level fields)
    rows = ["| field | type | required |", "|---|---|---|"]
    try:
        j = json.loads(mf.read_text(encoding="utf-8"))
        for f in (j.get("fields") or [])[:200]:
            rows.append(f"| {f.get('name','')} | {f.get('type','')} | "
                        f"{f.get('required', False)} |")
    except Exception:
        pass
    return ({"chain-tree": tree, "field-table": "\n".join(rows)}, unresolved)


# ── preflight / promotion ──────────────────────────────────────────

def preflight_ok(path: pathlib.Path, category: str) -> bool:
    if not path.is_file():
        return False
    txt = path.read_text(encoding="utf-8")
    fm = re.match(r"---\n(.*?)\n---\n", txt, re.DOTALL)
    if not fm:
        return False
    try:
        meta = yaml.safe_load(fm.group(1)) or {}
    except yaml.YAMLError:
        return False
    if not all(meta.get(k) for k in ("topic_id", "category", "status")):
        return False
    gen = set(meta.get("generated_sections") or [])
    usr = set(meta.get("user_sections") or [])
    a_ids = set(re.findall(r"<!-- AUTO:([\w-]+):start -->", txt))
    u_ids = set(re.findall(r"<!-- USER:([\w-]+):start -->", txt))
    if gen != a_ids or usr != u_ids:
        return False
    if len(a_ids) != len(re.findall(r"<!-- AUTO:[\w-]+:start -->", txt)):
        return False
    return True


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("nf")
    ap.add_argument("--force", action="store_true")
    args = ap.parse_args()
    nf = args.nf

    design = REPO / "design" / nf
    seed_path = design / "_contract_seed.yaml"
    man_path = design / "_manifest.yaml"
    if not seed_path.is_file() or not man_path.is_file():
        sys.exit(f"[materialize] {seed_path} 또는 {man_path} 없음 — /nf-spec-discover 먼저")
    seed = load_yaml(seed_path)
    manifest = load_yaml(man_path)
    yamls = primary_yamls(nf, manifest)
    topics = seed.get("topics") or {}
    all_dm = [t for t in topics if t.split("/")[0] == "data-model"]

    created, dm_unresolved = [], {}
    for tid, topic in topics.items():
        category = tid.split("/")[0]
        if category not in SCHEMA:
            continue
        fpath = REPO / topic["file"]
        fpath.parent.mkdir(parents=True, exist_ok=True)
        preserved = extract_user_blocks(fpath)
        if category == "data-model":
            auto, unres = materialize_datamodel(tid, topic, nf, yamls, all_dm,
                                                topic.get("status", "draft"))
            dm_unresolved[tid] = unres
        elif category == "api":
            auto = auto_api(topic, yamls)
        elif category == "interface":
            auto = auto_interface(yamls)
        elif category == "error-handling":
            auto = auto_error(yamls, seed)
        else:
            auto = auto_index(category, tid, seed)
        fpath.write_text(
            assemble(tid, category, topic.get("status", "draft"), auto, preserved),
            encoding="utf-8")
        created.append(tid)

    # ── promotion (preflight 후) ──
    promoted = []
    for tid, topic in topics.items():
        category = tid.split("/")[0]
        if category not in SPEC_DERIVED:
            continue
        if not preflight_ok(REPO / topic["file"], category):
            continue
        if category == "data-model" and dm_unresolved.get(tid):
            continue
        promoted.append(tid)

    for tid in promoted:
        topics[tid]["status"] = "handoff_ready"
        # .md frontmatter status 도 함께 갱신 — 안 하면 seed/JSON=handoff_ready
        # 인데 .md=draft 인 3-way split (frontmatter 만 patch, USER/AUTO 보존).
        fp = REPO / topics[tid]["file"]
        if fp.is_file():
            t = fp.read_text(encoding="utf-8")
            t2 = re.sub(r"(?s)(\A---\n.*?\nstatus: )\w+(\n.*?\n---\n)",
                        r"\1handoff_ready\2", t, count=1)
            if t2 != t:
                fp.write_text(t2, encoding="utf-8")
        mf = topics[tid].get("machine_file")
        if mf and (REPO / mf).is_file():
            try:
                j = json.loads((REPO / mf).read_text(encoding="utf-8"))
                j["status"] = "handoff_ready"
                (REPO / mf).write_text(
                    json.dumps(j, ensure_ascii=False, indent=2), encoding="utf-8")
            except Exception:
                pass

    cats = seed.get("categories") or {}
    for cat, cmeta in cats.items():
        child = [t for t in topics if t.split("/")[0] == cat]
        if child and all(topics[t].get("status") == "handoff_ready" for t in child):
            cmeta["status"] = "handoff_ready"
        else:
            cmeta["status"] = "draft"

    seed_path.write_text(
        yaml.safe_dump(seed, allow_unicode=True, sort_keys=False), encoding="utf-8")

    print(f"[materialize] {nf}: topics materialized={len(created)}, "
          f"promoted(handoff_ready)={len(promoted)}", file=sys.stderr)
    draft = [t for t in topics if topics[t].get("status") != "handoff_ready"]
    print(f"[materialize] draft 유지={len(draft)} {draft}", file=sys.stderr)
    blocked = {k: v for k, v in dm_unresolved.items() if v}
    if blocked:
        print(f"[materialize] data-model unresolved (draft 유지) — "
              f"{list(blocked)}", file=sys.stderr)


if __name__ == "__main__":
    main()
