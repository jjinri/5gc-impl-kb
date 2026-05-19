#!/usr/bin/env python3
# engineering/<nf>/engineering-design.md 의 eng_frozen 충족을 검사해 _engineering_status.yaml 산출
"""
Usage:
    .venv/bin/python3 design/scripts/nf-eng-status.py <nf> [--no-write]

옵션:
    --no-write   engineering/<nf>/_engineering_status.yaml 저장 없이 stdout 만 보고

본 script 는 Engineering Design Freeze 단계 status 검사다 (ADR-0002).
contract=nf-status.py, architecture=nf-arch-status.py, impl=nf-impl-status.py
와 분리된다. eng_frozen 은 자율 코드 생성의 *유일한 GO 신호* 다.

설계 원칙 (ADR-0002 + plan docs/plans/2026-05-19-eng-design-freeze-skill-build-plan.md).
  - read-only. 산출 수정 안 함. _engineering_status.yaml 만 기계가 매번 새로 씀.
  - blocking 은 *결정론 구조 검사* 만. LLM/sub-agent judge 절대 blocking 금지.
  - inventory 하드코딩 금지 — core slot 은 design/schemas/engineering-core-slots.yaml
    profile 을 read-only 로 읽고, NF-specific 은 per-NF 연기 레지스터
    (design/<nf>/architecture/decisions/ADR-0001-architecture-baseline.md
    ## Open choices) 에서 흡수. 둘의 합집합이 inventory.
  - 출력 namespace 분리 — gates.eng_frozen (PASS/FAIL, blocking) 과
    advisory.impl_plan_alignment (PASS/WARN/SKIP, 비차단) 는 절대 섞지 않는다.
  - deferral register parse fail/missing = eng_frozen FAIL (F-hard 입력).

canonical 섹션 상수 ENG_CANON 은 nf-eng-design/SKILL.md mandate 와 *동일 source*.
한쪽만 바꾸면 drift — 함께 갱신해야 한다.

engineering-design.md ## Decisions 포맷 — 그 섹션 아래 *첫 ```yaml fenced block*
이 `inventory_id: {row}` 매핑. row 공통 필드 + slot typed 필드. (SKILL.md mandate
가 동일 포맷 강제 — single source.)
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent
PROFILE_PATH = REPO / "design" / "schemas" / "engineering-core-slots.yaml"

ENG_CANON = [
    "## Purpose", "## Decisions", "## Out of scope",
    "## Open Questions", "## References",
]

# per-NF 연기 레지스터 — F architecture 산출 (전역 docs/adr/ADR-0001 아님).
DEFERRAL_REL = "architecture/decisions/ADR-0001-architecture-baseline.md"
DEFERRAL_HEADING = "## Open choices"

# advisory 전용 — 미결정 표현 (acceptance/open-risk 에 남으면 WARN 후보).
UNDECIDED_RE = re.compile(r"\b(TBD|decide|choose|select|determine)\b", re.I)

GATE_DEFS = [
    ("eng_frozen", [
        "eng_profile_valid",
        "eng_design_present",
        "eng_sections_exact",
        "eng_open_questions_empty",
        "eng_deferral_register_ok",
        "eng_inventory_covered",
        "eng_rows_well_formed",
        "eng_slot_typed_shape",
    ]),
]


def slugify(text: str) -> str:
    """unicode-aware. 연기 레지스터 choice 가 한국어 산문이므로 ASCII-only 면
    한글 행이 빈 slug 로 붕괴·충돌한다. \\w(unicode) 보존, 그 외 → '_'."""
    s = re.sub(r"[^\w]+", "_", text.strip().lower(), flags=re.UNICODE)
    return s.strip("_")


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


def section_body(text: str, heading: str) -> str | None:
    """`heading` 다음부터 다음 `## ` 직전까지의 본문. heading 없으면 None."""
    lines = text.splitlines()
    out, capturing = [], False
    for ln in lines:
        if ln.strip() == heading:
            capturing = True
            continue
        if capturing and ln.startswith("## "):
            break
        if capturing:
            out.append(ln)
    return "\n".join(out) if capturing else None


def first_yaml_block(body: str) -> tuple[object, str | None]:
    """본문 안 첫 ```yaml ... ``` fenced block 을 파싱. (obj, error)."""
    m = re.search(r"```ya?ml\s*\n(.*?)\n```", body, re.S)
    if not m:
        return None, "```yaml fenced block 없음"
    try:
        return yaml.safe_load(m.group(1)), None
    except yaml.YAMLError as e:
        return None, f"yaml 파싱 실패: {e}"


def h2(text: str) -> list[str]:
    return [ln.strip() for ln in text.splitlines() if ln.startswith("## ")]


def load_profile() -> tuple[dict, str | None]:
    if not PROFILE_PATH.exists():
        return {}, f"{PROFILE_PATH.relative_to(REPO)} 없음"
    try:
        d = yaml.safe_load(PROFILE_PATH.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        return {}, f"profile 파싱 실패: {e}"
    if "version" not in d or "slots" not in d or "common_row_fields" not in d:
        return {}, "profile 에 version/slots/common_row_fields 필요"
    return d, None


def parse_deferral_register(nf: str) -> tuple[list[str], str | None]:
    """per-NF 연기 레지스터의 choice 행 → inventory id 리스트.
    파일/heading/표 누락·파싱 실패는 error 반환 (= eng_frozen FAIL)."""
    p = REPO / "design" / nf / DEFERRAL_REL
    if not p.exists():
        return [], f"{p.relative_to(REPO)} 없음 (F-hard 입력 누락)"
    body = section_body(p.read_text(encoding="utf-8"), DEFERRAL_HEADING)
    if body is None:
        return [], f"{DEFERRAL_HEADING} 섹션 없음"
    rows = []
    for ln in body.splitlines():
        ln = ln.strip()
        if not ln.startswith("|") or ln.startswith("|--") or ln.startswith("|-"):
            continue
        cells = [c.strip() for c in ln.strip("|").split("|")]
        if len(cells) < 2:
            continue
        head = cells[0].lower()
        if head in ("choice", "결정", "decision") or not cells[0]:
            continue  # 헤더 행
        rows.append(slugify(cells[0]))
    if not rows:
        return [], f"{DEFERRAL_HEADING} 표에서 choice 행 0 (파싱 실패 의심)"
    return rows, None


# ---- checks (Tier1 binary, eng_frozen AND) ----

def chk_profile(profile: dict, perr: str | None) -> dict:
    base = {"id": "eng_profile_valid", "tier": 1,
            "name": "engineering-core-slots profile 유효",
            "criterion": f"{PROFILE_PATH.relative_to(REPO)} 파싱 + version/slots/"
                         "common_row_fields 보유."}
    if perr:
        base.update(status="FAIL", current=perr,
                    to_pass=["profile 생성/수정 — pipeline policy PR"])
    else:
        base.update(status="PASS",
                    current=f"version={profile['version']} slots={len(profile['slots'])}",
                    to_pass=[])
    return base


def chk_design_present(ed: pathlib.Path) -> dict:
    base = {"id": "eng_design_present", "tier": 1,
            "name": "engineering/<nf>/engineering-design.md 존재",
            "criterion": "engineering/<nf>/engineering-design.md 파일 존재."}
    if ed.exists():
        base.update(status="PASS", current="존재", to_pass=[])
    else:
        base.update(status="FAIL", current="없음",
                    to_pass=[f"/nf-eng-design <nf> 으로 {ed.name} 생성"])
    return base


def chk_sections(ed: pathlib.Path) -> dict:
    base = {"id": "eng_sections_exact", "tier": 1,
            "name": "engineering-design.md H2 == canonical",
            "criterion": f"`## ` 목록 == {ENG_CANON} (순서까지)."}
    if not ed.exists():
        base.update(status="FAIL", current="파일 없음", to_pass=["파일 생성 후 재실행"])
        return base
    _, body = parse_frontmatter(ed.read_text(encoding="utf-8"))
    got = h2(body)
    if got != ENG_CANON:
        base.update(status="FAIL", current=f"불일치 — {got}",
                    to_pass=["canonical 섹션 순서·집합 일치 "
                             "(nf-eng-design SKILL.md mandate 참조)"])
    else:
        base.update(status="PASS", current="canonical 일치", to_pass=[])
    return base


def chk_open_questions_empty(ed: pathlib.Path) -> dict:
    base = {"id": "eng_open_questions_empty", "tier": 1,
            "name": "## Open Questions 비어있음 (R3 PASS 규칙)",
            "criterion": "## Open Questions 섹션에 실질 내용 없음 — 잔존 Open Question "
                         "은 미결정이라 freeze 와 모순."}
    if not ed.exists():
        base.update(status="FAIL", current="파일 없음", to_pass=["파일 생성 후 재실행"])
        return base
    _, body = parse_frontmatter(ed.read_text(encoding="utf-8"))
    sec = section_body(body, "## Open Questions")
    if sec is None:
        base.update(status="FAIL", current="## Open Questions 섹션 없음",
                    to_pass=["canonical 섹션 보강"])
        return base
    # 실질 내용 = 공백·주석·placeholder(none/없음/n/a) 외 텍스트.
    meaningful = [
        ln for ln in sec.splitlines()
        if ln.strip()
        and ln.strip().lower() not in ("none", "n/a", "없음", "-", "—")
        and not ln.strip().startswith(("<!--", "#"))
    ]
    if meaningful:
        base.update(status="FAIL",
                    current=f"잔존 Open Question {len(meaningful)}줄",
                    to_pass=["모든 Open Question 을 Decisions 로 해소 후 비움"])
    else:
        base.update(status="PASS", current="비어있음", to_pass=[])
    return base


def chk_deferral_register(reg_err: str | None, reg_ids: list[str]) -> dict:
    base = {"id": "eng_deferral_register_ok", "tier": 1,
            "name": "per-NF 연기 레지스터 파싱 성공 (F-hard 입력)",
            "criterion": f"design/<nf>/{DEFERRAL_REL} 의 {DEFERRAL_HEADING} 표 파싱 성공. "
                         "누락·파싱실패 = FAIL (조용한 PASS 금지)."}
    if reg_err:
        base.update(status="FAIL", current=reg_err,
                    to_pass=["F architecture 의 연기 레지스터 표 복구/생성"])
    else:
        base.update(status="PASS", current=f"register rows={len(reg_ids)}", to_pass=[])
    return base


def chk_inventory_covered(rows: dict, inv_ids: list[str],
                          rows_err: str | None) -> dict:
    base = {"id": "eng_inventory_covered", "tier": 1,
            "name": "inventory(core slot ∪ 연기 레지스터) 전부 ## Decisions 에 존재",
            "criterion": "profile core slot + per-NF register row 모든 id 가 "
                         "engineering-design.md ## Decisions yaml 매핑에 키로 존재."}
    if rows_err:
        base.update(status="FAIL", current=rows_err,
                    to_pass=["## Decisions 에 ```yaml inventory 매핑 작성"])
        return base
    missing = [i for i in inv_ids if i not in rows]
    if missing:
        base.update(status="FAIL", current=f"미포함 id — {missing}",
                    to_pass=[f"## Decisions 에 누락 inventory 행 추가 — {missing}"])
    else:
        base.update(status="PASS", current=f"{len(inv_ids)} id 모두 존재", to_pass=[])
    return base


def chk_rows_well_formed(rows: dict, profile: dict, inv_ids: list[str],
                         rows_err: str | None) -> dict:
    base = {"id": "eng_rows_well_formed", "tier": 1,
            "name": "각 row 공통 필드·status·ratify·미결정표현 규칙 충족",
            "criterion": "common_row_fields 존재; status∈enum; ratified_by·date 는 "
                         "모든 frozen row 필수; explicitly_out_of_scope 는 "
                         "rationale/consequence/source 실질; TBD/unresolved/bare "
                         "owner:dev 잔존 없음."}
    if rows_err:
        base.update(status="FAIL", current=rows_err, to_pass=["## Decisions 작성"])
        return base
    crf = profile["common_row_fields"]
    req = crf["required"]
    enum = set(crf["status_enum"])
    ratify_for = set(crf.get("ratify_required_for", enum))
    oos_sub = crf.get("out_of_scope_substantive", [])
    bad = []
    for iid in inv_ids:
        r = rows.get(iid)
        if not isinstance(r, dict):
            bad.append(f"{iid} (행 아님)")
            continue
        miss = [k for k in req if k not in r or r.get(k) in (None, "")]
        if miss:
            bad.append(f"{iid} 누락 {miss}")
        st = r.get("status")
        if st not in enum:
            bad.append(f"{iid} status={st!r} ∉ {sorted(enum)}")
            continue
        if st in ratify_for:
            for k in ("ratified_by", "date"):
                if not r.get(k):
                    bad.append(f"{iid} frozen 인데 {k} 없음")
        if st == "explicitly_out_of_scope":
            for k in oos_sub:
                if not str(r.get(k, "")).strip():
                    bad.append(f"{iid} out_of_scope {k} 비실질")
        blob = " ".join(str(v) for v in r.values())
        if UNDECIDED_RE.search(blob) or "owner: dev" in blob or "owner:dev" in blob:
            bad.append(f"{iid} 미결정/owner:dev 표현 잔존")
    if bad:
        base.update(status="FAIL", current=f"불량 — {bad}",
                    to_pass=["각 row 를 decided|explicitly_out_of_scope 로 ratify "
                             "(ratified_by·date 필수), 미결정 표현 제거"])
    else:
        base.update(status="PASS", current=f"{len(inv_ids)} row 적합", to_pass=[])
    return base


def _shape_fields(slotdef: dict, row: dict) -> tuple[list[str], str | None]:
    """slot 의 required field 이름 리스트 + (불충족 시) 사유."""
    if "required_shape" in slotdef:
        rs = slotdef["required_shape"]
        return (list(rs) if isinstance(rs, dict) else list(rs)), None
    if "discriminant" in slotdef:
        disc = slotdef["discriminant"]
        val = row.get(disc)
        if val is None:
            return [], f"discriminant `{disc}` 값 없음"
        variants = slotdef.get("variants", {})
        # PyYAML 은 bare `enabled: true|false` 를 bool 로 읽어 str(True)=="True"
        # 가 되므로 variant 키("true"/"false")와 안 맞는다. 정상 YAML boolean
        # 결정문이 false-negative 로 eng_frozen 을 막지 않게 정규화한다.
        # profile variant 키는 전부 lowercase 라 .lower() 매칭이 안전.
        key = str(val).strip().lower()
        var = variants.get(key)
        if var is None:
            return [], (f"discriminant {disc}={val!r} 가 variants "
                        f"{sorted(variants)} 에 없음")
        return list(var.get("required", [])), None
    return [], "slot 정의에 required_shape/discriminant 없음"


def chk_slot_typed_shape(rows: dict, profile: dict,
                         rows_err: str | None) -> dict:
    base = {"id": "eng_slot_typed_shape", "tier": 1,
            "name": "core slot row 가 profile typed shape 충족 (conditional 포함)",
            "criterion": "각 core slot row 가 required_shape 의 필드를, conditional "
                         "slot 은 discriminant 값에 해당하는 variant 의 required "
                         "필드를 모두 보유."}
    if rows_err:
        base.update(status="FAIL", current=rows_err, to_pass=["## Decisions 작성"])
        return base
    bad = []
    for slot, sdef in profile["slots"].items():
        r = rows.get(slot)
        if not isinstance(r, dict):
            bad.append(f"{slot} (행 없음/형식 오류)")
            continue
        # explicitly_out_of_scope 는 typed shape 면제 (제외 결정 자체가 합법).
        if r.get("status") == "explicitly_out_of_scope":
            continue
        fields, why = _shape_fields(sdef, r)
        if why:
            bad.append(f"{slot}: {why}")
            continue
        miss = [f for f in fields if f not in r or r.get(f) in (None, "")]
        if miss:
            bad.append(f"{slot} typed 필드 누락 {miss}")
    if bad:
        base.update(status="FAIL", current=f"불충족 — {bad}",
                    to_pass=["profile 의 slot typed shape 에 맞춰 결정값 채움 "
                             "(rdbms 면 tables/columns/PK/indexes/... 필수)"])
    else:
        base.update(status="PASS",
                    current=f"{len(profile['slots'])} slot typed shape 충족",
                    to_pass=[])
    return base


# ---- advisory (non-blocking, gate 비포함, 별도 namespace) ----

def advisory_impl_plan_alignment(nf: str, rows: dict) -> dict:
    base = {"id": "impl_plan_alignment",
            "name": "G impl-plan tasks 의 미결정 표현이 결정과 매칭 (advisory)",
            "criterion": "dev/<nf>/tasks.yaml 의 acceptance/open-risk 에 미결정 표현이 "
                         "있고 ## Decisions 와 매칭 안 되면 WARN. tasks.yaml 부재 = "
                         "SKIP. 절대 FAIL/blocking 아님."}
    tp = REPO / "dev" / nf / "tasks.yaml"
    if not tp.exists():
        base.update(status="SKIP", current="dev/<nf>/tasks.yaml 없음", to_pass=[])
        return base
    try:
        td = yaml.safe_load(tp.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        base.update(status="WARN", current=f"tasks.yaml 파싱 실패: {e}",
                    to_pass=["tasks.yaml 점검 (advisory)"])
        return base
    decided_blob = " ".join(str(v) for r in rows.values()
                            if isinstance(r, dict) for v in r.values()).lower()
    flagged = []
    tasks = td.get("tasks", td if isinstance(td, list) else [])
    for t in (tasks if isinstance(tasks, list) else []):
        if not isinstance(t, dict):
            continue
        txt = " ".join(str(t.get(k, "")) for k in
                       ("acceptance", "open_risk", "open_risks", "risk"))
        if UNDECIDED_RE.search(txt):
            tid = str(t.get("id", t.get("name", "?")))
            # 결정과 매칭 — task id 또는 키워드가 decided blob 에 있으면 OK.
            if tid.lower() not in decided_blob:
                flagged.append(tid)
    if flagged:
        base.update(status="WARN",
                    current=f"미결정 표현·Decisions 미매칭 task — {flagged}",
                    to_pass=["해당 결정을 engineering-design.md ## Decisions 에 "
                             "반영하거나 task 표현 정리 (비차단)"])
    else:
        base.update(status="PASS", current="미결정 표현 잔존 없음", to_pass=[])
    return base


def load_prev_pass_anyway(p: pathlib.Path) -> list:
    if not p.exists():
        return []
    try:
        prev = yaml.safe_load(p.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError:
        return []
    raw = (prev.get("manual_overrides") or {}).get("pass_anyway") or []
    return raw if isinstance(raw, list) else []


def apply_overrides(checks: list[dict], raw: list) -> tuple[list, list]:
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
            ignored.append(f"{cid} (FAIL 아님)")
        else:
            c["overridden"] = True
            applied.append(cid)
    return applied, ignored


def compute_gates(checks: list[dict]) -> list[dict]:
    by_id = {c["id"]: c for c in checks}
    out = []
    for gid, required in GATE_DEFS:
        blocked = []
        for cid in required:
            c = by_id.get(cid)
            if c is None:
                blocked.append(f"{cid} (미존재)")
            elif c["status"] == "FAIL" and not c.get("overridden"):
                blocked.append(cid)
        out.append({"id": gid, "requires_pass": required,
                    "status": "FAIL" if blocked else "PASS",
                    "blocked_by": blocked})
    return out


def render_yaml(nf: str, checks: list[dict], gates: list[dict],
                advisory: dict, pass_anyway: list, ignored: list) -> str:
    ordered = []
    for c in checks:
        keys = ["id", "tier", "name", "criterion", "status"]
        if c.get("overridden"):
            keys.append("overridden")
        keys += ["current", "to_pass"]
        ordered.append({k: c.get(k, "" if k != "to_pass" else []) for k in keys})
    doc = {
        "nf": nf,
        "stage": "engineering-design-freeze",
        "target_schema": "eng-status-v1",
        "checks": ordered,
        # namespace 분리 — gates(blocking) 과 advisory(비차단) 절대 안 섞음.
        "gates": gates,
        "advisory": {advisory["id"]: {
            "status": advisory["status"], "name": advisory["name"],
            "criterion": advisory["criterion"], "current": advisory["current"],
            "to_pass": advisory["to_pass"],
        }},
        "manual_overrides": {"pass_anyway": pass_anyway or []},
    }
    header = [
        "# Auto-generated by design/scripts/nf-eng-status.py",
        "# checks/gates/advisory 는 기계 재생성 — 손으로 수정하지 않음.",
        "# gates.eng_frozen = 자율 코드 생성 GO 신호 (blocking, 결정론).",
        "# advisory.impl_plan_alignment = 비차단 (절대 GO/STOP 미결정).",
        "# manual_overrides.pass_anyway 만 사용자 편집: {check,reason,confirmed_by} 필수.",
    ]
    if ignored:
        header.append(f"# 직전 무시된 override — {ignored}")
    body = yaml.safe_dump(doc, allow_unicode=True, sort_keys=False,
                          default_flow_style=False)
    return "\n".join(header) + "\n\n" + body


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("nf")
    ap.add_argument("--no-write", action="store_true")
    args = ap.parse_args()
    nf = args.nf

    nf_design = REPO / "design" / nf
    if not nf_design.is_dir():
        print(f"[nf-eng-status] design/{nf}/ 없음 — F architecture 먼저",
              file=sys.stderr)
        sys.exit(1)

    eng_dir = REPO / "engineering" / nf
    ed = eng_dir / "engineering-design.md"

    profile, perr = load_profile()
    reg_ids, reg_err = parse_deferral_register(nf)

    # ## Decisions 의 첫 ```yaml block → inventory 매핑.
    rows, rows_err = {}, None
    if ed.exists():
        _, body = parse_frontmatter(ed.read_text(encoding="utf-8"))
        dsec = section_body(body, "## Decisions")
        if dsec is None:
            rows_err = "## Decisions 섹션 없음"
        else:
            obj, e = first_yaml_block(dsec)
            if e:
                rows_err = e
            elif not isinstance(obj, dict):
                rows_err = "## Decisions yaml 이 매핑 아님"
            else:
                rows = obj
    else:
        rows_err = "engineering-design.md 없음"

    core_ids = list(profile.get("slots", {}))
    inv_ids = core_ids + [i for i in reg_ids if i not in core_ids]

    checks = [
        chk_profile(profile, perr),
        chk_design_present(ed),
        chk_sections(ed),
        chk_open_questions_empty(ed),
        chk_deferral_register(reg_err, reg_ids),
        chk_inventory_covered(rows, inv_ids, rows_err),
        chk_rows_well_formed(rows, profile, inv_ids, rows_err)
        if not perr else
        {"id": "eng_rows_well_formed", "tier": 1,
         "name": "각 row 공통 필드·status·ratify·미결정표현 규칙 충족",
         "criterion": "profile 유효해야 평가 가능.",
         "status": "FAIL", "current": "profile 무효로 평가 불가",
         "to_pass": ["profile 복구 후 재실행"]},
        chk_slot_typed_shape(rows, profile, rows_err)
        if not perr else
        {"id": "eng_slot_typed_shape", "tier": 1,
         "name": "core slot row 가 profile typed shape 충족",
         "criterion": "profile 유효해야 평가 가능.",
         "status": "FAIL", "current": "profile 무효로 평가 불가",
         "to_pass": ["profile 복구 후 재실행"]},
    ]

    out_path = eng_dir / "_engineering_status.yaml"
    pass_anyway = load_prev_pass_anyway(out_path)
    applied, ignored = apply_overrides(checks, pass_anyway)
    gates = compute_gates(checks)
    advisory = advisory_impl_plan_alignment(nf, rows)
    yaml_text = render_yaml(nf, checks, gates, advisory, pass_anyway, ignored)

    if not args.no_write:
        eng_dir.mkdir(parents=True, exist_ok=True)
        out_path.write_text(yaml_text, encoding="utf-8")
        print(f"[nf-eng-status] wrote {out_path.relative_to(REPO)}", file=sys.stderr)
    if applied:
        print(f"[nf-eng-status] override 적용 — {applied}", file=sys.stderr)
    if ignored:
        print(f"[nf-eng-status] override 무시 — {ignored}", file=sys.stderr)

    print(yaml_text)

    p = sum(1 for c in checks if c["status"] == "PASS")
    f = sum(1 for c in checks if c["status"] == "FAIL")
    print(f"\n[nf-eng-status] {nf}: PASS {p}, FAIL {f} | "
          f"advisory.impl_plan_alignment={advisory['status']}", file=sys.stderr)
    for g in gates:
        if g["status"] == "FAIL":
            print(f"  gate {g['id']}: FAIL — blocked by {g['blocked_by']}",
                  file=sys.stderr)
        else:
            print(f"  gate {g['id']}: PASS", file=sys.stderr)
    sys.exit(1 if any(g["status"] == "FAIL" for g in gates) else 0)


if __name__ == "__main__":
    main()
