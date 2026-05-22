#!/usr/bin/env python3
# specs/ 스캔으로 design/nf-registry.yaml 의 generated 영역을 자동 생성·보강한다.
"""
Usage:
    .venv/bin/python3 design/scripts/nf-registry-bootstrap.py [--write] [--registry <path>] [--specs-root <path>]

옵션:
    --write          design/nf-registry.yaml 에 갱신 반영. 미지정 시 dry-run (stdout 만).
    --registry       registry 경로 override (테스트용). 기본 design/nf-registry.yaml.
    --specs-root     specs 루트 override (테스트용). 기본 specs/.

본 script 는 spec OpenAPI YAML 의 신호를 모아 NF 별 primary_spec 후보를
도출하고, confidence (high/medium/low) 를 매긴다.

읽기 영역.
  generated:        bootstrap 이 매번 새로 씀.
  manual_overrides: 사람만 편집. tool 은 *읽기만* 하며 절대 덮어쓰지 않는다.

confidence 정책 (PR E1, 2026-05-22 — plan §3.1 amendment).
  high:   동일 NF + 동일 primary spec 이 2 개 이상의 *독립 신호 타입* 에서 일치
          (filename token / OpenAPI info.title / paths prefix) + 다른 NF 와 충돌 없음.
  medium: 한 강한 신호만 일치하거나, 일부 파일이 누락/비표준이지만 NF token 은 유지.
  low:    NF token 불일치/다중 NF 후보/비표준 YAML/known-table 만으로 추정.

bootstrap 은 generated 영역만 갱신하며, manual_overrides 는 보존된다.
manual_overrides 가 같은 NF 를 가질 경우 *적용 우선순위는 wrapper* (PR E2) 책임.
본 script 는 *발견* 만 한다.
"""

from __future__ import annotations

import argparse
import datetime
import pathlib
import re
import sys

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent
DEFAULT_REGISTRY = REPO / "design" / "nf-registry.yaml"
DEFAULT_SPECS_ROOT = REPO / "specs"
SCHEMA_VERSION = "nf-registry-v1"

# Filename token: `TS<digits>_N<NF>_<...>.yaml` 의 `N<NF>` 부분.
_FILENAME_NF_RE = re.compile(r"^TS\d+_N([A-Za-z][\w]*)_", re.IGNORECASE)

# 본 list 는 *최후 fallback* 만 — 신호 0건일 때 known mapping. 비어있어도 동작.
KNOWN_SPEC_TO_NF: dict[str, str] = {
    # spec 번호 → canonical NF lowercase. 신호 부재 시에만 사용.
    # bootstrap 의 primary 책임은 YAML 신호이므로 의도적으로 비워둠.
}

# Title 신호 화이트리스트 — title-only candidate 가 noise 가 되는 것 차단.
# 3GPP TS 23.501 표 7.2-1 / 29.500 표 5.2.7.3-1 의 NF abbreviation. 새 NF 추가는
# 본 set 에 명시. filename / path 신호는 본 화이트리스트와 무관하게 통과 (3GPP
# 정규식이 충분히 unique).
KNOWN_NF_CODES = {
    "amf", "smf", "upf", "nrf", "nssf", "nef", "udm", "udr", "udsf",
    "ausf", "pcf", "smsf", "chf", "bsf", "nwdaf", "scp", "sepp",
    "n3iwf", "tngf", "twif", "wagf", "lmf", "gmlc", "ucmf", "mnpf",
    "tsctsf", "easdf", "mfaf", "dccf", "anlf",
}


def _norm_nf(token: str) -> str:
    """NF 토큰 lowercase 정규화. 3GPP 컨벤션 — `Nnssf_` 의 'N' 은 service-prefix
    이고 그 뒤 'nssf' 가 NF code 다. 정규식이 이미 prefix 'N' 을 소비했으므로
    여기서는 lowercase strip 만 한다 ('n' prefix 추가 제거 금지)."""
    return token.strip().lower()


def _signal_filename(name: str) -> str | None:
    m = _FILENAME_NF_RE.match(name)
    if m:
        return _norm_nf(m.group(1))
    return None


def _signal_title(info: dict) -> str | None:
    """OpenAPI info.title 에서 NF code 추출. 3GPP convention 2개 패턴.
       - "Nudm_SDM"           → service-prefix 'N' + NF code lowercase (`udm`)
       - "NSSF NS Selection"  → all-caps NF abbreviation (`nssf`)
       그 외는 첫 단어 lowercase (whitelist 로 noise 차단)."""
    title = (info or {}).get("title")
    if not isinstance(title, str):
        return None
    s = title.strip()
    m = re.match(r"^N([a-z]{2,6})[_\s]", s)
    if m:
        return m.group(1).lower()
    m = re.match(r"^([A-Z]{2,6})\b", s)
    if m:
        return m.group(1).lower()
    m = re.match(r"^([a-z][a-z]{1,5})\b", s.lower())
    if m:
        return m.group(1)
    return None


def _signal_paths(paths: dict) -> set[str]:
    """`/nnssf-...`, `/namf-...` 같은 path prefix 의 NF 토큰 집합."""
    out: set[str] = set()
    if not isinstance(paths, dict):
        return out
    for k in paths:
        if not isinstance(k, str):
            continue
        m = re.match(r"^/n([a-z]+)[-/]", k.lower())
        if m:
            out.add(_norm_nf(m.group(1)))
    return out


def _scan_yaml(p: pathlib.Path) -> tuple[set[str], dict]:
    """반환 (signal_types, observed_nfs).
    signal_types ⊆ {'filename','title','path'}.
    observed_nfs[signal_type] = nf or set[nf]."""
    signals: set[str] = set()
    nfs: dict[str, object] = {}
    nf_fn = _signal_filename(p.name)
    if nf_fn:
        signals.add("filename")
        nfs["filename"] = nf_fn
    try:
        doc = yaml.safe_load(p.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError:
        doc = {}
    if isinstance(doc, dict):
        nf_t = _signal_title(doc.get("info") or {})
        # title 신호는 화이트리스트만 인정 — 'common'/'data' 같은 일반어 차단.
        if nf_t and nf_t in KNOWN_NF_CODES:
            signals.add("title")
            nfs["title"] = nf_t
        nf_paths = _signal_paths(doc.get("paths") or {})
        if nf_paths:
            signals.add("path")
            nfs["path"] = nf_paths if len(nf_paths) > 1 else next(iter(nf_paths))
    return signals, nfs


def _dominant_nf(observed: dict, signals: set[str]) -> tuple[str | None, bool]:
    """signal 별 NF 값을 모아 가장 자주 등장한 NF + 충돌 여부 반환."""
    votes: dict[str, int] = {}
    multi = False
    for s in signals:
        v = observed.get(s)
        if isinstance(v, str):
            votes[v] = votes.get(v, 0) + 1
        elif isinstance(v, set):
            if len(v) > 1:
                multi = True
            for nf in v:
                votes[nf] = votes.get(nf, 0) + 1
    if not votes:
        return None, False
    sorted_votes = sorted(votes.items(), key=lambda kv: (-kv[1], kv[0]))
    top, top_count = sorted_votes[0]
    runner_up = sorted_votes[1][1] if len(sorted_votes) > 1 else 0
    conflict = multi or runner_up >= top_count
    return top, conflict


def _confidence(signal_types: set[str], conflict: bool,
                from_known_table: bool) -> str:
    if from_known_table and not signal_types:
        return "low"
    if conflict:
        return "low"
    n = len(signal_types)
    if n >= 2:
        return "high"
    if n == 1:
        return "medium"
    return "low"


def scan_specs(specs_root: pathlib.Path) -> dict[str, dict]:
    """반환 generated.nfs dict — nf_name → entry."""
    if not specs_root.is_dir():
        return {}
    # (nf, spec) → {signal_types, files, docx, conflict_log}
    pair_acc: dict[tuple[str, str], dict] = {}
    spec_dirs = sorted([p for p in specs_root.iterdir() if p.is_dir()
                        and not p.name.startswith("_")])
    for sd in spec_dirs:
        spec_id = sd.name
        yaml_files = sorted([p for p in sd.iterdir()
                             if p.suffix.lower() in (".yaml", ".yml")])
        docx_files = sorted([p.name for p in sd.iterdir()
                             if p.suffix.lower() == ".docx"])
        for yp in yaml_files:
            signal_types, observed = _scan_yaml(yp)
            nf, conflict = _dominant_nf(observed, signal_types)
            if nf is None:
                continue
            key = (nf, spec_id)
            entry = pair_acc.setdefault(key, {
                "signal_types": set(),
                "yaml_files": [],
                "conflict": False,
                "docx_files": docx_files,
            })
            entry["signal_types"] |= signal_types
            entry["yaml_files"].append(yp.name)
            entry["conflict"] = entry["conflict"] or conflict

        # known table fallback — spec 에 yaml 이 NF token 없는 경우만 적용
        if not any(k for k in pair_acc if k[1] == spec_id):
            tbl_nf = KNOWN_SPEC_TO_NF.get(spec_id)
            if tbl_nf:
                pair_acc[(tbl_nf, spec_id)] = {
                    "signal_types": set(),
                    "yaml_files": [],
                    "conflict": False,
                    "docx_files": docx_files,
                    "from_known_table": True,
                }

    # nf → list[(spec, confidence, entry)]
    by_nf: dict[str, list[tuple[str, str, dict]]] = {}
    for (nf, spec), e in pair_acc.items():
        conf = _confidence(e["signal_types"], e["conflict"],
                           e.get("from_known_table", False))
        by_nf.setdefault(nf, []).append((spec, conf, e))

    out: dict[str, dict] = {}
    for nf, cands in by_nf.items():
        # primary = 가장 강한 confidence + 가장 많은 signal type.
        rank = {"high": 0, "medium": 1, "low": 2}
        cands_sorted = sorted(cands,
                              key=lambda t: (rank.get(t[1], 9),
                                             -len(t[2]["signal_types"]),
                                             t[0]))
        prim_spec, prim_conf, prim_entry = cands_sorted[0]
        supporting = [t[0] for t in cands_sorted[1:]]
        rationale_bits = []
        if prim_entry["signal_types"]:
            rationale_bits.append(
                "signals=" + ",".join(sorted(prim_entry["signal_types"])))
        if prim_entry["conflict"]:
            rationale_bits.append("multi-NF candidates detected")
        if prim_entry.get("from_known_table"):
            rationale_bits.append("known-table fallback (no YAML signal)")
        out[nf] = {
            "primary_spec": prim_spec,
            "primary_spec_confidence": prim_conf,
            "discovery_source": "auto",
            "discovered_from": [f"specs/{prim_spec}/"],
            "supporting_specs": supporting,
            "expected_files": {
                "yaml": prim_entry["yaml_files"],
                "docx": prim_entry["docx_files"],
            },
            "rationale": "; ".join(rationale_bits) or "no signal",
        }
    return dict(sorted(out.items()))


def load_registry(path: pathlib.Path) -> dict:
    if not path.exists():
        return {
            "schema_version": SCHEMA_VERSION,
            "manual_overrides": {"nfs": {}},
        }
    try:
        d = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        raise SystemExit(f"[nf-registry-bootstrap] {path} YAML 파싱 실패 — {e}")
    if not isinstance(d, dict):
        raise SystemExit(
            f"[nf-registry-bootstrap] {path} top-level mapping 아님")
    if d.get("schema_version") not in (SCHEMA_VERSION, None):
        raise SystemExit(
            f"[nf-registry-bootstrap] {path} schema_version 불일치 — "
            f"기대 {SCHEMA_VERSION!r}, 실제 {d.get('schema_version')!r}")
    if "manual_overrides" not in d:
        d["manual_overrides"] = {"nfs": {}}
    return d


def render_yaml(generated_nfs: dict, manual_overrides: dict) -> str:
    doc = {
        "schema_version": SCHEMA_VERSION,
        "generated": {
            "generated_at": datetime.datetime.utcnow().strftime(
                "%Y-%m-%dT%H:%M:%SZ"),
            "generator": "design/scripts/nf-registry-bootstrap.py",
            "nfs": generated_nfs,
        },
        "manual_overrides": manual_overrides,
    }
    header = [
        "# Auto-managed by design/scripts/nf-registry-bootstrap.py",
        "# `generated.*` 는 bootstrap 이 매번 새로 씀 — 수동 편집 금지.",
        "# `manual_overrides.nfs` 만 사람이 편집 — bootstrap 은 절대 덮어쓰지 않는다.",
        "# schema_version = nf-registry-v1 (PR E1, 2026-05-22 plan §3.1).",
        "#",
        "# primary_spec_confidence — high / medium / low.",
        "#   high   = 2+ 독립 signal type (filename/title/path) 일치 + 충돌 없음",
        "#   medium = 1 signal type 일치",
        "#   low    = 다중 NF 후보 / known-table fallback / 신호 없음",
        "# /nf-readiness wrapper (PR E2) 가 confidence != high 인 경우 어떻게",
        "# 처리할지는 그 wrapper 의 책임 — 기본은 manual_overrides 확인 후 진행.",
    ]
    body = yaml.safe_dump(doc, allow_unicode=True, sort_keys=False,
                          default_flow_style=False)
    return "\n".join(header) + "\n\n" + body


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true",
                    help="design/nf-registry.yaml 에 갱신 반영")
    ap.add_argument("--registry", default=str(DEFAULT_REGISTRY))
    ap.add_argument("--specs-root", default=str(DEFAULT_SPECS_ROOT))
    args = ap.parse_args()

    reg_path = pathlib.Path(args.registry).resolve()
    specs_root = pathlib.Path(args.specs_root).resolve()

    registry = load_registry(reg_path)
    manual = registry.get("manual_overrides") or {"nfs": {}}
    if not isinstance(manual, dict):
        raise SystemExit("[nf-registry-bootstrap] manual_overrides 가 mapping 아님")
    if "nfs" not in manual or not isinstance(manual.get("nfs"), dict):
        manual["nfs"] = manual.get("nfs") or {}

    generated_nfs = scan_specs(specs_root)
    yaml_text = render_yaml(generated_nfs, manual)

    if args.write:
        reg_path.parent.mkdir(parents=True, exist_ok=True)
        reg_path.write_text(yaml_text, encoding="utf-8")
        print(f"[nf-registry-bootstrap] wrote {reg_path}", file=sys.stderr)
    else:
        print(yaml_text)

    # confidence 별 NF 수 보고
    conf_count: dict[str, int] = {}
    for nf, e in generated_nfs.items():
        conf_count[e["primary_spec_confidence"]] = conf_count.get(
            e["primary_spec_confidence"], 0) + 1
    summary = ", ".join(f"{c}={conf_count.get(c, 0)}"
                        for c in ("high", "medium", "low"))
    print(f"[nf-registry-bootstrap] discovered {len(generated_nfs)} NF: "
          f"{summary}", file=sys.stderr)
    if not args.write:
        print("[nf-registry-bootstrap] dry-run — --write 으로 갱신 반영",
              file=sys.stderr)


if __name__ == "__main__":
    main()
