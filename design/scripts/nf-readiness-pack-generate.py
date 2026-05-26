#!/usr/bin/env python3
"""nf-readiness-pack-generate — readiness pack 산출의 deterministic render/check.

PR-4 scope (Phase C 시작 — Plan 2026-05-26 §7).
- 첫 target = dev/<nf>/traceability.md only.
- AUTO/USER marker 패턴 (materialize-contract.py reference 모범).
- --check: render to memory, compare with tracked, drift 시 exit 1.
- --write: render to tracked path, 멱등.

본 generator 의 책임 — *deterministic AUTO 영역 render*. USER 영역은 사람
산문 보존 — generator 가 만지지 않는다 (placeholder TODO 만 fresh 시).

PR-4 기준 AUTO 영역은 frontmatter (manifest) + references (fixed per NF
source-of-truth list) 만. 다른 영역은 모두 USER. PR-5+ 가 contract-module
table, module-test table 등 추가 AUTO 영역으로 확장.

No timestamp noise — `ratified_date` 등 config-driven 정보만 출력.
"""

import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent.parent

# Target schema — file path template → (auto ids, user ids).
# 본 generator 가 render 하는 모든 산출의 *기계 계약*.
TARGETS = {
    "dev/<nf>/traceability.md": {
        "auto_ids": ["references"],
        "user_ids": [
            "intro-note",
            "contract-module-table",
            "module-test-table",
            "open-gaps",
        ],
    },
}

# References — fixed per NF source-of-truth list. PR-5+ 가 readiness-config
# / arch 산출에서 derive 로 전환.
REFERENCES_BY_NF = {
    "nssf": [
        ("docs/adr/ADR-0004-project-security-baseline.md",
         "TLS / mTLS / OAuth2 production-capable code path 의무."),
        ("design/policies/security-baseline.yaml",
         "baseline 7 mandate (M1~M7) machine-readable."),
        ("design/policies/lifecycle.yaml",
         "lifecycle stage / gate vocabulary."),
        ("design/policies/gaps.yaml",
         "gap category enum (6) — open-gaps 의 category 컬럼 source."),
        ("design/policies/source-precedence.yaml",
         "ADR-0004 baseline > engineering-design > architecture > dev"
         " enforcement order."),
        ("design/policies/spec-dependencies.yaml",
         "33.501 / 33.310 / 33.210 absorbed by ADR-0004."),
        ("design/schemas/engineering-core-slots.yaml",
         "13 core slot typed shape (v2 + dependency_shape)."),
        ("design/schemas/nf-readiness-config-v1.yaml",
         "readiness-config shape contract."),
        ("design/nssf/readiness-config.yaml",
         "NSSF 사람 ratify config — implementation·security·phase·gap·"
         "deferred 결정 source."),
        ("engineering/nssf/engineering-design.md",
         "현재 적용 lib/구조 source (readiness-config.implementation 흡수"
         " 대상)."),
        ("dev/nssf/implementation-plan.md", None),
        ("dev/nssf/tasks.yaml", None),
        ("dev/nssf/test-matrix.md", None),
        ("design/nssf/architecture/", "전체"),
        ("design/nssf/module-decomposition/", "전체"),
        ("handoff/nssf/contract.yaml", "8 API topic + data-model."),
    ],
}

# Title per target. NF 별 NF 라벨 + suffix.
TARGET_TITLES = {
    "dev/<nf>/traceability.md": "{nf_upper} Traceability",
}

# Frontmatter 의 stage 라벨.
TARGET_STAGES = {
    "dev/<nf>/traceability.md": "implementation-planning",
}


# ─── marker helpers ─────────────────────────────────────────────────

def auto_block(aid: str, body: str) -> str:
    return f"<!-- AUTO:{aid}:start -->\n{body}\n<!-- AUTO:{aid}:end -->"


def user_block(uid: str, body: str) -> str:
    return f"<!-- USER:{uid}:start -->\n{body}\n<!-- USER:{uid}:end -->"


def extract_blocks(text: str, kind: str) -> dict[str, str]:
    """kind ∈ {AUTO, USER}. dict[id] = body."""
    out: dict[str, str] = {}
    pattern = rf"<!-- {kind}:([\w-]+):start -->\n(.*?)\n<!-- {kind}:\1:end -->"
    for m in re.finditer(pattern, text, re.DOTALL):
        out[m.group(1)] = m.group(2)
    return out


# ─── AUTO renderers ─────────────────────────────────────────────────

def render_references(nf: str) -> str:
    items = REFERENCES_BY_NF.get(nf)
    if not items:
        return f"## References\n\n_PR-4 references list for {nf!r} 미정의._"
    lines = ["## References", ""]
    for path, note in items:
        if note is None:
            lines.append(f"- `{path}`.")
        elif note == "전체":
            lines.append(f"- `{path}` 전체.")
        else:
            lines.append(f"- `{path}` — {note}")
    return "\n".join(lines)


# ─── target render ──────────────────────────────────────────────────

def render_traceability(nf: str, current_text: str) -> str:
    target = "dev/<nf>/traceability.md"
    schema = TARGETS[target]
    autos = {
        "references": render_references(nf),
    }
    users_existing = extract_blocks(current_text, "USER") if current_text else {}

    # USER defaults — fresh placeholder. 기존 USER 가 있으면 보존.
    def user_body(uid: str) -> str:
        body = users_existing.get(uid)
        if body is None or not body.strip():
            return f"TODO: {uid} — 사람이 보강 (fresh placeholder)."
        return body

    title = TARGET_TITLES[target].format(nf_upper=nf.upper())
    stage = TARGET_STAGES[target]

    fm_lines = [
        "---",
        f"nf: {nf}",
        f"stage: {stage}",
        "generator: design/scripts/nf-readiness-pack-generate.py",
        f"source_readiness_config: design/{nf}/readiness-config.yaml",
        "generated_sections:",
    ]
    for aid in schema["auto_ids"]:
        fm_lines.append(f"  - {aid}")
    fm_lines.append("user_sections:")
    for uid in schema["user_ids"]:
        fm_lines.append(f"  - {uid}")
    fm_lines.append("---")
    fm = "\n".join(fm_lines)

    parts = [
        fm,
        "",
        f"# {title}",
        "",
        user_block("intro-note", user_body("intro-note")),
        "",
        "## Contract → Module",
        "",
        user_block("contract-module-table", user_body("contract-module-table")),
        "",
        "## Module → Test",
        "",
        user_block("module-test-table", user_body("module-test-table")),
        "",
        "## Open Gaps",
        "",
        user_block("open-gaps", user_body("open-gaps")),
        "",
        auto_block("references", autos["references"]),
        "",
    ]
    return "\n".join(parts).rstrip() + "\n"


# Target dispatch — file template → renderer.
RENDERERS = {
    "dev/<nf>/traceability.md": render_traceability,
}


# ─── action ─────────────────────────────────────────────────────────

def render_target(nf: str, target_tpl: str) -> tuple[pathlib.Path, str, str]:
    """반환 (path, current_text, rendered_text)."""
    rel = target_tpl.replace("<nf>", nf)
    path = REPO / rel
    current = path.read_text(encoding="utf-8") if path.is_file() else ""
    rendered = RENDERERS[target_tpl](nf, current)
    return path, current, rendered


def diff_summary(current: str, rendered: str, max_lines: int = 10) -> list[str]:
    """단순 라인 diff 요약 — 자세한 diff 는 `git diff` 가 책임."""
    cur = current.splitlines()
    ren = rendered.splitlines()
    out = []
    n = max(len(cur), len(ren))
    for i in range(n):
        a = cur[i] if i < len(cur) else "<EOF>"
        b = ren[i] if i < len(ren) else "<EOF>"
        if a != b:
            out.append(f"  line {i+1}:")
            out.append(f"    current : {a}")
            out.append(f"    rendered: {b}")
            if len(out) >= max_lines * 3:
                out.append(f"  ... (truncated; more diff at line {i+1}+)")
                break
    return out


def cmd_check(nf: str) -> int:
    fail = 0
    for target_tpl in TARGETS:
        path, current, rendered = render_target(nf, target_tpl)
        rel = path.relative_to(REPO)
        if current == rendered:
            print(f"PASS — {rel}")
            continue
        if not current:
            print(f"FAIL — {rel} 없음 (run --write)")
        else:
            print(f"FAIL — {rel} drift detected")
            for ln in diff_summary(current, rendered):
                print(ln)
        fail += 1
    return 0 if fail == 0 else 1


def cmd_write(nf: str) -> int:
    for target_tpl in TARGETS:
        path, current, rendered = render_target(nf, target_tpl)
        rel = path.relative_to(REPO)
        if current == rendered:
            print(f"unchanged — {rel}")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(rendered, encoding="utf-8")
            print(f"wrote — {rel}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="readiness pack render/check (PR-4: traceability only)")
    parser.add_argument("nf", help="NF key (예 nssf)")
    grp = parser.add_mutually_exclusive_group(required=True)
    grp.add_argument("--check", action="store_true",
                     help="render to memory, compare with tracked, exit 1 if drift")
    grp.add_argument("--write", action="store_true",
                     help="render to tracked path, 멱등")
    args = parser.parse_args()

    nf = args.nf
    config_path = REPO / "design" / nf / "readiness-config.yaml"
    if not config_path.is_file():
        print(f"FAIL — {config_path.relative_to(REPO)} 없음. PR-3 (readiness-config) "
              "이 머지됐는지 확인.", file=sys.stderr)
        return 2

    if args.check:
        return cmd_check(nf)
    return cmd_write(nf)


if __name__ == "__main__":
    sys.exit(main())
