#!/usr/bin/env python3
# 3GPP TS docx 를 § 단위 .md 들로 split 해 specs/<spec>/_extracted/ 에 캐시하는 도구
"""
Usage:
    .venv/bin/python3 design/scripts/spec-split.py <spec-or-docx> [--force] [--dry-run]

산출 — `specs/<spec>/_extracted/` 안의 split .md 들과 `_index.md` 인벤토리.
매 빌드마다 600KB docx 를 재추출 + grep 하는 비용을 제거한다.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
GENERATOR_VERSION = 1

# 본문 numeric heading. 줄 시작 점-구분 숫자 + (탭 또는 공백) + 제목.
# 줄 끝에 페이지 번호 (\t숫자) 가 붙으면 ToC 행이므로 별도 필터.
HEADING_RE = re.compile(r"^(?P<num>\d+(?:\.\d+)*)[ \t]+(?P<title>\S.*?)\s*$")
ANNEX_RE = re.compile(r"^Annex\s+(?P<letter>[A-Z])\s*(?:\((?P<kind>[^)]*)\))?:?\s*(?P<title>.*)$")
TOC_TAIL_RE = re.compile(r"\t\d+\s*$")  # ToC 행은 끝에 \t<페이지번호>


@dataclass
class Section:
    kind: str            # "front" | "body" | "annex"
    file_stem: str       # 출력 파일명 (확장자 제외)
    section_id: str      # "1", "6.1", "Annex A" 등
    title: str
    lines: list[str] = field(default_factory=list)


def slugify(text: str) -> str:
    """제목 → ascii slug. 영문/숫자/하이픈만 남기고 lower-case."""
    text = re.sub(r"[^A-Za-z0-9]+", "-", text).strip("-").lower()
    return text or "section"


def fail(msg: str, code: int = 1) -> None:
    print(f"[spec-split.py] {msg}", file=sys.stderr)
    sys.exit(code)


def info(msg: str) -> None:
    print(f"[spec-split.py] {msg}")


def resolve_docx(arg: str) -> tuple[str, Path]:
    """인자를 spec 번호 또는 docx 경로로 해석해 (spec_number, docx_path) 반환."""
    p = Path(arg)
    if p.suffix.lower() == ".docx" and p.exists():
        # specs/<spec>/<file>.docx 형태에서 spec 추출
        spec = p.parent.name
        return spec, p.resolve()
    # spec 번호로 본 — specs/<spec>/ 에서 가장 새 docx
    if re.fullmatch(r"\d+\.\d+", arg):
        spec_dir = REPO_ROOT / "specs" / arg
        if not spec_dir.is_dir():
            fail(f"spec 디렉터리 없음 — {spec_dir}")
        candidates = sorted(spec_dir.glob("*.docx"), key=lambda x: x.stat().st_mtime, reverse=True)
        if not candidates:
            fail(f"{spec_dir} 안에 .docx 없음")
        return arg, candidates[0].resolve()
    fail(f"인자 해석 실패 — '{arg}' (spec 번호 또는 .docx 경로 필요)")
    raise AssertionError("unreachable")


def extract_text(docx_path: Path) -> str:
    """extract.py 의 extract_docx 를 import 해 평문 획득. max_chars 는 사실상 무한."""
    sys.path.insert(0, str(REPO_ROOT / "design" / "scripts"))
    try:
        import extract  # type: ignore
    except ImportError as e:
        fail(f"extract.py import 실패 — {e}")
    return extract.extract_docx(docx_path, max_chars=10**9)


SCOPE_HEADING_RE = re.compile(r"^1[ \t]+Scope\s*$")


def find_body_start(lines: list[str]) -> int:
    """ToC + Foreword 를 건너뛰기 위해 정확히 '1\\tScope' 인 첫 줄 인덱스 반환.

    3GPP TS 본문 §1 제목은 관행상 "Scope". ToC 행은 끝에 페이지 번호가 붙어
    `1\\tScope\\t10` 형태이므로 SCOPE_HEADING_RE 가 매치하지 않는다.
    """
    for i, ln in enumerate(lines):
        if SCOPE_HEADING_RE.match(ln):
            return i
    return 0


def is_body_heading(line: str) -> tuple[str, str] | None:
    """본문 heading 이면 (num, title), 아니면 None."""
    if TOC_TAIL_RE.search(line):
        return None
    m = HEADING_RE.match(line)
    if not m:
        return None
    return m.group("num"), m.group("title")


def split_sections(lines: list[str], start: int) -> list[Section]:
    """평문 → Section 리스트. 분할 단위 — depth 1 (§N) + §6 의 직접 자식 (§6.x).

    docx 추출은 paragraphs 다음에 tables 가 따라오는 특성이 있어 Annex 진입 후
    본문 numeric heading 처럼 보이는 표 행이 다수 등장한다. 따라서 Annex 진입
    후엔 새 heading 분기를 봉쇄하고 현재 Annex 에 모두 누적한다.
    """
    sections: list[Section] = []
    front = Section(kind="front", file_stem="00-front", section_id="1-3", title="Scope, References, Definitions")
    sections.append(front)
    current = front
    in_annex = False  # Annex 진입 후엔 새 heading 분기 봉쇄
    # §4 이상 본문에 한 번이라도 진입하면 그 후 등장하는 §1·§2·§3 패턴은
    # numbered list (e.g. "1\tThe AMF shall send...") 이지 진짜 §1 재진입이 아니다.
    past_intro = False

    def open_body(num: str, title: str) -> Section | None:
        nonlocal past_intro
        parts = num.split(".")
        depth = len(parts)
        if parts[0] in {"1", "2", "3"}:
            if past_intro:
                # 본문 중간에 등장한 numbered list 항목 — heading 아님. 무시.
                return None
            return front
        # §4 이상 진입
        past_intro = True
        if depth == 1:
            # §6 은 자식 (§6.1, §6.2, ...) 으로 분할되므로 부모 본문 파일명에
            # `0` 슬롯을 둬 sort 시 자식들 앞에 오도록 한다.
            if parts[0] == "6":
                stem = f"06-0-{slugify(title)}"
            else:
                stem = f"{int(parts[0]):02d}-{slugify(title)}"
            return Section(kind="body", file_stem=stem, section_id=num, title=title)
        if depth == 2 and parts[0] == "6":
            stem = f"06-{int(parts[1])}-{slugify(title)}"
            return Section(kind="body", file_stem=stem, section_id=num, title=title)
        return current

    i = start
    n = len(lines)
    while i < n:
        line = lines[i]
        am = ANNEX_RE.match(line)
        if am:
            letter = am.group("letter")
            title = (am.group("title") or "").strip()
            title_line_idx = None
            if not title:
                j = i + 1
                while j < n and not lines[j].strip():
                    j += 1
                if j < n:
                    title = lines[j].strip()
                    title_line_idx = j
            kind = (am.group("kind") or "").strip()
            stem = f"annex-{letter.lower()}-{slugify(title)}"
            display_title = f"Annex {letter}"
            if kind:
                display_title += f" ({kind})"
            if title:
                display_title += f": {title}"
            new_sec = Section(kind="annex", file_stem=stem, section_id=f"Annex {letter}", title=display_title)
            sections.append(new_sec)
            current = new_sec
            in_annex = True
            new_sec.lines.append(line)
            if title_line_idx is not None:
                # 제목 줄을 섹션 lines 에 포함 (손실 없음). 빈 줄도 그대로 보존.
                for k in range(i + 1, title_line_idx + 1):
                    new_sec.lines.append(lines[k])
                i = title_line_idx + 1
            else:
                i += 1
            continue

        if not in_annex:
            bh = is_body_heading(line)
            if bh:
                num, title = bh
                target = open_body(num, title)
                if target is None:
                    # heading 으로 보이지만 실제론 numbered list — current 에 그대로 누적
                    current.lines.append(line)
                    i += 1
                    continue
                if target is not current:
                    if target.kind != "front" and target not in sections:
                        sections.append(target)
                    current = target
                current.lines.append(line)
                i += 1
                continue

        current.lines.append(line)
        i += 1

    return sections


def build_section_md(sec: Section, spec: str, docx_rel: str, mtime: float) -> str:
    body = "\n".join(sec.lines).rstrip() + "\n"
    chars = len(body)
    # frontmatter
    fm = [
        "---",
        f'source_spec: "{spec}"',
        f"source_path: {docx_rel}",
        f"source_mtime: {mtime}",
        f'section: "{sec.section_id}"',
        f"title: {yaml_escape(sec.title)}",
        "generator: spec-split.py",
        f"generator_version: {GENERATOR_VERSION}",
        f"chars: {chars}",
        "---",
        "",
        f"# {sec.title}",
        "",
        body,
    ]
    return "\n".join(fm)


def yaml_escape(s: str) -> str:
    """YAML scalar 에 안전하도록 따옴표 감싸기. 내부 " 는 \\" 로."""
    s2 = s.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{s2}"'


def write_outputs(spec: str, docx_path: Path, sections: list[Section], dry_run: bool) -> list[tuple[str, int]]:
    out_dir = REPO_ROOT / "specs" / spec / "_extracted"
    if not dry_run:
        out_dir.mkdir(parents=True, exist_ok=True)
        # 기존 .md 비우고 재생성 (전체 split 의 일관성을 위해)
        for old in out_dir.glob("*.md"):
            old.unlink()

    docx_rel = str(docx_path.resolve().relative_to(REPO_ROOT))
    mtime = docx_path.stat().st_mtime
    summary: list[tuple[str, int]] = []

    for sec in sections:
        if not sec.lines:
            continue
        # 너무 작은 본문 (heading 만 있고 사실상 빈 §) 은 skip — §6 처럼 직접 자식
        # 외 본문이 거의 없는 경우. front/annex 는 항상 보존.
        body_text = "\n".join(sec.lines).rstrip()
        chars = len(body_text) + 1
        md = build_section_md(sec, spec, docx_rel, mtime)
        summary.append((f"{sec.file_stem}.md", chars))
        if dry_run:
            continue
        (out_dir / f"{sec.file_stem}.md").write_text(md, encoding="utf-8")

    # _index.md 작성
    index_lines = [
        f"# `{spec}` _extracted index",
        "",
        f"- source — `{docx_rel}`",
        f"- source_mtime — `{mtime}`",
        f"- generator — `spec-split.py` v{GENERATOR_VERSION}",
        "",
        "| file | section | chars |",
        "| --- | --- | --- |",
    ]
    for sec in sections:
        if not sec.lines:
            continue
        chars = len("\n".join(sec.lines).rstrip()) + 1
        # Annex 의 title 은 이미 'Annex X (...): Subject' 자기설명. number § 만 prefix.
        label = sec.title if sec.kind == "annex" else f"{sec.section_id} {sec.title}"
        index_lines.append(f"| `{sec.file_stem}.md` | {escape_pipe(label)} | {chars} |")
    index_lines.append("")
    if not dry_run:
        (out_dir / "_index.md").write_text("\n".join(index_lines), encoding="utf-8")
    return summary


def escape_pipe(s: str) -> str:
    return s.replace("|", "\\|")


def cache_fresh(spec: str, docx_path: Path) -> bool:
    """_index.md 의 source_mtime 이 docx mtime 보다 최신이면 fresh."""
    index = REPO_ROOT / "specs" / spec / "_extracted" / "_index.md"
    if not index.exists():
        return False
    docx_mtime = docx_path.stat().st_mtime
    m = re.search(r"source_mtime\s+—\s+`([\d.]+)`", index.read_text(encoding="utf-8"))
    if not m:
        return False
    try:
        cached_mtime = float(m.group(1))
    except ValueError:
        return False
    return cached_mtime >= docx_mtime


def main() -> None:
    parser = argparse.ArgumentParser(description="3GPP TS docx 를 § 단위 .md 들로 split")
    parser.add_argument("target", help="spec 번호 (예 29.531) 또는 docx 경로")
    parser.add_argument("--force", action="store_true", help="cache fresh 여도 재생성")
    parser.add_argument("--dry-run", action="store_true", help="파일 쓰지 않고 결과만 출력")
    args = parser.parse_args()

    spec, docx_path = resolve_docx(args.target)

    if not args.force and not args.dry_run and cache_fresh(spec, docx_path):
        info(f"cache fresh — skip ({docx_path.name})")
        return

    info(f"extracting — {docx_path}")
    text = extract_text(docx_path)
    lines = text.splitlines()
    start = find_body_start(lines)
    info(f"본문 시작 줄 — {start} (총 {len(lines)} 줄)")

    sections = split_sections(lines, start)
    info(f"분할 결과 — {sum(1 for s in sections if s.lines)} 섹션")

    summary = write_outputs(spec, docx_path, sections, args.dry_run)

    out_dir_rel = f"specs/{spec}/_extracted"
    if args.dry_run:
        print(f"\n[dry-run] {out_dir_rel}/ 에 작성될 파일 — ")
    else:
        print(f"\n{out_dir_rel}/ 산출 — ")
    for name, chars in summary:
        print(f"  {name:50s} {chars:>8d} chars")


if __name__ == "__main__":
    main()
