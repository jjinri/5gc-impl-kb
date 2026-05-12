from __future__ import annotations

from design.scripts.lib.marker_parser import (
    extract_markers,
    duplicate_marker_ids,
    sync_diff,
    Marker,
)


SAMPLE = """---
id: api-nsselection-get
generated_sections:
  - api-matrix
  - errors-table
user_sections:
  - implementation-notes
---

# Body

<!-- AUTO:api-matrix:start -->
| col | col |
| --- | --- |
| a | b |
<!-- AUTO:api-matrix:end -->

<!-- AUTO:errors-table:start -->
none yet
<!-- AUTO:errors-table:end -->

<!-- USER:implementation-notes:start -->
사람이 쓴 산문.
<!-- USER:implementation-notes:end -->
"""


def test_extract_markers_finds_three() -> None:
    ms = extract_markers(SAMPLE)
    kinds = [(m.kind, m.id) for m in ms]
    assert kinds == [
        ("AUTO", "api-matrix"),
        ("AUTO", "errors-table"),
        ("USER", "implementation-notes"),
    ]


def test_no_duplicates() -> None:
    assert duplicate_marker_ids(SAMPLE) == []


def test_duplicate_detection() -> None:
    text = SAMPLE + "\n<!-- AUTO:api-matrix:start -->\nx\n<!-- AUTO:api-matrix:end -->\n"
    assert duplicate_marker_ids(text) == [("AUTO", "api-matrix")]


def test_sync_diff_clean() -> None:
    fm = {
        "generated_sections": ["api-matrix", "errors-table"],
        "user_sections": ["implementation-notes"],
    }
    diff = sync_diff(fm=fm, markers=extract_markers(SAMPLE))
    assert diff.frontmatter_only_auto == []
    assert diff.body_only_auto == []
    assert diff.frontmatter_only_user == []
    assert diff.body_only_user == []


def test_sync_diff_mismatch() -> None:
    fm = {
        "generated_sections": ["api-matrix"],   # missing errors-table
        "user_sections": ["implementation-notes", "ghost"],  # extra ghost
    }
    diff = sync_diff(fm=fm, markers=extract_markers(SAMPLE))
    assert diff.body_only_auto == ["errors-table"]
    assert diff.frontmatter_only_user == ["ghost"]


def test_unmatched_start_only() -> None:
    text = "<!-- AUTO:foo:start -->\nbody\n"
    ms = extract_markers(text)
    assert ms == []  # unmatched pair → 무시 (별도 룰에서 처리)
