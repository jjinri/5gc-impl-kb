#!/usr/bin/env python3
# Cross-source drift validator for execution-control + autonomous-impl-prep pack.
"""
Usage:
    .venv/bin/python3 design/scripts/validate-execution-control-drift.py [<nf>]

목적
- agent-execution-plan / team-execution-plan / verification-matrix /
  codegen-work-items / pr-slicing-plan / readiness-config 사이의 id /
  reference 일관성 검증.
- 자율 agent 가 서로 다른 source 를 읽고 엇갈리는 위험 차단.

검사 항목
1. WI id 일관성 — codegen-work-items 가 진실 source. 다른 yaml 의 모든
   WI 참조가 codegen-work-items.items[].id 와 매칭.
2. phase id 일관성 — agent-execution-plan.phases[].id 가 진실 source.
   verification-matrix / pr-slicing-plan 의 phase 참조가 매칭 (ALL 은
   project-wide 의미).
3. check id 일관성 — verification-matrix.checks[].id 가 진실 source.
   agent-execution-plan 의 required_checks 와 pr-slicing-plan 의 required_
   checks 가 매칭.
4. PR slice id 일관성 — pr-slicing-plan.prs[].id 가 진실 source. depends_on
   참조가 모두 존재하고 cycle 없음.
5. phase ↔ work_items aggregate — readiness-config.phase_policy.phases.
   phaseN.work_items 가 agent-execution-plan.phases[].work_items 의
   union 과 일치 (Phase 1 은 wave 0 + wave 1 합집합).

산출
- stderr 에 검사별 PASS/FAIL 보고.
- exit 0 = PASS, exit 1 = FAIL.

본 script 는 *read-only*. 어떤 산출도 수정하지 않는다.
"""

from __future__ import annotations

import pathlib
import re
import sys

import yaml


REPO = pathlib.Path(__file__).resolve().parent.parent.parent
DEFAULT_NF = "nssf"

# special tokens that bypass id matching.
WI_SPECIAL = {"ALL", "ALL_WRITE"}
PHASE_SPECIAL = {"ALL"}


def _load_yaml(path: pathlib.Path) -> dict:
    if not path.is_file():
        return {}
    try:
        return yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        print(f"[drift] FAIL YAML parse {path.relative_to(REPO)} — {e}", file=sys.stderr)
        sys.exit(2)


def _wi_ids_from_codegen(doc: dict) -> set[str]:
    return {str(it.get("id")) for it in doc.get("items", []) if isinstance(it, dict) and it.get("id")}


def _phase_ids_from_agent(doc: dict) -> set[str]:
    return {str(p.get("id")) for p in doc.get("phases", []) if isinstance(p, dict) and p.get("id")}


def _check_ids_from_matrix(doc: dict) -> set[str]:
    return {str(c.get("id")) for c in doc.get("checks", []) if isinstance(c, dict) and c.get("id")}


def _pr_ids_from_slicing(doc: dict) -> set[str]:
    return {str(p.get("id")) for p in doc.get("prs", []) if isinstance(p, dict) and p.get("id")}


def _as_list(value) -> list:
    if value is None:
        return []
    if isinstance(value, list):
        return value
    return [value]


def check_wi_consistency(*, codegen: dict, agent: dict, matrix: dict, slicing: dict, readiness: dict) -> list[str]:
    fails: list[str] = []
    truth = _wi_ids_from_codegen(codegen)
    if not truth:
        fails.append("codegen-work-items.items[].id empty — 진실 source 부재")
        return fails

    # agent-execution-plan.phases[].work_items[]
    for phase in agent.get("phases", []):
        if not isinstance(phase, dict):
            continue
        phase_id = phase.get("id", "?")
        for wi in _as_list(phase.get("work_items")):
            if str(wi) and str(wi) not in truth:
                fails.append(f"agent-execution-plan.phases[{phase_id}].work_items: '{wi}' not in codegen-work-items")

    # verification-matrix.checks[].work_item
    for check in matrix.get("checks", []):
        if not isinstance(check, dict):
            continue
        check_id = check.get("id", "?")
        wi_value = check.get("work_item")
        for wi in _as_list(wi_value):
            wi_str = str(wi)
            if wi_str in WI_SPECIAL:
                continue
            if wi_str and wi_str not in truth:
                fails.append(f"verification-matrix.checks[{check_id}].work_item: '{wi_str}' not in codegen-work-items")

    # pr-slicing-plan.prs[].work_items[]
    for pr in slicing.get("prs", []):
        if not isinstance(pr, dict):
            continue
        pr_id = pr.get("id", "?")
        for wi in _as_list(pr.get("work_items")):
            wi_str = str(wi)
            if wi_str and wi_str not in truth:
                fails.append(f"pr-slicing-plan.prs[{pr_id}].work_items: '{wi_str}' not in codegen-work-items")

    # readiness-config phase_policy work_items
    phases = (readiness.get("phase_policy") or {}).get("phases") or {}
    for ph_key, body in phases.items():
        if not isinstance(body, dict):
            continue
        for wi in _as_list(body.get("work_items")):
            wi_str = str(wi)
            if wi_str and wi_str not in truth:
                fails.append(f"readiness-config.phase_policy.{ph_key}.work_items: '{wi_str}' not in codegen-work-items")
    return fails


def check_phase_consistency(*, agent: dict, matrix: dict, slicing: dict) -> list[str]:
    fails: list[str] = []
    truth = _phase_ids_from_agent(agent)
    if not truth:
        fails.append("agent-execution-plan.phases[].id empty — 진실 source 부재")
        return fails

    # verification-matrix.checks[].phase
    for check in matrix.get("checks", []):
        if not isinstance(check, dict):
            continue
        check_id = check.get("id", "?")
        ph = str(check.get("phase", ""))
        if not ph:
            continue
        if ph in PHASE_SPECIAL:
            continue
        if ph not in truth:
            fails.append(f"verification-matrix.checks[{check_id}].phase: '{ph}' not in agent-execution-plan phases")

    # pr-slicing-plan.prs[].phase
    for pr in slicing.get("prs", []):
        if not isinstance(pr, dict):
            continue
        pr_id = pr.get("id", "?")
        ph = str(pr.get("phase", ""))
        if not ph:
            continue
        if ph in PHASE_SPECIAL:
            continue
        if ph not in truth:
            fails.append(f"pr-slicing-plan.prs[{pr_id}].phase: '{ph}' not in agent-execution-plan phases")
    return fails


def check_check_id_consistency(*, agent: dict, matrix: dict, slicing: dict) -> list[str]:
    fails: list[str] = []
    truth = _check_ids_from_matrix(matrix)
    if not truth:
        fails.append("verification-matrix.checks[].id empty — 진실 source 부재")
        return fails

    # agent-execution-plan.phases[].required_checks[]
    for phase in agent.get("phases", []):
        if not isinstance(phase, dict):
            continue
        phase_id = phase.get("id", "?")
        for chk in _as_list(phase.get("required_checks")):
            chk_str = str(chk)
            if chk_str and chk_str not in truth:
                fails.append(f"agent-execution-plan.phases[{phase_id}].required_checks: '{chk_str}' not in verification-matrix")

    # pr-slicing-plan.prs[].required_checks[]
    for pr in slicing.get("prs", []):
        if not isinstance(pr, dict):
            continue
        pr_id = pr.get("id", "?")
        for chk in _as_list(pr.get("required_checks")):
            chk_str = str(chk)
            if chk_str and chk_str not in truth:
                fails.append(f"pr-slicing-plan.prs[{pr_id}].required_checks: '{chk_str}' not in verification-matrix")
    return fails


def check_pr_slice_consistency(*, slicing: dict) -> list[str]:
    fails: list[str] = []
    prs = slicing.get("prs", []) or []
    truth = _pr_ids_from_slicing(slicing)
    if not truth:
        fails.append("pr-slicing-plan.prs[].id empty — 진실 source 부재")
        return fails

    # depends_on 참조 존재
    depends: dict[str, list[str]] = {}
    for pr in prs:
        if not isinstance(pr, dict):
            continue
        pr_id = pr.get("id", "?")
        deps_list: list[str] = []
        for dep in _as_list(pr.get("depends_on")):
            dep_str = str(dep)
            deps_list.append(dep_str)
            if dep_str and dep_str not in truth:
                fails.append(f"pr-slicing-plan.prs[{pr_id}].depends_on: '{dep_str}' not in PR slice ids")
        depends[str(pr_id)] = deps_list

    # cycle 검출 — Kahn algorithm
    indeg = {k: 0 for k in depends}
    for k, deps_list in depends.items():
        for d in deps_list:
            if d in indeg:
                indeg[k] += 1
    queue = [k for k, v in indeg.items() if v == 0]
    visited = 0
    while queue:
        n = queue.pop()
        visited += 1
        for k, deps_list in depends.items():
            if n in deps_list:
                indeg[k] -= 1
                if indeg[k] == 0:
                    queue.append(k)
    if visited < len(depends):
        fails.append(f"pr-slicing-plan.prs.depends_on cycle detected — {len(depends) - visited} node(s) unresolved")
    return fails


def check_phase_wi_aggregate(*, agent: dict, readiness: dict) -> list[str]:
    """readiness-config.phase_policy 의 phase 별 WI set 과 agent-execution-plan
    의 같은 phase prefix 의 WI set union 이 일치.

    예: readiness-config.phase1.work_items
       == agent-execution-plan.phase1_wave0.work_items
        + agent-execution-plan.phase1_wave1.work_items
    """
    fails: list[str] = []
    phases_cfg = (readiness.get("phase_policy") or {}).get("phases") or {}
    if not phases_cfg:
        return fails  # nothing to compare

    # group agent phases by leading "phaseN" prefix.
    agent_by_prefix: dict[str, set[str]] = {}
    for ph in agent.get("phases", []):
        if not isinstance(ph, dict):
            continue
        ph_id = str(ph.get("id", ""))
        m = re.match(r"^(phase\d+)", ph_id)
        if not m:
            continue
        key = m.group(1)
        wis = {str(w) for w in _as_list(ph.get("work_items"))}
        agent_by_prefix.setdefault(key, set()).update(wis)

    for ph_key, body in phases_cfg.items():
        if not isinstance(body, dict):
            continue
        cfg_set = {str(w) for w in _as_list(body.get("work_items"))}
        agent_set = agent_by_prefix.get(ph_key, set())
        if cfg_set != agent_set:
            missing_in_agent = cfg_set - agent_set
            missing_in_cfg = agent_set - cfg_set
            parts = []
            if missing_in_agent:
                parts.append(f"missing in agent: {sorted(missing_in_agent)}")
            if missing_in_cfg:
                parts.append(f"missing in readiness-config: {sorted(missing_in_cfg)}")
            fails.append(f"phase WI aggregate mismatch [{ph_key}] — " + "; ".join(parts))
    return fails


def main() -> int:
    nf = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_NF
    paths = {
        "codegen": REPO / "dev" / nf / "codegen-work-items.yaml",
        "agent": REPO / "dev" / nf / "agent-execution-plan.yaml",
        "matrix": REPO / "dev" / nf / "verification-matrix.yaml",
        "slicing": REPO / "dev" / nf / "pr-slicing-plan.yaml",
        "readiness": REPO / "design" / nf / "readiness-config.yaml",
    }
    missing = [k for k, p in paths.items() if not p.is_file()]
    if missing:
        print(f"[drift] FAIL missing source(s) — {missing}", file=sys.stderr)
        return 2

    docs = {k: _load_yaml(p) for k, p in paths.items()}

    sections = [
        ("wi_consistency",
         check_wi_consistency(codegen=docs["codegen"], agent=docs["agent"],
                              matrix=docs["matrix"], slicing=docs["slicing"],
                              readiness=docs["readiness"])),
        ("phase_consistency",
         check_phase_consistency(agent=docs["agent"], matrix=docs["matrix"],
                                 slicing=docs["slicing"])),
        ("check_id_consistency",
         check_check_id_consistency(agent=docs["agent"], matrix=docs["matrix"],
                                    slicing=docs["slicing"])),
        ("pr_slice_consistency",
         check_pr_slice_consistency(slicing=docs["slicing"])),
        ("phase_wi_aggregate",
         check_phase_wi_aggregate(agent=docs["agent"], readiness=docs["readiness"])),
    ]

    total_fails = 0
    for name, fails in sections:
        if fails:
            total_fails += len(fails)
            print(f"[drift] FAIL {name} — {len(fails)} issue(s)", file=sys.stderr)
            for f in fails:
                print(f"  - {f}", file=sys.stderr)
        else:
            print(f"[drift] PASS {name}", file=sys.stderr)

    if total_fails > 0:
        print(f"[drift] {total_fails} drift issue(s) — execution-control sources mismatch", file=sys.stderr)
        return 1
    print("[drift] PASS — execution-control sources consistent.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
