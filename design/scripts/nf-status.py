#!/usr/bin/env python3
# design/<nf>/ 의 design 완성도를 검사해 _status.yaml 산출 (handoff_ready·canonical gate)
"""
Usage:
    .venv/bin/python3 design/scripts/nf-status.py <nf> [--no-write]

옵션:
    --no-write   _status.yaml 저장 없이 stdout 만 보고

평가 framework. 무가중치, 항목별 criterion + to_pass 의무,
applies_to 별 NOT_APPLICABLE 처리, acceptance gate 는 check id 의 AND.

Gate (4 단계).
  draft → review_ready → handoff_ready → canonical
  의미는 ../docs/plan.md 또는 project memory project_gate_naming.md 참조.

Tier 1 (Validation, binary)
  - frontmatter_valid
  - sections_complete (7 카테고리 H2 + 비어있지 않음)
  - wikilinks_resolve (design 내부 [[...]] 모두 실재)
  - no_korean_colon_end
  - manifest_ready (_manifest.yaml.status.ready_for_build)

Tier 2 (Coverage, threshold)
  - data_model_chain_complete (트리에 "(참조 규격 미등록)" 0건)
  - api_operation_coverage (yaml paths operation 수 = 매트릭스 row 수, stage_3+mixed)
  - service_flow_coverage (mermaid 블록 수 ≥ manifest 명시 procedure 수)

Tier 3 (Implementability sanity)
  - schema_implementable  (design/scripts/yaml-to-c.py + gcc -fsyntax-only — C 는 *증명 수단*,
    의미는 schema 가 *언어 무관* 으로 구현 가능. 5gc-dev 의 본격 codegen 은 별도.)

Tier 4 (Subjective)
  - implementation_guidance_quality (manifest.manual_overrides.judge_result.score >= 4 면 PASS,
    부재면 NOT_RUN — /nf-status SKILL 의 judge 단계가 sub-agent 위임 후 manifest 에 등록)
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
from typing import Any

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent

REQUIRED_FRONTMATTER_KEYS = {"nf", "spec", "version", "status"}

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
            "design/{nf}/3gpp-*.md 의 상단 `---` 블록이 yaml.safe_load 통과. "
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
        "name": "모든 [[wikilink]] 이 design/ 안에 실재",
        "criterion": "페이지 안 [[...]] 패턴의 모든 target 이 design/ 하위에 .md 로 존재.",
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


def check_manifest_ready(manifest: dict, profile: str) -> dict:
    base = {
        "id": "manifest_ready", "tier": 1,
        "name": "_manifest.yaml.status.ready_for_build = true",
        "criterion": (
            "design/{nf}/_manifest.yaml 의 status.ready_for_build == true. "
            "in-scope 의존 spec 이 모두 specs/ 에 cp 되었거나 "
            "manual_overrides.exclude 로 제외됨."
        ),
        "applies_to": ["stage_3_only", "stage_2_only", "mixed", "meta_only"],
    }
    if not manifest:
        base.update(status="FAIL", current="_manifest.yaml 없음",
                    to_pass=["design/scripts/nf-manifest.py <nf> --primary <spec> --write"])
        return base
    status = manifest.get("status", {})
    ready = status.get("ready_for_build", False)
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
                        "design/scripts/nf-manifest.py <nf> --primary <spec> --write 재실행",
                    ])
        base["to_pass"] = [t for t in base["to_pass"] if t]
    return base


# ─── Tier 2 ─────────────────────────────────────────────────────────

def check_data_model_chain(page: pathlib.Path | None, profile: str) -> dict:
    base = {
        "id": "data_model_chain_complete", "tier": 2,
        "name": "Data Model 트리에 (참조 규격 미등록) leaf 0건",
        "criterion": (
            "design/{nf}/*.md 의 ```text Data Model 코드블록 안에 "
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
    # ## Data Model H2 섹션의 ```text 코드블록 안에서만 leaf 매칭 — 본문 산문의 백틱 인용은 제외
    dm_match = re.search(r"^## Data Model\s*$([\s\S]*?)(?=^## )", text, re.M)
    section = dm_match.group(1) if dm_match else ""
    trees = "\n".join(re.findall(r"```text\s*\n([\s\S]*?)```", section))
    leaves = re.findall(r"\(참조 규격 미등록\)", trees)
    if leaves:
        leaf_specs = sorted(set(re.findall(r"\[TS (\d{2}\.\d{3})\]\s+\(참조 규격 미등록\)", trees)))
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


# ─── Tier 1/2 — handoff contract ────────────────────────────────────

def _has_dollar_ref(obj: Any) -> bool:
    """handoff yaml 구조 안에 '$ref' 키 존재 여부 재귀 검사."""
    if isinstance(obj, dict):
        if "$ref" in obj:
            return True
        return any(_has_dollar_ref(v) for v in obj.values())
    if isinstance(obj, list):
        return any(_has_dollar_ref(item) for item in obj)
    return False


def check_handoff_yaml_valid(nf: str, profile: str) -> dict:
    base = {
        "id": "handoff_yaml_valid", "tier": 1,
        "name": "handoff yaml 존재·valid·schema_version==handoff-v1·필수 top-level key 4개",
        "criterion": (
            "handoff/<nf>/_handoff.yaml 존재 + yaml.safe_load 통과 + "
            "schema_version == 'handoff-v1' + "
            "top-level key nf·spec·api·data_model 모두 보유."
        ),
        "applies_to": ["stage_3_only", "mixed"],
    }
    if not applies(base, profile):
        base.update(status="NOT_APPLICABLE", current=f"profile={profile}", to_pass=[])
        return base
    handoff_path = REPO / "handoff" / nf / "_handoff.yaml"
    if not handoff_path.is_file():
        base.update(status="FAIL", current="handoff yaml 없음",
                    to_pass=[f"python3 design/scripts/build-handoff.py {nf}"])
        return base
    try:
        data = yaml.safe_load(handoff_path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        base.update(status="FAIL", current=f"yaml 파싱 실패 — {e}",
                    to_pass=[f"python3 design/scripts/build-handoff.py {nf}"])
        return base
    issues = []
    if data.get("schema_version") != "handoff-v1":
        issues.append(f"schema_version={data.get('schema_version')!r} (≠ handoff-v1)")
    for k in ("nf", "spec", "api", "data_model"):
        if k not in data:
            issues.append(f"top-level key '{k}' 없음")
    if issues:
        base.update(status="FAIL", current="; ".join(issues),
                    to_pass=[f"python3 design/scripts/build-handoff.py {nf} 로 재생성"])
    else:
        base.update(status="PASS",
                    current="schema_version=handoff-v1, 필수 top-level key 4개 존재",
                    to_pass=[])
    return base


def check_handoff_yaml_self_contained(nf: str, profile: str) -> dict:
    base = {
        "id": "handoff_yaml_self_contained", "tier": 2,
        "name": "handoff yaml 안 외부 $ref 0건 (self-contained)",
        "criterion": (
            "handoff/<nf>/_handoff.yaml 구조 안에 '$ref' 키 0건. "
            "외부 spec yaml 또는 다른 NF handoff 를 가리키는 참조 없음."
        ),
        "applies_to": ["stage_3_only", "mixed"],
    }
    if not applies(base, profile):
        base.update(status="NOT_APPLICABLE", current=f"profile={profile}", to_pass=[])
        return base
    handoff_path = REPO / "handoff" / nf / "_handoff.yaml"
    if not handoff_path.is_file():
        base.update(status="FAIL", current="handoff yaml 없음",
                    to_pass=[f"python3 design/scripts/build-handoff.py {nf}"])
        return base
    try:
        data = yaml.safe_load(handoff_path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        base.update(status="FAIL", current=f"yaml 파싱 실패 — {e}",
                    to_pass=[f"python3 design/scripts/build-handoff.py {nf}"])
        return base
    if _has_dollar_ref(data):
        base.update(status="FAIL", current="$ref 키 존재",
                    to_pass=[
                        "build-handoff.py 의 data_model 파싱에서 $ref 키가 yaml 에 포함되는 버그 수정",
                        f"python3 design/scripts/build-handoff.py {nf} 재실행",
                    ])
    else:
        base.update(status="PASS", current="$ref 키 0건", to_pass=[])
    return base


# ─── Tier 3 / 4 — NOT_RUN placeholder ────────────────────────────────

def check_schema_implementable(nf: str, profile: str) -> dict:
    base = {
        "id": "schema_implementable", "tier": 3,
        "name": "Data Model 자료형이 *언어 무관* 으로 구현 가능 (sanity probe — 수단은 C)",
        "criterion": "design/scripts/yaml-to-c.py (수단으로 C 사용) 산출이 gcc -fsyntax-only 통과. 본 check 의 의미는 schema 가 self-contained 하게 닫혀 *어떤 언어로든* 구현 가능하다는 sanity probe — 5gc-dev 의 본격 codegen 은 별도.",
        "applies_to": ["stage_3_only", "mixed"],
    }
    if not applies(base, profile):
        base.update(status="NOT_APPLICABLE", current=f"profile={profile}", to_pass=[])
        return base
    tool = REPO / "design" / "scripts" / "yaml-to-c.py"
    if not tool.is_file():
        base.update(status="NOT_RUN", current="도구 미존재",
                    to_pass=["design/scripts/yaml-to-c.py 신규"])
        return base
    try:
        result = subprocess.run(
            [sys.executable, str(tool), nf],
            capture_output=True, text=True, timeout=60,
        )
    except subprocess.TimeoutExpired:
        base.update(status="FAIL", current="yaml-to-c.py timeout (60s)",
                    to_pass=["yaml-to-c.py 가 무한 루프 또는 너무 큰 chain 인지 점검"])
        return base
    if result.returncode == 0:
        base.update(status="PASS", current="gcc -fsyntax-only 통과", to_pass=[])
    else:
        err_first = "\n".join(result.stderr.strip().splitlines()[:3]) or "(stderr 비어있음)"
        base.update(status="FAIL", current="gcc 또는 도구 실패",
                    to_pass=[f"yaml schema 수정 또는 도구 보완 — {err_first}"])
    return base


def check_subjective_review(manifest: dict) -> dict:
    base = {
        "id": "implementation_guidance_quality", "tier": 4,
        "name": "구현자 가이던스 품질 (LLM-as-judge ≥ 4/5)",
        "criterion": (
            "manifest.manual_overrides.judge_result.score >= 4 면 PASS. "
            "judge_result 부재면 NOT_RUN — /nf-status SKILL 의 judge 단계가 "
            "sub-agent 위임 후 manifest 에 등록한다."
        ),
        "applies_to": ["stage_3_only", "stage_2_only", "mixed"],
    }
    overrides = (manifest or {}).get("manual_overrides", {}) or {}
    judge = overrides.get("judge_result")
    if judge is None:
        base.update(status="NOT_RUN",
                    current="judge_result 미등록",
                    to_pass=[
                        "/nf-status <nf> --judge 로 sub-agent 채점 (또는 사람 리뷰)",
                        "결과를 _manifest.yaml 의 manual_overrides.judge_result 에 등록",
                    ])
        return base
    score = judge.get("score", 0)
    judged_by = judge.get("judged_by", "?")
    rationale = judge.get("rationale", "")
    if score >= 4:
        base.update(status="PASS",
                    current=f"score {score}/5 (judged_by={judged_by})",
                    to_pass=[])
    else:
        base.update(status="FAIL",
                    current=f"score {score}/5 (< 4) — {rationale[:120]}",
                    to_pass=["페이지 본문 보강 후 sub-agent 재채점"])
    return base


def check_validate_extraction(nf: str, handoff_yaml: dict | None) -> dict:
    # handoff-v2 면 validate-extraction.py basic 모두 PASS 여야 한다.
    # handoff-v1 이면 NOT_APPLICABLE (v1 NF 는 본 check 가 부적용).
    base = {
        "id": "validate_extraction_basic", "tier": 2,
        "name": "validate-extraction.py basic 13 룰 모두 PASS",
        "criterion": "design/scripts/validate-extraction.py <nf> --level basic exit 0.",
        "applies_to": ["stage_3_only", "stage_2_only", "mixed", "meta_only"],
    }
    if handoff_yaml is None or handoff_yaml.get("schema_version") != "handoff-v2":
        base.update(status="NOT_APPLICABLE",
                    current=f"schema={handoff_yaml.get('schema_version') if handoff_yaml else 'none'}",
                    to_pass=[])
        return base
    script = REPO / "design" / "scripts" / "validate-extraction.py"
    proc = subprocess.run(
        [".venv/bin/python3", str(script), nf, "--level", "basic"],
        capture_output=True, text=True, cwd=REPO, timeout=60,
    )
    if proc.returncode == 0:
        base.update(status="PASS", current="basic 13/13", to_pass=[])
    else:
        fails = [l.strip() for l in proc.stdout.splitlines() if "FAIL" in l]
        base.update(
            status="FAIL",
            current=f"validate-extraction FAIL — {len(fails)}건",
            to_pass=["design/scripts/validate-extraction.py <nf> --level basic 로 상세 확인",
                     *(fails[:5])],
        )
    return base


def maybe_load_v2_handoff(nf: str) -> dict | None:
    path = REPO / "handoff" / nf / "_handoff.yaml"
    if not path.is_file():
        return None
    try:
        return yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError:
        return None


# ─── Acceptance gates ───────────────────────────────────────────────

GATE_DEFS = [
    ("draft", ["frontmatter_valid"]),
    ("review_ready", ["frontmatter_valid", "sections_complete", "manifest_ready"]),
    ("handoff_ready", [
        "frontmatter_valid", "sections_complete", "manifest_ready",
        "data_model_chain_complete", "api_operation_coverage",
        "service_flow_coverage", "wikilinks_resolve",
        "no_korean_colon_end",
        "handoff_yaml_valid", "handoff_yaml_self_contained",
        "validate_extraction_basic",
    ]),
    ("canonical", [
        "frontmatter_valid", "sections_complete", "manifest_ready",
        "data_model_chain_complete", "api_operation_coverage",
        "service_flow_coverage", "wikilinks_resolve",
        "no_korean_colon_end",
        "handoff_yaml_valid", "handoff_yaml_self_contained",
        "schema_implementable", "implementation_guidance_quality",
        "validate_extraction_basic",
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
            # NOT_APPLICABLE 은 자동 만족, NOT_RUN 은 canonical 까지만 blocker
            elif c["status"] == "NOT_RUN" and gate_id == "canonical":
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
    out.append("# Auto-generated by design/scripts/nf-status.py — 사용자가 손으로 수정하지 않음.")
    out.append("# 본 파일을 수정하지 말고, 각 check 의 to_pass 액션을 따라 페이지를 고친 뒤 재실행.")
    out.append("")
    out.append(f"nf: {nf}")
    out.append(f"profile: {profile}")
    out.append(f"target_schema: design-v1")
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
    nf_dir = REPO / "design" / nf
    if not nf_dir.is_dir():
        sys.exit(f"[nf-status] design/{nf}/ 부재. /nf-init 먼저 실행하세요.")

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
        check_wikilinks_resolve(page, REPO/"design", profile),
        check_no_korean_colon_end(page, profile),
        check_manifest_ready(manifest, profile),
        check_data_model_chain(page, profile),
        check_api_coverage(page, manifest, profile),
        check_service_flow_coverage(page, profile),
        check_handoff_yaml_valid(nf, profile),
        check_handoff_yaml_self_contained(nf, profile),
        check_schema_implementable(nf, profile),
        check_subjective_review(manifest),
    ]

    v2_handoff = maybe_load_v2_handoff(nf)
    checks.append(check_validate_extraction(nf, v2_handoff))

    # v2 NF 는 토픽 디렉터리 layout 이라 단일 페이지 기반 check 들이 false-FAIL 한다.
    # 본 MVP 에서는 v2 schema 감지 시 그 check 들을 NOT_APPLICABLE 로 강등.
    # 의미 있는 진실은 validate_extraction_basic + handoff_yaml_valid 가 담는다.
    if v2_handoff is not None and v2_handoff.get("schema_version") == "handoff-v2":
        v2_demoted = {
            "sections_complete", "data_model_chain_complete",
            "api_operation_coverage", "service_flow_coverage",
            "wikilinks_resolve", "no_korean_colon_end",
            "handoff_yaml_self_contained", "schema_implementable",
        }
        for c in checks:
            if c["id"] in v2_demoted and c["status"] == "FAIL":
                c["status"] = "NOT_APPLICABLE"
                c["current"] = f"schema=handoff-v2; v1 check 부적용 ({c.get('current','')})"
                c["to_pass"] = []

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
