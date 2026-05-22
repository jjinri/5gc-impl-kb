#!/usr/bin/env python3
# 토픽 디렉터리 layout + seed yaml → handoff-v2 self-contained yaml.
"""
Usage:
    .venv/bin/python3 design/scripts/build-handoff.py <nf>

입력:
    design/<nf>/_contract_seed.yaml — 사람·/nf-build 가 작성한 seed (필수)
        nf, categories, topics, tasks, sources

산출:
    handoff/<nf>/contract.yaml — schema_version: handoff-v2.
        + agent_contract (도구가 채움), spec_index (도구가 채움)
"""

from __future__ import annotations

import argparse
import datetime
import os
import pathlib
import sys

import yaml


REPO_ROOT = pathlib.Path(
    os.environ.get("FIVEGC_REPO_ROOT")
    or pathlib.Path(__file__).resolve().parent.parent.parent
).resolve()


AGENT_CONTRACT = {
    "status_precedence": "topic_over_category",
    "default_read_order": [
        "handoff/<nf>/contract.yaml",
        "handoff/<nf>/contract.yaml#categories",
        "design/<nf>/contract/<topic>/<id>.md (target)",
        "design/<nf>/contract/<topic>/<id>.json (target machine)",
        "design/<nf>/contract/<topic>/<dep>.md (depends_on)",
        "design/<nf>/contract/error-handling.md (error_refs)",
        "design/<nf>/contract/<topic>/<related>.md (related)",
    ],
    "must_not": [
        "status 가 draft 인 토픽으로 구현 시작",
        "status 가 blocked 인 토픽으로 구현 시작",
        "status 가 not_applicable 인 토픽을 생성",
        "spec_refs / Implementation Notes 에 근거 없는 행동·자료형·정책 invent",
        "agent_contract 외부 (design/<nf>/_archive/) 의 자료를 현행 contract 로 인용",
    ],
    "may_decide": [
        "내부 패키지/모듈 이름 (design 산출이 라이브러리 비종속)",
        "프레임워크 종속 handler 구조 (예 router 등록 방식)",
        "테스트 프레임워크 매핑 (Test Matrix 의 케이스 → 실제 test runner)",
        "로깅 라이브러리 선택 (Configuration 의 관측 키 충족 한)",
    ],
    "must_ask_or_block": [
        "필수 정책 값 부재 (timeout/retry/idempotency 미정)",
        "OpenAPI chain leaf 가 '(참조 규격 미등록)' 인데 구현이 필요",
        "역방향 status 불일치 (category=handoff_ready 인데 산하 topic=draft) — basic #5 영역",
        "depends_on 의 target 토픽이 yaml 에 부재",
        "Cross-NF 호출의 상대 NF op 가 아직 미정의",
    ],
}


def _build_spec_index(topics: dict) -> dict:
    idx: dict[str, list[str]] = {}
    for tid, t in topics.items():
        for ref in (t.get("spec_refs") or []):
            idx.setdefault(ref, []).append(tid)
    for ref in idx:
        idx[ref].sort()
    return dict(sorted(idx.items()))


def _ref_name(node) -> str | None:
    if isinstance(node, dict) and "$ref" in node:
        return str(node["$ref"]).split("/")[-1]
    return None


def _primary_yamls(nf: str) -> list[pathlib.Path]:
    manifest_path = REPO_ROOT / "design" / nf / "_manifest.yaml"
    if not manifest_path.is_file():
        return []
    m = yaml.safe_load(manifest_path.read_text(encoding="utf-8")) or {}
    spec = str(m.get("primary_spec") or "")
    ys = (m.get("primary_files") or {}).get("yamls") or []
    return [REPO_ROOT / "specs" / spec / y for y in ys]


def _enrich_api_topic(topic: dict, yamls: list[pathlib.Path]) -> None:
    """PR F1.2 — api topic 에 responses/security_requirements/error_responses/source_refs 추가.

    nf-status.py 의 api_operation_complete check (6 키 mandate) 충족용. seed 는 method/path
    만 보유 → build-handoff.py 가 yaml 분석해 4 키 채움. 본 enrichment 는 in-place — handoff
    yaml self-contained 정책 유지 (codegen 시 spec 재독해 불요).
    """
    path = topic.get("path")
    method = str(topic.get("method", "")).lower()
    if not path or not method:
        return
    for yp in yamls:
        if not yp.is_file():
            continue
        d = yaml.safe_load(yp.read_text(encoding="utf-8")) or {}
        op_block = (d.get("paths") or {}).get(path)
        if not isinstance(op_block, dict) or method not in {k.lower() for k in op_block}:
            continue
        op = op_block[method]
        # security_requirements — operation-level 우선, global fallback.
        sec = op.get("security") or d.get("security") or []
        # responses + error_responses — status code → schema 이름 매핑.
        responses: dict = {}
        error_responses: dict = {}
        for code, r in (op.get("responses") or {}).items():
            schema_name = None
            for ct in ((r or {}).get("content") or {}).values():
                schema_name = _ref_name(ct.get("schema")) or _ref_name(
                    (ct.get("schema") or {}).get("items")
                )
                if schema_name:
                    break
            entry = {
                "schema": schema_name,
                "description": (r or {}).get("description", "").strip(),
            }
            code_str = str(code)
            if code_str[:1] in {"4", "5"} or code_str == "default":
                error_responses[code_str] = entry
            else:
                responses[code_str] = entry
        # source_refs — spec_refs (seed) + yaml path. nf-seed-gen 이 spec_refs 못 넣는 경우 대비.
        source_refs = list(topic.get("spec_refs") or [])
        rel_yaml = str(yp.relative_to(REPO_ROOT)) if yp.is_relative_to(REPO_ROOT) else str(yp)
        if rel_yaml not in source_refs:
            source_refs.append(rel_yaml)
        topic.setdefault("responses", responses)
        topic.setdefault("error_responses", error_responses)
        topic.setdefault("security_requirements", sec or [{"none": []}])
        topic.setdefault("source_refs", source_refs)
        return
    # path/method 못 찾으면 빈 dict 라도 emit (check 가 빈 값도 FAIL 처리).
    topic.setdefault("responses", {})
    topic.setdefault("error_responses", {})
    topic.setdefault("security_requirements", [])
    topic.setdefault("source_refs", list(topic.get("spec_refs") or []))


def _enrich_error_handling_topic(topic: dict, yamls: list[pathlib.Path]) -> None:
    """PR F1.2 — error-handling topic 의 operations × causes 매트릭스 emit.

    nf-status.py 의 problem_details_matrix_complete check 충족용. yaml 모든 path × method 의
    4xx/5xx responses 를 모아 operation 별 causes list 생성. cause 는 spec response description
    (3GPP cause enum 텍스트) 사용.
    """
    operations: dict = {}
    for yp in yamls:
        if not yp.is_file():
            continue
        d = yaml.safe_load(yp.read_text(encoding="utf-8")) or {}
        for path, ops in (d.get("paths") or {}).items():
            if not isinstance(ops, dict):
                continue
            for method, op in ops.items():
                if not isinstance(op, dict) or method.lower() not in (
                    "get", "put", "post", "patch", "delete", "head", "options"
                ):
                    continue
                op_id = op.get("operationId") or f"{method.upper()} {path}"
                causes: list = []
                for code, r in (op.get("responses") or {}).items():
                    cs = str(code)
                    if cs[:1] in {"4", "5"} or cs == "default":
                        causes.append({
                            "status": cs,
                            "description": (r or {}).get("description", "").strip(),
                        })
                operations[op_id] = {"path": path, "method": method.upper(), "causes": causes}
    if operations:
        topic.setdefault("operations", operations)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="토픽 디렉터리 layout + seed → handoff-v2 yaml",
    )
    parser.add_argument("nf", help="NF 폴더명 (소문자)")
    args = parser.parse_args()

    nf = args.nf.lower()
    design_dir = REPO_ROOT / "design" / nf
    if not design_dir.is_dir():
        sys.exit(f"[build-handoff] design/{nf}/ 없음")

    seed_path = design_dir / "_contract_seed.yaml"
    if not seed_path.is_file():
        sys.exit(f"[build-handoff] {seed_path.relative_to(REPO_ROOT)} 없음. "
                 f"seed 를 먼저 작성하거나 /nf-build <nf> 를 실행.")
    seed = yaml.safe_load(seed_path.read_text(encoding="utf-8")) or {}

    if seed.get("nf") != nf:
        sys.exit(f"[build-handoff] seed.nf={seed.get('nf')!r} ≠ {nf!r}")

    categories = seed.get("categories") or {}
    topics = seed.get("topics") or {}
    tasks = seed.get("tasks") or {}
    sources = seed.get("sources") or {}

    # PR F1.2 — handoff yaml self-contained 강화. seed topic 에 부족한 contract 키 (api 4 키,
    # error-handling operations) 를 OpenAPI yaml 분석해 in-place 추가.
    yamls = _primary_yamls(nf)
    for tid, t in topics.items():
        if isinstance(t, dict) and tid.startswith("api/"):
            _enrich_api_topic(t, yamls)
    eh = topics.get("error-handling") or topics.get("error_handling")
    if isinstance(eh, dict):
        _enrich_error_handling_topic(eh, yamls)

    out_payload = {
        "schema_version": "handoff-v2",
        "nf": nf,
        "generated_at": datetime.datetime.utcnow().isoformat() + "Z",
        "agent_contract": AGENT_CONTRACT,
        "categories": categories,
        "topics": topics,
        "tasks": tasks,
        "spec_index": _build_spec_index(topics),
        "sources": sources,
    }

    out_dir = REPO_ROOT / "handoff" / nf
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / "contract.yaml"
    out_path.write_text(
        yaml.dump(out_payload, allow_unicode=True, default_flow_style=False, sort_keys=False),
        encoding="utf-8",
    )
    print(f"[build-handoff] wrote {out_path.relative_to(REPO_ROOT)}", file=sys.stderr)
    print(f"[build-handoff] categories={len(categories)} topics={len(topics)} tasks={len(tasks)}",
          file=sys.stderr)


if __name__ == "__main__":
    main()
