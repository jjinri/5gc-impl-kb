#!/usr/bin/env python3
# docs/lifecycle-artifacts.md §3 file index ↔ 실제 repo 산출 drift 검증
"""
Usage:
    .venv/bin/python3 design/scripts/validate-artifact-catalog.py

목적
- docs/lifecycle-artifacts.md §3 File index 에 등재된 *tracked* 파일/패턴
  이 실제 repo 에 매칭되는지 검사.
- catalog 가 stale 되거나 잘못된 path 가 들어가면 CI 가 차단.

검사 정책
- tracked = ✓ 인 행만 검사 (untracked 는 fresh-clone 비존재 정상).
- <nf> placeholder → design/ 하위 NF 폴더 (policies/scripts/schemas/lib
  같은 meta 폴더 제외) 로 확장.
- <Op>, <Schema>, <Module> placeholder → glob `*`.
- glob 매칭 0 건이면 FAIL.

산출
- exit 0 = PASS, exit 1 = FAIL.
- stderr 에 행 단위 PASS/FAIL 보고.

본 script 는 *read-only*. catalog 또는 repo 를 수정하지 않는다.
"""

from __future__ import annotations

import pathlib
import re
import sys


REPO = pathlib.Path(__file__).resolve().parent.parent.parent
CATALOG = REPO / "docs" / "lifecycle-artifacts.md"
DESIGN_DIR = REPO / "design"

# design/ 하위에서 meta 디렉터리 제외 — NF 폴더만 추출.
META_DIRS = {"policies", "scripts", "schemas", "lib"}


def _discover_nfs() -> list[str]:
    if not DESIGN_DIR.is_dir():
        return []
    return sorted(
        p.name for p in DESIGN_DIR.iterdir()
        if p.is_dir() and p.name not in META_DIRS and not p.name.startswith(".")
    )


def _load_catalog_rows() -> list[list[str]]:
    sys.path.insert(0, str(REPO / "design" / "scripts"))
    from lib.md_table import parse_section
    text = CATALOG.read_text(encoding="utf-8")
    rows = parse_section(text, "3. File index — alphabetical")
    if not rows:
        sys.exit(
            f"[validate-catalog] §3 file index table 부재 또는 빈 표 — "
            f"{CATALOG.relative_to(REPO)}"
        )
    return rows


def _resolve_path(pattern: str, nfs: list[str]) -> list[pathlib.Path]:
    """패턴에서 placeholder 를 실제 경로로 확장하고 glob 매칭 반환."""
    # backtick 제거 (catalog 의 inline code wrapper).
    p = pattern.strip("`").strip()
    # `dev/<nf>/<readiness pack md/yaml>` 같은 묶음 표기는 skip.
    if "<readiness" in p or "(Agent" in p or "(Human" in p:
        return []
    # placeholder substitution.
    # <Op>, <Schema>, <Module>, <spec> 등 일반 placeholder → glob `*`.
    # <nf> 는 별도로 알려진 NF 폴더로 확장 (cross-NF 다중 매칭 가능).
    if "<nf>" not in p:
        # <spec>, <Op> 등 일반 placeholder 만 → glob `*` 한 번에.
        expanded = re.sub(r"<[A-Za-z_]+>", "*", p)
        return list(REPO.glob(expanded))
    matches: list[pathlib.Path] = []
    for nf in nfs:
        expanded = p.replace("<nf>", nf)
        # 잔여 placeholder (<Op>, <Schema>, <Module>) → `*`.
        expanded = re.sub(r"<[A-Za-z_]+>", "*", expanded)
        matches.extend(REPO.glob(expanded))
    return matches


def main() -> None:
    if not CATALOG.is_file():
        sys.exit(f"[validate-catalog] {CATALOG.relative_to(REPO)} 없음")

    nfs = _discover_nfs()
    if not nfs:
        print(
            "[validate-catalog] 경고 — design/ 아래 NF 폴더 없음. "
            "tracked 패턴 검증을 skip.",
            file=sys.stderr,
        )

    rows = _load_catalog_rows()
    print(
        f"[validate-catalog] catalog rows={len(rows)} nfs={nfs}",
        file=sys.stderr,
    )

    fails: list[tuple[str, str]] = []
    skipped = 0
    passed = 0
    for row in rows:
        if len(row) < 4:
            continue
        path_pattern, _stage, _type, tracked = row[0], row[1], row[2], row[3]
        if tracked != "✓":
            skipped += 1
            continue
        # 실제 매칭 확인.
        matches = _resolve_path(path_pattern, nfs)
        if not matches:
            fails.append((path_pattern, "no glob match in repo"))
            print(
                f"[validate-catalog] FAIL  {path_pattern} — no glob match",
                file=sys.stderr,
            )
        else:
            passed += 1

    print(
        f"[validate-catalog] PASS {passed}  FAIL {len(fails)}  SKIP {skipped}",
        file=sys.stderr,
    )
    if fails:
        print(
            f"[validate-catalog] catalog drift detected — "
            f"{len(fails)} tracked path(s) without repo match. "
            f"docs/lifecycle-artifacts.md §3 가 stale 이거나 path 오기.",
            file=sys.stderr,
        )
        sys.exit(1)
    print("[validate-catalog] catalog ↔ repo consistent.", file=sys.stderr)


if __name__ == "__main__":
    main()
