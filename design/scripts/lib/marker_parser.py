# AUTO / USER marker 추출 + frontmatter sync diff. spec §2 의 기계 계약.
from __future__ import annotations

import dataclasses
import re


@dataclasses.dataclass(frozen=True)
class Marker:
    kind: str   # "AUTO" or "USER"
    id: str
    start: int  # body offset
    end: int


_START = re.compile(r"<!--\s*(AUTO|USER):([a-zA-Z0-9][\w\-]*):start\s*-->")
_END = re.compile(r"<!--\s*(AUTO|USER):([a-zA-Z0-9][\w\-]*):end\s*-->")


def extract_markers(text: str) -> list[Marker]:
    out: list[Marker] = []
    for s in _START.finditer(text):
        kind, mid = s.group(1), s.group(2)
        e_pat = re.compile(
            rf"<!--\s*{kind}:{re.escape(mid)}:end\s*-->"
        )
        e = e_pat.search(text, s.end())
        if e:
            out.append(Marker(kind=kind, id=mid, start=s.start(), end=e.end()))
    return out


def duplicate_marker_ids(text: str) -> list[tuple[str, str]]:
    seen: dict[tuple[str, str], int] = {}
    for m in extract_markers(text):
        seen[(m.kind, m.id)] = seen.get((m.kind, m.id), 0) + 1
    return [k for k, v in seen.items() if v > 1]


@dataclasses.dataclass(frozen=True)
class SyncDiff:
    frontmatter_only_auto: list[str]
    body_only_auto: list[str]
    frontmatter_only_user: list[str]
    body_only_user: list[str]


def sync_diff(*, fm: dict, markers: list[Marker]) -> SyncDiff:
    fm_auto = set(fm.get("generated_sections") or [])
    fm_user = set(fm.get("user_sections") or [])
    body_auto = {m.id for m in markers if m.kind == "AUTO"}
    body_user = {m.id for m in markers if m.kind == "USER"}
    return SyncDiff(
        frontmatter_only_auto=sorted(fm_auto - body_auto),
        body_only_auto=sorted(body_auto - fm_auto),
        frontmatter_only_user=sorted(fm_user - body_user),
        body_only_user=sorted(body_user - fm_user),
    )
