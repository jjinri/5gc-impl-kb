#!/usr/bin/env python3
# handoff-v2 yaml + 토픽 파일 정합 검증. spec §4 의 basic 13 룰.
"""
Usage:
    .venv/bin/python3 design/scripts/validate-extraction.py <nf> [--level basic|strict]

basic (hard gate, FAIL → handoff_ready 통과 차단):
    #1  schema_version == handoff-v2
    #2  status enum ∈ [canonical, handoff_ready, draft, blocked, not_applicable]
    #3  topic file exists (path_resolution)
    #4  cross-reference target exists (depends_on / related / error_refs / tasks.read)
    #5  category/topic consistency (category=handoff_ready → 산하 topic ∈ [canonical, handoff_ready])
    #6  blocked/not_applicable semantics (blocked_reason / na_reason 필수)
    #7  marker ID unique  (T7)
    #8  frontmatter ↔ marker sync (T7)
    #9  data-model machine_file 존재 (T8)
    #10 machine_file JSON parse valid (T8)
    #11 JSON ↔ handoff topic 정합 (T8)
    #12 JSON unresolved_refs ↔ status (T8)
    #13 JSON dependencies target exists (T8)

strict (report-only, T6 단계에서는 stub):
    service flow participant 어절 일치 등 — 후속 사이클.
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import sys
from typing import Any

import yaml


REPO_ROOT = pathlib.Path(
    os.environ.get("FIVEGC_REPO_ROOT")
    or pathlib.Path(__file__).resolve().parent.parent.parent
).resolve()

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from lib.path_resolution import resolve_topic_path, parse_topic_ref  # noqa: E402
from lib.marker_parser import extract_markers, duplicate_marker_ids, sync_diff  # noqa: E402


VALID_STATUS = {"canonical", "handoff_ready", "draft", "blocked", "not_applicable"}
VALID_SCHEMA = "handoff-v2"


def _load(path: pathlib.Path) -> Any:
    return yaml.safe_load(path.read_text(encoding="utf-8"))


def rule_1_schema(data: dict) -> list[str]:
    if data.get("schema_version") != VALID_SCHEMA:
        return [f"#1 schema_version: expected {VALID_SCHEMA}, got {data.get('schema_version')!r}"]
    return []


def rule_2_status_enum(data: dict) -> list[str]:
    fails = []
    for tid, t in (data.get("topics") or {}).items():
        s = t.get("status")
        if s not in VALID_STATUS:
            fails.append(f"#2 status: topic {tid!r} status={s!r} not in {sorted(VALID_STATUS)}")
    for cid, c in (data.get("categories") or {}).items():
        s = c.get("status")
        if s not in VALID_STATUS:
            fails.append(f"#2 status: category {cid!r} status={s!r}")
    return fails


def rule_3_topic_file_exists(nf: str, data: dict) -> list[str]:
    fails = []
    nf_root = REPO_ROOT / "design" / nf
    layout = {cid: c.get("layout", "directory") for cid, c in (data.get("categories") or {}).items()}
    for tid, t in (data.get("topics") or {}).items():
        if t.get("status") == "not_applicable":
            continue
        try:
            r = resolve_topic_path(nf_root=nf_root, topic_id=tid, category_layout=layout)
        except ValueError as e:
            fails.append(f"#3 topic file: {tid!r} {e}")
            continue
        if not r.exists:
            fails.append(f"#3 topic file: {tid!r} → {r.path.relative_to(REPO_ROOT)} 부재")
    return fails


def rule_4_cross_ref(nf: str, data: dict) -> list[str]:
    fails = []
    topics = data.get("topics") or {}
    known = set(topics.keys())
    nf_root = REPO_ROOT / "design" / nf
    layout = {cid: c.get("layout", "directory") for cid, c in (data.get("categories") or {}).items()}
    fields = ("depends_on", "related", "error_refs")

    def _check(ref: str, ctx: str) -> None:
        head = ref.split(".", 1)[0].split("#", 1)[0]
        if head not in known:
            fails.append(f"#4 cross-reference: {ctx} → {ref!r} (handoff yaml 에 없음)")
            return
        if "#" not in ref:
            return
        anchor = ref.split("#", 1)[1]
        try:
            r = resolve_topic_path(
                nf_root=nf_root,
                topic_id=f"{head}#{anchor}",
                category_layout=layout,
            )
        except ValueError:
            return  # category/layout issue already surfaced by rule #3
        if r.exists and not r.anchor_found:
            fails.append(
                f"#4 cross-reference: {ctx} → {ref!r} "
                f"(anchor {anchor!r} not in {head!r}; "
                f'expected `<a id="{anchor}"></a>` or a heading slugging to {anchor!r})'
            )

    for tid, t in topics.items():
        for f in fields:
            for ref in (t.get(f) or []):
                _check(ref, f"{tid!r}.{f}")
    for task_id, task in (data.get("tasks") or {}).items():
        for ref in (task.get("read") or []):
            _check(ref, f"task {task_id!r}.read")
    return fails


def rule_5_category_topic_consistency(data: dict) -> list[str]:
    fails = []
    cats = data.get("categories") or {}
    for tid, t in (data.get("topics") or {}).items():
        cat = parse_topic_ref(tid).category
        cstatus = (cats.get(cat) or {}).get("status")
        tstatus = t.get("status")
        if cstatus == "handoff_ready" and tstatus not in ("canonical", "handoff_ready"):
            fails.append(
                f"#5 category/topic: category {cat!r}=handoff_ready but topic {tid!r}={tstatus!r}"
            )
    return fails


def rule_6_blocked_na_semantics(data: dict) -> list[str]:
    fails = []
    for tid, t in (data.get("topics") or {}).items():
        if t.get("status") == "blocked" and not t.get("blocked_reason"):
            fails.append(f"#6 blocked: topic {tid!r} missing blocked_reason")
        if t.get("status") == "not_applicable" and not t.get("na_reason"):
            fails.append(f"#6 not_applicable: topic {tid!r} missing na_reason")
    return fails


def _read_topic_md(nf: str, data: dict, topic_id: str) -> tuple[dict, str] | None:
    layout = {cid: c.get("layout", "directory") for cid, c in (data.get("categories") or {}).items()}
    nf_root = REPO_ROOT / "design" / nf
    try:
        r = resolve_topic_path(nf_root=nf_root, topic_id=topic_id, category_layout=layout)
    except ValueError:
        return None
    if not r.exists:
        return None
    text = r.path.read_text(encoding="utf-8")
    if not text.startswith("---"):
        return {}, text
    end = text.find("\n---", 3)
    if end < 0:
        return {}, text
    try:
        fm = yaml.safe_load(text[3:end]) or {}
    except yaml.YAMLError:
        fm = {}
    return fm, text[end + 4:]


def rule_7_marker_unique(nf: str, data: dict) -> list[str]:
    fails = []
    for tid in (data.get("topics") or {}):
        loaded = _read_topic_md(nf, data, tid)
        if loaded is None:
            continue
        _, body = loaded
        dups = duplicate_marker_ids(body)
        for kind, mid in dups:
            fails.append(f"#7 marker unique: {tid!r} duplicates {kind}:{mid}")
    return fails


def rule_8_frontmatter_marker_sync(nf: str, data: dict) -> list[str]:
    fails = []
    for tid in (data.get("topics") or {}):
        loaded = _read_topic_md(nf, data, tid)
        if loaded is None:
            continue
        fm, body = loaded
        diff = sync_diff(fm=fm, markers=extract_markers(body))
        if any([diff.frontmatter_only_auto, diff.body_only_auto,
                diff.frontmatter_only_user, diff.body_only_user]):
            fails.append(
                f"#8 frontmatter↔marker sync {tid!r}: "
                f"fm_only_auto={diff.frontmatter_only_auto}, "
                f"body_only_auto={diff.body_only_auto}, "
                f"fm_only_user={diff.frontmatter_only_user}, "
                f"body_only_user={diff.body_only_user}"
            )
    return fails


DATA_MODEL_REQUIRED_KEYS = {
    "schema_version", "topic_id", "status", "fields", "dependencies", "unresolved_refs",
}


def _data_model_topics(data: dict) -> list[tuple[str, dict]]:
    return [(tid, t) for tid, t in (data.get("topics") or {}).items()
            if tid.startswith("data-model/")]


def rule_9_machine_file_exists(data: dict) -> list[str]:
    fails = []
    for tid, t in _data_model_topics(data):
        mf = t.get("machine_file")
        if not mf:
            fails.append(f"#9 machine_file: topic {tid!r} missing machine_file key")
            continue
        path = REPO_ROOT / mf
        if not path.is_file():
            fails.append(f"#9 machine_file: {tid!r} → {mf} 부재")
    return fails


def _load_machine(t: dict) -> tuple[dict | None, str | None]:
    mf = t.get("machine_file")
    if not mf:
        return None, "missing machine_file"
    path = REPO_ROOT / mf
    if not path.is_file():
        return None, "file missing"
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        return None, f"json parse error: {e}"
    return data, None


def rule_10_json_parse(data: dict) -> list[str]:
    fails = []
    for tid, t in _data_model_topics(data):
        mf = t.get("machine_file")
        if not mf:
            continue
        path = REPO_ROOT / mf
        if not path.is_file():
            continue
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as e:
            fails.append(f"#10 JSON parse: {tid!r} {e}")
            continue
        missing = DATA_MODEL_REQUIRED_KEYS - set(payload.keys())
        if missing:
            fails.append(f"#10 JSON shape: {tid!r} missing keys {sorted(missing)}")
    return fails


def rule_11_topic_id_status_match(data: dict) -> list[str]:
    fails = []
    for tid, t in _data_model_topics(data):
        payload, err = _load_machine(t)
        if payload is None:
            continue
        if payload.get("topic_id") != tid:
            fails.append(f"#11 JSON topic_id: yaml={tid!r} json={payload.get('topic_id')!r}")
        if payload.get("status") != t.get("status"):
            fails.append(
                f"#11 JSON status: {tid!r} yaml={t.get('status')!r} json={payload.get('status')!r}"
            )
    return fails


def rule_12_unresolved_vs_status(data: dict) -> list[str]:
    fails = []
    for tid, t in _data_model_topics(data):
        payload, err = _load_machine(t)
        if payload is None:
            continue
        if payload.get("unresolved_refs") and t.get("status") in ("canonical", "handoff_ready"):
            fails.append(
                f"#12 unresolved_refs vs status: {tid!r} has unresolved but status={t.get('status')!r}"
            )
    return fails


def rule_13_dependencies_target(data: dict) -> list[str]:
    fails = []
    known = set((data.get("topics") or {}).keys())
    for tid, t in _data_model_topics(data):
        payload, err = _load_machine(t)
        if payload is None:
            continue
        for dep in (payload.get("dependencies") or []):
            if dep not in known:
                fails.append(f"#13 dependencies: {tid!r} → {dep!r} not in handoff topics")
    return fails


def run_basic(nf: str, data: dict) -> tuple[int, int, list[str]]:
    """Return (pass_count, fail_count, failure_messages)."""
    rules = [
        ("#1", rule_1_schema(data)),
        ("#2", rule_2_status_enum(data)),
        ("#3", rule_3_topic_file_exists(nf, data)),
        ("#4", rule_4_cross_ref(nf, data)),
        ("#5", rule_5_category_topic_consistency(data)),
        ("#6", rule_6_blocked_na_semantics(data)),
        ("#7", rule_7_marker_unique(nf, data)),
        ("#8", rule_8_frontmatter_marker_sync(nf, data)),
        ("#9", rule_9_machine_file_exists(data)),
        ("#10", rule_10_json_parse(data)),
        ("#11", rule_11_topic_id_status_match(data)),
        ("#12", rule_12_unresolved_vs_status(data)),
        ("#13", rule_13_dependencies_target(data)),
    ]
    failures: list[str] = []
    pass_count = 0
    for rid, errs in rules:
        if errs:
            failures.extend(errs)
        else:
            pass_count += 1
    return pass_count, len(failures), failures


def _handoff_path(nf: str) -> pathlib.Path:
    return REPO_ROOT / "handoff" / nf / "contract.yaml"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("nf")
    parser.add_argument("--level", choices=["basic", "strict"], default="basic")
    args = parser.parse_args()

    nf = args.nf.lower()
    handoff_yaml = _handoff_path(nf)
    if not handoff_yaml.is_file():
        sys.exit(f"[validate] {handoff_yaml.relative_to(REPO_ROOT)} 없음")
    data = _load(handoff_yaml) or {}

    print(f"[validate-extraction] {nf} --level {args.level}")
    if args.level == "basic":
        passed, failed, msgs = run_basic(nf, data)
        print(f"  basic: passed={passed} failed={failed}")
        for m in msgs:
            print(f"    FAIL {m}")
        sys.exit(0 if failed == 0 else 1)
    else:
        print("  strict: (not implemented in MVP — report-only stubs in T8)")
        sys.exit(0)


if __name__ == "__main__":
    main()
