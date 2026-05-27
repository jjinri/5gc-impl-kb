# validate-execution-control-drift.py smoke + drift detection test
"""validate-execution-control-drift.py 의 cross-source 일관성 검사 test.

본 validator 는 agent-execution-plan / team-execution-plan /
verification-matrix / codegen-work-items / pr-slicing-plan / readiness-
config 의 id reference 정합 검사. hyphen 이름이라 import 안 됨 →
subprocess + tmp_path fixture 로 인공 drift 주입 검증.
"""

from __future__ import annotations

import pathlib
import shutil
import subprocess
import sys

import pytest
import yaml


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "validate-execution-control-drift.py"


def _run(cwd: pathlib.Path, args: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        capture_output=True, text=True, cwd=cwd, timeout=30,
    )


def test_current_repo_passes() -> None:
    """현 repo 의 nssf source 들이 drift 없이 정합."""
    out = _run(REPO, ["nssf"])
    assert out.returncode == 0, (
        f"drift validator FAIL on current repo (exit {out.returncode}).\n"
        f"stderr:\n{out.stderr}"
    )
    assert "execution-control sources consistent" in out.stderr


def test_pass_section_labels_present() -> None:
    """PASS 출력에 5 section 모두 보고."""
    out = _run(REPO, ["nssf"])
    assert out.returncode == 0
    for section in [
        "wi_consistency",
        "phase_consistency",
        "check_id_consistency",
        "pr_slice_consistency",
        "phase_wi_aggregate",
    ]:
        assert f"PASS {section}" in out.stderr, f"section {section} 미보고"


def _copy_nssf_sources(dest: pathlib.Path) -> None:
    """nssf 의 5 source + readiness-config 를 dest 의 동일 경로로 복사."""
    # validator REPO root 가 script parent.parent.parent. 본 test 는
    # cwd=dest 가 아니라 dest 안에 동일 layout 만 만들면 안 됨 — validator 가
    # __file__ 로 REPO 를 잡으므로. 대신 tmp_path 안에 partial layout 만들고
    # validator 의 REPO 를 monkey-patch 하기는 무거움. 본 test 는 *원본
    # source 를 tmp 에 복사 + 1 곳 mutate 후 *원본 script* 를 tmp REPO 처럼
    # 만들기 어려움.
    raise NotImplementedError(
        "본 test 는 in-place mutation 대신 별도 fixture-based 시나리오 사용. "
        "현 implementation 은 main check (test_current_repo_passes) 가 메인 "
        "smoke 이고, drift 시나리오 시뮬레이션은 PoC 단계 미포함."
    )


def test_missing_wi_in_codegen_triggers_fail(tmp_path: pathlib.Path,
                                              monkeypatch: pytest.MonkeyPatch) -> None:
    """agent-execution-plan 이 codegen-work-items 에 없는 WI 를 참조하면 FAIL.

    *시나리오* — tmp_path 에 새 REPO layout 만들고 script 의 REPO constant 를
    monkey-patch. 본 test 는 *통합 검증* 보다 *세부 행동* 검증.
    """
    # tmp_path 안에 design + dev + engineering 구조 만들기.
    nf = "demo"
    (tmp_path / "design" / nf).mkdir(parents=True)
    (tmp_path / "dev" / nf).mkdir(parents=True)

    # minimal codegen-work-items — WI-A 하나만.
    codegen = {
        "schema_version": "codegen-work-items-v1",
        "items": [{"id": "WI-A", "owner_lane": "code", "depends_on": [],
                   "inputs": [], "expected_files": ["f"], "acceptance": ["a"],
                   "tests": ["t"], "verification_commands": ["c"]}],
    }
    (tmp_path / "dev" / nf / "codegen-work-items.yaml").write_text(
        yaml.safe_dump(codegen), encoding="utf-8"
    )
    # agent-execution-plan — WI-A 외에 WI-PHANTOM 참조 (drift 시나리오).
    agent = {
        "schema_version": "agent-execution-plan-v1",
        "phases": [
            {"id": "phase1_wave0", "work_items": ["WI-A", "WI-PHANTOM"]},
        ],
    }
    (tmp_path / "dev" / nf / "agent-execution-plan.yaml").write_text(
        yaml.safe_dump(agent), encoding="utf-8"
    )
    # verification-matrix — phase phase1_wave0 만 reference.
    matrix = {
        "schema_version": "verification-matrix-v1",
        "checks": [{"id": "check-1", "phase": "phase1_wave0", "work_item": "WI-A"}],
    }
    (tmp_path / "dev" / nf / "verification-matrix.yaml").write_text(
        yaml.safe_dump(matrix), encoding="utf-8"
    )
    # pr-slicing — 단일 PR.
    slicing = {
        "schema_version": "pr-slicing-plan-v1",
        "prs": [{"id": "PR-1", "phase": "phase1_wave0",
                 "work_items": ["WI-A"], "depends_on": [],
                 "required_checks": ["check-1"]}],
    }
    (tmp_path / "dev" / nf / "pr-slicing-plan.yaml").write_text(
        yaml.safe_dump(slicing), encoding="utf-8"
    )
    # readiness-config phase_policy.
    readiness = {
        "schema_version": "readiness-config-v1",
        "phase_policy": {"phases": {"phase1": {"work_items": ["WI-A"]}}},
    }
    (tmp_path / "design" / nf / "readiness-config.yaml").write_text(
        yaml.safe_dump(readiness), encoding="utf-8"
    )

    # validator 의 REPO 를 tmp_path 로 monkey-patch — subprocess 가 아니라
    # import 후 main() 호출.
    sys.path.insert(0, str(REPO / "design" / "scripts"))
    monkeypatch.setattr(sys, "argv", ["validate-execution-control-drift.py", nf])
    # validator 의 모듈명은 hyphen — importlib 사용.
    import importlib.util
    spec = importlib.util.spec_from_file_location("drift_validator", SCRIPT)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    monkeypatch.setattr(mod, "REPO", tmp_path)

    rc = mod.main()
    assert rc == 1, "WI-PHANTOM phantom WI 가 drift 로 검출되어야 함"


def test_extra_arg_uses_specified_nf(tmp_path: pathlib.Path) -> None:
    """첫 인자가 NF 이름으로 처리됨 — 없는 NF 면 missing source 보고 + exit 2."""
    out = _run(REPO, ["nonexistent_nf_xyz"])
    assert out.returncode == 2
    assert "missing source" in out.stderr
