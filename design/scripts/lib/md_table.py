"""Markdown table cell/row contract — shared between generator and validators.

본 모듈은 *escape-aware* 양방향 contract 다.

Contract
========

`cell(value)`  — encode-side (renderer).
- 입력: 아무 Python value.
- 출력: markdown table 의 *단일 cell* 로 안전한 string.
- 규칙:
  * stringify (`str()`).
  * leading/trailing whitespace strip.
  * 연속 whitespace (newline/tab/multi-space) → single space (한 cell 한 line 유지).
  * literal `|` → `\\|` (cell separator escape).
  * None/empty → `""`.

`split_row(line)` — decode-side (parser).
- 입력: markdown table row 한 line.
- 출력: 각 cell 의 string list. cell 내부 `\\|` 는 *unescape* (literal `|` 로 복원).
- 규칙:
  * unescaped `|` 만 separator.
  * 선행 backslash 가 있는 `\\|` 는 cell 내부 literal pipe.
  * 시작/끝의 wrapper `|` strip.
  * 각 cell stripped (양옆 whitespace 제거).

`parse_section(text, header)` — convenience.
- 입력: 전체 markdown text + `## {header}` H2 이름.
- 출력: 해당 section 안 첫 markdown table 의 data row 목록 (separator + header row 제외).
- 동작: section 추출 → `split_row` per data line.

Round-trip
==========

`cell(x)` → markdown row 작성 → 다른 process 가 `split_row` → 원본 `x` 의
*whitespace-collapsed* 형태를 복원 (cell 의 `|` escape/unescape 가 대칭이라
literal pipe 도 cycle 안 손실).

배경
====

PR-12 가 `_cell()` 을 generator 안에 추가했고, PR-13a (본 모듈) 가 그
contract 의 *parser 측* 을 escape-aware 로 강화한다 (Pane 2 review #61
post-merge caveat 반영). 이전 `nf-impl-status.py:parse_md_table_section`
의 naive `split('|')` 은 `\\|` 를 cell separator 로 잘못 인식해 cell index
shift 위험이 있었다.
"""

from __future__ import annotations


def cell(value) -> str:
    """Encode any value into a markdown table cell. See module docstring."""
    if value is None:
        return ""
    text = str(value).strip()
    if not text:
        return ""
    text = " ".join(text.split())
    return text.replace("|", "\\|")


def split_row(line: str) -> list[str]:
    """State-machine split of a markdown table row.

    `\\|` (backslash + pipe) → cell-internal literal pipe (unescaped).
    Unescaped `|` → cell separator.
    Wrapper `|` (start/end) → stripped.
    Each cell stripped of surrounding whitespace.
    """
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|"):
        s = s[:-1]
    cells: list[str] = []
    cur: list[str] = []
    i = 0
    n = len(s)
    while i < n:
        c = s[i]
        if c == "\\" and i + 1 < n and s[i + 1] == "|":
            cur.append("|")
            i += 2
            continue
        if c == "|":
            cells.append("".join(cur).strip())
            cur = []
            i += 1
            continue
        cur.append(c)
        i += 1
    cells.append("".join(cur).strip())
    return cells


def parse_section(text: str, header: str) -> list[list[str]]:
    """Return data rows of the first markdown table inside `## {header}` H2.

    Header row + separator row skipped. Returns `[]` when section absent or
    table absent. Per-cell parsing uses `split_row` — escape-aware.
    """
    needle = f"## {header}"
    start = text.find(needle)
    if start == -1:
        return []
    next_h = text.find("\n## ", start + len(needle))
    section = text[start:next_h] if next_h != -1 else text[start:]
    rows: list[list[str]] = []
    for ln in section.splitlines():
        s = ln.strip()
        if not s.startswith("|") or not s.endswith("|"):
            continue
        cells = split_row(s)
        # separator row (---/===)
        if all(set(c) <= set("-: ") for c in cells):
            continue
        rows.append(cells)
    return rows[1:] if rows else []
