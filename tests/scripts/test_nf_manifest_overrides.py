"""Tests for nf-manifest.py policy-derived overrides (PR-16).

`load_policy_derived_excludes` + `merge_overrides_with_policy` 가 CI-safe
contract materialization 의 핵심 — 사용자 manual edit 없이 manifest
ready_for_build=true 진입 가능하게 만든다.

nf-manifest.py 는 hyphen 이 들어가서 module import 불가 → importlib.util
경유로 load.
"""

from __future__ import annotations

import importlib.util
import pathlib

import pytest


REPO = pathlib.Path(__file__).resolve().parent.parent.parent
NF_MANIFEST_PATH = REPO / "design" / "scripts" / "nf-manifest.py"


@pytest.fixture(scope="module")
def nf_manifest():
    spec = importlib.util.spec_from_file_location("nf_manifest", NF_MANIFEST_PATH)
    m = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(m)
    return m


def test_load_policy_derived_excludes_nssf(nf_manifest):
    """NSSF 의 policy-derived exclude 목록 확인.

    Expected sources:
    - design/policies/spec-dependencies.yaml absorbed: 33.501 / 33.310 / 33.210.
    - design/nssf/readiness-config.yaml excluded_operational: 38.413.
    Total = 4.
    """
    derived = nf_manifest.load_policy_derived_excludes("nssf")
    specs = {d["spec"] for d in derived}
    assert specs == {"33.501", "33.310", "33.210", "38.413"}

    # Source attribution
    by_spec = {d["spec"]: d for d in derived}
    assert by_spec["33.501"]["source"] == "policies/spec-dependencies.yaml"
    assert by_spec["38.413"]["source"] == "design/nssf/readiness-config.yaml"

    # Reasons non-empty
    for d in derived:
        assert d["reason"], f"empty reason for {d['spec']}"


def test_load_policy_derived_excludes_absent_nf(nf_manifest):
    """존재하지 않는 NF 도 policy (spec-dependencies) 만 derive — readiness-
    config 없음 → 3 (only absorbed)."""
    derived = nf_manifest.load_policy_derived_excludes("nonexistent_nf")
    specs = {d["spec"] for d in derived}
    # readiness-config 가 없어 38.413 빠짐
    assert specs == {"33.501", "33.310", "33.210"}


def test_merge_with_empty_manual(nf_manifest):
    """manual_overrides 비어있으면 policy-derived 가 그대로 exclude 에 들어감."""
    derived = [
        {"spec": "A.001", "reason": "abs", "source": "policies/x"},
        {"spec": "B.002", "reason": "ops", "source": "readiness-config"},
    ]
    merged = nf_manifest.merge_overrides_with_policy({"exclude": [], "add": []},
                                                     derived)
    assert {e["spec"] for e in merged["exclude"]} == {"A.001", "B.002"}
    assert merged["add"] == []


def test_merge_manual_wins(nf_manifest):
    """같은 spec 이 manual 에도 있으면 manual reason 우선 보존."""
    derived = [{"spec": "33.501", "reason": "policy reason", "source": "x"}]
    manual = {
        "exclude": [{"spec": "33.501", "reason": "user reason"}],
        "add": [],
    }
    merged = nf_manifest.merge_overrides_with_policy(manual, derived)
    by_spec = {e["spec"]: e for e in merged["exclude"]}
    assert by_spec["33.501"]["reason"] == "user reason"
    # No duplicate
    assert len(merged["exclude"]) == 1


def test_merge_preserves_add(nf_manifest):
    """manual_overrides.add 는 policy merge 에 영향 없이 보존."""
    manual = {
        "exclude": [],
        "add": [{"spec": "99.999", "category": "other", "role": "test"}],
    }
    merged = nf_manifest.merge_overrides_with_policy(manual, [])
    assert merged["add"] == manual["add"]


def test_merge_disjoint_specs(nf_manifest):
    """manual + derived 의 spec 이 disjoint 면 합집합."""
    manual = {"exclude": [{"spec": "X.000", "reason": "manual"}], "add": []}
    derived = [{"spec": "Y.001", "reason": "policy", "source": "x"}]
    merged = nf_manifest.merge_overrides_with_policy(manual, derived)
    assert {e["spec"] for e in merged["exclude"]} == {"X.000", "Y.001"}
