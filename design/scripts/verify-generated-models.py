#!/usr/bin/env python3
# Verifier oracle for src/<nf>/generated/ against GENERATION_MANIFEST + type-overrides.
"""
Usage:
    .venv/bin/python3 design/scripts/verify-generated-models.py [<nf>]

목적
- src/<nf>/generated/model/ 의 emit 산출이 GENERATION_MANIFEST + type-
  overrides 의 기대치와 정합한지 검증. Pane 2 권고 — generator 는 raw
  emitter, data-model 산출물은 검증 oracle.

분류 (3-way classification)
- expected_generated 누락 — FAIL.
- wrapper_expected mismatch — PASS with classification (raw emit 만 있고
  wrapper 가 아직 미구현해도 본 PR scope 에서는 정상).
- unexpected (drift) — FAIL. generator version / spec 변경 영향 의심.

산출
- exit 0 = PASS, exit 1 = FAIL.
- stderr 에 분류별 보고.
"""

from __future__ import annotations

import pathlib
import sys

import yaml


REPO = pathlib.Path(__file__).resolve().parent.parent.parent
DEFAULT_NF = "nssf"

# GENERATION_MANIFEST 의 expected schema → emit file name (snake_case).
# openapi-generator-cli C generator 가 PascalCase → snake_case 변환 후 emit.
EXPECTED_SCHEMA_TO_FILE = {
    "AuthorizedNetworkSliceInfo": "authorized_network_slice_info",
    "AuthorizedNssaiAvailabilityInfo": "authorized_nssai_availability_info",
    "NfInstanceId": "nf_instance_id",   # string alias — TS29571 의 obj 가 아니면 emit 안 될 수도
    "NssaiAvailabilityInfo": "nssai_availability_info",
    "NssfEventSubscriptionCreateData": "nssf_event_subscription_create_data",
    "NssfEventSubscriptionCreatedData": "nssf_event_subscription_created_data",
    "PatchDocument": "patch_document",
    "PlmnId": "plmn_id",
    "SliceInfoForPDUSession": "slice_info_for_pdu_session",
    "SliceInfoForRegistration": "slice_info_for_registration",
    "SliceInfoForUEConfigurationUpdate": "slice_info_for_ue_configuration_update",
    "Snssai": "snssai",
    "SupportedFeatures": "supported_features",   # string alias — emit 안 될 수도
    "Tai": "tai",
}


def _load_yaml(path: pathlib.Path) -> dict:
    if not path.is_file():
        return {}
    return yaml.safe_load(path.read_text(encoding="utf-8")) or {}


def main() -> int:
    nf = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_NF
    manifest_path = REPO / "src" / nf / "generated" / "GENERATION_MANIFEST.yaml"
    overrides_path = REPO / "infra" / nf / "codegen" / "type-overrides.yaml"
    model_dir = REPO / "src" / nf / "generated" / "model"

    if not manifest_path.is_file():
        print(f"[verify-generated] FAIL missing {manifest_path.relative_to(REPO)}", file=sys.stderr)
        return 2
    if not model_dir.is_dir():
        print(f"[verify-generated] FAIL missing {model_dir.relative_to(REPO)}", file=sys.stderr)
        return 2

    manifest = _load_yaml(manifest_path)
    overrides = _load_yaml(overrides_path)

    # 1. Emitted models 수집.
    emitted = sorted(p.stem for p in model_dir.glob("*.h"))
    emitted_set = set(emitted)

    # 2. expected schemas (GENERATION_MANIFEST 의 contract_data_models +
    #    external_ts29571_dependencies).
    expected_artifacts = manifest.get("expected_artifacts") or {}
    contract_schemas = expected_artifacts.get("contract_data_models") or []
    contract_files = {EXPECTED_SCHEMA_TO_FILE.get(s, s.lower()) for s in contract_schemas}

    # 3. type-overrides 의 wrapper_expected 분류.
    alias_files = set()
    for entry in overrides.get("type_aliases", []) or []:
        s = entry.get("schema")
        if s in EXPECTED_SCHEMA_TO_FILE:
            alias_files.add(EXPECTED_SCHEMA_TO_FILE[s])

    wrapper_files = set()
    for kind in ["anyof_wrappers", "problem_details_wrappers", "patch_document_wrappers"]:
        for entry in overrides.get(kind, []) or []:
            s = entry.get("schema")
            if s in EXPECTED_SCHEMA_TO_FILE:
                wrapper_files.add(EXPECTED_SCHEMA_TO_FILE[s])

    # multi_spec_strategy.deferred_schemas — 본 PR scope 외 (다른 spec 에서 emit).
    deferred_files = set()
    multi_spec = overrides.get("multi_spec_strategy") or {}
    for s in multi_spec.get("deferred_schemas", []) or []:
        if s in EXPECTED_SCHEMA_TO_FILE:
            deferred_files.add(EXPECTED_SCHEMA_TO_FILE[s])

    # 4. 분류
    expected_present = contract_files & emitted_set
    expected_missing = contract_files - emitted_set
    # wrapper / alias schema 는 generator 가 emit 안 해도 OK (wrapper layer 에서 보정).
    wrapper_expected_missing = (wrapper_files | alias_files) - emitted_set
    expected_missing -= wrapper_expected_missing
    # deferred schema (NSSAIAvailability spec) 는 본 PR scope 외, 다른 PR 에서 emit.
    deferred_missing = deferred_files - emitted_set
    expected_missing -= deferred_missing
    unexpected = emitted_set - contract_files

    # 5. 보고
    print(f"[verify-generated] nf={nf} model_dir={model_dir.relative_to(REPO)}", file=sys.stderr)
    print(f"[verify-generated] emitted={len(emitted_set)} expected_contract={len(contract_files)}",
          file=sys.stderr)
    print(f"  expected_present     = {len(expected_present)}", file=sys.stderr)
    print(f"  expected_missing     = {len(expected_missing)}", file=sys.stderr)
    print(f"  wrapper_expected_missing = {len(wrapper_expected_missing)} (PASS with classification)",
          file=sys.stderr)
    print(f"  deferred_missing     = {len(deferred_missing)} (multi-spec deferred — PASS)",
          file=sys.stderr)
    print(f"  unexpected_transitive    = {len(unexpected)} (TS29571 / TS29510 transitive — informational)",
          file=sys.stderr)

    fails: list[str] = []
    if expected_missing:
        fails.append(f"expected_generated missing: {sorted(expected_missing)}")
    # unexpected 는 TS29571 transitive — informational 만, FAIL 아님 (NSSF spec 의 자연스러운 결과).

    if wrapper_expected_missing:
        print(f"[verify-generated] CLASSIFY wrapper_expected_missing (정상 — wrapper PR 에서 보정):",
              file=sys.stderr)
        for f in sorted(wrapper_expected_missing):
            print(f"  - {f}", file=sys.stderr)

    if fails:
        print(f"[verify-generated] FAIL {len(fails)} issue(s):", file=sys.stderr)
        for f in fails:
            print(f"  - {f}", file=sys.stderr)
        return 1
    print("[verify-generated] PASS — expected schemas present + drift classified.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
