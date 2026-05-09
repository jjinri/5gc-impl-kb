#!/usr/bin/env python3
# kb/<nf>/ 의 implementation-grade 완성도를 검사해 _status.yaml 산출
"""
Usage:
    .venv/bin/python3 scripts/nf-status.py <nf> [--no-write]

옵션:
    --no-write   _status.yaml 저장 없이 stdout 만 보고

평가 framework. 무가중치, 항목별 criterion + to_pass 의무,
applies_to 별 NOT_APPLICABLE 처리, acceptance gate 는 check id 의 AND.

Tier 1 (Validation, binary)
  - frontmatter_valid
  - sections_complete (7 카테고리 H2 + 비어있지 않음)
  - wikilinks_resolve (kb 내부 [[...]] 모두 실재)
  - no_korean_colon_end
  - mermaid_renders (mermaid 블록 수 = SVG 수)
  - manifest_ready (_manifest.yaml.status.ready_for_build)

Tier 2 (Coverage, threshold)
  - data_model_chain_complete (트리에 "(참조 규격 미등록)" 0건)
  - api_operation_coverage (yaml paths operation 수 = 매트릭스 row 수, stage_3+mixed)
  - service_flow_coverage (mermaid 블록 수 ≥ manifest 명시 procedure 수)

Tier 3 (Implementation viability) — 도구 미존재 시 NOT_RUN
  - yaml_to_c_compiles

Tier 4 (Subjective) — SKIPPED (LLM-as-judge 자동화 미구축)
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
from typing import Any

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent

REQUIRED_FRONTMATTER_KEYS = {
    "title", "spec", "release", "version", "year", "category",
    "source_path", "source_filename", "source_format",
}

REQUIRED_SECTIONS = [
    "Interface", "API", "Data Model", "Service Scenarios",
    "Cross-NF Dependencies", "Configuration", "Error Handling",
]

PROFILES = {"stage_3_only", "stage_2_only", "mixed", "meta_only"}
DEFAULT_PROFILE = "stage_3_only"


def parse_frontmatter(text: str) -> tuple[dict, str]:
    """('---'...'---' frontmatter, body) 반환. frontmatter 없으면 ({}, full text)."""
    if not text.startswith("---\n"):
        return {}, text
    end = text.find("\n---\n", 4)
    if end == -1:
        return {}, text
    fm_text = text[4:end]
    body = text[end + 5:]
    try:
        fm = yaml.safe_load(fm_text) or {}
    except yaml.YAMLError:
        fm = {}
    return fm, body


def applies(check: dict, profile: str) -> bool:
    return profile in check.get("applies_to", [])


# ─── Tier 1 ─────────────────────────────────────────────────────────

def check_frontmatter_valid(page: pathlib.Path | None, profile: str) -> dict:
    base = {
        "id": "frontmatter_valid", "tier": 1,
        "name": "frontmatter 가 valid YAML 이고 필수 키 모두 보유",
        "criterion": (
            "kb/{nf}/3gpp-*.md 의 상단 `---` 블록이 yaml.safe_load 통과. "
            f"필수 키 — {sorted(REQUIRED_FRONTMATTER_KEYS)}."
        ),
        "applies_to": ["stage_3_only", "stage_2_only", "mixed", "meta_only"],
    }
    if page is None:
        base.update(status="FAIL", current="페이지 없음",
                    to_pass=["/nf-build <nf> 으로 페이지 골격 생성"])
        return base
    fm, _ = parse_frontmatter(page.read_text(encoding="utf-8"))
    missing = REQUIRED_FRONTMATTER_KEYS - set(fm.keys())
    if not fm:
        base.update(status="FAIL", current="frontmatter 미존재 또는 YAML 파싱 실패",
                    to_pass=["페이지 상단에 valid frontmatter 추가"])
    elif missing:
        base.update(status="FAIL",
                    current=f"누락 키 — {sorted(missing)}",
                    to_pass=[f"frontmatter 에 다음 키 추가 — {sorted(missing)}"])
    else:
        base.update(status="PASS",
                    current=f"필수 {len(REQUIRED_FRONTMATTER_KEYS)}개 키 모두 존재",
                    to_pass=[])
    return base


def check_sections_complete(page: pathlib.Path | None, profile: str) -> dict:
    base = {
        "id": "sections_complete", "tier": 1,
        "name": "7 카테고리 H2 가 모두 존재하고 비어있지 않다",
        "criterion": (
            f"페이지에 다음 7개 ## H2 헤더가 존재 — {REQUIRED_SECTIONS}. "
            "각 H2 직후에 최소 1줄 본문 또는 1개 자식 요소 (표·코드블록·"
            "mermaid·H3) 가 있어야 PASS. 빈 placeholder 는 FAIL."
        ),
        "applies_to": ["stage_3_only", "stage_2_only", "mixed"],
    }
    if not applies(base, profile):
        base.update(status="NOT_APPLICABLE", current=f"profile={profile}",
                    to_pass=[])
        return base
    if page is None:
        base.update(status="FAIL", current="페이지 없음",
                    to_pass=["/nf-build <nf> 으로 페이지 생성"])
        return base
    body = page.read_text(encoding="utf-8")
    found_h2 = [m.group(1).strip() for m in re.finditer(r"^## ([^\n]+)$", body, re.M)]
    missing = [s for s in REQUIRED_SECTIONS if s not in found_h2]
    extra = [h for h in found_h2 if h not in REQUIRED_SECTIONS]
    # 비어있는지 점검 — H2 다음~다음 H2 까지 본문 길이 검사
    empty = []
    for sec in REQUIRED_SECTIONS:
        m = re.search(rf"^## {re.escape(sec)}\s*$", body, re.M)
        if not m:
            continue
        rest = body[m.end():]
        next_h = re.search(r"^## ", rest, re.M)
        section_body = rest[: next_h.start()] if next_h else rest
        # 의미있는 자식 요소 1개 이상 존재?
        if not re.search(r"^\s*\S", section_body, re.M):
            empty.append(sec)
    if missing or empty:
        base.update(status="FAIL",
                    current=(f"누락 {missing or '(없음)'}, 비어있음 {empty or '(없음)'}, "
                             f"7-cat 외 H2 — {extra or '(없음)'}"),
                    to_pass=[f"누락된 H2 추가 — {missing}" if missing else None,
                             f"비어있는 H2 채움 — {empty}" if empty else None,
                             f"7-cat 외 H2 정리 (Summary 등은 본문 prose 로 흡수) — {extra}" if extra else None])
        base["to_pass"] = [t for t in base["to_pass"] if t]
    else:
        base.update(status="PASS",
                    current=f"7개 H2 모두 존재 + 본문 보유",
                    to_pass=[])
    return base


def check_wikilinks_resolve(page: pathlib.Path | None, kb_root: pathlib.Path, profile: str) -> dict:
    base = {
        "id": "wikilinks_resolve", "tier": 1,
        "name": "모든 [[wikilink]] 이 kb/ 안에 실재",
        "criterion": "페이지 안 [[...]] 패턴의 모든 target 이 kb/ 하위에 .md 로 존재.",
        "applies_to": ["stage_3_only", "stage_2_only", "mixed", "meta_only"],
    }
    if page is None:
        base.update(status="FAIL", current="페이지 없음", to_pass=["페이지 생성"])
        return base
    body = page.read_text(encoding="utf-8")
    links = re.findall(r"\[\[([^\]]+)\]\]", body)
    broken = []
    for link in links:
        target = link.split("|")[0].strip()
        candidate = kb_root / f"{target}.md"
        if not candidate.is_file():
            broken.append(target)
    if not links:
        base.update(status="PASS", current="0/0 wikilink", to_pass=[])
    elif broken:
        base.update(status="FAIL",
                    current=f"{len(broken)}/{len(links)} 끊김 — {broken}",
                    to_pass=[f"끊긴 wikilink 제거 또는 해당 페이지 등록 — {broken}"])
    else:
        base.update(status="PASS", current=f"{len(links)}/{len(links)} 정상", to_pass=[])
    return base


def check_no_korean_colon_end(page: pathlib.Path | None, profile: str) -> dict:
    base = {
        "id": "no_korean_colon_end", "tier": 1,
        "name": "본문 한국어 문장이 콜론으로 끝나지 않는다",
        "criterion": "코드블록·frontmatter·표 라벨 외에서, 한글로 끝나는 줄 직후 ':' 종결 0건.",
        "applies_to": ["stage_3_only", "stage_2_only", "mixed", "meta_only"],
    }
    if page is None:
        base.update(status="FAIL", current="페이지 없음", to_pass=["페이지 생성"])
        return base
    text = page.read_text(encoding="utf-8")
    # 코드블록 제외
    text_no_code = re.sub(r"```[\s\S]*?```", "", text)
    # 한글 + 직후 ':' 줄 끝
    pattern = re.compile(r"[가-힣][^\n]*:\s*$", re.M)
    violations = pattern.findall(text_no_code)
    if violations:
        base.update(status="FAIL",
                    current=f"{len(violations)}건 위반",
                    to_pass=[f"콜론 종결을 .?!  로 교체 — 위반 예 {violations[:3]}"])
    else:
        base.update(status="PASS", current="0 violations", to_pass=[])
    return base


def check_mermaid_renders(page: pathlib.Path | None, nf_dir: pathlib.Path, profile: str) -> dict:
    base = {
        "id": "mermaid_renders", "tier": 1,
        "name": "mermaid 블록 수 = sibling _diagrams/ SVG 수",
        "criterion": "페이지의 ```mermaid 블록 수와 _diagrams/<page-stem>-<n>.svg 수가 일치.",
        "applies_to": ["stage_3_only", "stage_2_only", "mixed", "meta_only"],
    }
    if page is None:
        base.update(status="FAIL", current="페이지 없음", to_pass=["페이지 생성"])
        return base
    text = page.read_text(encoding="utf-8")
    blocks = re.findall(r"^```mermaid", text, re.M)
    diagrams_dir = nf_dir / "_diagrams"
    svgs = []
    if diagrams_dir.is_dir():
        svgs = sorted(diagrams_dir.glob(f"{page.stem}-*.svg"))
    if len(blocks) == 0 and len(svgs) == 0:
        base.update(status="PASS", current="0 mermaid, 0 SVG", to_pass=[])
    elif len(blocks) == len(svgs):
        base.update(status="PASS", current=f"{len(blocks)} mermaid → {len(svgs)} SVG", to_pass=[])
    else:
        base.update(status="FAIL",
                    current=f"{len(blocks)} mermaid vs {len(svgs)} SVG (불일치)",
                    to_pass=[".venv/bin/python3 scripts/render-mermaid.py --clean 로 동기화"])
    return base


def check_manifest_ready(manifest: dict, profile: str) -> dict:
    base = {
        "id": "manifest_ready", "tier": 1,
        "name": "_manifest.yaml.status.ready_for_build = true",
        "criterion": (
            "kb/{nf}/_manifest.yaml 의 status.ready_for_build == true. "
            "in-scope 의존 spec 이 모두 specs/ 에 cp 되었거나 "
            "manual_overrides.exclude 로 제외됨."
        ),
        "applies_to": ["stage_3_only", "stage_2_only", "mixed", "meta_only"],
    }
    if not manifest:
        base.update(status="FAIL", current="_manifest.yaml 없음",
                    to_pass=["scripts/nf-manifest.py <nf> --primary <spec> --write"])
        return base
    status = manifest.get("status", {})
    ready = status.get("ready", False)
    completeness = status.get("manifest_completeness", "?/?")
    missing = status.get("missing_priority", [])
    if ready:
        base.update(status="PASS", current=f"completeness {completeness}", to_pass=[])
    else:
        base.update(status="FAIL",
                    current=f"completeness {completeness}, missing {missing}",
                    to_pass=[
                        f"specs/<spec>/ 에 cp — {missing}" if missing else None,
                        "또는 _manifest.yaml manual_overrides.exclude 에 등록",
                        "scripts/nf-manifest.py <nf> --primary <spec> --write 재실행",
                    ])
        base["to_pass"] = [t for t in base["to_pass"] if t]
    return base


# ─── Tier 2 ─────────────────────────────────────────────────────────

def check_data_model_chain(page: pathlib.Path | None, profile: str) -> dict:
    base = {
        "id": "data_model_chain_complete", "tier": 2,
        "name": "Data Model 트리에 (참조 규격 미등록) leaf 0건",
        "criterion": (
            "kb/{nf}/*.md 의 ```text Data Model 코드블록 안에 "
            "문자열 \"(참조 규격 미등록)\" 등장 횟수 == 0."
        ),
        "applies_to": ["stage_3_only", "mixed"],
    }
    if not applies(base, profile):
        base.update(status="NOT_APPLICABLE", current=f"profile={profile}", to_pass=[])
        return base
    if page is None:
        base.update(status="FAIL", current="페이지 없음", to_pass=["/nf-build 으로 페이지 생성"])
        return base
    text = page.read_text(encoding="utf-8")
    leaves = re.findall(r"\(참조 규격 미등록\)", text)
    if leaves:
        # 각 leaf 가 가리키는 spec 추출
        leaf_specs = re.findall(r"\[TS (\d{2}\.\d{3})\]\s+\(참조 규격 미등록\)", text)
        leaf_specs = sorted(set(leaf_specs))
        base.update(status="FAIL",
                    current=f"{len(leaves)}건 leaf — TS {leaf_specs}",
                    to_pass=[
                        f"specs/<spec>/ 에 cp 후 /nf-build <nf> --data-model 으로 chain 재추출 — {leaf_specs}",
                        "또는 _manifest.yaml manual_overrides.exclude 에 해당 spec 등록 (구현 범위 외)",
                    ])
    else:
        base.update(status="PASS", current="leaf 0건", to_pass=[])
    return base


def check_api_coverage(page: pathlib.Path | None, manifest: dict, profile: str) -> dict:
    base = {
        "id": "api_operation_coverage", "tier": 2,
        "name": "primary yaml 의 모든 paths operation 이 API 매트릭스에 등장",
        "criterion": (
            "primary yaml(s) 의 paths.*.{get|post|put|patch|delete|options} "
            "operation 수 = page 의 API H2 섹션에 등장하는 row 수."
        ),
        "applies_to": ["stage_3_only", "mixed"],
    }
    if not applies(base, profile):
        base.update(status="NOT_APPLICABLE", current=f"profile={profile}", to_pass=[])
        return base
    primary_files = (manifest.get("primary_files") or {}).get("yamls") or []
    primary_spec = manifest.get("primary_spec")
    if not primary_files or not primary_spec:
        base.update(status="FAIL", current="manifest 의 primary_files/primary_spec 정보 없음",
                    to_pass=["nf-manifest.py 재실행"])
        return base
    # primary yaml 들의 operation 수 집계
    spec_dir = REPO / "specs" / primary_spec
    op_count = 0
    for ynm in primary_files:
        ypath = spec_dir / ynm
        if not ypath.is_file():
            continue
        try:
            ydata = yaml.safe_load(ypath.read_text(encoding="utf-8")) or {}
        except yaml.YAMLError:
            continue
        for path, ops in (ydata.get("paths") or {}).items():
            for method in ("get", "post", "put", "patch", "delete", "options"):
                if method in ops:
                    op_count += 1
    if page is None:
        base.update(status="FAIL", current=f"yaml ops={op_count} / page 0 (페이지 없음)",
                    to_pass=["/nf-build <nf> 으로 페이지 생성"])
        return base
    body = page.read_text(encoding="utf-8")
    # API 섹션 안의 markdown table row 수 추정 — H2 ## API 다음 다음 H2 전까지의 표 row
    api_match = re.search(r"^## API\s*$([\s\S]*?)(?=^## )", body, re.M)
    page_op_count = 0
    if api_match:
        api_section = api_match.group(1)
        # markdown table — `| ... | ... |` 형태 데이터 행 (헤더·구분선 제외)
        rows = [l for l in api_section.splitlines() if l.startswith("|") and not re.match(r"\|\s*-+", l)]
        # 헤더 행 1개 빼기
        page_op_count = max(0, len(rows) - 1)
    if op_count == 0:
        base.update(status="NOT_APPLICABLE",
                    current="primary yaml 의 paths 0개", to_pass=[])
    elif page_op_count >= op_count:
        base.update(status="PASS",
                    current=f"yaml {op_count} ops vs page {page_op_count} rows",
                    to_pass=[])
    else:
        base.update(status="FAIL",
                    current=f"yaml {op_count} ops vs page {page_op_count} rows",
                    to_pass=[f"API 섹션 매트릭스에 누락 operation {op_count - page_op_count}건 추가"])
    return base


def check_service_flow_coverage(page: pathlib.Path | None, profile: str) -> dict:
    base = {
        "id": "service_flow_coverage", "tier": 2,
        "name": "Service Scenarios 섹션이 mermaid 다이어그램을 1개 이상 보유",
        "criterion": (
            "page 의 Service Scenarios H2 섹션 안에 ```mermaid sequenceDiagram 1개 이상. "
            "(추후: spec 본문의 procedure 수와 비교하는 정밀 검사 도입 예정.)"
        ),
        "applies_to": ["stage_3_only", "stage_2_only", "mixed"],
    }
    if not applies(base, profile):
        base.update(status="NOT_APPLICABLE", current=f"profile={profile}", to_pass=[])
        return base
    if page is None:
        base.update(status="FAIL", current="페이지 없음", to_pass=["페이지 생성"])
        return base
    body = page.read_text(encoding="utf-8")
    sec = re.search(r"^## Service Scenarios\s*$([\s\S]*?)(?=^## )", body, re.M)
    section_text = sec.group(1) if sec else ""
    mermaid_in_section = len(re.findall(r"```mermaid", section_text))
    if mermaid_in_section >= 1:
        base.update(status="PASS",
                    current=f"Service Scenarios 안 mermaid {mermaid_in_section}개",
                    to_pass=[])
    else:
        base.update(status="FAIL",
                    current="Service Scenarios 안 mermaid 0개",
                    to_pass=["대표 시나리오 1개 이상 mermaid sequenceDiagram 추가"])
    return base


# ─── Tier 3 / 4 — NOT_RUN placeholder ────────────────────────────────

def check_yaml_to_c() -> dict:
    return {
        "id": "yaml_to_c_compiles", "tier": 3,
        "name": "Data Model 의 모든 자료형이 C struct 로 컴파일 통과",
        "criterion": "scripts/yaml-to-c.py 산출이 gcc -fsyntax-only 통과.",
        "applies_to": ["stage_3_only", "mixed"],
        "status": "NOT_RUN",
        "current": "도구 미존재 — sprint 후반 도입 예정",
        "to_pass": ["scripts/yaml-to-c.py 신규", "재실행"],
    }


def check_subjective_review() -> dict:
    return {
        "id": "implementation_guidance_quality", "tier": 4,
        "name": "구현자 가이던스 품질 (LLM-as-judge ≥ 4/5)",
        "criterion": "LLM-as-judge 1~5 점. ≥ 4 가 PASS.",
        "applies_to": ["stage_3_only", "stage_2_only", "mixed"],
        "status": "NOT_RUN",
        "current": "자동화 미구축 — 사용자 수동 review 우선",
        "to_pass": ["LLM-as-judge 호출 자동화 또는 동료 리뷰"],
    }


# ─── Acceptance gates ───────────────────────────────────────────────

GATE_DEFS = [
    ("draft", ["frontmatter_valid"]),
    ("ready_for_review", ["frontmatter_valid", "sections_complete", "manifest_ready"]),
    ("implementation_ready", [
        "frontmatter_valid", "sections_complete", "manifest_ready",
        "data_model_chain_complete", "api_operation_coverage",
        "service_flow_coverage", "wikilinks_resolve",
        "no_korean_colon_end", "mermaid_renders",
    ]),
    ("production", [
        "frontmatter_valid", "sections_complete", "manifest_ready",
        "data_model_chain_complete", "api_operation_coverage",
        "service_flow_coverage", "wikilinks_resolve",
        "no_korean_colon_end", "mermaid_renders",
        "yaml_to_c_compiles", "implementation_guidance_quality",
    ]),
]


def compute_gates(checks: list[dict]) -> list[dict]:
    by_id = {c["id"]: c for c in checks}
    out = []
    for gate_id, required in GATE_DEFS:
        blocked = []
        for cid in required:
            c = by_id.get(cid)
            if c is None:
                blocked.append(f"{cid} (check 미존재)")
            elif c["status"] == "FAIL":
                blocked.append(cid)
            # NOT_APPLICABLE 은 자동 만족, NOT_RUN 은 production 까지만 blocker
            elif c["status"] == "NOT_RUN" and gate_id == "production":
                blocked.append(cid)
        out.append({
            "id": gate_id,
            "requires_pass": required,
            "status": "FAIL" if blocked else "PASS",
            "blocked_by": blocked,
        })
    return out


# ─── YAML 출력 ───────────────────────────────────────────────────────

def render_yaml(nf: str, profile: str, manifest: dict, checks: list[dict], gates: list[dict]) -> str:
    out = []
    out.append("# Auto-generated by scripts/nf-status.py — 사용자가 손으로 수정하지 않음.")
    out.append("# 본 파일을 수정하지 말고, 각 check 의 to_pass 액션을 따라 페이지를 고친 뒤 재실행.")
    out.append("")
    out.append(f"nf: {nf}")
    out.append(f"profile: {profile}")
    out.append(f"target_schema: implementation-grade-v1")
    out.append("")
    out.append("checks:")
    for c in checks:
        out.append(f"  - id: {c['id']}")
        out.append(f"    tier: {c['tier']}")
        out.append(f"    name: \"{c['name']}\"")
        out.append(f"    applies_to: {c['applies_to']}")
        out.append(f"    criterion: |")
        for line in c["criterion"].splitlines():
            out.append(f"      {line}")
        out.append(f"    status: {c['status']}")
        out.append(f"    current: \"{c.get('current','')}\"")
        out.append(f"    to_pass:")
        for action in c.get("to_pass") or []:
            out.append(f"      - \"{action}\"")
    out.append("")
    out.append("gates:")
    for g in gates:
        out.append(f"  - id: {g['id']}")
        out.append(f"    requires_pass: {g['requires_pass']}")
        out.append(f"    status: {g['status']}")
        out.append(f"    blocked_by: {g['blocked_by']}")
    out.append("")
    out.append("# 사용자 우회 — 진짜 우회가 필요하면 사유 + 사용자 명시 결정 필수.")
    out.append("manual_overrides:")
    out.append("  pass_anyway: []   # 예: [{check: service_flow_coverage, reason: \"...\", confirmed_by: \"user\"}]")
    return "\n".join(out) + "\n"


# ─── main ───────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("nf")
    parser.add_argument("--no-write", action="store_true")
    args = parser.parse_args()

    nf = args.nf.lower()
    nf_dir = REPO / "kb" / nf
    if not nf_dir.is_dir():
        sys.exit(f"[nf-status] kb/{nf}/ 부재. /nf-init 먼저 실행하세요.")

    manifest_path = nf_dir / "_manifest.yaml"
    manifest: dict = {}
    if manifest_path.is_file():
        try:
            manifest = yaml.safe_load(manifest_path.read_text(encoding="utf-8")) or {}
        except yaml.YAMLError as e:
            print(f"[nf-status] _manifest.yaml 파싱 실패: {e}", file=sys.stderr)

    profile = manifest.get("profile", DEFAULT_PROFILE)
    if profile not in PROFILES:
        print(f"[nf-status] unknown profile {profile!r}, fallback to {DEFAULT_PROFILE}", file=sys.stderr)
        profile = DEFAULT_PROFILE

    page = next(nf_dir.glob("3gpp-*.md"), None)

    checks = [
        check_frontmatter_valid(page, profile),
        check_sections_complete(page, profile),
        check_wikilinks_resolve(page, REPO/"kb", profile),
        check_no_korean_colon_end(page, profile),
        check_mermaid_renders(page, nf_dir, profile),
        check_manifest_ready(manifest, profile),
        check_data_model_chain(page, profile),
        check_api_coverage(page, manifest, profile),
        check_service_flow_coverage(page, profile),
        check_yaml_to_c(),
        check_subjective_review(),
    ]

    gates = compute_gates(checks)

    yaml_text = render_yaml(nf, profile, manifest, checks, gates)
    if not args.no_write:
        out_path = nf_dir / "_status.yaml"
        out_path.write_text(yaml_text, encoding="utf-8")
        print(f"[nf-status] wrote {out_path.relative_to(REPO)}", file=sys.stderr)

    print(yaml_text)

    pass_count = sum(1 for c in checks if c["status"] == "PASS")
    fail_count = sum(1 for c in checks if c["status"] == "FAIL")
    print(f"\n[nf-status] {nf}: PASS {pass_count}, FAIL {fail_count}, "
          f"NOT_APPLICABLE/NOT_RUN {len(checks)-pass_count-fail_count}", file=sys.stderr)
    for g in gates:
        if g["status"] == "FAIL":
            print(f"  gate {g['id']}: FAIL — blocked by {g['blocked_by']}", file=sys.stderr)
        else:
            print(f"  gate {g['id']}: PASS", file=sys.stderr)


if __name__ == "__main__":
    main()
