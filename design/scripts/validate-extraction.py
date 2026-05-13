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


def rule_4_cross_ref(data: dict) -> list[str]:
    fails = []
    topics = data.get("topics") or {}
    known = set(topics.keys())
    fields = ("depends_on", "related", "error_refs")
    for tid, t in topics.items():
        for f in fields:
            for ref in (t.get(f) or []):
                head = ref.split("#", 1)[0]
                if head not in known:
                    fails.append(f"#4 cross-reference: {tid!r}.{f} → {ref!r} (handoff yaml 에 없음)")
    for task_id, task in (data.get("tasks") or {}).items():
        for ref in (task.get("read") or []):
            head = ref.split(".", 1)[0].split("#", 1)[0]
            if head not in known:
                fails.append(f"#4 cross-reference: task {task_id!r}.read → {ref!r}")
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


def run_basic(nf: str, data: dict) -> tuple[int, int, list[str]]:
    """Return (pass_count, fail_count, failure_messages)."""
    rules = [
        ("#1", rule_1_schema(data)),
        ("#2", rule_2_status_enum(data)),
        ("#3", rule_3_topic_file_exists(nf, data)),
        ("#4", rule_4_cross_ref(data)),
        ("#5", rule_5_category_topic_consistency(data)),
        ("#6", rule_6_blocked_na_semantics(data)),
    ]
    failures: list[str] = []
    pass_count = 0
    for rid, errs in rules:
        if errs:
            failures.extend(errs)
        else:
            pass_count += 1
    return pass_count, len(failures), failures


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("nf")
    parser.add_argument("--level", choices=["basic", "strict"], default="basic")
    args = parser.parse_args()

    nf = args.nf.lower()
    handoff_yaml = REPO_ROOT / "handoff" / nf / "_handoff.yaml"
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
