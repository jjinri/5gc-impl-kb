from __future__ import annotations

import os
import pathlib
import subprocess

import yaml


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "nf-registry-bootstrap.py"
PY = REPO / ".venv" / "bin" / "python3"


def _run(args: list[str], cwd: pathlib.Path | None = None) -> subprocess.CompletedProcess:
    env = os.environ.copy()
    return subprocess.run(
        [str(PY), str(SCRIPT), *args],
        cwd=str(cwd or REPO),
        env=env,
        capture_output=True,
        text=True,
        check=False,
    )


def test_registry_schema_round_trip():
    """(a) registry YAML parse + schema_version + required key 존재."""
    reg = REPO / "design" / "nf-registry.yaml"
    assert reg.exists(), f"{reg} 없음 — bootstrap --write 먼저"
    doc = yaml.safe_load(reg.read_text(encoding="utf-8"))
    assert isinstance(doc, dict)
    assert doc.get("schema_version") == "nf-registry-v1"
    gen = doc.get("generated") or {}
    assert "nfs" in gen, "generated.nfs 누락"
    assert isinstance(gen["nfs"], dict)
    mo = doc.get("manual_overrides") or {}
    assert "nfs" in mo, "manual_overrides.nfs 누락"
    # 각 NF 행 필수 키
    for nf, entry in gen["nfs"].items():
        for k in (
            "primary_spec", "primary_spec_confidence", "discovery_source",
            "discovered_from", "supporting_specs", "expected_files", "rationale",
        ):
            assert k in entry, f"generated.nfs[{nf}] {k!r} 누락"
        assert entry["primary_spec_confidence"] in ("high", "medium", "low")


def test_bootstrap_rediscovers_nssf_high(tmp_path: pathlib.Path):
    """(b) bootstrap dry-run 이 현재 specs 에서 NSSF 를 high 로 재발견 +
    manual_overrides 보존 확인."""
    # 임시 registry 에 manual_overrides seed 후 bootstrap --write 실행
    tmp_reg = tmp_path / "nf-registry.yaml"
    tmp_reg.write_text(yaml.safe_dump({
        "schema_version": "nf-registry-v1",
        "manual_overrides": {"nfs": {
            "operator_test_nf": {
                "primary_spec": "99.999",
                "primary_spec_confidence": "high",
                "discovery_source": "manual",
                "discovered_from": ["specs/99.999/"],
                "supporting_specs": [],
                "expected_files": {"yaml": [], "docx": []},
                "rationale": "operator-only smoke fixture",
            },
        }},
    }, allow_unicode=True, sort_keys=False), encoding="utf-8")

    r = _run([
        "--write",
        "--registry", str(tmp_reg),
        "--specs-root", str(REPO / "specs"),
    ])
    assert r.returncode == 0, r.stderr
    doc = yaml.safe_load(tmp_reg.read_text(encoding="utf-8"))
    gen = doc["generated"]["nfs"]
    assert "nssf" in gen, gen
    nssf = gen["nssf"]
    assert nssf["primary_spec"] == "29.531"
    assert nssf["primary_spec_confidence"] == "high"
    assert "signals=" in nssf["rationale"]
    # manual_overrides 보존
    mo = doc["manual_overrides"]["nfs"]
    assert "operator_test_nf" in mo, mo
    assert mo["operator_test_nf"]["primary_spec"] == "99.999"


def test_bootstrap_low_confidence_on_ambiguous_fixture(tmp_path: pathlib.Path):
    """(c) negative — 신호 부족한 fixture 는 discovery 결과에 등장하지 않거나 low.
    실제 specs/ 는 변형하지 않고 임시 specs-root 사용."""
    tmp_specs = tmp_path / "specs"
    (tmp_specs / "88.888").mkdir(parents=True)
    # 신호 없는 YAML — 일반 단어 title, 비-3GPP 파일명, generic path.
    (tmp_specs / "88.888" / "GenericApi.yaml").write_text(
        yaml.safe_dump({
            "openapi": "3.0.0",
            "info": {"title": "Common Data Types", "version": "1.0.0"},
            "paths": {"/items/{id}": {"get": {"responses": {"200": {
                "description": "ok"}}}}},
        }, allow_unicode=True, sort_keys=False),
        encoding="utf-8",
    )
    tmp_reg = tmp_path / "nf-registry.yaml"
    r = _run([
        "--write",
        "--registry", str(tmp_reg),
        "--specs-root", str(tmp_specs),
    ])
    assert r.returncode == 0, r.stderr
    doc = yaml.safe_load(tmp_reg.read_text(encoding="utf-8"))
    gen = doc["generated"]["nfs"]
    # 신호 없음 → discovery 결과 비어야 함 (또는 모두 low).
    if gen:
        for nf, entry in gen.items():
            assert entry["primary_spec_confidence"] == "low", (
                f"기대 low, 실제 {entry['primary_spec_confidence']!r} "
                f"(nf={nf}, rationale={entry['rationale']!r})")
    else:
        # 빈 discovery 도 valid — title 화이트리스트 차단으로 신호 0 처리.
        assert True


def test_bootstrap_preserves_manual_overrides_on_rerun(tmp_path: pathlib.Path):
    """수동 영역은 bootstrap 의 반복 호출에 안정적."""
    tmp_reg = tmp_path / "nf-registry.yaml"
    initial = {
        "schema_version": "nf-registry-v1",
        "manual_overrides": {"nfs": {
            "operator_override": {
                "primary_spec": "00.000",
                "primary_spec_confidence": "medium",
                "discovery_source": "manual",
                "discovered_from": [],
                "supporting_specs": [],
                "expected_files": {"yaml": [], "docx": []},
                "rationale": "preserved across reruns",
            },
        }},
    }
    tmp_reg.write_text(yaml.safe_dump(initial, allow_unicode=True,
                                       sort_keys=False), encoding="utf-8")
    for _ in range(2):
        r = _run([
            "--write", "--registry", str(tmp_reg),
            "--specs-root", str(REPO / "specs"),
        ])
        assert r.returncode == 0, r.stderr
    doc = yaml.safe_load(tmp_reg.read_text(encoding="utf-8"))
    mo = doc["manual_overrides"]["nfs"]["operator_override"]
    assert mo["primary_spec"] == "00.000"
    assert mo["rationale"] == "preserved across reruns"
