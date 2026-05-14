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
