"""Tests for design/scripts/lib/md_table.py — PR-13a parser hardening.

Cover cases:
- cell(): escape, whitespace collapse, None/empty.
- split_row(): wrapper strip, escaped pipe unescape, multi-cell.
- parse_section(): header find, separator skip, data rows.
- cell() ↔ split_row() round-trip (escape symmetry).
"""

from __future__ import annotations

from design.scripts.lib.md_table import cell, split_row, parse_section


# ─── cell() ─────────────────────────────────────────────────────────

def test_cell_none_empty():
    assert cell(None) == ""
    assert cell("") == ""
    assert cell("   ") == ""


def test_cell_strip_whitespace():
    assert cell("  hello  ") == "hello"


def test_cell_collapse_whitespace():
    assert cell("a\nb") == "a b"
    assert cell("a  b\t c") == "a b c"
    assert cell("a\n\nb") == "a b"


def test_cell_escape_pipe():
    assert cell("a|b") == "a\\|b"
    assert cell("|x|y|") == "\\|x\\|y\\|"


def test_cell_combined():
    # pipe + whitespace
    assert cell("a\n|\tb") == "a \\| b"


# ─── split_row() ────────────────────────────────────────────────────

def test_split_row_simple():
    assert split_row("| a | b | c |") == ["a", "b", "c"]


def test_split_row_empty_cells():
    assert split_row("|  |  |") == ["", ""]


def test_split_row_no_wrappers():
    assert split_row("a | b | c") == ["a", "b", "c"]


def test_split_row_escaped_pipe_in_cell():
    # `\|` should unescape to `|` and stay in the same cell.
    assert split_row(r"| a | b\|c | d |") == ["a", "b|c", "d"]


def test_split_row_multiple_escapes():
    assert split_row(r"| a\|b\|c |") == ["a|b|c"]


def test_split_row_escaped_pipe_at_cell_boundary():
    # leading escaped pipe in a cell content
    assert split_row(r"| \|start | end |") == ["|start", "end"]


# ─── parse_section() ────────────────────────────────────────────────

SAMPLE_SECTION = """\
# Title

intro paragraph.

## Things

a paragraph above.

| id | val |
|---|---|
| x | y |
| z | w |

## Other

| skipped |
|---|
| a |
"""


def test_parse_section_first_table():
    rows = parse_section(SAMPLE_SECTION, "Things")
    assert rows == [["x", "y"], ["z", "w"]]


def test_parse_section_absent_header():
    assert parse_section(SAMPLE_SECTION, "Nope") == []


def test_parse_section_honors_escape():
    text = (
        "## Bag\n\n"
        "| id | desc |\n"
        "|---|---|\n"
        r"| a | one\|two |"
        "\n"
        "| b | three |\n"
    )
    rows = parse_section(text, "Bag")
    assert rows == [["a", "one|two"], ["b", "three"]]


# ─── round-trip (cell → row → split_row) ────────────────────────────

def test_round_trip_pipe_in_cell():
    original = "value with | inside"
    encoded = cell(original)
    line = f"| left | {encoded} | right |"
    decoded = split_row(line)
    assert decoded == ["left", "value with | inside", "right"]


def test_round_trip_newline_collapse():
    original = "multi\nline value"
    encoded = cell(original)
    # newline collapsed during encode — exact preservation not guaranteed
    line = f"| {encoded} |"
    decoded = split_row(line)
    assert decoded == ["multi line value"]


def test_round_trip_multi_special():
    original = "has | pipe\nand newline"
    encoded = cell(original)
    line = f"| a | {encoded} | b |"
    decoded = split_row(line)
    assert decoded == ["a", "has | pipe and newline", "b"]
