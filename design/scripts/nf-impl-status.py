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

# Markdown table parser — escape-aware contract (PR-13a). 본 모듈 의
# parse_md_table_section 은 thin wrapper — semantic 은 lib/md_table.py.
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from lib.md_table import parse_section as _md_parse_section  # noqa: E402

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

# Implementation Readiness Pack — plan §4.4 (2026-05-21).
# SKILL.md mandate 와 1:1 동기화. SKILL.md 갱신 시 본 상수도 함께 갱신.
READINESS_PACK_MD_CANON = {
    "api-implementation-matrix.md": [
        "## Operation Matrix", "## Coverage", "## Open Questions", "## References",
    ],
    "data-model-implementation-map.md": [
        "## Schema Classification", "## Wrapper Rationale", "## Open Schemas",
        "## References",
    ],
    "team-execution-plan.md": [
        "## Orchestrator Lane", "## Code Lane", "## Reviewer Lane",
        "## Tester Lane", "## Verifier Lane", "## Integration Order",
        "## References",
    ],
    "verification-plan.md": [
        "## Unit", "## Integration", "## Contract", "## Security",
        "## End-to-End", "## Observability", "## References",
    ],
    "implementation-readiness-review.md": [
        "## Executive Summary", "## Scope Confirmation", "## Risk Assessment",
        "## Decision Audit", "## Recommendation", "## References",
    ],
    "design-adequacy-checklist.md": [
        "## Checklist", "## Outstanding", "## References",
    ],
    "spec-to-design-coverage.md": [
        "## Coverage Trace", "## Gaps", "## References",
    ],
    "open-gaps-and-assumptions.md": [
        "## Gaps", "## Summary", "## References",
    ],
}
READINESS_PACK_FILES = list(READINESS_PACK_MD_CANON) + ["codegen-work-items.yaml"]
HUMAN_REVIEW_PACK = (
    "implementation-readiness-review.md",
    "design-adequacy-checklist.md",
    "spec-to-design-coverage.md",
    "open-gaps-and-assumptions.md",
)
WORK_ITEMS_SCHEMA = "codegen-work-items-v1"
# SKILL.md mandate — 각 items[] 가 가져야 하는 8 키.
# `depends_on` 은 empty list 허용, 나머지는 non-empty 요구.
WORK_ITEM_REQUIRED_SCALAR_KEYS = ("id", "owner_lane")
WORK_ITEM_REQUIRED_LIST_KEYS_NONEMPTY = (
    "inputs", "expected_files", "acceptance", "tests", "verification_commands",
)
WORK_ITEM_REQUIRED_LIST_KEYS_MAY_EMPTY = ("depends_on",)
GAP_CATEGORIES = {
    "blocker", "deferred", "operator-provided", "library-assumed",
    "test-gap", "assumption",
}

GATE_DEFS = [
    ("impl_consistent", [
        "impl_files_present",
        "impl_sections_exact",
        "tasks_schema_valid",
        "impl_frontmatter_valid",
    ]),
    ("impl_ready_for_codegen", [
        "readiness_pack_files_present",
        "api_matrix_covers_all_operations",
        "data_model_map_covers_all_contract_models",
        "all_work_items_have_expected_files",
        "all_work_items_have_tests",
        "all_work_items_have_verification_commands",
        "team_execution_plan_present",
        "phase_wi_coverage",
        "wi_depends_on_valid",
        "wi_phase_order_valid",
        "human_review_pack_traceable",
        "gaps_classified",
        "blocker_gaps_zero",
        "no_spec_reread_required",
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


def parse_md_table_section(text: str, header: str) -> list[list[str]]:
    """Thin wrapper around `lib.md_table.parse_section` — escape-aware.

    `\\|` 가 cell-internal pipe 로 처리되고 unescape 됨 (PR-13a).
    이전 naive split('|') 은 `\\|` 를 cell separator 로 잘못 해석해 cell
    index shift 위험이 있었음. 본 wrapper 는 의미적으로 동일하지만
    escape contract 를 lib 으로 위임.
    """
    return _md_parse_section(text, header)


def load_contract(nf: str) -> dict | None:
    p = REPO / "handoff" / nf / "contract.yaml"
    if not p.exists():
        return None
    try:
        return yaml.safe_load(p.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError:
        return None


def contract_topic_names(contract: dict | None, prefix: str) -> set[str]:
    if not contract:
        return set()
    topics = contract.get("topics") or {}
    if not isinstance(topics, dict):
        return set()
    out = set()
    pref = prefix.rstrip("/") + "/"
    for k in topics:
        if isinstance(k, str) and k.startswith(pref):
            tail = k[len(pref):]
            if tail:
                out.add(tail)
    return out


def check_readiness_pack_files_present(dev_dir: pathlib.Path) -> dict:
    base = {
        "id": "readiness_pack_files_present", "tier": 1,
        "name": "Readiness Pack 9 파일 존재",
        "criterion": "dev/<nf>/ 에 " + ", ".join(READINESS_PACK_FILES) + " 모두 존재.",
    }
    missing = [f for f in READINESS_PACK_FILES if not (dev_dir / f).exists()]
    if missing:
        base.update(status="FAIL", current=f"누락 — {missing}",
                    to_pass=[f"/nf-impl-plan <nf> 으로 누락 파일 생성 — {missing}"])
    else:
        base.update(status="PASS", current="9개 파일 모두 존재", to_pass=[])
    return base


def _matrix_first_column(dev_dir: pathlib.Path, fname: str, header: str) -> set[str]:
    p = dev_dir / fname
    if not p.exists():
        return set()
    rows = parse_md_table_section(p.read_text(encoding="utf-8"), header)
    return {r[0] for r in rows if r and r[0] and r[0].upper() != "TODO"}


def check_api_matrix_covers_all_operations(dev_dir: pathlib.Path,
                                           contract: dict | None) -> dict:
    base = {
        "id": "api_matrix_covers_all_operations", "tier": 1,
        "name": "api-implementation-matrix.md 가 contract API operation 전체 cover",
        "criterion": ("api-implementation-matrix.md `## Operation Matrix` 표의 "
                      "operationId 열이 handoff/<nf>/contract.yaml 의 "
                      "`topics.api/<Op>` 키 집합을 모두 포함."),
    }
    contract_ops = contract_topic_names(contract, "api")
    if not contract_ops:
        base.update(status="FAIL",
                    current="contract.yaml 의 topics.api/* 키 없음 또는 load 실패",
                    to_pass=["/nf-contract-check <nf> 로 contract 재생성 후 재실행"])
        return base
    have = _matrix_first_column(dev_dir, "api-implementation-matrix.md",
                                "Operation Matrix")
    missing = sorted(contract_ops - have)
    if missing:
        base.update(status="FAIL",
                    current=f"누락 operation — {missing}",
                    to_pass=[f"api-implementation-matrix.md `## Operation Matrix` "
                             f"표에 {missing} 행 추가"])
    else:
        base.update(status="PASS",
                    current=f"{len(contract_ops)}개 operation 모두 cover",
                    to_pass=[])
    return base


def check_data_model_map_covers_all_contract_models(dev_dir: pathlib.Path,
                                                    contract: dict | None) -> dict:
    base = {
        "id": "data_model_map_covers_all_contract_models", "tier": 1,
        "name": "data-model-implementation-map.md 가 contract data-model 전체 cover",
        "criterion": ("data-model-implementation-map.md `## Schema Classification` "
                      "표의 schema 열이 contract.yaml 의 `topics.data-model/<S>` 키 "
                      "집합을 모두 포함."),
    }
    contract_models = contract_topic_names(contract, "data-model")
    if not contract_models:
        base.update(status="FAIL",
                    current="contract.yaml 의 topics.data-model/* 키 없음 또는 load 실패",
                    to_pass=["/nf-contract-check <nf> 로 contract 재생성 후 재실행"])
        return base
    have = _matrix_first_column(dev_dir, "data-model-implementation-map.md",
                                "Schema Classification")
    missing = sorted(contract_models - have)
    if missing:
        base.update(status="FAIL",
                    current=f"누락 schema — {missing[:8]}"
                            + (" ..." if len(missing) > 8 else ""),
                    to_pass=[f"data-model-implementation-map.md `## Schema "
                             f"Classification` 표에 {len(missing)}개 schema 행 추가"])
    else:
        base.update(status="PASS",
                    current=f"{len(contract_models)}개 schema 모두 cover",
                    to_pass=[])
    return base


def _load_work_items(dev_dir: pathlib.Path) -> tuple[list, str | None]:
    p = dev_dir / "codegen-work-items.yaml"
    if not p.exists():
        return [], "codegen-work-items.yaml 없음"
    try:
        data = yaml.safe_load(p.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        return [], f"YAML 파싱 실패 — {e}"
    if data.get("schema_version") != WORK_ITEMS_SCHEMA:
        return [], (f"schema_version != {WORK_ITEMS_SCHEMA} "
                    f"(현 {data.get('schema_version')!r})")
    items = data.get("items") or []
    if not isinstance(items, list):
        return [], "items 가 list 가 아님"
    if not items:
        return [], "items 비어있음"
    return items, None


def _work_item_schema_errors(items: list) -> list[str]:
    """SKILL.md mandate 의 8 키 schema 검증. PR #42 Finding 1 보강.
    각 items[i] 가 다음을 만족해야 함.
      - scalar keys (id, owner_lane) — 비어있지 않은 string
      - list keys non-empty (inputs/expected_files/acceptance/tests/
        verification_commands) — non-empty list 이며 모든 원소 TODO 아님
      - list keys may-empty (depends_on) — 키 존재 + list 타입 (empty 허용)
    """
    errs: list[str] = []
    for i, it in enumerate(items):
        if not isinstance(it, dict):
            errs.append(f"items[{i}] (dict 아님)")
            continue
        ident = it.get("id", f"index {i}")
        for k in WORK_ITEM_REQUIRED_SCALAR_KEYS:
            v = it.get(k)
            if not (isinstance(v, str) and v.strip()
                    and v.strip().upper() != "TODO"):
                errs.append(f"items[{i}] id={ident!r} scalar {k!r} 누락/TODO")
        for k in WORK_ITEM_REQUIRED_LIST_KEYS_MAY_EMPTY:
            v = it.get(k)
            if not isinstance(v, list):
                errs.append(f"items[{i}] id={ident!r} list-key {k!r} "
                            f"누락 또는 list 아님 (empty list 허용)")
    return errs


def _check_work_items_key(dev_dir: pathlib.Path, key: str, check_id: str,
                          name_ko: str) -> dict:
    base = {
        "id": check_id, "tier": 1, "name": name_ko,
        "criterion": (f"codegen-work-items.yaml schema_version "
                      f"{WORK_ITEMS_SCHEMA}, 각 items[] 가 SKILL.md mandate "
                      f"8 키 모두 보유 + 본 check 의 {key!r} 가 비어있지 않은 list."),
    }
    items, err = _load_work_items(dev_dir)
    if err:
        base.update(status="FAIL", current=err,
                    to_pass=["/nf-impl-plan <nf> 으로 codegen-work-items.yaml 재생성"])
        return base
    schema_errs = _work_item_schema_errors(items)
    bad = []
    for i, it in enumerate(items):
        if not isinstance(it, dict):
            continue  # scheme errs 가 이미 잡음
        v = it.get(key)
        if not (isinstance(v, list) and v and any(
                str(x).strip() and str(x).strip().upper() != "TODO" for x in v)):
            bad.append(f"items[{i}] id={it.get('id', '?')} {key} 비어있음/TODO")
    all_problems = schema_errs + bad
    if all_problems:
        base.update(status="FAIL", current=f"불량 — {all_problems[:5]}"
                                            + (" ..." if len(all_problems) > 5
                                               else ""),
                    to_pass=[f"각 work item 의 SKILL.md mandate 8 키 + "
                             f"{key!r} non-empty 보강"])
    else:
        base.update(status="PASS",
                    current=f"{len(items)}개 work item 모두 8 키 + {key} 보유",
                    to_pass=[])
    return base


def check_all_work_items_have_expected_files(dev_dir: pathlib.Path) -> dict:
    return _check_work_items_key(
        dev_dir, "expected_files",
        "all_work_items_have_expected_files",
        "모든 codegen work item 이 expected_files 보유")


def check_all_work_items_have_tests(dev_dir: pathlib.Path) -> dict:
    return _check_work_items_key(
        dev_dir, "tests",
        "all_work_items_have_tests",
        "모든 codegen work item 이 tests 보유")


def check_all_work_items_have_verification_commands(dev_dir: pathlib.Path) -> dict:
    return _check_work_items_key(
        dev_dir, "verification_commands",
        "all_work_items_have_verification_commands",
        "모든 codegen work item 이 verification_commands 보유")


def check_team_execution_plan_present(dev_dir: pathlib.Path) -> dict:
    """relaxed — PR-10 (2026-05-26). 5 lane H2 mandate 제거. 본 check 는
    file 존재 + 비-placeholder 본문 *만* 강제. lane structure 는 hint —
    runtime 이 actual agent/team topology 결정. blocking gate 는
    phase_wi_coverage / wi_depends_on_valid / wi_phase_order_valid 에서 보장."""
    base = {
        "id": "team_execution_plan_present", "tier": 1,
        "name": "team-execution-plan.md 존재 + 비-placeholder 본문",
        "criterion": ("team-execution-plan.md 가 존재하고, frontmatter +"
                      " AUTO/USER marker 외 의미 있는 본문 (>200자)."
                      " lane H2 구조는 *hint*, mandate 아님."),
    }
    p = dev_dir / "team-execution-plan.md"
    if not p.exists():
        base.update(status="FAIL", current="team-execution-plan.md 없음",
                    to_pass=["/nf-impl-plan <nf> 으로 team-execution-plan.md 생성"])
        return base
    txt = p.read_text(encoding="utf-8")
    # frontmatter strip
    _, body = parse_frontmatter(txt)
    # marker strip (AUTO/USER marker 라인 자체 + TODO placeholder 라인 제거).
    cleaned_lines = []
    for ln in body.splitlines():
        if "<!-- AUTO:" in ln or "<!-- USER:" in ln:
            continue
        if "TODO:" in ln and "사람이 보강" in ln:
            continue
        cleaned_lines.append(ln)
    cleaned = "".join(cleaned_lines).strip()
    if len(cleaned) < 200:
        base.update(status="FAIL",
                    current=f"본문 길이 {len(cleaned)}자 (TODO placeholder 가능성)",
                    to_pass=["team-execution-plan.md USER block 에 실제 lane/role"
                             " hint 또는 execution context 본문 작성"])
        return base
    base.update(status="PASS",
                current=f"본문 {len(cleaned)}자, 비-placeholder 확인",
                to_pass=[])
    return base


def _load_codegen_work_items(dev_dir: pathlib.Path) -> tuple[dict | None, str | None]:
    p = dev_dir / "codegen-work-items.yaml"
    if not p.exists():
        return None, "codegen-work-items.yaml 없음"
    try:
        return yaml.safe_load(p.read_text(encoding="utf-8")) or {}, None
    except yaml.YAMLError as e:
        return None, f"YAML parse error: {e}"


def _load_readiness_config_for_nf(nf: str) -> dict | None:
    p = REPO / "design" / nf / "readiness-config.yaml"
    if not p.exists():
        return None
    try:
        return yaml.safe_load(p.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError:
        return None


def _wi_id_set(items: list) -> set[str]:
    out: set[str] = set()
    for it in items or []:
        if isinstance(it, dict) and isinstance(it.get("id"), str):
            out.add(it["id"])
    return out


def _wi_by_id(items: list) -> dict[str, dict]:
    out: dict[str, dict] = {}
    for it in items or []:
        if isinstance(it, dict) and isinstance(it.get("id"), str):
            out[it["id"]] = it
    return out


def check_phase_wi_coverage(dev_dir: pathlib.Path, nf: str) -> dict:
    """**bidirectional + uniqueness** (PR-10 review fix x2, 2026-05-26).
    - items[].id 내부 중복 차단 (items 정의 단계 무결성).
    - phase_policy.phases[*].work_items 의 flat 중복 차단 (phase 간/내 중복
      할당은 phase ordering 위반 위험).
    - forward: phase_policy WI ⊆ items[].id.
    - reverse: items[].id ⊆ phase_policy WI (unphased 차단)."""
    base = {
        "id": "phase_wi_coverage", "tier": 1,
        "name": "phase WI ↔ items[] 집합 동일 + 중복 없음",
        "criterion": ("design/<nf>/readiness-config.yaml `phase_policy.phases."
                      "*.work_items` 와 dev/<nf>/codegen-work-items.yaml "
                      "`items[].id` 가 *집합 동일 + 각 source 안에서 중복 없음*."
                      " forward unresolved + reverse unphased + items 중복 +"
                      " phase 중복 모두 차단."),
    }
    rc = _load_readiness_config_for_nf(nf)
    if rc is None:
        base.update(status="FAIL", current="readiness-config.yaml 없음/파싱 실패",
                    to_pass=["design/<nf>/readiness-config.yaml 생성/수정"])
        return base
    wi_doc, err = _load_codegen_work_items(dev_dir)
    if err:
        base.update(status="FAIL", current=err,
                    to_pass=["codegen-work-items.yaml 생성"])
        return base

    items = wi_doc.get("items") or []
    # codegen-work-items.yaml items[].id 중복 검사.
    items_list: list[str] = []
    for it in items:
        if isinstance(it, dict) and isinstance(it.get("id"), str):
            items_list.append(it["id"])
    items_dup = sorted({i for i in items_list if items_list.count(i) > 1})
    wi_ids = set(items_list)

    # readiness-config phase_policy.phases[*].work_items 중복 검사 (flat).
    phases = ((rc.get("phase_policy") or {}).get("phases") or {})
    phase_list: list[str] = []
    for pid, body in phases.items():
        if not isinstance(body, dict):
            continue
        for wi in (body.get("work_items") or []):
            phase_list.append(wi)
    phase_dup = sorted({w for w in phase_list if phase_list.count(w) > 1})
    phase_wis = set(phase_list)

    unresolved = sorted(phase_wis - wi_ids)
    unphased = sorted(wi_ids - phase_wis)

    if items_dup or phase_dup or unresolved or unphased:
        msgs: list[str] = []
        if items_dup:
            msgs.append(f"items[] 중복 id {items_dup[:5]}"
                        + (" ..." if len(items_dup) > 5 else ""))
        if phase_dup:
            msgs.append(f"phase_policy 중복 ref {phase_dup[:5]}"
                        + (" ..." if len(phase_dup) > 5 else ""))
        if unresolved:
            msgs.append(f"unresolved (phase→items) {unresolved[:5]}"
                        + (" ..." if len(unresolved) > 5 else ""))
        if unphased:
            msgs.append(f"unphased (items→phase) {unphased[:5]}"
                        + (" ..." if len(unphased) > 5 else ""))
        base.update(status="FAIL", current=" ; ".join(msgs),
                    to_pass=["readiness-config phase_policy 와 codegen-work-"
                             "items.yaml items[] 의 WI ID 집합을 동일하게 + 각"
                             " source 안에서 중복 제거"])
    else:
        base.update(status="PASS",
                    current=f"양방향 정합 + 중복 없음 — {len(wi_ids)} WI",
                    to_pass=[])
    return base


def check_wi_depends_on_valid(dev_dir: pathlib.Path) -> dict:
    """각 WI 의 depends_on 원소가 다른 WI id 로 resolve + DAG (no cycle)."""
    base = {
        "id": "wi_depends_on_valid", "tier": 1,
        "name": "WI depends_on 그래프 유효 (refs resolve + DAG)",
        "criterion": ("codegen-work-items.yaml `items[].depends_on` 의 모든"
                      " 원소가 다른 items[].id 로 resolve + 그래프에 cycle 없음."),
    }
    wi_doc, err = _load_codegen_work_items(dev_dir)
    if err:
        base.update(status="FAIL", current=err, to_pass=["codegen-work-items.yaml 생성"])
        return base
    items = wi_doc.get("items") or []
    by_id = _wi_by_id(items)
    ids = set(by_id.keys())
    unresolved: list[str] = []
    graph: dict[str, list[str]] = {}
    for wi_id, it in by_id.items():
        deps = it.get("depends_on") or []
        if not isinstance(deps, list):
            unresolved.append(f"{wi_id} depends_on 비-list")
            continue
        for d in deps:
            if d not in ids:
                unresolved.append(f"{wi_id} → {d} (unresolved)")
        graph[wi_id] = [d for d in deps if d in ids]
    if unresolved:
        base.update(status="FAIL",
                    current=f"unresolved refs — {unresolved[:5]}"
                    + (" ..." if len(unresolved) > 5 else ""),
                    to_pass=["depends_on 원소를 valid WI id 로 교정"])
        return base
    # Cycle detection — Kahn topological sort.
    indeg = {n: 0 for n in graph}
    for n, deps in graph.items():
        for d in deps:
            indeg[n] += 1  # edge d→n (d 가 먼저)
    # Kahn requires reverse edges 처리 — depends_on 은 *선행* WI 다.
    # n 이 d 의존 → edge d→n. n 의 in-degree = len(deps).
    queue = [n for n, d in indeg.items() if d == 0]
    visited = 0
    incoming: dict[str, list[str]] = {n: [] for n in graph}
    for n, deps in graph.items():
        for d in deps:
            incoming[n].append(d)
    indeg2 = {n: len(incoming[n]) for n in graph}
    q = [n for n in graph if indeg2[n] == 0]
    order: list[str] = []
    while q:
        n = q.pop(0)
        order.append(n)
        for m in graph:
            if n in incoming[m]:
                indeg2[m] -= 1
                if indeg2[m] == 0:
                    q.append(m)
    if len(order) != len(graph):
        cyclic = [n for n in graph if n not in order]
        base.update(status="FAIL",
                    current=f"cycle 검출 — 잔여 {cyclic[:5]}",
                    to_pass=["depends_on 그래프에서 cycle 을 끊음"])
        return base
    base.update(status="PASS",
                current=f"WI {len(graph)} 개 DAG, cycle 없음",
                to_pass=[])
    return base


def check_wi_phase_order_valid(dev_dir: pathlib.Path, nf: str) -> dict:
    """각 WI x 가 depends_on y 이면 phase(y) ≤ phase(x). phase order 위반 0."""
    base = {
        "id": "wi_phase_order_valid", "tier": 1,
        "name": "WI depends_on phase 순서 유효",
        "criterion": ("WI x 의 depends_on 에 있는 모든 y 의 phase 가 x 의 phase"
                      " 이전 또는 같음. phase 는 readiness-config.phase_policy."
                      "phases 에서 순서대로 phase1→phase2... 로 ordinal 부여."),
    }
    rc = _load_readiness_config_for_nf(nf)
    if rc is None:
        base.update(status="FAIL", current="readiness-config.yaml 없음/파싱 실패",
                    to_pass=["design/<nf>/readiness-config.yaml 생성/수정"])
        return base
    wi_doc, err = _load_codegen_work_items(dev_dir)
    if err:
        base.update(status="FAIL", current=err, to_pass=["codegen-work-items.yaml 생성"])
        return base
    phases = ((rc.get("phase_policy") or {}).get("phases") or {})
    # WI → phase ordinal (insertion order = ordinal).
    wi_phase: dict[str, int] = {}
    for idx, (pid, body) in enumerate(phases.items()):
        if not isinstance(body, dict):
            continue
        for wi in (body.get("work_items") or []):
            wi_phase[wi] = idx
    items = wi_doc.get("items") or []
    by_id = _wi_by_id(items)
    violations: list[str] = []
    for wi_id, it in by_id.items():
        if wi_id not in wi_phase:
            continue  # WI 가 phase_policy 외에 — phase_wi_coverage 에서 처리.
        x_phase = wi_phase[wi_id]
        for d in (it.get("depends_on") or []):
            if d not in wi_phase:
                continue
            if wi_phase[d] > x_phase:
                violations.append(
                    f"{wi_id} (phase {x_phase}) ← depends_on {d} (phase {wi_phase[d]})")
    if violations:
        base.update(status="FAIL",
                    current=f"phase 순서 위반 — {violations[:5]}"
                    + (" ..." if len(violations) > 5 else ""),
                    to_pass=["phase_policy 순서 또는 depends_on 을 정합 — 선행 WI"
                             " 의 phase 가 후행 WI 의 phase 이하"])
    else:
        base.update(status="PASS",
                    current=f"phase mapped WI {len(wi_phase)} 개 순서 정합",
                    to_pass=[])
    return base


def _checklist_human_reviewed(dev_dir: pathlib.Path) -> str | None:
    """design-adequacy-checklist.md 의 `## Checklist` 표에 status 컬럼 (열 3) 이
    'pending'/'TODO' 외 (예 ok/concern) 인 행이 1 개 이상."""
    p = dev_dir / "design-adequacy-checklist.md"
    if not p.exists():
        return "design-adequacy-checklist.md 없음"
    rows = parse_md_table_section(p.read_text(encoding="utf-8"), "Checklist")
    if not rows:
        return "design-adequacy-checklist.md `## Checklist` 표 없음"
    for r in rows:
        if len(r) >= 3 and r[2].strip().lower() not in ("pending", "todo", ""):
            return None
    return ("design-adequacy-checklist.md 의 Checklist 모든 행이 status=pending/TODO "
            "— 사람 audit 흔적 없음")


def _coverage_trace_has_content(dev_dir: pathlib.Path) -> str | None:
    """spec-to-design-coverage.md `## Coverage Trace` 표에 첫 컬럼 non-TODO 행 1+."""
    p = dev_dir / "spec-to-design-coverage.md"
    if not p.exists():
        return "spec-to-design-coverage.md 없음"
    rows = parse_md_table_section(p.read_text(encoding="utf-8"), "Coverage Trace")
    if any(r and r[0].strip() and r[0].strip().upper() != "TODO" for r in rows):
        return None
    return ("spec-to-design-coverage.md 의 Coverage Trace 가 비어있거나 "
            "첫 컬럼이 모두 TODO")


def _gaps_table_non_todo(dev_dir: pathlib.Path) -> str | None:
    """open-gaps-and-assumptions.md `## Gaps` 에 id 컬럼 non-TODO 행 1+."""
    rows, err = _parse_gaps_table(dev_dir)
    if err:
        return err
    if any(r and r[0].strip() and r[0].strip().upper() != "TODO" for r in rows):
        return None
    return "open-gaps-and-assumptions.md 의 Gaps 모든 행이 id=TODO"


def _review_recommendation_non_todo(dev_dir: pathlib.Path) -> str | None:
    """implementation-readiness-review.md `## Recommendation` 안에 TODO 가 아닌
    실질적 본문 (텍스트 길이 ≥ 30 글자) 1줄 이상."""
    p = dev_dir / "implementation-readiness-review.md"
    if not p.exists():
        return "implementation-readiness-review.md 없음"
    text = p.read_text(encoding="utf-8")
    start = text.find("## Recommendation")
    if start == -1:
        return "implementation-readiness-review.md `## Recommendation` 섹션 없음"
    nxt = text.find("\n## ", start + len("## Recommendation"))
    section = text[start:nxt] if nxt != -1 else text[start:]
    for ln in section.splitlines()[1:]:  # skip H2 헤더
        s = ln.strip()
        if not s or s.startswith("#"):
            continue
        cleaned = s.lstrip("- *").strip()
        if cleaned.upper() == "TODO":
            continue
        if "TODO" in cleaned and len(cleaned) < 40:
            continue
        if len(cleaned) >= 30:
            return None
    return ("implementation-readiness-review.md Recommendation 섹션에 30자 이상 "
            "비-TODO 본문 없음")


def check_human_review_pack_traceable(dev_dir: pathlib.Path) -> dict:
    base = {
        "id": "human_review_pack_traceable", "tier": 1,
        "name": "Human Review Pack 4 파일 존재 + frontmatter + 사람 audit 흔적",
        "criterion": (", ".join(HUMAN_REVIEW_PACK) + " 4 파일 모두 존재, "
                      "frontmatter 필수 키 충족, "
                      "design-adequacy-checklist 의 Checklist 표에 status != "
                      "pending/TODO 행 1+, spec-to-design-coverage Coverage Trace "
                      "에 non-TODO 행 1+, open-gaps Gaps 에 id != TODO 행 1+, "
                      "implementation-readiness-review Recommendation 에 30자+ "
                      "비-TODO 본문 1+."),
    }
    bad: list[str] = []
    # 1. 4 파일 존재 + frontmatter
    for fn in HUMAN_REVIEW_PACK:
        p = dev_dir / fn
        if not p.exists():
            bad.append(f"{fn} 없음")
            continue
        fm, _ = parse_frontmatter(p.read_text(encoding="utf-8"))
        if not fm or (DEV_FM_KEYS - set(fm.keys())):
            bad.append(f"{fn} frontmatter 누락")
    # 2. per-file substantive 증거 — boilerplate 통과 차단 (PR #42 Finding 2 보강)
    for err in (
        _checklist_human_reviewed(dev_dir),
        _coverage_trace_has_content(dev_dir),
        _gaps_table_non_todo(dev_dir),
        _review_recommendation_non_todo(dev_dir),
    ):
        if err:
            bad.append(err)
    if bad:
        base.update(status="FAIL", current=f"불량 — {bad}",
                    to_pass=["Human Review Pack 본문을 사람이 audit 가능한 "
                             "수준으로 작성 — 각 표/섹션의 placeholder 행 채움"])
    else:
        base.update(status="PASS",
                    current="4 파일 모두 사람 audit 흔적 보유", to_pass=[])
    return base


def _parse_gaps_table(dev_dir: pathlib.Path) -> tuple[list[list[str]], str | None]:
    p = dev_dir / "open-gaps-and-assumptions.md"
    if not p.exists():
        return [], "open-gaps-and-assumptions.md 없음"
    rows = parse_md_table_section(p.read_text(encoding="utf-8"), "Gaps")
    if not rows:
        return [], "## Gaps 섹션에 표가 없음"
    return rows, None


def check_gaps_classified(dev_dir: pathlib.Path) -> dict:
    base = {
        "id": "gaps_classified", "tier": 1,
        "name": "open-gaps-and-assumptions.md 의 모든 gap 행이 category 분류됨",
        "criterion": ("`## Gaps` 표의 모든 행이 category 컬럼 (열 2) 값이 "
                      f"{sorted(GAP_CATEGORIES)} 중 하나. TODO/빈 값 금지."),
    }
    rows, err = _parse_gaps_table(dev_dir)
    if err:
        base.update(status="FAIL", current=err,
                    to_pass=["/nf-impl-plan <nf> 으로 open-gaps-and-assumptions.md 생성"])
        return base
    bad = []
    for r in rows:
        if len(r) < 2:
            bad.append(f"row {r} (열 부족)")
            continue
        cat = r[1].lower()
        if cat == "todo" or not cat:
            bad.append(f"id={r[0]!r} category=TODO")
        elif cat not in GAP_CATEGORIES:
            bad.append(f"id={r[0]!r} category={cat!r} (허용 집합 밖)")
    if bad:
        base.update(status="FAIL", current=f"불량 — {bad[:5]}"
                                            + (" ..." if len(bad) > 5 else ""),
                    to_pass=[f"각 gap 행의 category 를 {sorted(GAP_CATEGORIES)} "
                             "중 하나로 분류"])
    else:
        base.update(status="PASS", current=f"{len(rows)}개 gap 모두 분류됨",
                    to_pass=[])
    return base


def check_blocker_gaps_zero(dev_dir: pathlib.Path) -> dict:
    base = {
        "id": "blocker_gaps_zero", "tier": 1,
        "name": "open-gaps-and-assumptions.md 의 `blocker` gap 수 = 0",
        "criterion": "`## Gaps` 표의 category=blocker 행이 0개.",
    }
    rows, err = _parse_gaps_table(dev_dir)
    if err:
        base.update(status="FAIL", current=err,
                    to_pass=["/nf-impl-plan <nf> 으로 open-gaps-and-assumptions.md 생성"])
        return base
    blockers = [r for r in rows if len(r) >= 2 and r[1].lower() == "blocker"]
    if blockers:
        ids = [r[0] for r in blockers]
        base.update(status="FAIL", current=f"blocker {len(blockers)}개 — {ids}",
                    to_pass=["blocker gap 을 모두 해결하거나 다른 category 로 "
                             "재분류 후 codegen 시작"])
    else:
        base.update(status="PASS", current="blocker 0개", to_pass=[])
    return base


def check_no_spec_reread_required(dev_dir: pathlib.Path) -> dict:
    """Aggregate sentinel — autonomous codegen 이 원본 OpenAPI YAML 을 다시 읽지
    않아도 됨을 보장. 다음 조건 AND.
      1. blocker gap 0 (= blocker_gaps_zero 와 동일)
      2. spec-to-design-coverage.md `## Coverage Trace` 에 최소 1 non-TODO 행
      3. open-gaps 또는 design-adequacy-checklist 본문에 "spec 재독해" 단서 없음
    """
    base = {
        "id": "no_spec_reread_required", "tier": 1,
        "name": "원본 spec 재독해 없이 codegen 가능 (aggregate sentinel)",
        "criterion": ("blocker_gaps_zero PASS + spec-to-design-coverage.md "
                      "`## Coverage Trace` 표에 최소 1 non-TODO 행 + open-gaps/"
                      "design-adequacy 본문에 'spec 재독해/re-read' 단서 부재."),
    }
    rows, gap_err = _parse_gaps_table(dev_dir)
    fails = []
    if gap_err:
        # gap table 미존재/파싱 실패 = blocker count 신뢰 불가 = FAIL (PR #42 Finding 3 보강).
        fails.append(f"gap table 신뢰 불가 — {gap_err}")
        blocker_n = 0
    else:
        blocker_n = sum(1 for r in rows if len(r) >= 2 and r[1].lower() == "blocker")
        if blocker_n:
            fails.append(f"blocker {blocker_n}개")
    cov_path = dev_dir / "spec-to-design-coverage.md"
    cov_ok = False
    if cov_path.exists():
        cov_rows = parse_md_table_section(
            cov_path.read_text(encoding="utf-8"), "Coverage Trace")
        cov_ok = any(r and r[0] and r[0].upper() != "TODO" for r in cov_rows)
    sentinel_hits = []
    for fn in ("open-gaps-and-assumptions.md", "design-adequacy-checklist.md"):
        p = dev_dir / fn
        if not p.exists():
            continue
        t = p.read_text(encoding="utf-8")
        for kw in ("spec 재독해", "re-read spec", "spec 재해석"):
            if kw in t:
                sentinel_hits.append(f"{fn}:{kw}")
    if not cov_ok:
        fails.append("Coverage Trace 가 비어있거나 TODO 만")
    if sentinel_hits:
        fails.append(f"재독해 단서 — {sentinel_hits}")
    if fails:
        base.update(status="FAIL", current=", ".join(fails),
                    to_pass=["blocker 0 + Coverage Trace 채움 + 재독해 단서 해소"])
    else:
        base.update(status="PASS",
                    current="blocker 0 + coverage trace 존재 + 재독해 단서 없음",
                    to_pass=[])
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

    contract = load_contract(nf)
    checks = [
        check_impl_files_present(dev_dir),
        check_impl_sections_exact(dev_dir),
        check_tasks_schema_valid(dev_dir),
        check_impl_frontmatter_valid(dev_dir),
        check_traceability_xref(dev_dir),
        check_readiness_pack_files_present(dev_dir),
        check_api_matrix_covers_all_operations(dev_dir, contract),
        check_data_model_map_covers_all_contract_models(dev_dir, contract),
        check_all_work_items_have_expected_files(dev_dir),
        check_all_work_items_have_tests(dev_dir),
        check_all_work_items_have_verification_commands(dev_dir),
        check_team_execution_plan_present(dev_dir),
        check_phase_wi_coverage(dev_dir, nf),
        check_wi_depends_on_valid(dev_dir),
        check_wi_phase_order_valid(dev_dir, nf),
        check_human_review_pack_traceable(dev_dir),
        check_gaps_classified(dev_dir),
        check_blocker_gaps_zero(dev_dir),
        check_no_spec_reread_required(dev_dir),
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
