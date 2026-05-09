#!/usr/bin/env python3
# NF 의 specs/ 의존성을 yaml $ref chain + docx clause 2 References 로 자동 검출해 _manifest.yaml 초안을 만든다
"""
Usage:
    .venv/bin/python3 scripts/nf-manifest.py <nf> --primary <spec> [--write]

옵션:
    --primary <spec>    NF 의 주 spec 번호 (예 29.531). 필수.
    --write             산출 매니페스트를 kb/<nf>/_manifest.yaml 로 저장. 미지정 시 stdout 만.
    --max-chars N       docx 추출 한도 (기본 80,000)

동작:
    1) specs/<primary-spec>/ 의 docx + yaml 들 식별.
    2) 모든 yaml 의 cross-spec $ref 수집.
    3) docx clause 2 References 섹션을 추출해 모든 3GPP TS/TR 인용 수집 (NBSP-tolerant).
    4) 카테고리 분류 (architecture / procedures / cross-nf / data_types / discovery / sba_common / security / ids / zzz_skip).
    5) specs/ 실재 여부 표시.
    6) status 보고 — manifest_completeness 비율, 부재 spec 우선순위 목록, /nf-build 가능 여부.

본 도구는 *제안* 만 한다. 사용자가 결과를 보고 kb/<nf>/_manifest.yaml 을 직접 수정/추가/제외할 수 있다.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent
EXTRACT = REPO / "scripts" / "extract.py"

# spec → (category, role) 매핑. 본 표는 NSSF 컨텍스트 위주이며 다른 NF 호출 시 확장될 수 있다.
CATEGORY_TABLE: dict[str, tuple[str, str]] = {
    # Stage 2 / TR — 일반적으로 out of scope
    "21.900": ("zzz_skip", "TSG methodology — out of scope"),
    "21.905": ("zzz_skip", "vocabulary — out of scope"),
    "24.501": ("zzz_skip", "NAS — SBI 구현 out of scope"),

    # Architecture / procedures
    "23.501": ("architecture", "stage 2 architecture (5GC 전체)"),
    "23.502": ("procedures", "stage 2 procedures (호출 컨텍스트)"),
    "23.288": ("cross-nf", "NWDAF analytics"),
    "23.003": ("ids", "NF instance ID·FQDN·numbering"),

    # SBA common
    "29.500": ("sba_common", "SBA HTTP/2 운반·공통 에러"),
    "29.501": ("sba_common", "SBI service 정의 원칙"),

    # NF SBI peers (cross-NF) — NF 별로 케이스 분류
    "29.503": ("data_types", "UDM SDM (chain target — RecurTime 등)"),
    "29.518": ("cross-nf", "AMF Communication"),
    "29.508": ("cross-nf", "AMF Event Exposure"),
    "29.509": ("cross-nf", "AUSF UE Authentication"),
    "29.502": ("cross-nf", "SMF PDU Session"),
    "29.504": ("cross-nf", "UDR Data Repository"),
    "29.522": ("cross-nf", "NEF Northbound"),
    "29.520": ("cross-nf", "NWDAF event"),

    # Discovery / common
    "29.510": ("discovery", "NRF discovery + NFType enum"),
    "29.571": ("data_types", "공통 자료형 chain target"),

    # Security
    "33.501": ("security", "5GC 보안 (TLS·OAuth·키 계층)"),

    # Other (NG-RAN, etc.)
    "38.413": ("cross-nf", "NG-RAN NGAP (cross-NF context)"),
}


def classify(spec: str) -> tuple[str, str]:
    if spec in CATEGORY_TABLE:
        return CATEGORY_TABLE[spec]
    head = spec.split(".")[0]
    # 헤드별 fallback
    if head == "21":
        return ("zzz_skip", "TR vocabulary/methodology — out of scope")
    if head == "23":
        return ("architecture", "stage 2 spec (분류 미정)")
    if head == "24":
        return ("zzz_skip", "NAS series — SBI 구현 out of scope")
    if head == "29":
        return ("cross-nf", "5GC NF SBI (분류 미정)")
    if head == "33":
        return ("security", "security series")
    if head == "38":
        return ("cross-nf", "NG-RAN series")
    return ("other", "분류 미정")


def find_primary_files(spec_dir: pathlib.Path) -> tuple[pathlib.Path | None, list[pathlib.Path]]:
    """spec 폴더에서 (대표 docx, yaml 목록) 반환. docx 가 여러 개면 letter+NM 큰 것."""
    docs = sorted(spec_dir.glob("*.docx")) + sorted(spec_dir.glob("*.pdf")) + sorted(spec_dir.glob("*.doc"))
    yamls = sorted(spec_dir.glob("*.yaml"))
    if not docs:
        return None, yamls

    def keyfn(p: pathlib.Path) -> tuple:
        m = re.search(r"-([a-z])(\d{2})\.(?:docx|pdf|doc)$", p.name)
        if not m:
            return (0, 0, p.name)
        letter_rank = ord(m.group(1)) - ord("a")
        nm = int(m.group(2))
        ext_pri = {".docx": 3, ".pdf": 2, ".doc": 1}.get(p.suffix, 0)
        return (letter_rank, nm, ext_pri)

    return max(docs, key=keyfn), yamls


def collect_yaml_refs(yamls: list[pathlib.Path]) -> set[str]:
    refs: set[str] = set()
    for y in yamls:
        text = y.read_text(encoding="utf-8")
        for m in re.finditer(r"\$ref: 'TS(\d{2})(\d{3})_", text):
            refs.add(f"{m.group(1)}.{m.group(2)}")
    return refs


def extract_docx_refs(docx: pathlib.Path, max_chars: int) -> list[str]:
    """clause 2 References 섹션을 추출해 인용된 모든 3GPP TS/TR 번호 반환."""
    out = subprocess.run(
        [".venv/bin/python3", str(EXTRACT), str(docx), "--max-chars", str(max_chars)],
        capture_output=True, text=True, timeout=180, cwd=REPO,
    )
    if out.returncode != 0:
        print(f"[nf-manifest] extract.py 실패: {out.stderr}", file=sys.stderr)
        return []
    full = out.stdout
    # 본 References 섹션 (TOC 가 아닌) — `^2\tReferences$` 다음에 `^3\tDefinitions` 까지
    m = re.search(r"(?ms)^2\tReferences$\n(.*?)(?=^3\t)", full)
    if not m:
        return []
    section = m.group(1)
    # 3GPP 의 abbreviation 사이 NBSP 가능 — \s+ 로 잡는다
    found = re.findall(r"3GPP\s+T[SR]\s+(\d{2}\.\d{3})", section)
    return sorted(set(found))


def load_existing_overrides(manifest_path: pathlib.Path) -> dict:
    """기존 _manifest.yaml 이 있으면 manual_overrides 블록만 보존해 돌려준다."""
    default = {"exclude": [], "add": []}
    if not manifest_path.is_file():
        return default
    try:
        data = yaml.safe_load(manifest_path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError:
        return default
    ov = data.get("manual_overrides") or {}
    return {
        "exclude": ov.get("exclude") or [],
        "add": ov.get("add") or [],
    }


def render_yaml(manifest: dict) -> str:
    """수동 의존성 없이 안정적인 YAML 문자열을 직접 생성."""
    out = []
    out.append(f"# Auto-generated by scripts/nf-manifest.py — 사용자가 수정/추가/제외 가능.")
    out.append(f"# 다시 생성하려면 본 도구를 재실행. 사용자 수정은 manual_overrides 절에 남겨라.")
    out.append("")
    out.append(f"nf: {manifest['nf']}")
    out.append(f"primary_spec: \"{manifest['primary_spec']}\"")
    out.append(f"primary_files:")
    out.append(f"  docx: {manifest['primary_files']['docx']}")
    out.append(f"  yamls:")
    for y in manifest["primary_files"]["yamls"]:
        out.append(f"    - {y}")
    out.append("")
    out.append(f"# 카테고리별 의존 spec — yaml chain + docx clause 2 References 자동 검출")
    out.append(f"# manual_overrides.exclude 의 spec 은 zzz_skip 과 동격으로 취급 (in-scope 카운트 제외).")
    out.append(f"# manual_overrides.add 로 추가된 spec 은 from: [manual] 로 표시.")
    out.append(f"deps:")
    by_cat: dict[str, list[dict]] = {}
    for d in manifest["deps"]:
        by_cat.setdefault(d["category"], []).append(d)
    cat_order = ["architecture", "procedures", "sba_common", "data_types",
                 "discovery", "security", "ids", "cross-nf", "other", "zzz_skip"]
    for cat in cat_order:
        if cat not in by_cat:
            continue
        out.append(f"  {cat}:")
        for d in sorted(by_cat[cat], key=lambda x: x["spec"]):
            line = f"    - {{ spec: \"{d['spec']}\", present: {str(d['present']).lower()}"
            if d.get("from"):
                line += f", from: [{', '.join(d['from'])}]"
            if d.get("excluded"):
                line += f", excluded: \"{d['excluded']}\""
            line += f", role: \"{d['role']}\" }}"
            out.append(line)
    out.append("")
    out.append(f"# /nf-init 산출 status")
    out.append(f"status:")
    out.append(f"  manifest_completeness: \"{manifest['status']['present_count']}/{manifest['status']['in_scope_count']}\"")
    out.append(f"  ready_for_build: {str(manifest['status']['ready']).lower()}")
    out.append(f"  missing_priority:")
    for n in manifest["status"]["missing_priority"]:
        out.append(f"    - \"{n}\"")
    out.append("")
    out.append(f"# 사용자가 수정한 항목은 여기에 — 자동 재생성이 덮어쓰지 않는다.")
    out.append(f"# exclude — 본 spec 을 in-scope 에서 제외 (ready_for_build 계산에서 빠짐).")
    out.append(f"# add     — 자동 검출되지 않은 spec 을 deps 에 추가.")
    out.append(f"manual_overrides:")
    overrides = manifest.get("overrides") or {"exclude": [], "add": []}
    excludes = overrides.get("exclude") or []
    adds = overrides.get("add") or []
    if excludes:
        out.append(f"  exclude:")
        for e in excludes:
            if isinstance(e, dict):
                spec = e.get("spec", "?")
                reason = e.get("reason", "")
                out.append(f"    - {{ spec: \"{spec}\", reason: \"{reason}\" }}")
    else:
        out.append(f"  exclude: []   # 예: [{{ spec: \"38.413\", reason: \"NG-RAN out of scope\" }}]")
    if adds:
        out.append(f"  add:")
        for a in adds:
            if isinstance(a, dict):
                spec = a.get("spec", "?")
                cat = a.get("category", "other")
                role = a.get("role", "")
                out.append(f"    - {{ spec: \"{spec}\", category: \"{cat}\", role: \"{role}\" }}")
    else:
        out.append(f"  add: []       # 자동 검출되지 않은 추가 spec")
    return "\n".join(out) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("nf", help="NF 폴더명 (소문자, 예 nssf)")
    parser.add_argument("--primary", required=True, help="주 spec 번호 (점 포함, 예 29.531)")
    parser.add_argument("--max-chars", type=int, default=80_000)
    parser.add_argument("--write", action="store_true", help="kb/<nf>/_manifest.yaml 로 저장")
    args = parser.parse_args()

    nf = args.nf.lower()
    primary = args.primary
    spec_dir = REPO / "specs" / primary
    if not spec_dir.is_dir():
        sys.exit(f"[nf-manifest] {spec_dir} 부재. 먼저 specs/{primary}/ 에 원본 cp.")

    docx, yamls = find_primary_files(spec_dir)
    if docx is None:
        sys.exit(f"[nf-manifest] {spec_dir} 안에 docx/pdf/doc 가 없습니다.")

    yaml_refs = collect_yaml_refs(yamls)
    docx_refs = set(extract_docx_refs(docx, args.max_chars))
    all_refs = (yaml_refs | docx_refs) - {primary}

    # 기존 manifest 의 manual_overrides 를 읽어 보존·적용
    out_dir = REPO / "kb" / nf
    manifest_path = out_dir / "_manifest.yaml"
    overrides = load_existing_overrides(manifest_path)
    exclude_map = {
        e.get("spec"): (e.get("reason") or "manual exclude")
        for e in overrides.get("exclude", [])
        if isinstance(e, dict) and e.get("spec")
    }

    papers_dirs = {p.name for p in (REPO/"specs").iterdir() if p.is_dir()}
    deps = []
    for spec in sorted(all_refs):
        cat, role = classify(spec)
        sources = []
        if spec in yaml_refs:
            sources.append("yaml_ref")
        if spec in docx_refs:
            sources.append("docx_clause_2")
        entry = {
            "spec": spec,
            "category": cat,
            "role": role,
            "present": spec in papers_dirs,
            "from": sources,
        }
        if spec in exclude_map:
            entry["excluded"] = exclude_map[spec]
        deps.append(entry)

    # manual_overrides.add — 자동 검출되지 않은 spec 추가
    existing_specs = {d["spec"] for d in deps}
    for a in overrides.get("add", []):
        if not isinstance(a, dict):
            continue
        spec = a.get("spec")
        if not spec or spec in existing_specs:
            continue
        deps.append({
            "spec": spec,
            "category": a.get("category", "other"),
            "role": a.get("role", "manual addition"),
            "present": spec in papers_dirs,
            "from": ["manual"],
        })

    # in-scope = zzz_skip 도 아니고 excluded 도 아닌 것
    in_scope = [d for d in deps if d["category"] != "zzz_skip" and "excluded" not in d]
    in_scope_present = [d for d in in_scope if d["present"]]
    in_scope_missing = [d for d in in_scope if not d["present"]]
    ready = len(in_scope_missing) == 0

    # 우선순위 — security/sba_common/data_types/ids 먼저, 그 다음 procedures/architecture/cross-nf
    priority_order = {"sba_common": 0, "security": 1, "data_types": 2, "ids": 3,
                      "procedures": 4, "architecture": 5, "discovery": 6, "cross-nf": 7, "other": 8}
    missing_sorted = sorted(in_scope_missing, key=lambda d: (priority_order.get(d["category"], 99), d["spec"]))
    missing_priority = [d["spec"] for d in missing_sorted]

    manifest = {
        "nf": nf,
        "primary_spec": primary,
        "primary_files": {
            "docx": docx.name,
            "yamls": [y.name for y in yamls],
        },
        "deps": deps,
        "status": {
            "present_count": len(in_scope_present),
            "in_scope_count": len(in_scope),
            "ready": ready,
            "missing_priority": missing_priority,
        },
        "overrides": overrides,
    }

    yaml_text = render_yaml(manifest)
    if args.write:
        out_dir = REPO / "kb" / nf
        out_dir.mkdir(parents=True, exist_ok=True)
        out_path = out_dir / "_manifest.yaml"
        out_path.write_text(yaml_text, encoding="utf-8")
        print(f"[nf-manifest] wrote {out_path.relative_to(REPO)}", file=sys.stderr)

    # stdout 보고
    print(yaml_text)
    print("=" * 60, file=sys.stderr)
    print(f"[nf-manifest] {nf} status: {len(in_scope_present)}/{len(in_scope)} in-scope refs present",
          file=sys.stderr)
    print(f"  ready_for_build: {ready}", file=sys.stderr)
    if not ready:
        print(f"  추가 필요 (priority 순): {missing_priority}", file=sys.stderr)


if __name__ == "__main__":
    main()
