#!/usr/bin/env python3
# design/<nf>/architecture + module-decomposition 의 자기 일관성을 검사해 _arch_status.yaml 산출
"""
Usage:
    .venv/bin/python3 design/scripts/nf-arch-status.py <nf> [--no-write]

옵션:
    --no-write   _arch_status.yaml 저장 없이 stdout 만 보고

본 script 는 architecture 단계 status 검사다. contract 단계는 nf-contract-check.py 의 책임이며
본 script 는 그것과 분리된다 (ADR-0001 § Migration notes (2026-05-14) follow-up).

설계 원칙 (plan docs/plans/2026-05-18-arch-impl-status-validator-plan.md).
  - read-only. 어떤 산출도 수정 안 함. _arch_status.yaml 만 기계가 매번 새로 씀.
  - discover 기반. NSSF module 명·operation 수 하드코딩 금지. module 은 glob 으로 discover.
  - semantic judge 제외. 구조·참조 정합성까지만. 3GPP 해석 품질 평가 안 함.
  - Phase 1 — exact headings + required files + frontmatter. cross-ref 는 advisory WARN (non-gate).

canonical 섹션 상수는 PR #20 으로 머지된 nf-arch-design/SKILL.md mandate 와 *동일 source*.
변경 시 SKILL.md mandate 와 1:1 로 함께 갱신해야 한다.

평가 framework — nf-contract-check.py 와 동형. 무가중치, check 별 criterion + to_pass 의무,
acceptance gate 는 binary check id 의 AND. WARN 은 advisory, gate 비포함.
"""

from __future__ import annotations

import argparse
import pathlib
import sys

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent

ARCH_FILES = [
    "overview.md",
    "module-boundaries.md",
    "request-flow.md",
    "runtime-model.md",
    "state-persistence.md",
    "configuration-strategy.md",
    "error-propagation.md",
    "observability.md",
    "test-strategy.md",
]
ADR_FILE = "decisions/ADR-0001-architecture-baseline.md"

ARCH_CANON = [
    "## Purpose", "## Inputs (contract)", "## Boundaries",
    "## Decisions", "## Open Questions", "## References",
]
ADR_CANON = [
    "## Context", "## Decision", "## Consequences",
    "## Open choices", "## References",
]
MODULE_CANON = [
    "## Responsibility", "## Inputs", "## Outputs", "## State",
    "## Decisions", "## Open Questions", "## References",
]

ARCH_FM_KEYS = {"nf", "stage", "status", "source_contract", "generated_date"}

GATE_DEFS = [
    ("arch_consistent", [
        "arch_files_present",
        "arch_sections_exact",
        "module_decomposition_present",
        "module_sections_exact",
        "arch_frontmatter_valid",
    ]),
]


def parse_frontmatter(text: str) -> tuple[dict, str]:
    """('---'...'---' frontmatter, body) 반환. frontmatter 없으면 ({}, full text)."""
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


def module_files(nf: str) -> list[pathlib.Path]:
    md = REPO / "design" / nf / "module-decomposition"
    if not md.is_dir():
        return []
    # _ prefix (template 잔재 등) 제외 — discover 기반, 실제 module 만.
    return sorted(p for p in md.glob("*.md") if not p.name.startswith("_"))


def check_arch_files_present(arch_dir: pathlib.Path) -> dict:
    base = {
        "id": "arch_files_present", "tier": 1,
        "name": "architecture 표준 파일 10 + ADR-0001 존재",
        "criterion": (
            "design/<nf>/architecture/ 에 " + ", ".join(ARCH_FILES)
            + f", {ADR_FILE} 모두 존재."
        ),
    }
    missing = [f for f in ARCH_FILES + [ADR_FILE] if not (arch_dir / f).exists()]
    if missing:
        base.update(status="FAIL", current=f"누락 — {missing}",
                    to_pass=[f"/nf-arch-design <nf> 으로 누락 파일 생성 — {missing}"])
    else:
        base.update(status="PASS",
                    current=f"{len(ARCH_FILES)+1}개 파일 모두 존재", to_pass=[])
    return base


def check_arch_sections_exact(arch_dir: pathlib.Path) -> dict:
    base = {
        "id": "arch_sections_exact", "tier": 1,
        "name": "arch 파일 H2 집합이 canonical 과 순서까지 일치 (ADR 은 ADR set)",
        "criterion": (
            f"각 arch .md 의 `## ` 목록 == {ARCH_CANON}. "
            f"{ADR_FILE} 의 `## ` 목록 == {ADR_CANON}."
        ),
    }
    bad = []
    for f in ARCH_FILES:
        p = arch_dir / f
        if not p.exists():
            bad.append(f"{f} (파일 없음)")
            continue
        got = h2(p)
        if got != ARCH_CANON:
            bad.append(f"{f}: {got}")
    adr = arch_dir / ADR_FILE
    if adr.exists():
        got = h2(adr)
        if got != ADR_CANON:
            bad.append(f"{ADR_FILE}: {got}")
    else:
        bad.append(f"{ADR_FILE} (파일 없음)")
    if bad:
        base.update(status="FAIL", current=f"불일치 — {bad}",
                    to_pass=["canonical 섹션 순서·집합을 정확히 맞춤 "
                             "(nf-arch-design SKILL.md mandate 참조)"])
    else:
        base.update(status="PASS",
                    current=f"{len(ARCH_FILES)} arch + ADR 모두 canonical 일치",
                    to_pass=[])
    return base


def check_module_decomposition_present(mods: list[pathlib.Path]) -> dict:
    base = {
        "id": "module_decomposition_present", "tier": 1,
        "name": "module-decomposition/*.md 1개 이상 존재",
        "criterion": "design/<nf>/module-decomposition/ 에 `_` prefix 가 아닌 .md ≥ 1.",
    }
    if not mods:
        base.update(status="FAIL", current="module 파일 0",
                    to_pass=["/nf-arch-design <nf> 으로 module-decomposition 생성"])
    else:
        base.update(status="PASS",
                    current=f"{len(mods)}개 — {[m.name for m in mods]}", to_pass=[])
    return base


def check_module_sections_exact(mods: list[pathlib.Path]) -> dict:
    base = {
        "id": "module_sections_exact", "tier": 1,
        "name": "각 module .md H2 집합이 canonical 7 과 순서까지 일치",
        "criterion": f"각 module-decomposition/*.md 의 `## ` 목록 == {MODULE_CANON}.",
    }
    if not mods:
        base.update(status="FAIL", current="module 파일 0 (검사 불가)",
                    to_pass=["module-decomposition 생성 후 재실행"])
        return base
    bad = [f"{m.name}: {h2(m)}" for m in mods if h2(m) != MODULE_CANON]
    if bad:
        base.update(status="FAIL", current=f"불일치 — {bad}",
                    to_pass=["module 섹션을 canonical 7 로 정확히 맞춤"])
    else:
        base.update(status="PASS",
                    current=f"{len(mods)}개 module 모두 canonical 일치", to_pass=[])
    return base


def check_arch_frontmatter_valid(arch_dir: pathlib.Path,
                                 mods: list[pathlib.Path]) -> dict:
    base = {
        "id": "arch_frontmatter_valid", "tier": 1,
        "name": "arch+module frontmatter valid YAML + 필수 키",
        "criterion": f"각 .md 상단 `---` 블록 yaml 통과 + 필수 키 {sorted(ARCH_FM_KEYS)}.",
    }
    bad = []
    targets = [arch_dir / f for f in ARCH_FILES] + mods
    for p in targets:
        if not p.exists():
            continue
        fm, _ = parse_frontmatter(p.read_text(encoding="utf-8"))
        if not fm:
            bad.append(f"{p.name} (frontmatter 없음/파싱 실패)")
            continue
        miss = ARCH_FM_KEYS - set(fm.keys())
        if miss:
            bad.append(f"{p.name} (누락 {sorted(miss)})")
    if bad:
        base.update(status="FAIL", current=f"불량 — {bad}",
                    to_pass=[f"frontmatter 필수 키 보강 — {sorted(ARCH_FM_KEYS)}"])
    else:
        base.update(status="PASS", current="모든 대상 frontmatter 유효", to_pass=[])
    return base


def check_module_boundaries_xref(arch_dir: pathlib.Path,
                                 mods: list[pathlib.Path]) -> dict:
    # advisory WARN — gate 비포함 (Phase 1). 깊은 정합은 Phase 2.
    base = {
        "id": "module_boundaries_xref", "tier": 2,
        "name": "module-boundaries.md 가 module-decomposition 파일을 언급 (advisory)",
        "criterion": ("module-boundaries.md 본문이 각 module 파일 stem 을 1회 이상 언급. "
                      "WARN 은 gate 비포함 — Phase 2 에서 id-relation 으로 강화."),
    }
    mb = arch_dir / "module-boundaries.md"
    if not mb.exists() or not mods:
        base.update(status="WARN", current="module-boundaries.md 또는 module 부재",
                    to_pass=["선행 check 통과 후 재평가"])
        return base
    text = mb.read_text(encoding="utf-8")
    unref = [m.stem for m in mods if m.stem not in text]
    if unref:
        base.update(status="WARN",
                    current=f"module-boundaries.md 미언급 — {unref}",
                    to_pass=[f"module-boundaries.md 에 {unref} 책임 행 추가 (advisory)"])
    else:
        base.update(status="PASS",
                    current=f"{len(mods)}개 module 모두 언급됨", to_pass=[])
    return base


def load_prev_pass_anyway(status_path: pathlib.Path) -> list:
    """기존 _arch_status.yaml 의 manual_overrides.pass_anyway 를 읽는다.
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
        "stage": "architecture-design",
        "target_schema": "arch-status-v1",
        "checks": ordered,
        "gates": gates,
        "manual_overrides": {"pass_anyway": pass_anyway or []},
    }
    header = [
        "# Auto-generated by design/scripts/nf-arch-status.py",
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

    nf_dir = REPO / "design" / nf
    if not nf_dir.is_dir():
        print(f"[nf-arch-status] design/{nf}/ 없음 — /nf-arch-design {nf} 먼저",
              file=sys.stderr)
        sys.exit(1)
    arch_dir = nf_dir / "architecture"
    mods = module_files(nf)

    checks = [
        check_arch_files_present(arch_dir),
        check_arch_sections_exact(arch_dir),
        check_module_decomposition_present(mods),
        check_module_sections_exact(mods),
        check_arch_frontmatter_valid(arch_dir, mods),
        check_module_boundaries_xref(arch_dir, mods),
    ]
    out_path = nf_dir / "_arch_status.yaml"
    pass_anyway = load_prev_pass_anyway(out_path)
    applied, ignored = apply_overrides(checks, pass_anyway)
    gates = compute_gates(checks)
    yaml_text = render_yaml(nf, checks, gates, pass_anyway, ignored)

    if not args.no_write:
        out_path.write_text(yaml_text, encoding="utf-8")
        print(f"[nf-arch-status] wrote {out_path.relative_to(REPO)}", file=sys.stderr)
    if applied:
        print(f"[nf-arch-status] override 적용 — {applied}", file=sys.stderr)
    if ignored:
        print(f"[nf-arch-status] override 무시 — {ignored}", file=sys.stderr)

    print(yaml_text)

    p = sum(1 for c in checks if c["status"] == "PASS")
    f = sum(1 for c in checks if c["status"] == "FAIL")
    w = sum(1 for c in checks if c["status"] == "WARN")
    print(f"\n[nf-arch-status] {nf}: PASS {p}, FAIL {f}, WARN {w}", file=sys.stderr)
    for g in gates:
        if g["status"] == "FAIL":
            print(f"  gate {g['id']}: FAIL — blocked by {g['blocked_by']}",
                  file=sys.stderr)
        else:
            print(f"  gate {g['id']}: PASS", file=sys.stderr)
    sys.exit(1 if any(g["status"] == "FAIL" for g in gates) else 0)


if __name__ == "__main__":
    main()
