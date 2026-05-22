#!/usr/bin/env python3
# /nf-readiness <nf> wrapper 의 Phase 0 — registry resolve + bootstrap + confidence 평가.
"""
Usage:
    .venv/bin/python3 design/scripts/nf-readiness-resolve.py <nf> [--no-bootstrap-write]

본 helper 는 `/nf-readiness <nf>` wrapper (PR E2) 의 Phase 0 진입 검증을 수행한다.

기능.
  1. design/nf-registry.yaml 부재/stale 감지 + 필요 시 bootstrap 자동 실행.
     - registry 부재 / schema_version 불일치 / <nf> entry 부재 시 bootstrap dry-run
       → semantic-diff 가 있으면 `--write` 실행 (generated_at 단독 변경은 무시).
     - `--no-bootstrap-write` 면 dry-run 만, write 단계 skip.
  2. effective entry resolve — manual_overrides.nfs.<nf> 우선 적용 후 generated.nfs.<nf>.
  3. confidence policy 적용 (PR E1 plan §3.1).
     - high                     → proceed.
     - medium                   → manual_override 또는 기존 tracked generated entry
                                  와 primary_spec 일치 시 proceed, 아니면 blocker.
     - low                      → blocker. manual_override 가 medium/high 면 override.
  4. effective_source / effective_primary_spec / effective_confidence /
     decision (proceed/blocker) / blocker_reason 을 stdout 에 YAML 로 보고.

본 helper 는 lifecycle skill chain 을 실행하지 않는다. wrapper SKILL.md 가
script 결과를 읽고 step 0 verdict 로 사용한다.
"""

from __future__ import annotations

import argparse
import copy
import pathlib
import subprocess
import sys

import yaml

REPO = pathlib.Path(__file__).resolve().parent.parent.parent
REGISTRY_PATH = REPO / "design" / "nf-registry.yaml"
BOOTSTRAP_SCRIPT = REPO / "design" / "scripts" / "nf-registry-bootstrap.py"
SPECS_ROOT = REPO / "specs"
SCHEMA_VERSION = "nf-registry-v1"


def load_registry(path: pathlib.Path) -> dict | None:
    if not path.exists():
        return None
    try:
        return yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        raise SystemExit(f"[nf-readiness-resolve] {path} YAML 파싱 실패 — {e}")


def _generated_normalized(doc: dict) -> dict:
    """semantic diff 비교용 — generated_at 만 제외, 나머지는 동일성 기준."""
    g = copy.deepcopy(doc.get("generated") or {})
    g.pop("generated_at", None)
    return g


def run_bootstrap(write: bool) -> str:
    cmd = [str(REPO / ".venv" / "bin" / "python3"), str(BOOTSTRAP_SCRIPT)]
    if write:
        cmd.append("--write")
    r = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if r.returncode != 0:
        raise SystemExit(
            f"[nf-readiness-resolve] bootstrap 실패 (exit {r.returncode}) — "
            f"stderr={r.stderr.strip()}")
    return r.stdout


def maybe_bootstrap_write(current_doc: dict | None,
                          allow_write: bool) -> tuple[dict, str]:
    """필요 시 dry-run 으로 새 doc 생성 후 semantic-diff 있을 때만 --write.
    반환 (effective doc, action_note)."""
    out = run_bootstrap(write=False)
    try:
        new_doc = yaml.safe_load(out) or {}
    except yaml.YAMLError as e:
        raise SystemExit(f"[nf-readiness-resolve] bootstrap dry-run yaml 파싱 실패 — {e}")
    if not isinstance(new_doc, dict):
        raise SystemExit("[nf-readiness-resolve] bootstrap dry-run 결과 mapping 아님")
    if current_doc is None:
        if allow_write:
            run_bootstrap(write=True)
            return load_registry(REGISTRY_PATH) or new_doc, "bootstrap --write (registry 부재)"
        return new_doc, "bootstrap dry-run (--no-bootstrap-write)"
    cur_norm = _generated_normalized(current_doc)
    new_norm = _generated_normalized(new_doc)
    if cur_norm == new_norm:
        return current_doc, "registry up-to-date (no semantic diff)"
    if allow_write:
        run_bootstrap(write=True)
        return load_registry(REGISTRY_PATH) or new_doc, "bootstrap --write (semantic diff)"
    return new_doc, "bootstrap semantic diff detected — dry-run only (--no-bootstrap-write)"


def resolve_entry(doc: dict, nf: str) -> tuple[dict | None, str]:
    """effective entry + source 반환 ('manual'|'generated'|'')."""
    mo = ((doc.get("manual_overrides") or {}).get("nfs") or {}).get(nf)
    if isinstance(mo, dict):
        return mo, "manual"
    gen = ((doc.get("generated") or {}).get("nfs") or {}).get(nf)
    if isinstance(gen, dict):
        return gen, "generated"
    return None, ""


def decide(entry: dict | None, source: str, doc: dict, nf: str) -> tuple[str, str]:
    """반환 (decision, reason). decision ∈ {proceed, blocker}.

    정책 (PR E1 plan §3.1 + PR #45 review Medium fix).
      manual:
        high / medium → proceed
        low           → blocker
      generated:
        high          → proceed
        medium        → blocker (manual_overrides.nfs.<nf> 로 확정 필요)
        low           → blocker
      entry 부재    → blocker

    이전 버그 — generated medium 의 fallback 에서 entry == gen_entry 의
    primary_spec 자기 비교로 항상 proceed 했음 (PR #45 Medium finding).
    수정: generated medium 은 manual_override 가 resolve_entry 에서 우선
    적용된 경우 (source=='manual') 에만 proceed. source='generated' medium
    은 무조건 blocker — manual_overrides.nfs.<nf> 등록을 요구한다.
    """
    if entry is None:
        return "blocker", (f"effective entry 없음 — registry 의 generated.nfs.{nf} "
                           f"또는 manual_overrides.nfs.{nf} 가 필요")
    conf = entry.get("primary_spec_confidence")
    if conf not in ("high", "medium", "low"):
        return "blocker", f"primary_spec_confidence 잘못된 값 — {conf!r}"
    if source == "manual":
        if conf in ("high", "medium"):
            return "proceed", f"manual_override confidence={conf}"
        return "blocker", "manual_override confidence=low — high 또는 medium 으로 갱신 필요"
    # source == "generated"
    if conf == "high":
        return "proceed", "generated.nfs entry confidence=high"
    if conf == "medium":
        return "blocker", (f"primary_spec_confidence=medium — generated 단독으로는 "
                           f"insufficient. manual_overrides.nfs.{nf} 에 primary_spec/"
                           f"confidence(>=medium)/rationale 을 추가해 확정 필요")
    return "blocker", (f"primary_spec_confidence=low — manual_overrides.nfs.{nf} 로 "
                       f"primary_spec/confidence/rationale 등록 필요")


def check_specs_exist(entry: dict) -> tuple[bool, str | None]:
    spec = entry.get("primary_spec")
    if not spec:
        return False, "effective entry 의 primary_spec 비어있음"
    p = SPECS_ROOT / str(spec)
    if not p.is_dir():
        return False, f"effective primary_spec={spec!r} 인데 specs/{spec}/ 존재하지 않음"
    return True, None


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("nf")
    ap.add_argument("--no-bootstrap-write", action="store_true")
    args = ap.parse_args()
    nf = args.nf
    allow_write = not args.no_bootstrap_write

    doc = load_registry(REGISTRY_PATH)
    if doc is not None and doc.get("schema_version") != SCHEMA_VERSION:
        report = {
            "nf": nf,
            "decision": "blocker",
            "reason": (f"nf-registry schema_version != {SCHEMA_VERSION} "
                       f"(현 {doc.get('schema_version')!r}) — regenerate or migrate registry"),
            "bootstrap_action": "skip (schema mismatch)",
        }
        print(yaml.safe_dump(report, allow_unicode=True, sort_keys=False))
        sys.exit(1)

    needs_bootstrap = doc is None
    if doc is not None:
        entry, _ = resolve_entry(doc, nf)
        if entry is None:
            needs_bootstrap = True

    bootstrap_action = "skipped (entry already present)"
    if needs_bootstrap:
        doc, bootstrap_action = maybe_bootstrap_write(doc, allow_write)
    else:
        # 항상 dry-run 한 번 — semantic-diff 있으면 write (signals 가 바뀐 경우 등)
        doc, bootstrap_action = maybe_bootstrap_write(doc, allow_write)

    entry, source = resolve_entry(doc, nf)
    decision, reason = decide(entry, source, doc, nf)

    specs_ok, specs_msg = True, None
    if decision == "proceed" and entry is not None:
        specs_ok, specs_msg = check_specs_exist(entry)
        if not specs_ok:
            decision, reason = "blocker", specs_msg

    report = {
        "nf": nf,
        "decision": decision,
        "reason": reason,
        "bootstrap_action": bootstrap_action,
        "effective_source": source or None,
        "effective_primary_spec": (entry or {}).get("primary_spec"),
        "effective_confidence": (entry or {}).get("primary_spec_confidence"),
        "effective_supporting_specs": (entry or {}).get("supporting_specs") or [],
        "effective_rationale": (entry or {}).get("rationale"),
    }
    print(yaml.safe_dump(report, allow_unicode=True, sort_keys=False))
    sys.exit(0 if decision == "proceed" else 1)


if __name__ == "__main__":
    main()
