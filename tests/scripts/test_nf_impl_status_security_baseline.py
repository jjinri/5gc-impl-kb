# nf-impl-status.py 의 security-baseline loader 가 missing/parse-error/non-list 케이스를 FAIL 로 보고하는지 검증 (PR-17b loader hardening)
"""Tests for `_load_security_baseline_mandate_count` in nf-impl-status.py.

PR-17b validator enforce 단계의 `verification_plan_auto_complete` check 는
security-baseline.yaml `baseline_mandates` 수와 verification-plan.md 의
`security-gate-matrix` row 수가 일치하는지 검증한다. Loader 가 `None` 만
반환할 때 caller 가 silent skip 해서 false positive PASS 가 발생했던
codex review (PR #70) finding 을 막기 위한 negative case.

nf-impl-status.py 는 hyphen 이 들어가서 module import 불가 → importlib.util
경유로 load (test_nf_manifest_overrides.py 와 동일 패턴).
"""

from __future__ import annotations

import importlib.util
import pathlib

import pytest


REPO = pathlib.Path(__file__).resolve().parent.parent.parent
NF_IMPL_STATUS_PATH = REPO / "design" / "scripts" / "nf-impl-status.py"


@pytest.fixture(scope="module")
def nf_impl_status():
    spec = importlib.util.spec_from_file_location(
        "nf_impl_status", NF_IMPL_STATUS_PATH,
    )
    m = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(m)
    return m


# ─── Negative cases — loader 가 (None, error) 반환해야 함 ─────────────

def test_loader_missing_file(nf_impl_status, tmp_path):
    p = tmp_path / "absent.yaml"
    count, err = nf_impl_status._load_security_baseline_mandate_count(p)
    assert count is None
    assert err is not None and "파일 부재" in err


def test_loader_yaml_parse_error(nf_impl_status, tmp_path):
    p = tmp_path / "broken.yaml"
    # tab + : 조합으로 yaml.YAMLError 유발
    p.write_text("foo:\n\t- bar: : :\n", encoding="utf-8")
    count, err = nf_impl_status._load_security_baseline_mandate_count(p)
    assert count is None
    assert err is not None and "YAML parse" in err


def test_loader_top_level_not_mapping(nf_impl_status, tmp_path):
    p = tmp_path / "list-root.yaml"
    p.write_text("- a\n- b\n", encoding="utf-8")
    count, err = nf_impl_status._load_security_baseline_mandate_count(p)
    assert count is None
    assert err is not None and "mapping" in err


def test_loader_missing_baseline_mandates_key(nf_impl_status, tmp_path):
    p = tmp_path / "no-key.yaml"
    p.write_text("schema_version: policies-v1\n", encoding="utf-8")
    count, err = nf_impl_status._load_security_baseline_mandate_count(p)
    assert count is None
    assert err is not None and "키 부재" in err


def test_loader_baseline_mandates_not_list(nf_impl_status, tmp_path):
    p = tmp_path / "non-list.yaml"
    p.write_text("baseline_mandates:\n  m1: {}\n", encoding="utf-8")
    count, err = nf_impl_status._load_security_baseline_mandate_count(p)
    assert count is None
    assert err is not None and "list" in err


# ─── Positive case — 정상 baseline 은 (count, None) ─────────────────

def test_loader_valid_mandates(nf_impl_status, tmp_path):
    p = tmp_path / "ok.yaml"
    p.write_text(
        "baseline_mandates:\n"
        "  - id: M1\n    name: a\n"
        "  - id: M2\n    name: b\n"
        "  - id: M3\n    name: c\n",
        encoding="utf-8",
    )
    count, err = nf_impl_status._load_security_baseline_mandate_count(p)
    assert err is None
    assert count == 3


def test_loader_default_path_resolves(nf_impl_status):
    # path 인자 없이 호출하면 REPO/design/policies/security-baseline.yaml 을 본다.
    count, err = nf_impl_status._load_security_baseline_mandate_count()
    # 본 repo 상태에서는 file 존재 + parse 가능 → count 는 int, err 는 None.
    assert err is None
    assert isinstance(count, int) and count > 0
