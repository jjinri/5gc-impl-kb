#!/usr/bin/env python3
"""nf-readiness-pack-generate — readiness pack 산출의 deterministic render/check.

PR-4 scope (Phase C 시작 — Plan 2026-05-26 §7) — 첫 target = dev/<nf>/
traceability.md only. AUTO 영역 = frontmatter (manifest) + references.

PR-5 확장 — AUTO 추가:
  * test-inventory-index — dev/<nf>/test-matrix.md `## Test Inventory`
    표를 parse 해 kind 별 test ID 그룹 render. 두 파일 사이 drift 검출.

PR-6 확장 — AUTO 추가:
  * deferred-decisions-trace — readiness-config.deferred_decisions.rows
    table render. config 가 source 라 readiness-config 변경 시 자동 갱신
    (config-driven render 증명).

PR-7 확장 — multi-target:
  * dev/<nf>/implementation-plan.md 신규 target.
  * AUTO engineering-decisions-summary — readiness-config.implementation.
    slots derive 의 13 slot 요약 table. config-driven render, 2번째
    파일 적용 — generator multi-target scaling 증명.

PR-8 확장 — 3번째 target + policy-derived AUTO:
  * dev/<nf>/test-matrix.md 신규 target.
  * AUTO security-coverage-summary — design/policies/security-baseline.yaml
    (M1~M7 mandate) + test-matrix.md `## Test Inventory` 의 kind=security
    test count cross-source derive. policy 변경 시 render 갱신 증명.

PR-9 확장 — 4번째 target + phase config view:
  * dev/<nf>/team-execution-plan.md 신규 target.
  * AUTO phase-integration-map — readiness-config.phase_policy.phases
    derive 의 phase → work_items table. config-driven render 4번째 파일.

PR-11 확장 — 5번째 target + config-driven gaps table:
  * dev/<nf>/open-gaps-and-assumptions.md 신규 target.
  * AUTO gaps-table — readiness-config.gaps.rows derive 의 gap table
    {id, category, description, owner, target_resolution}. config 변경 시
    table 갱신. validator (_gaps_table_non_todo / gaps_classified /
    blocker_gaps_zero) 가 본 generated table 을 입력으로 사용.

PR-13b1 확장 — 6번째 target + security gate matrix:
  * dev/<nf>/verification-plan.md 신규 target.
  * AUTO security-gate-matrix — design/policies/security-baseline.yaml
    baseline_mandates (M1~M7) × verification kind × test ID prefix.
    policy 변경 시 matrix 갱신.

AUTO/USER marker 패턴 (materialize-contract.py reference 모범). USER 영역
사람 산문 보존 — generator 가 만지지 않음.

--check: render to memory, compare with tracked, drift 시 exit 1.
--write: render to tracked path, 멱등.

No timestamp noise — `ratified_date` 등 config-driven 정보만 출력.
"""

import argparse
import pathlib
import re
import sys

import yaml

# Markdown table contract — see lib/md_table.py module docstring (PR-13a).
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from lib.md_table import cell as _cell  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parent.parent.parent

# Target schema — file path template → (auto ids, user ids).
# 본 generator 가 render 하는 모든 산출의 *기계 계약*.
TARGETS = {
    "dev/<nf>/traceability.md": {
        "auto_ids": [
            "test-inventory-index",
            "deferred-decisions-trace",
            "references",
        ],
        "user_ids": [
            "intro-note",
            "contract-module-table",
            "module-test-table",
            "open-gaps",
        ],
    },
    "dev/<nf>/implementation-plan.md": {
        "auto_ids": [
            "engineering-decisions-summary",
        ],
        "user_ids": [
            "scope-body",
            "phases-body",
            "test-plan-body",
            "open-risks-body",
            "references-body",
        ],
    },
    "dev/<nf>/test-matrix.md": {
        "auto_ids": [
            "security-coverage-summary",
        ],
        "user_ids": [
            "intro-note",
            "purpose-body",
            "test-inventory-body",
            "coverage-rules-body",
            "open-questions-body",
            "references-body",
        ],
    },
    "dev/<nf>/team-execution-plan.md": {
        # PR-10 lane relaxation 후속 (PR-10 review fix, 2026-05-26) —
        # 5 lane H2 hard-code 제거. user_ids 는 free-form body 단일 블록
        # + intro/references 보존. lane structure 는 USER:body 안에서
        # 사용자가 자유롭게 둔다.
        "auto_ids": [
            "phase-integration-map",
        ],
        "user_ids": [
            "intro-note",
            "body",
            "references-body",
        ],
    },
    "dev/<nf>/open-gaps-and-assumptions.md": {
        # PR-11 — gaps table 을 readiness-config.gaps.rows 에서 deterministic
        # 으로 render. validator (gaps_classified / blocker_gaps_zero /
        # human_review_pack_traceable.gaps_table_non_todo) 가 본 table 을
        # 입력으로 사용.
        "auto_ids": [
            "gaps-table",
        ],
        "user_ids": [
            "intro-note",
            "summary-body",
            "references-body",
        ],
    },
    "dev/<nf>/verification-plan.md": {
        # PR-13b1 — security-baseline policy → verification-plan H3 matrix.
        "auto_ids": [
            "security-gate-matrix",
        ],
        "user_ids": [
            "intro-note",
            "unit-body",
            "integration-body",
            "contract-body",
            "security-body",
            "end-to-end-body",
            "observability-body",
            "references-body",
        ],
    },
}

# References — fixed per NF source-of-truth list. PR-5+ 가 readiness-config
# / arch 산출에서 derive 로 전환.
REFERENCES_BY_NF = {
    "nssf": [
        ("docs/adr/ADR-0004-project-security-baseline.md",
         "TLS / mTLS / OAuth2 production-capable code path 의무."),
        ("design/policies/security-baseline.yaml",
         "baseline 7 mandate (M1~M7) machine-readable."),
        ("design/policies/lifecycle.yaml",
         "lifecycle stage / gate vocabulary."),
        ("design/policies/gaps.yaml",
         "gap category enum (6) — open-gaps 의 category 컬럼 source."),
        ("design/policies/source-precedence.yaml",
         "ADR-0004 baseline > engineering-design > architecture > dev"
         " enforcement order."),
        ("design/policies/spec-dependencies.yaml",
         "33.501 / 33.310 / 33.210 absorbed by ADR-0004."),
        ("design/schemas/engineering-core-slots.yaml",
         "13 core slot typed shape (v2 + dependency_shape)."),
        ("design/schemas/nf-readiness-config-v1.yaml",
         "readiness-config shape contract."),
        ("design/nssf/readiness-config.yaml",
         "NSSF 사람 ratify config — implementation·security·phase·gap·"
         "deferred 결정 source."),
        ("engineering/nssf/engineering-design.md",
         "현재 적용 lib/구조 source (readiness-config.implementation 흡수"
         " 대상)."),
        ("dev/nssf/implementation-plan.md", None),
        ("dev/nssf/tasks.yaml", None),
        ("dev/nssf/test-matrix.md", None),
        ("design/nssf/architecture/", "전체"),
        ("design/nssf/module-decomposition/", "전체"),
        ("handoff/nssf/contract.yaml", "8 API topic + data-model."),
    ],
}

# Title per target. NF 별 NF 라벨 + suffix.
TARGET_TITLES = {
    "dev/<nf>/traceability.md": "{nf_upper} Traceability",
    "dev/<nf>/implementation-plan.md": "{nf_upper} Implementation Plan",
    "dev/<nf>/test-matrix.md": "{nf_upper} Test Matrix",
    "dev/<nf>/team-execution-plan.md": "{nf_upper} Team Execution Plan",
    "dev/<nf>/open-gaps-and-assumptions.md": "{nf_upper} Open Gaps and Assumptions",
    "dev/<nf>/verification-plan.md": "{nf_upper} Verification Plan",
}

# Frontmatter 의 stage 라벨.
TARGET_STAGES = {
    "dev/<nf>/traceability.md": "implementation-planning",
    "dev/<nf>/implementation-plan.md": "implementation-planning",
    "dev/<nf>/test-matrix.md": "implementation-planning",
    "dev/<nf>/team-execution-plan.md": "implementation-planning",
    "dev/<nf>/open-gaps-and-assumptions.md": "implementation-planning",
    "dev/<nf>/verification-plan.md": "implementation-planning",
}

# nf-impl-status.py DEV_FM_KEYS 와 정합 — generator 출력이 검사를 통과해야
# impl_frontmatter_valid PASS. 추가 키 (generator, source_readiness_config,
# generated_sections, user_sections) 도 함께 노출하지만, 본 6 키는 *반드시*
# 출력에 포함된다.
LEGACY_FM_KEYS = {
    "nf", "stage", "status", "source_architecture", "source_contract",
    "generated_date",
}


# ─── marker helpers ─────────────────────────────────────────────────

def auto_block(aid: str, body: str) -> str:
    return f"<!-- AUTO:{aid}:start -->\n{body}\n<!-- AUTO:{aid}:end -->"


def user_block(uid: str, body: str) -> str:
    return f"<!-- USER:{uid}:start -->\n{body}\n<!-- USER:{uid}:end -->"


def extract_blocks(text: str, kind: str) -> dict[str, str]:
    """kind ∈ {AUTO, USER}. dict[id] = body."""
    out: dict[str, str] = {}
    pattern = rf"<!-- {kind}:([\w-]+):start -->\n(.*?)\n<!-- {kind}:\1:end -->"
    for m in re.finditer(pattern, text, re.DOTALL):
        out[m.group(1)] = m.group(2)
    return out


# ─── readiness-config loader ────────────────────────────────────────

def load_readiness_config(nf: str) -> dict:
    path = REPO / "design" / nf / "readiness-config.yaml"
    return yaml.safe_load(path.read_text(encoding="utf-8")) or {}


# ─── AUTO renderers ─────────────────────────────────────────────────

# ─── test-matrix.md parser ──────────────────────────────────────────

# Test kind canonical order — render 순서를 결정론적으로 유지.
TEST_KIND_ORDER = [
    "unit",
    "integration",
    "module-integration",
    "contract",
    "security",
    "end-to-end",
]


def parse_test_inventory(nf: str) -> list[tuple[str, str]]:
    """dev/<nf>/test-matrix.md `## Test Inventory` 첫 table 의 (id, kind)
    목록을 반환. 행 순서는 파일 순서 유지."""
    path = REPO / "dev" / nf / "test-matrix.md"
    if not path.is_file():
        return []
    txt = path.read_text(encoding="utf-8")
    # `## Test Inventory` 직후 첫 markdown table.
    marker = "## Test Inventory"
    idx = txt.find(marker)
    if idx == -1:
        return []
    rest = txt[idx + len(marker):]
    # 다음 `## ` (또는 EOF) 직전까지가 본 섹션.
    next_h2 = rest.find("\n## ")
    section = rest if next_h2 == -1 else rest[:next_h2]
    rows: list[tuple[str, str]] = []
    in_table = False
    for ln in section.splitlines():
        ln = ln.rstrip()
        if not ln:
            continue
        if ln.startswith("|") and "|" in ln[1:]:
            # header / separator / data
            cells = [c.strip() for c in ln.strip("|").split("|")]
            if not in_table:
                # header row — column 0 must be "id", column 1 "kind".
                if cells[:2] == ["id", "kind"]:
                    in_table = True
                continue
            # separator row — `---` only.
            if all(set(c) <= set("-:") for c in cells):
                continue
            # data row.
            if len(cells) >= 2:
                tid_raw = cells[0]
                kind = cells[1]
                # id 는 backtick 으로 감싸짐 — strip.
                tid = tid_raw.strip("`")
                rows.append((tid, kind))
    return rows


def render_test_inventory_index(nf: str) -> str:
    """H3-only render — `## Module → Test` H2 아래 sub-section 으로 둔다.
    impl_sections_exact gate 가 traceability 의 H2 set 을 canonical
    {Contract → Module, Module → Test, Open Gaps, References} 으로 강제 —
    AUTO 가 새 H2 도입하면 게이트 깨짐."""
    rows = parse_test_inventory(nf)
    if not rows:
        return ("### Test Inventory Index\n\n"
                "_test-matrix.md `## Test Inventory` 표 부재 — render skip._")
    # kind 별 그룹.
    by_kind: dict[str, list[str]] = {}
    for tid, kind in rows:
        by_kind.setdefault(kind, []).append(tid)
    # canonical order + 알려지지 않은 kind 는 알파벳순으로 뒤.
    known = [k for k in TEST_KIND_ORDER if k in by_kind]
    unknown = sorted(k for k in by_kind if k not in TEST_KIND_ORDER)
    order = known + unknown
    lines = ["### Test Inventory Index", ""]
    lines.append(f"`dev/{nf}/test-matrix.md` `## Test Inventory` 표에서 "
                 f"derive — 총 {len(rows)}개 test, {len(by_kind)} kind. "
                 "test-matrix.md 변경 시 본 index 가 같이 갱신된다 "
                 "(generator drift check).")
    lines.append("")
    for kind in order:
        ids = by_kind[kind]
        lines.append(f"#### {kind} ({len(ids)})")
        lines.append("")
        for tid in ids:
            lines.append(f"- `{tid}`")
        lines.append("")
    return "\n".join(lines).rstrip()


def render_gaps_table(nf: str, config: dict) -> str:
    """`## Gaps` H2 아래 markdown table render. readiness-config.gaps.rows
    derive. _gaps_table_non_todo / gaps_classified / blocker_gaps_zero
    validator 가 본 table 을 입력으로 사용한다."""
    rows = (config.get("gaps") or {}).get("rows") or []
    if not rows:
        return "_readiness-config.gaps.rows 부재 — render skip._"
    lines = [
        "| id | category | description | owner | target_resolution |",
        "|---|---|---|---|---|",
    ]
    for r in rows:
        if not isinstance(r, dict):
            continue
        rid = _cell(r.get("id", "?"))
        cat = _cell(r.get("category", "?"))
        desc = _cell(r.get("description", "?"))
        owner = _cell(r.get("owner", "?"))
        target = _cell(r.get("target_resolution", "?"))
        lines.append(f"| {rid} | {cat} | {desc} | {owner} | {target} |")
    return "\n".join(lines)


def render_phase_integration_map(nf: str, config: dict) -> str:
    """H2 render — team-execution-plan.md 의 lane H2 mandate 가 PR-10 에서
    제거됐고 user_ids 구조 reorg 후속 (PR-10 review fix) 으로 본 AUTO 도
    free-form layout 안 H2 로 격상."""
    phase_policy = config.get("phase_policy") or {}
    phases = phase_policy.get("phases") or {}
    if not phases:
        return ("## Phase Integration Map\n\n"
                "_readiness-config.phase_policy.phases 부재 — render skip._")
    tracer = phase_policy.get("tracer_bullet_operation", "?")
    lines = ["## Phase Integration Map", ""]
    lines.append(f"`design/{nf}/readiness-config.yaml` `phase_policy` derive — "
                 f"tracer-bullet operation = `{tracer}`, "
                 f"총 {len(phases)} phase. config 변경 시 본 map 이 같이 갱신된다.")
    lines.append("")
    lines.append("| phase | description | work_items |")
    lines.append("|---|---|---|")
    for phase_id, body in phases.items():
        if not isinstance(body, dict):
            continue
        pid = _cell(phase_id)
        desc = _cell(body.get("description", ""))
        wis = body.get("work_items") or []
        wis_str = ", ".join(f"`{_cell(w)}`" for w in wis) if wis else "_none_"
        lines.append(f"| `{pid}` | {desc} | {wis_str} |")
    return "\n".join(lines)


# M{n} → verification kind / test ID prefix mapping. Hardcoded (현재 NSSF
# scope 의 mandate 5/6/7 는 code-level verification 또는 외부 처리라 test
# ID prefix 없음). policy schema 가 안정되면 security-baseline.yaml 의
# 각 mandate 행에 `verification` block 으로 promote 후 generator 가 그를
# 사용 (backlog).
SECURITY_MANDATE_VERIFICATION = {
    "M1": ("security", "t-tls-*"),
    "M2": ("security", "t-mtls-*"),
    "M3": ("security", "t-oauth2-inbound-*"),
    "M4": ("security", "t-oauth2-outbound-*"),
    "M5": ("security", "(test-matrix kind=security 종합)"),
    "M6": ("code review", "—"),
    "M7": ("operator/library 외부 처리", "—"),
}


def render_security_gate_matrix(nf: str) -> str:
    """H3 render — `## Security` H2 아래 sub-section. security-baseline
    policy 의 M1~M7 mandate 와 verification 매핑."""
    baseline = load_security_baseline()
    mandates = baseline.get("baseline_mandates") or []
    if not mandates:
        return ("### Security Gate Matrix\n\n"
                "_design/policies/security-baseline.yaml `baseline_mandates`"
                " 부재 — render skip._")
    lines = ["### Security Gate Matrix", ""]
    lines.append("`design/policies/security-baseline.yaml` `baseline_mandates`"
                 " (ADR-0004) derive — verification 적용 매핑. policy 변경 시"
                 " 본 matrix 갱신.")
    lines.append("")
    lines.append("| mandate | name | verification kind | test id prefix |")
    lines.append("|---|---|---|---|")
    for m in mandates:
        if not isinstance(m, dict):
            continue
        mid = _cell(m.get("id", "?"))
        name = _cell(m.get("name", "?"))
        kind, prefix = SECURITY_MANDATE_VERIFICATION.get(
            str(m.get("id", "")), ("(미매핑)", "—"))
        lines.append(f"| `{mid}` | `{name}` | {_cell(kind)} | {_cell(prefix)} |")
    return "\n".join(lines)


def load_security_baseline() -> dict:
    path = REPO / "design" / "policies" / "security-baseline.yaml"
    if not path.is_file():
        return {}
    return yaml.safe_load(path.read_text(encoding="utf-8")) or {}


def render_security_coverage_summary(nf: str) -> str:
    """H3 render — `## Coverage Rules` H2 아래 sub-section. cross-source
    derive: design/policies/security-baseline.yaml (M1~M7 mandate) +
    dev/<nf>/test-matrix.md `## Test Inventory` 의 kind=security row count."""
    baseline = load_security_baseline()
    mandates = baseline.get("baseline_mandates") or []
    rows = parse_test_inventory(nf)
    sec_tests = [tid for tid, kind in rows if kind == "security"]

    lines = ["### Security Coverage Summary", ""]
    if not mandates:
        lines.append("_design/policies/security-baseline.yaml `baseline_mandates`"
                     " 부재 — render skip._")
        return "\n".join(lines)
    lines.append("`design/policies/security-baseline.yaml` (ADR-0004 baseline) "
                 f"+ 본 파일 `## Test Inventory` cross-source derive — "
                 f"총 {len(mandates)} mandate, kind=security test {len(sec_tests)}개. "
                 "두 source 중 하나가 바뀌면 본 summary 가 갱신된다.")
    lines.append("")
    lines.append(f"**kind=security test count**: {len(sec_tests)}.")
    lines.append("")
    lines.append("**Project security mandates (ADR-0004)**:")
    lines.append("")
    for m in mandates:
        if not isinstance(m, dict):
            continue
        mid = str(m.get("id", "?"))
        name = str(m.get("name", "?"))
        applies = str(m.get("applies_to", "?"))
        lines.append(f"- `{mid}` `{name}` (applies_to={applies}).")
    lines.append("")
    lines.append("Per-mandate test 매핑은 `## Test Inventory` 의 `refs` 컬럼"
                 " (예 `ADR-0004` cross-ref) 과 dev/<nf>/traceability.md 의"
                 " `## Module → Test` 표를 참조.")
    return "\n".join(lines)


def render_engineering_decisions_summary(nf: str, config: dict) -> str:
    """H3 render — `## Scope` H2 아래 sub-section. readiness-config.
    implementation.slots 의 13 slot 을 요약 table 로 render. config-driven."""
    slots = (config.get("implementation") or {}).get("slots") or {}
    if not slots:
        return ("### Engineering Decisions Summary\n\n"
                "_readiness-config.implementation.slots 부재 — render skip._")

    lines = ["### Engineering Decisions Summary", ""]
    lines.append(f"`design/{nf}/readiness-config.yaml` `implementation.slots`"
                 f" derive — 총 {len(slots)} slot. config 변경 시 본 summary 가"
                 " 같이 갱신된다.")
    lines.append("")
    lines.append("| slot | decision | status | ratified |")
    lines.append("|---|---|---|---|")
    # slot 순서는 config 의 insertion order 보존 (yaml.safe_load 결과는 dict
    # 순서 = 파일 순서).
    for name, row in slots.items():
        if not isinstance(row, dict):
            continue
        # decision 은 첫 줄만 + 80자 제한 후 _cell 통과.
        decision_first = str(row.get("decision", "?")).splitlines()[0].strip()
        if len(decision_first) > 80:
            decision_first = decision_first[:77] + "..."
        slot_name = _cell(name)
        decision = _cell(decision_first)
        status = _cell(row.get("status", "?"))
        date = _cell(row.get("date", "?"))
        lines.append(f"| `{slot_name}` | {decision} | {status} | {date} |")
    return "\n".join(lines)


def render_deferred_decisions_trace(nf: str, config: dict) -> str:
    """H3-only render — `## Open Gaps` H2 아래 sub-section. config 의
    deferred_decisions.rows 를 table 로 render. status=decided 행은
    references_slot 으로 흡수 추적, explicitly_out_of_scope 행은 out-of-scope.
    """
    rows = (config.get("deferred_decisions") or {}).get("rows") or []
    if not rows:
        return ("### Deferred Decisions Trace\n\n"
                "_readiness-config.deferred_decisions 부재 — render skip._")
    lines = ["### Deferred Decisions Trace", ""]
    lines.append(f"`design/{nf}/readiness-config.yaml` `deferred_decisions.rows`"
                 f" derive — 총 {len(rows)} row. config 변경 시 본 trace 가"
                 " 같이 갱신된다.")
    lines.append("")
    lines.append("| id | status | resolved by | ratified_by | date |")
    lines.append("|---|---|---|---|---|")
    for r in rows:
        rid = _cell(r.get("id", "?"))
        status_raw = str(r.get("status", "?"))
        status = _cell(status_raw)
        if status_raw == "decided":
            resolved = f"slot `{_cell(r.get('references_slot', '?'))}`"
        elif status_raw == "explicitly_out_of_scope":
            resolved = "_out of scope_"
        else:
            resolved = f"_{_cell(status_raw)}_"
        rb = _cell(r.get("ratified_by", "?"))
        date = _cell(r.get("date", "?"))
        lines.append(f"| `{rid}` | {status} | {resolved} | {rb} | {date} |")
    return "\n".join(lines)


def render_references(nf: str) -> str:
    items = REFERENCES_BY_NF.get(nf)
    if not items:
        return f"## References\n\n_PR-4 references list for {nf!r} 미정의._"
    lines = ["## References", ""]
    for path, note in items:
        if note is None:
            lines.append(f"- `{path}`.")
        elif note == "전체":
            lines.append(f"- `{path}` 전체.")
        else:
            lines.append(f"- `{path}` — {note}")
    return "\n".join(lines)


# ─── target render ──────────────────────────────────────────────────

def render_traceability(nf: str, current_text: str, config: dict) -> str:
    target = "dev/<nf>/traceability.md"
    schema = TARGETS[target]
    autos = {
        "test-inventory-index": render_test_inventory_index(nf),
        "deferred-decisions-trace": render_deferred_decisions_trace(nf, config),
        "references": render_references(nf),
    }
    users_existing = extract_blocks(current_text, "USER") if current_text else {}

    # USER defaults — fresh placeholder. 기존 USER 가 있으면 보존.
    def user_body(uid: str) -> str:
        body = users_existing.get(uid)
        if body is None or not body.strip():
            return f"TODO: {uid} — 사람이 보강 (fresh placeholder)."
        return body

    title = TARGET_TITLES[target].format(nf_upper=nf.upper())
    stage = TARGET_STAGES[target]
    # generated_date 는 readiness-config.ratified_date 에서 받는다 — 결정론적,
    # timestamp noise 없음. config 변경 시에만 변경.
    ratified_date = str(config.get("ratified_date", ""))

    # nf-impl-status DEV_FM_KEYS 6개 (status, source_architecture,
    # source_contract, generated_date, nf, stage) 를 *반드시* 출력해
    # impl_frontmatter_valid PASS 유지. status 는 'draft' 고정 (PR-D 단계에서
    # readiness-config 산출 단계 지표로 전환 가능).
    fm_lines = [
        "---",
        f"nf: {nf}",
        f"stage: {stage}",
        "status: draft",
        f"source_architecture: design/{nf}/architecture",
        f"source_contract: handoff/{nf}/contract.yaml",
        f"generated_date: '{ratified_date}'",
        "generator: design/scripts/nf-readiness-pack-generate.py",
        f"source_readiness_config: design/{nf}/readiness-config.yaml",
        "generated_sections:",
    ]
    for aid in schema["auto_ids"]:
        fm_lines.append(f"  - {aid}")
    fm_lines.append("user_sections:")
    for uid in schema["user_ids"]:
        fm_lines.append(f"  - {uid}")
    fm_lines.append("---")
    fm = "\n".join(fm_lines)

    parts = [
        fm,
        "",
        f"# {title}",
        "",
        user_block("intro-note", user_body("intro-note")),
        "",
        "## Contract → Module",
        "",
        user_block("contract-module-table", user_body("contract-module-table")),
        "",
        "## Module → Test",
        "",
        user_block("module-test-table", user_body("module-test-table")),
        "",
        auto_block("test-inventory-index", autos["test-inventory-index"]),
        "",
        "## Open Gaps",
        "",
        user_block("open-gaps", user_body("open-gaps")),
        "",
        auto_block("deferred-decisions-trace", autos["deferred-decisions-trace"]),
        "",
        auto_block("references", autos["references"]),
        "",
    ]
    return "\n".join(parts).rstrip() + "\n"


def render_implementation_plan(nf: str, current_text: str, config: dict) -> str:
    target = "dev/<nf>/implementation-plan.md"
    schema = TARGETS[target]
    autos = {
        "engineering-decisions-summary":
            render_engineering_decisions_summary(nf, config),
    }
    users_existing = extract_blocks(current_text, "USER") if current_text else {}

    def user_body(uid: str) -> str:
        body = users_existing.get(uid)
        if body is None or not body.strip():
            return f"TODO: {uid} — 사람이 보강 (fresh placeholder)."
        return body

    title = TARGET_TITLES[target].format(nf_upper=nf.upper())
    stage = TARGET_STAGES[target]
    ratified_date = str(config.get("ratified_date", ""))

    fm_lines = [
        "---",
        f"nf: {nf}",
        f"stage: {stage}",
        "status: draft",
        f"source_architecture: design/{nf}/architecture",
        f"source_contract: handoff/{nf}/contract.yaml",
        f"generated_date: '{ratified_date}'",
        "generator: design/scripts/nf-readiness-pack-generate.py",
        f"source_readiness_config: design/{nf}/readiness-config.yaml",
        "generated_sections:",
    ]
    for aid in schema["auto_ids"]:
        fm_lines.append(f"  - {aid}")
    fm_lines.append("user_sections:")
    for uid in schema["user_ids"]:
        fm_lines.append(f"  - {uid}")
    fm_lines.append("---")
    fm = "\n".join(fm_lines)

    parts = [
        fm,
        "",
        f"# {title}",
        "",
        "## Scope",
        "",
        user_block("scope-body", user_body("scope-body")),
        "",
        auto_block("engineering-decisions-summary",
                   autos["engineering-decisions-summary"]),
        "",
        "## Phases",
        "",
        user_block("phases-body", user_body("phases-body")),
        "",
        "## Test Plan",
        "",
        user_block("test-plan-body", user_body("test-plan-body")),
        "",
        "## Open Risks",
        "",
        user_block("open-risks-body", user_body("open-risks-body")),
        "",
        "## References",
        "",
        user_block("references-body", user_body("references-body")),
        "",
    ]
    return "\n".join(parts).rstrip() + "\n"


def render_test_matrix(nf: str, current_text: str, config: dict) -> str:
    target = "dev/<nf>/test-matrix.md"
    schema = TARGETS[target]
    autos = {
        "security-coverage-summary": render_security_coverage_summary(nf),
    }
    users_existing = extract_blocks(current_text, "USER") if current_text else {}

    def user_body(uid: str) -> str:
        body = users_existing.get(uid)
        if body is None or not body.strip():
            return f"TODO: {uid} — 사람이 보강 (fresh placeholder)."
        return body

    title = TARGET_TITLES[target].format(nf_upper=nf.upper())
    stage = TARGET_STAGES[target]
    ratified_date = str(config.get("ratified_date", ""))

    fm_lines = [
        "---",
        f"nf: {nf}",
        f"stage: {stage}",
        "status: draft",
        f"source_architecture: design/{nf}/architecture",
        f"source_contract: handoff/{nf}/contract.yaml",
        f"generated_date: '{ratified_date}'",
        "generator: design/scripts/nf-readiness-pack-generate.py",
        f"source_readiness_config: design/{nf}/readiness-config.yaml",
        "generated_sections:",
    ]
    for aid in schema["auto_ids"]:
        fm_lines.append(f"  - {aid}")
    fm_lines.append("user_sections:")
    for uid in schema["user_ids"]:
        fm_lines.append(f"  - {uid}")
    fm_lines.append("---")
    fm = "\n".join(fm_lines)

    parts = [
        fm,
        "",
        f"# {title}",
        "",
        user_block("intro-note", user_body("intro-note")),
        "",
        "## Purpose",
        "",
        user_block("purpose-body", user_body("purpose-body")),
        "",
        "## Test Inventory",
        "",
        user_block("test-inventory-body", user_body("test-inventory-body")),
        "",
        "## Coverage Rules",
        "",
        user_block("coverage-rules-body", user_body("coverage-rules-body")),
        "",
        auto_block("security-coverage-summary",
                   autos["security-coverage-summary"]),
        "",
        "## Open Questions",
        "",
        user_block("open-questions-body", user_body("open-questions-body")),
        "",
        "## References",
        "",
        user_block("references-body", user_body("references-body")),
        "",
    ]
    return "\n".join(parts).rstrip() + "\n"


def render_team_execution_plan(nf: str, current_text: str, config: dict) -> str:
    target = "dev/<nf>/team-execution-plan.md"
    schema = TARGETS[target]
    autos = {
        "phase-integration-map":
            render_phase_integration_map(nf, config),
    }
    users_existing = extract_blocks(current_text, "USER") if current_text else {}

    def user_body(uid: str) -> str:
        body = users_existing.get(uid)
        if body is None or not body.strip():
            return f"TODO: {uid} — 사람이 보강 (fresh placeholder)."
        return body

    title = TARGET_TITLES[target].format(nf_upper=nf.upper())
    stage = TARGET_STAGES[target]
    ratified_date = str(config.get("ratified_date", ""))

    fm_lines = [
        "---",
        f"nf: {nf}",
        f"stage: {stage}",
        "status: draft",
        f"source_architecture: design/{nf}/architecture",
        f"source_contract: handoff/{nf}/contract.yaml",
        f"generated_date: '{ratified_date}'",
        "generator: design/scripts/nf-readiness-pack-generate.py",
        f"source_readiness_config: design/{nf}/readiness-config.yaml",
        "generated_sections:",
    ]
    for aid in schema["auto_ids"]:
        fm_lines.append(f"  - {aid}")
    fm_lines.append("user_sections:")
    for uid in schema["user_ids"]:
        fm_lines.append(f"  - {uid}")
    fm_lines.append("---")
    fm = "\n".join(fm_lines)

    parts = [
        fm,
        "",
        f"# {title}",
        "",
        user_block("intro-note", user_body("intro-note")),
        "",
        auto_block("phase-integration-map", autos["phase-integration-map"]),
        "",
        user_block("body", user_body("body")),
        "",
        "## References",
        "",
        user_block("references-body", user_body("references-body")),
        "",
    ]
    return "\n".join(parts).rstrip() + "\n"


def render_open_gaps_and_assumptions(nf: str, current_text: str, config: dict) -> str:
    target = "dev/<nf>/open-gaps-and-assumptions.md"
    schema = TARGETS[target]
    autos = {
        "gaps-table": render_gaps_table(nf, config),
    }
    users_existing = extract_blocks(current_text, "USER") if current_text else {}

    def user_body(uid: str) -> str:
        body = users_existing.get(uid)
        if body is None or not body.strip():
            return f"TODO: {uid} — 사람이 보강 (fresh placeholder)."
        return body

    title = TARGET_TITLES[target].format(nf_upper=nf.upper())
    stage = TARGET_STAGES[target]
    ratified_date = str(config.get("ratified_date", ""))

    fm_lines = [
        "---",
        f"nf: {nf}",
        f"stage: {stage}",
        "status: draft",
        f"source_architecture: design/{nf}/architecture",
        f"source_contract: handoff/{nf}/contract.yaml",
        f"generated_date: '{ratified_date}'",
        "generator: design/scripts/nf-readiness-pack-generate.py",
        f"source_readiness_config: design/{nf}/readiness-config.yaml",
        "generated_sections:",
    ]
    for aid in schema["auto_ids"]:
        fm_lines.append(f"  - {aid}")
    fm_lines.append("user_sections:")
    for uid in schema["user_ids"]:
        fm_lines.append(f"  - {uid}")
    fm_lines.append("---")
    fm = "\n".join(fm_lines)

    parts = [
        fm,
        "",
        f"# {title}",
        "",
        user_block("intro-note", user_body("intro-note")),
        "",
        "## Gaps",
        "",
        auto_block("gaps-table", autos["gaps-table"]),
        "",
        "## Summary",
        "",
        user_block("summary-body", user_body("summary-body")),
        "",
        "## References",
        "",
        user_block("references-body", user_body("references-body")),
        "",
    ]
    return "\n".join(parts).rstrip() + "\n"


def render_verification_plan(nf: str, current_text: str, config: dict) -> str:
    target = "dev/<nf>/verification-plan.md"
    schema = TARGETS[target]
    autos = {
        "security-gate-matrix": render_security_gate_matrix(nf),
    }
    users_existing = extract_blocks(current_text, "USER") if current_text else {}

    def user_body(uid: str) -> str:
        body = users_existing.get(uid)
        if body is None or not body.strip():
            return f"TODO: {uid} — 사람이 보강 (fresh placeholder)."
        return body

    title = TARGET_TITLES[target].format(nf_upper=nf.upper())
    stage = TARGET_STAGES[target]
    ratified_date = str(config.get("ratified_date", ""))

    fm_lines = [
        "---",
        f"nf: {nf}",
        f"stage: {stage}",
        "status: draft",
        f"source_architecture: design/{nf}/architecture",
        f"source_contract: handoff/{nf}/contract.yaml",
        f"generated_date: '{ratified_date}'",
        "generator: design/scripts/nf-readiness-pack-generate.py",
        f"source_readiness_config: design/{nf}/readiness-config.yaml",
        "generated_sections:",
    ]
    for aid in schema["auto_ids"]:
        fm_lines.append(f"  - {aid}")
    fm_lines.append("user_sections:")
    for uid in schema["user_ids"]:
        fm_lines.append(f"  - {uid}")
    fm_lines.append("---")
    fm = "\n".join(fm_lines)

    parts = [
        fm,
        "",
        f"# {title}",
        "",
        user_block("intro-note", user_body("intro-note")),
        "",
        "## Unit",
        "",
        user_block("unit-body", user_body("unit-body")),
        "",
        "## Integration",
        "",
        user_block("integration-body", user_body("integration-body")),
        "",
        "## Contract",
        "",
        user_block("contract-body", user_body("contract-body")),
        "",
        "## Security",
        "",
        user_block("security-body", user_body("security-body")),
        "",
        auto_block("security-gate-matrix", autos["security-gate-matrix"]),
        "",
        "## End-to-End",
        "",
        user_block("end-to-end-body", user_body("end-to-end-body")),
        "",
        "## Observability",
        "",
        user_block("observability-body", user_body("observability-body")),
        "",
        "## References",
        "",
        user_block("references-body", user_body("references-body")),
        "",
    ]
    return "\n".join(parts).rstrip() + "\n"


# Target dispatch — file template → renderer.
RENDERERS = {
    "dev/<nf>/traceability.md": render_traceability,
    "dev/<nf>/implementation-plan.md": render_implementation_plan,
    "dev/<nf>/test-matrix.md": render_test_matrix,
    "dev/<nf>/team-execution-plan.md": render_team_execution_plan,
    "dev/<nf>/open-gaps-and-assumptions.md": render_open_gaps_and_assumptions,
    "dev/<nf>/verification-plan.md": render_verification_plan,
}


# ─── action ─────────────────────────────────────────────────────────

def render_target(nf: str, target_tpl: str, config: dict) -> tuple[pathlib.Path, str, str]:
    """반환 (path, current_text, rendered_text)."""
    rel = target_tpl.replace("<nf>", nf)
    path = REPO / rel
    current = path.read_text(encoding="utf-8") if path.is_file() else ""
    rendered = RENDERERS[target_tpl](nf, current, config)
    return path, current, rendered


def diff_summary(current: str, rendered: str, max_lines: int = 10) -> list[str]:
    """단순 라인 diff 요약 — 자세한 diff 는 `git diff` 가 책임."""
    cur = current.splitlines()
    ren = rendered.splitlines()
    out = []
    n = max(len(cur), len(ren))
    for i in range(n):
        a = cur[i] if i < len(cur) else "<EOF>"
        b = ren[i] if i < len(ren) else "<EOF>"
        if a != b:
            out.append(f"  line {i+1}:")
            out.append(f"    current : {a}")
            out.append(f"    rendered: {b}")
            if len(out) >= max_lines * 3:
                out.append(f"  ... (truncated; more diff at line {i+1}+)")
                break
    return out


def cmd_check(nf: str) -> int:
    config = load_readiness_config(nf)
    fail = 0
    for target_tpl in TARGETS:
        path, current, rendered = render_target(nf, target_tpl, config)
        rel = path.relative_to(REPO)
        if current == rendered:
            print(f"PASS — {rel}")
            continue
        if not current:
            print(f"FAIL — {rel} 없음 (run --write)")
        else:
            print(f"FAIL — {rel} drift detected")
            for ln in diff_summary(current, rendered):
                print(ln)
        fail += 1
    return 0 if fail == 0 else 1


def cmd_write(nf: str) -> int:
    config = load_readiness_config(nf)
    for target_tpl in TARGETS:
        path, current, rendered = render_target(nf, target_tpl, config)
        rel = path.relative_to(REPO)
        if current == rendered:
            print(f"unchanged — {rel}")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(rendered, encoding="utf-8")
            print(f"wrote — {rel}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="readiness pack render/check (PR-4: traceability only)")
    parser.add_argument("nf", help="NF key (예 nssf)")
    grp = parser.add_mutually_exclusive_group(required=True)
    grp.add_argument("--check", action="store_true",
                     help="render to memory, compare with tracked, exit 1 if drift")
    grp.add_argument("--write", action="store_true",
                     help="render to tracked path, 멱등")
    args = parser.parse_args()

    nf = args.nf
    config_path = REPO / "design" / nf / "readiness-config.yaml"
    if not config_path.is_file():
        print(f"FAIL — {config_path.relative_to(REPO)} 없음. PR-3 (readiness-config) "
              "이 머지됐는지 확인.", file=sys.stderr)
        return 2

    if args.check:
        return cmd_check(nf)
    return cmd_write(nf)


if __name__ == "__main__":
    sys.exit(main())
