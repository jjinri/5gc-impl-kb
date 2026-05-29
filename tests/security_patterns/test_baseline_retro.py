"""Retro + positive tests for infra/security/baseline-patterns.yaml.

ADR-0005 D7 의 Layer A pattern yaml 이 false-positive 0 으로 머지된 main
HEAD 의 src/nssf/** / generated/nssf/** 에 대응해야 하고 (retro), 의도적
위반 fixture 에는 정확히 매칭되어야 한다 (positive). PR-B (#103 후속) 의
CI gate 의 일부.
"""

from __future__ import annotations

import fnmatch
import pathlib
import re
from collections.abc import Iterable

import pytest
import yaml

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
PATTERN_FILE = REPO_ROOT / "infra" / "security" / "baseline-patterns.yaml"
POSITIVE_FIXTURE = REPO_ROOT / "tests" / "security_patterns" / "fixtures" / "positive_violations.c"


@pytest.fixture(scope="module")
def patterns_doc() -> dict:
    with PATTERN_FILE.open() as f:
        return yaml.safe_load(f)


@pytest.fixture(scope="module")
def block_patterns(patterns_doc: dict) -> list[dict]:
    return [p for p in patterns_doc.get("patterns", []) if p.get("severity") == "block"]


@pytest.fixture(scope="module")
def all_patterns(patterns_doc: dict) -> list[dict]:
    return patterns_doc.get("patterns", [])


def _iter_repo_sources(
    applies_globs: Iterable[str], exclude_globs: Iterable[str]
) -> list[pathlib.Path]:
    found: list[pathlib.Path] = []
    for glob in applies_globs:
        for path in REPO_ROOT.glob(glob):
            if not path.is_file():
                continue
            rel = path.relative_to(REPO_ROOT).as_posix()
            if any(fnmatch.fnmatch(rel, ex) for ex in exclude_globs):
                continue
            found.append(path)
    # vendor / external 자동 제외 (deny pattern 의 false positive 표면 차단).
    return [p for p in found if "vendor" not in p.parts and "external" not in p.parts]


def test_pattern_yaml_schema(patterns_doc: dict) -> None:
    assert patterns_doc.get("schema_version") == "security-baseline-patterns-v1"
    assert patterns_doc.get("policy_id") is None  # 본 yaml 은 patterns table.
    for p in patterns_doc.get("patterns", []):
        assert "id" in p
        assert "adr_item" in p
        assert p.get("severity") in {"block", "advisory"}
        assert p.get("layer") in {"A", "B"}
        assert isinstance(p.get("deny", []), list)
        assert isinstance(p.get("allow", []), list)


def test_regex_compiles(all_patterns: list[dict]) -> None:
    for p in all_patterns:
        for r in p.get("deny", []) or []:
            re.compile(r)
        for r in p.get("allow", []) or []:
            re.compile(r)


def test_retro_no_false_positive_on_current_main(patterns_doc: dict, block_patterns: list[dict]) -> None:
    """현재 main HEAD 의 src/** + generated/** 에서 block deny 매칭이 0 이어야."""
    applies = patterns_doc.get("default_applies_to", [])
    excludes = patterns_doc.get("exclude_globs", [])
    files = _iter_repo_sources(applies, excludes)
    assert files, "no source files found — applies_to glob 또는 src 트리 점검."
    false_positives: list[tuple[str, str, str, int]] = []
    for p in block_patterns:
        deny_regexes = [re.compile(r) for r in p.get("deny", []) or []]
        allow_regexes = [re.compile(r) for r in p.get("allow", []) or []]
        for path in files:
            text = path.read_text(encoding="utf-8", errors="ignore")
            for lineno, line in enumerate(text.splitlines(), start=1):
                # allow 가 본 line 에 매칭되면 deny 통과 (allow 우선).
                if any(a.search(line) for a in allow_regexes):
                    continue
                for d in deny_regexes:
                    if d.search(line):
                        false_positives.append(
                            (p["id"], str(path.relative_to(REPO_ROOT)), line.strip(), lineno)
                        )
    assert not false_positives, (
        "block-severity deny patterns matched current main src — false positive set:\n"
        + "\n".join(f"  - {pid} @ {f}:{ln} → {snip}" for pid, f, snip, ln in false_positives)
    )


def test_positive_fixture_matches_each_block_pattern(block_patterns: list[dict]) -> None:
    """positive_violations.c 가 모든 block-severity pattern 을 1+ 매칭."""
    assert POSITIVE_FIXTURE.exists(), f"positive fixture not found: {POSITIVE_FIXTURE}"
    text = POSITIVE_FIXTURE.read_text(encoding="utf-8")
    unmatched: list[str] = []
    for p in block_patterns:
        deny_regexes = [re.compile(r) for r in p.get("deny", []) or []]
        if not deny_regexes:
            continue
        matched = any(d.search(text) for d in deny_regexes)
        if not matched:
            unmatched.append(p["id"])
    assert not unmatched, (
        "block pattern 들이 positive fixture 에서 매칭 실패 (sample 보강 필요): "
        + ", ".join(unmatched)
    )
