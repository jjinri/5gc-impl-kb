from __future__ import annotations

import pathlib
import pytest

from design.scripts.lib.path_resolution import (
    resolve_topic_path,
    parse_topic_ref,
    PathResolution,
)


def test_directory_layout(tmp_path: pathlib.Path) -> None:
    nf = tmp_path / "design" / "nssf"
    (nf / "api").mkdir(parents=True)
    f = nf / "api" / "NSSelectionGet.md"
    f.write_text("# NSSelectionGet\n", encoding="utf-8")

    r = resolve_topic_path(
        nf_root=nf, topic_id="api/NSSelectionGet",
        category_layout={"api": "directory"},
    )
    assert r.exists is True
    assert r.path == f
    assert r.anchor is None


def test_single_file_layout(tmp_path: pathlib.Path) -> None:
    nf = tmp_path / "design" / "nssf"
    nf.mkdir(parents=True)
    f = nf / "interface.md"
    f.write_text("# Interface\n", encoding="utf-8")

    r = resolve_topic_path(
        nf_root=nf, topic_id="interface",
        category_layout={"interface": "single-file"},
    )
    assert r.exists is True
    assert r.path == f


def test_topic_anchor(tmp_path: pathlib.Path) -> None:
    nf = tmp_path / "design" / "nssf"
    nf.mkdir(parents=True)
    f = nf / "error-handling.md"
    f.write_text(
        "# Error Handling\n\n## NSSelection 400\n\n<a id=\"nsselection-400\"></a>\n",
        encoding="utf-8",
    )

    r = resolve_topic_path(
        nf_root=nf, topic_id="error-handling#nsselection-400",
        category_layout={"error-handling": "single-file"},
    )
    assert r.exists is True
    assert r.path == f
    assert r.anchor == "nsselection-400"
    assert r.anchor_found is True


def test_missing_file(tmp_path: pathlib.Path) -> None:
    nf = tmp_path / "design" / "nssf"
    nf.mkdir(parents=True)
    r = resolve_topic_path(
        nf_root=nf, topic_id="api/Ghost",
        category_layout={"api": "directory"},
    )
    assert r.exists is False
    assert r.path == nf / "api" / "Ghost.md"


def test_unknown_category_raises(tmp_path: pathlib.Path) -> None:
    nf = tmp_path / "design" / "nssf"
    nf.mkdir(parents=True)
    with pytest.raises(ValueError, match="unknown category"):
        resolve_topic_path(
            nf_root=nf, topic_id="ghost/Foo",
            category_layout={"api": "directory"},
        )


def test_parse_topic_ref() -> None:
    p = parse_topic_ref("api/NSSelectionGet")
    assert p.category == "api"
    assert p.topic == "NSSelectionGet"
    assert p.anchor is None

    p = parse_topic_ref("error-handling#cause")
    assert p.category == "error-handling"
    assert p.topic == "error-handling"
    assert p.anchor == "cause"

    p = parse_topic_ref("interface")
    assert p.category == "interface"
    assert p.topic == "interface"
