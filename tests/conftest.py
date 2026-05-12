# pytest fixtures for design/scripts/* 테스트
from __future__ import annotations

import pathlib
import sys
import pytest

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT))


@pytest.fixture
def repo_root() -> pathlib.Path:
    return REPO_ROOT


@pytest.fixture
def nssf_yaml() -> pathlib.Path:
    return REPO_ROOT / "specs" / "29.531" / "TS29531_Nnssf_NSSelection.yaml"


@pytest.fixture
def tmp_nf(tmp_path: pathlib.Path) -> pathlib.Path:
    nf = tmp_path / "design" / "demo"
    (nf / "api").mkdir(parents=True)
    (nf / "data-model").mkdir(parents=True)
    return nf
