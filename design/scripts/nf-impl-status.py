#!/usr/bin/env python3
# dev/<nf>/ 의 implementation-planning 산출 자기 일관성을 검사해 _impl_status.yaml 산출
"""
Usage:
    .venv/bin/python3 design/scripts/nf-impl-status.py <nf> [--no-write]

옵션:
    --no-write   _impl_status.yaml 저장 없이 stdout 만 보고

본 script 는 implementation-planning 단계 status 검사다. contract 단계는 nf-status.py,
architecture 단계는 nf-arch-status.py 의 책임이며 본 script 는 그것들과 분리된다
(ADR-0001 L54 follow-up).

설계 원칙 (plan docs/plans/2026-05-18-arch-impl-status-validator-plan.md).
  - read-only. dev 산출 수정 안 함. _impl_status.yaml 만 기계가 매번 새로 씀.
  - discover 기반. task/test id 하드코딩 금지. 파일에서 읽어 관계만 검사.
  - semantic judge 제외. 구조·schema·참조 존재까지만.
  - Phase 1 — exact headings + required files + frontmatter + tasks.yaml schema.
    traceability_xref 는 advisory WARN (non-gate). 깊은 id-relation 은 Phase 2.

canonical 섹션 상수는 PR #20 으로 머지된 nf-impl-plan/SKILL.md mandate 와 *동일 source*.
변경 시 SKILL.md mandate 와 1:1 로 함께 갱신해야 한다.

평가 framework — nf-status.py 와 동형. 무가중치, check 별 criterion + to_pass 의무,
acceptance gate 는 binary check id 의 AND. WARN 은 advisory, gate 비포함.
"""

from __future__ import annotations

import argparse
import pathlib
import sys

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent

DEV_MD_CANON = {
    "implementation-plan.md": [
        "## Scope", "## Phases", "## Test Plan", "## Open Risks", "## References",
    ],
    "test-matrix.md": [
        "## Purpose", "## Test Inventory", "## Coverage Rules",
        "## Open Questions", "## References",
    ],
    "traceability.md": [
        "## Contract → Module", "## Module → Test", "## Open Gaps", "## References",
    ],
}
DEV_FILES = list(DEV_MD_CANON) + ["tasks.yaml"]
DEV_FM_KEYS = {"nf", "stage", "status", "source_architecture",
               "source_contract", "generated_date"}
TASKS_SCHEMA = "impl-plan-v1"

GATE_DEFS = [
    ("impl_consistent", [
        "impl_files_present",
        "impl_sections_exact",
        "tasks_schema_valid",
        "impl_frontmatter_valid",
    ]),
]


def parse_frontmatter(text: str) -> tuple[dict, str]:
    if not text.startswith("---\n"):
        return {}, text
    end = text.find("\n---\n", 4)
    if end == -1:
        return {}, text
    try:
        fm = yaml.safe_load(text[4:end]) or {}
    except yaml.YAMLError:
        fm = {}
    return fm, text[end + 5:]


def h2(path: pathlib.Path) -> list[str]:
    return [ln.strip() for ln in path.read_text(encoding="utf-8").splitlines()
            if ln.startswith("## ")]


def check_impl_files_present(dev_dir: pathlib.Path) -> dict:
    base = {
        "id": "impl_files_present", "tier": 1,
        "name": "dev 표준 파일 4 존재",
        "criterion": "dev/<nf>/ 에 " + ", ".join(DEV_FILES) + " 모두 존재.",
    }
    missing = [f for f in DEV_FILES if not (dev_dir / f).exists()]
    if missing:
        base.update(status="FAIL", current=f"누락 — {missing}",
                    to_pass=[f"/nf-impl-plan <nf> 으로 누락 파일 생성 — {missing}"])
    else:
        base.update(status="PASS", current="4개 파일 모두 존재", to_pass=[])
    return base


def check_impl_sections_exact(dev_dir: pathlib.Path) -> dict:
    base = {
        "id": "impl_sections_exact", "tier": 1,
        "name": "dev .md H2 집합이 canonical 과 순서까지 일치",
        "criterion": "implementation-plan 5 / test-matrix 5 / traceability 4 "
                     "canonical 섹션 순서 일치 (nf-impl-plan SKILL.md mandate).",
    }
    bad = []
    for fn, want in DEV_MD_CANON.items():
        p = dev_dir / fn
        if not p.exists():
            bad.append(f"{fn} (파일 없음)")
            continue
        got = h2(p)
        if got != want:
            bad.append(f"{fn}: {got}")
    if bad:
        base.update(status="FAIL", current=f"불일치 — {bad}",
                    to_pass=["canonical 섹션 순서·집합을 정확히 맞춤 "
                             "(nf-impl-plan SKILL.md mandate 참조)"])
    else:
        base.update(status="PASS", current="3개 .md 모두 canonical 일치",
                    to_pass=[])
    return base


def check_tasks_schema_valid(dev_dir: pathlib.Path) -> dict:
    base = {
        "id": "tasks_schema_valid", "tier": 1,
        "name": "tasks.yaml schema_version + 각 task id/trace_to",
        "criterion": (f"tasks.yaml `schema_version: {TASKS_SCHEMA}`. "
                      "각 task 에 비어있지 않은 `id` 와 `trace_to`."),
    }
    p = dev_dir / "tasks.yaml"
    if not p.exists():
        base.update(status="FAIL", current="tasks.yaml 없음",
                    to_pass=["/nf-impl-plan <nf> 으로 tasks.yaml 생성"])
        return base
    try:
        data = yaml.safe_load(p.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        base.update(status="FAIL", current=f"YAML 파싱 실패 — {e}",
                    to_pass=["tasks.yaml YAML 문법 수정"])
        return base
    problems = []
    if data.get("schema_version") != TASKS_SCHEMA:
        problems.append(f"schema_version != {TASKS_SCHEMA} "
                        f"(현 {data.get('schema_version')!r})")
    tasks = data.get("tasks") or []
    if not tasks:
        problems.append("tasks 비어있음")
    for i, t in enumerate(tasks):
        if not t.get("id"):
            problems.append(f"tasks[{i}] id 없음")
        if not t.get("trace_to"):
            problems.append(f"task {t.get('id', i)} trace_to 비어있음")
    if problems:
        base.update(status="FAIL", current=f"불량 — {problems}",
                    to_pass=["schema_version 와 각 task 의 id/trace_to 보강"])
    else:
        base.update(status="PASS",
                    current=f"schema={TASKS_SCHEMA}, task {len(tasks)}개 모두 traceable",
                    to_pass=[])
    return base


def check_impl_frontmatter_valid(dev_dir: pathlib.Path) -> dict:
    base = {
        "id": "impl_frontmatter_valid", "tier": 1,
        "name": "dev .md frontmatter valid YAML + 필수 키",
        "criterion": f"3개 .md 상단 `---` 블록 yaml 통과 + 필수 키 {sorted(DEV_FM_KEYS)}.",
    }
    bad = []
    for fn in DEV_MD_CANON:
        p = dev_dir / fn
        if not p.exists():
            continue
        fm, _ = parse_frontmatter(p.read_text(encoding="utf-8"))
        if not fm:
            bad.append(f"{fn} (frontmatter 없음/파싱 실패)")
            continue
        miss = DEV_FM_KEYS - set(fm.keys())
        if miss:
            bad.append(f"{fn} (누락 {sorted(miss)})")
    if bad:
        base.update(status="FAIL", current=f"불량 — {bad}",
                    to_pass=[f"frontmatter 필수 키 보강 — {sorted(DEV_FM_KEYS)}"])
    else:
        base.update(status="PASS", current="3개 .md frontmatter 유효", to_pass=[])
    return base


def check_traceability_xref(dev_dir: pathlib.Path) -> dict:
    # advisory WARN — gate 비포함 (Phase 1). 깊은 id-relation 은 Phase 2.
    base = {
        "id": "traceability_xref", "tier": 2,
        "name": "tasks.yaml id 가 traceability.md/test-matrix.md 에서 참조 (advisory)",
        "criterion": ("각 task id 가 traceability.md 또는 test-matrix.md 본문에 등장. "
                      "WARN 은 gate 비포함 — Phase 2 에서 id-relation 무결성으로 강화."),
    }
    tp = dev_dir / "tasks.yaml"
    tr = dev_dir / "traceability.md"
    tm = dev_dir / "test-matrix.md"
    if not tp.exists() or not tr.exists():
        base.update(status="WARN", current="tasks.yaml 또는 traceability.md 부재",
                    to_pass=["선행 check 통과 후 재평가"])
        return base
    try:
        tasks = (yaml.safe_load(tp.read_text(encoding="utf-8")) or {}).get("tasks") or []
    except yaml.YAMLError:
        base.update(status="WARN", current="tasks.yaml 파싱 실패 (선행 check 참조)",
                    to_pass=["tasks_schema_valid 먼저 통과"])
        return base
    text = tr.read_text(encoding="utf-8")
    if tm.exists():
        text += tm.read_text(encoding="utf-8")
    ids = [t.get("id") for t in tasks if t.get("id")]
    unref = [i for i in ids if i not in text]
    if not ids:
        base.update(status="WARN", current="task id 없음 (선행 check 참조)",
                    to_pass=["tasks_schema_valid 먼저 통과"])
    elif unref:
        base.update(status="WARN",
                    current=f"traceability.md/test-matrix.md 미참조 task — {unref}",
                    to_pass=[f"traceability.md 에 {unref} 매핑 행 추가 (advisory)"])
    else:
        base.update(status="PASS",
                    current=f"{len(ids)}개 task 모두 참조됨", to_pass=[])
    return base


def load_prev_pass_anyway(status_path: pathlib.Path) -> list:
    """기존 _impl_status.yaml 의 manual_overrides.pass_anyway 를 읽는다.
    이를 보존·적용하지 않으면 매 실행이 빈 리스트로 덮어써 우회가 영영 무효다."""
    if not status_path.exists():
        return []
    try:
        prev = yaml.safe_load(status_path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError:
        return []
    raw = (prev.get("manual_overrides") or {}).get("pass_anyway") or []
    return raw if isinstance(raw, list) else []


def apply_overrides(checks: list[dict], raw: list) -> tuple[list[str], list[str]]:
    """pass_anyway 항목을 검증·적용. 유효 항목은 해당 FAIL check 를 overridden 표시.
    유효 조건 — dict 이고 check/reason/confirmed_by 모두 비어있지 않음."""
    by_id = {c["id"]: c for c in checks}
    applied, ignored = [], []
    for e in raw:
        if not isinstance(e, dict):
            ignored.append(f"{e!r} (dict 아님)")
            continue
        cid = e.get("check")
        if not (cid and e.get("reason") and e.get("confirmed_by")):
            ignored.append(f"{cid or e!r} (check/reason/confirmed_by 필수)")
            continue
        c = by_id.get(cid)
        if c is None:
            ignored.append(f"{cid} (해당 check 없음)")
        elif c["status"] != "FAIL":
            ignored.append(f"{cid} (FAIL 아님 — override 불필요)")
        else:
            c["overridden"] = True
            applied.append(cid)
    return applied, ignored


def compute_gates(checks: list[dict]) -> list[dict]:
    by_id = {c["id"]: c for c in checks}
    out = []
    for gate_id, required in GATE_DEFS:
        blocked = []
        for cid in required:
            c = by_id.get(cid)
            if c is None:
                blocked.append(f"{cid} (check 미존재)")
            elif c["status"] == "FAIL" and not c.get("overridden"):
                blocked.append(cid)
        out.append({
            "id": gate_id, "requires_pass": required,
            "status": "FAIL" if blocked else "PASS",
            "blocked_by": blocked,
        })
    return out


def render_yaml(nf: str, checks: list[dict], gates: list[dict],
                pass_anyway: list, ignored: list[str]) -> str:
    """단일 YAML document 로 안전 직렬화 (yaml.safe_dump).
    수동 f-string 따옴표는 current 의 list repr 같은 특수문자에서 깨지고,
    그러면 load_prev_pass_anyway 의 재파싱이 실패해 우회가 무효가 된다."""
    ordered = []
    for c in checks:
        keys = ["id", "tier", "name", "criterion", "status"]
        if c.get("overridden"):
            keys.append("overridden")
        keys += ["current", "to_pass"]
        ordered.append({k: c.get(k, "" if k != "to_pass" else []) for k in keys})
    doc = {
        "nf": nf,
        "stage": "implementation-planning",
        "target_schema": "impl-status-v1",
        "checks": ordered,
        "gates": gates,
        "manual_overrides": {"pass_anyway": pass_anyway or []},
    }
    header = [
        "# Auto-generated by design/scripts/nf-impl-status.py",
        "# checks/gates 는 기계 재생성 — 손으로 수정하지 않음.",
        "# manual_overrides.pass_anyway 만 사용자가 편집: FAIL check 를 gate 에서 제외.",
        "#   각 항목 = {check, reason, confirmed_by} 모두 필수. 다음 실행에 보존·적용됨.",
    ]
    if ignored:
        header.append(f"# 직전 실행 무시된 override 항목 — {ignored}")
    body = yaml.safe_dump(doc, allow_unicode=True, sort_keys=False,
                          default_flow_style=False)
    return "\n".join(header) + "\n\n" + body


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("nf")
    ap.add_argument("--no-write", action="store_true")
    args = ap.parse_args()
    nf = args.nf

    dev_dir = REPO / "dev" / nf
    if not dev_dir.is_dir():
        print(f"[nf-impl-status] dev/{nf}/ 없음 — /nf-impl-plan {nf} 먼저",
              file=sys.stderr)
        sys.exit(1)

    checks = [
        check_impl_files_present(dev_dir),
        check_impl_sections_exact(dev_dir),
        check_tasks_schema_valid(dev_dir),
        check_impl_frontmatter_valid(dev_dir),
        check_traceability_xref(dev_dir),
    ]
    out_path = dev_dir / "_impl_status.yaml"
    pass_anyway = load_prev_pass_anyway(out_path)
    applied, ignored = apply_overrides(checks, pass_anyway)
    gates = compute_gates(checks)
    yaml_text = render_yaml(nf, checks, gates, pass_anyway, ignored)

    if not args.no_write:
        out_path.write_text(yaml_text, encoding="utf-8")
        print(f"[nf-impl-status] wrote {out_path.relative_to(REPO)}", file=sys.stderr)
    if applied:
        print(f"[nf-impl-status] override 적용 — {applied}", file=sys.stderr)
    if ignored:
        print(f"[nf-impl-status] override 무시 — {ignored}", file=sys.stderr)

    print(yaml_text)

    p = sum(1 for c in checks if c["status"] == "PASS")
    f = sum(1 for c in checks if c["status"] == "FAIL")
    w = sum(1 for c in checks if c["status"] == "WARN")
    print(f"\n[nf-impl-status] {nf}: PASS {p}, FAIL {f}, WARN {w}", file=sys.stderr)
    for g in gates:
        if g["status"] == "FAIL":
            print(f"  gate {g['id']}: FAIL — blocked by {g['blocked_by']}",
                  file=sys.stderr)
        else:
            print(f"  gate {g['id']}: PASS", file=sys.stderr)
    sys.exit(1 if any(g["status"] == "FAIL" for g in gates) else 0)


if __name__ == "__main__":
    main()
