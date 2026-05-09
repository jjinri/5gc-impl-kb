#!/usr/bin/env python3
# wiki/**/*.md 의 ```mermaid 블록을 SVG 로 렌더해 sibling _diagrams/ 폴더에 저장
"""
Usage:
    .venv/bin/python3 scripts/render-mermaid.py [--check] [--clean]

옵션:
    --check    렌더 없이 mermaid 블록 개수·예상 출력 경로만 보고 (idempotency 확인용)
    --clean    더 이상 매칭 블록이 없는 옛 _diagrams/<stem>-<n>.svg 까지 함께 삭제

산출물:
    wiki/{nf}/_diagrams/<stem>-<idx>.svg
    여기서 <stem> = wiki/{nf}/<stem>.md 의 베이스이름, <idx> = 페이지 안 mermaid 블록 1-based 순번.

의존성:
    `mmdc` (npm @mermaid-js/mermaid-cli) 가 PATH 또는 .npm-tools/node_modules/.bin/ 에 있어야 한다.
    부재 시 설치 안내 후 종료.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

# ```mermaid ... ``` 블록 정규식. 같은 줄에 ``` 만 오는 종료 펜스로 끝나야 한다.
MERMAID_RE = re.compile(
    r"^```mermaid[ \t]*\r?\n(.*?)^```[ \t]*\r?$",
    re.MULTILINE | re.DOTALL,
)


def find_mmdc(repo_root: pathlib.Path) -> str | None:
    sys_mmdc = shutil.which("mmdc")
    if sys_mmdc:
        return sys_mmdc
    local = repo_root / ".npm-tools" / "node_modules" / ".bin" / "mmdc"
    if local.is_file() and os.access(local, os.X_OK):
        return str(local)
    return None


def install_hint() -> str:
    return (
        "[render-mermaid] mmdc 미설치. 다음 중 하나로 설치하세요.\n"
        "  npm install --prefix .npm-tools @mermaid-js/mermaid-cli   # 프로젝트 로컬 (권장)\n"
        "  npm install -g @mermaid-js/mermaid-cli                     # 시스템 전역\n"
        "필요한 시스템 패키지 (puppeteer 가 chromium 을 다운로드·실행하기 위함):\n"
        "  sudo apt install ca-certificates fonts-liberation libasound2 libatk-bridge2.0-0 \\\n"
        "                   libatk1.0-0 libcups2 libdbus-1-3 libdrm2 libgbm1 libgtk-3-0 \\\n"
        "                   libnspr4 libnss3 libx11-xcb1 libxcomposite1 libxdamage1 libxfixes3 \\\n"
        "                   libxkbcommon0 libxrandr2 xdg-utils\n"
        "혹은 bash scripts/setup.sh 의 선택 항목에서 설치."
    )


def render_block(mmdc: str, block: str, out_svg: pathlib.Path) -> tuple[bool, str]:
    out_svg.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", suffix=".mmd", delete=False) as tmp:
        tmp.write(block)
        tmp_path = pathlib.Path(tmp.name)
    try:
        result = subprocess.run(
            [mmdc, "-i", str(tmp_path), "-o", str(out_svg), "-q"],
            capture_output=True,
            text=True,
            timeout=180,
        )
        if result.returncode != 0:
            return False, result.stderr.strip() or result.stdout.strip()
        return True, ""
    finally:
        tmp_path.unlink(missing_ok=True)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="wiki/**/*.md 의 mermaid 블록을 sibling _diagrams/*.svg 로 렌더"
    )
    parser.add_argument(
        "--check", action="store_true",
        help="렌더 없이 mermaid 블록 개수·예상 경로만 보고",
    )
    parser.add_argument(
        "--clean", action="store_true",
        help="원본 페이지의 mermaid 블록보다 많은 옛 _diagrams/<stem>-<n>.svg 를 삭제",
    )
    args = parser.parse_args()

    repo_root = pathlib.Path(__file__).resolve().parent.parent
    wiki_root = repo_root / "wiki"
    if not wiki_root.is_dir():
        print(f"[render-mermaid] wiki/ 폴더 없음: {wiki_root}", file=sys.stderr)
        sys.exit(1)

    mmdc = find_mmdc(repo_root)
    if mmdc is None and not args.check:
        print(install_hint(), file=sys.stderr)
        sys.exit(1)

    pages_with_blocks = 0
    rendered = 0
    failed = 0
    cleaned = 0

    for md in sorted(wiki_root.rglob("*.md")):
        text = md.read_text(encoding="utf-8")
        blocks = [m.group(1) for m in MERMAID_RE.finditer(text)]
        out_dir = md.parent / "_diagrams"
        if not blocks:
            # 이 페이지엔 더 이상 mermaid 블록이 없다. --clean 이면 옛 SVG 들 청소.
            if args.clean and out_dir.is_dir():
                for stale in out_dir.glob(f"{md.stem}-*.svg"):
                    stale.unlink()
                    cleaned += 1
                    print(f"  - removed {stale.relative_to(repo_root)}")
            continue
        pages_with_blocks += 1
        # --clean: 페이지 블록 수보다 인덱스가 큰 SVG 삭제
        if args.clean and out_dir.is_dir():
            for stale in out_dir.glob(f"{md.stem}-*.svg"):
                m = re.search(rf"^{re.escape(md.stem)}-(\d+)\.svg$", stale.name)
                if m and int(m.group(1)) > len(blocks):
                    stale.unlink()
                    cleaned += 1
                    print(f"  - removed {stale.relative_to(repo_root)}")
        for idx, block in enumerate(blocks, start=1):
            out_svg = out_dir / f"{md.stem}-{idx}.svg"
            rel = out_svg.relative_to(repo_root)
            if args.check:
                print(f"  would render: {md.relative_to(repo_root)} #{idx} → {rel}")
                continue
            ok, err = render_block(mmdc, block, out_svg)
            if ok:
                rendered += 1
                print(f"  ✓ {rel}")
            else:
                failed += 1
                print(f"  ✗ {rel}\n      {err}", file=sys.stderr)

    if args.check:
        total = sum(
            len(MERMAID_RE.findall(p.read_text(encoding="utf-8")))
            for p in wiki_root.rglob("*.md")
        )
        print(f"\n[render-mermaid] {pages_with_blocks} 페이지 · {total} 블록 (--check, 렌더 없음)")
    else:
        msg = f"\n[render-mermaid] {pages_with_blocks} 페이지 · 렌더 {rendered}건"
        if failed:
            msg += f" · 실패 {failed}건"
        if cleaned:
            msg += f" · 청소 {cleaned}건"
        print(msg)
        if failed:
            sys.exit(1)


if __name__ == "__main__":
    main()
