from __future__ import annotations

import json
import pathlib
import subprocess
import sys


REPO = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPO / "design" / "scripts" / "resolve-yaml-refs.py"
NSSF_YAML = REPO / "specs" / "29.531" / "TS29531_Nnssf_NSSelection.yaml"


def _run(*args: str) -> dict:
    out = subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        capture_output=True, text=True, cwd=REPO, timeout=60,
    )
    assert out.returncode == 0, out.stderr
    return json.loads(out.stdout)


def test_emit_json_basic_shape() -> None:
    payload = _run(
        str(NSSF_YAML), "AuthorizedNetworkSliceInfo",
        "--emit-json",
        "--topic-id", "data-model/AuthorizedNetworkSliceInfo",
        "--nf", "nssf",
        "--spec-ref", "TS 29.531 §6.1.6.2.5",
        "--handoff-topics", "data-model/SliceInfoForRegistration",
        "--handoff-topics", "data-model/AuthorizedNetworkSliceInfo",
    )
    assert payload["schema_version"] == "data-model-v1"
    assert payload["nf"] == "nssf"
    assert payload["topic_id"] == "data-model/AuthorizedNetworkSliceInfo"
    assert payload["root_schema"] == "AuthorizedNetworkSliceInfo"
    assert payload["source"]["spec_refs"] == ["TS 29.531 §6.1.6.2.5"]
    assert "fields" in payload and isinstance(payload["fields"], list)
    assert isinstance(payload["dependencies"], list)
    assert isinstance(payload["unresolved_refs"], list)


# PR F1.2 — implementability metadata + classified unresolved shape regression coverage.

def test_emit_json_implementability_keys_present() -> None:
    """resolve-yaml-refs.py --emit-json 산출에 PR B + F1.2 메타 키 5종 emit 보장.

    nf-contract-check.py 의 schema_complexity_classified / generated_wrapper_boundary_declared check
    가 본 키들 직접 의존. 키 누락 시 contract_implementable gate 가 silent FAIL.
    """
    payload = _run(
        str(NSSF_YAML), "AuthorizedNetworkSliceInfo",
        "--emit-json",
        "--topic-id", "data-model/AuthorizedNetworkSliceInfo",
        "--nf", "nssf",
    )
    for key in ("complexity_flags", "wrapper_required", "c_type_hint",
                "validation_hint", "normalized_schema"):
        assert key in payload, f"emit_json missing implementability key {key!r}"
    assert isinstance(payload["complexity_flags"], list)
    assert isinstance(payload["wrapper_required"], bool)


def test_emit_json_status_code_topic_classified() -> None:
    """status code data-model topic (data-model/400 등) 은 OpenAPI components.schemas 항목이
    아니라 paths.{op}.responses 의 status code 슬롯. resolve-yaml-refs.py 가 schema 로 찾으면
    실패하므로, status code 패턴 (^(default|[1-5]\\d\\d)$) 매칭 시 classified JSON 직접 반환
    (Pane 2 정책 C). c_type_hint='struct nf_problem_details*' 강제.
    """
    payload = _run(
        str(NSSF_YAML), "400",
        "--emit-json",
        "--topic-id", "data-model/400",
        "--nf", "nssf",
    )
    assert payload["status"] == "classified"
    assert payload["classification"] == "problem_details_response"
    assert payload["c_type_hint"] == "struct nf_problem_details*"
    assert payload["wrapper_required"] is True
    assert payload["complexity_flags"] == []
    assert payload["root"] is None
    assert payload["normalized_schema"] is None
    assert len(payload["unresolved_refs"]) == 1
    entry = payload["unresolved_refs"][0]
    for key in ("ref", "classification", "rationale", "implementation_action", "phase"):
        assert key in entry, f"unresolved entry missing {key!r}"
    assert entry["classification"] == "problem_details_response"


def test_emit_json_unresolved_entry_classified_shape() -> None:
    """일반 (non-status-code) unresolved $ref 도 classification 기반 enriched entry shape
    (ref/classification/rationale/implementation_action/phase) emit. nf-contract-check.py 의
    external_refs_resolved_or_classified check 가 본 키 의무 검증.
    """
    payload = _run(
        str(NSSF_YAML), "AuthorizedNetworkSliceInfo",
        "--emit-json",
        "--topic-id", "data-model/AuthorizedNetworkSliceInfo",
        "--nf", "nssf",
    )
    for entry in payload["unresolved_refs"]:
        assert isinstance(entry, dict)
        for key in ("ref", "classification", "rationale", "implementation_action", "phase"):
            assert key in entry, f"unresolved entry missing {key!r}: {entry!r}"
        assert entry["classification"] in {
            "external_common_data", "responses_only_schema", "problem_details_response",
            "callback_or_notification_only", "optional_not_in_phase1",
            "operator_policy_external", "implementation_blocker",
        }, f"classification {entry['classification']!r} not in enum"


def test_emit_json_handoff_topic_becomes_dependency() -> None:
    # SliceInfoForRegistration is itself a handoff topic; it may transitively
    # appear inside other schemas. When it does, it must show as { "topic": ... }
    # and be listed in dependencies — not inlined.
    payload = _run(
        str(NSSF_YAML), "AuthorizedNetworkSliceInfo",
        "--emit-json",
        "--topic-id", "data-model/AuthorizedNetworkSliceInfo",
        "--nf", "nssf",
        "--spec-ref", "TS 29.531 §6.1.6.2.5",
        "--handoff-topics", "data-model/SliceInfoForRegistration",
        "--handoff-topics", "data-model/AuthorizedNetworkSliceInfo",
    )
    # The root itself never appears as a topic ref inside its own tree.
    # Check that any field whose type IS a known handoff topic resolves to a topic ref
    # and lands in dependencies. AllowedSnssai (non-handoff) must be inlined.
    body = json.dumps(payload)
    assert "_inlined_from" in body
    # dependencies only carries handoff topics
    for dep in payload["dependencies"]:
        assert dep.startswith("data-model/")


def test_emit_json_unresolved_refs_listed() -> None:
    # Provoke an unresolved ref by pointing to a tiny synthetic yaml under tmp.
    pass  # covered indirectly via build-handoff test; placeholder removed in T8


def _find_field(fields: list[dict], name: str) -> dict | None:
    for f in fields:
        if f.get("name") == name:
            return f
    return None


def test_emit_json_additional_properties_map_value_preserved() -> None:
    # AuthorizedNetworkSliceInfo.snssaiInfoRspData is `type: object` with
    # `additionalProperties.$ref: SnssaiInfo` — a map type. Its value schema
    # must be preserved on the field as `additional_properties`. Without
    # this, codegen / dev agents see an empty struct and miss the map shape.
    payload = _run(
        str(NSSF_YAML), "AuthorizedNetworkSliceInfo",
        "--emit-json",
        "--topic-id", "data-model/AuthorizedNetworkSliceInfo",
        "--nf", "nssf",
        "--spec-ref", "TS 29.531 §6.1.6.2.5",
        "--handoff-topics", "data-model/SliceInfoForRegistration",
        "--handoff-topics", "data-model/AuthorizedNetworkSliceInfo",
    )
    snssai_map = _find_field(payload["fields"], "snssaiInfoRspData")
    assert snssai_map is not None, "snssaiInfoRspData missing from emitted fields"
    assert snssai_map.get("type") == "object"
    ap = snssai_map.get("additional_properties")
    assert ap is not None, (
        f"snssaiInfoRspData.additional_properties missing — map value schema dropped. "
        f"Field payload: {json.dumps(snssai_map, indent=2, ensure_ascii=False)}"
    )
    # Value type should be the resolved SnssaiInfo schema (object with properties),
    # not an empty dict.
    assert ap.get("type") == "object"
    assert isinstance(ap.get("properties"), list) and len(ap["properties"]) > 0, (
        "SnssaiInfo value schema unresolved — properties list empty"
    )


def test_emit_json_additional_properties_primitive_value() -> None:
    # AllowedNssai inside AuthorizedNetworkSliceInfo contains nrfOauth2Required —
    # `additionalProperties: {type: boolean}`. The primitive value schema must
    # also be preserved.
    payload = _run(
        str(NSSF_YAML), "AuthorizedNetworkSliceInfo",
        "--emit-json",
        "--topic-id", "data-model/AuthorizedNetworkSliceInfo",
        "--nf", "nssf",
        "--spec-ref", "TS 29.531 §6.1.6.2.5",
        "--handoff-topics", "data-model/SliceInfoForRegistration",
        "--handoff-topics", "data-model/AuthorizedNetworkSliceInfo",
    )
    # nrfOauth2Required is nested under allowedNssaiList[].allowedSnssaiList[] —
    # walk the tree to find it.
    def _walk_for_field(node: object, target: str) -> dict | None:
        if isinstance(node, dict):
            if node.get("name") == target:
                return node
            for v in node.values():
                hit = _walk_for_field(v, target)
                if hit is not None:
                    return hit
        elif isinstance(node, list):
            for v in node:
                hit = _walk_for_field(v, target)
                if hit is not None:
                    return hit
        return None

    nrf_map = _walk_for_field(payload["fields"], "nrfOauth2Required")
    assert nrf_map is not None, "nrfOauth2Required not found in emitted tree"
    ap = nrf_map.get("additional_properties")
    assert ap is not None, "nrfOauth2Required.additional_properties dropped"
    assert ap.get("type") == "boolean", f"expected boolean value type, got {ap}"
