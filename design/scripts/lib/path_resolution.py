# topic ID → file path resolver. spec §4.5 의 3 layout 을 한 곳에서.
from __future__ import annotations

import dataclasses
import pathlib
import re


@dataclasses.dataclass(frozen=True)
class TopicRef:
    category: str
    topic: str           # category 와 동일하면 single-file
    anchor: str | None   # "topic#anchor" 의 anchor


@dataclasses.dataclass(frozen=True)
class PathResolution:
    path: pathlib.Path
    exists: bool
    anchor: str | None
    anchor_found: bool   # anchor 없으면 True


def parse_topic_ref(topic_id: str) -> TopicRef:
    anchor = None
    if "#" in topic_id:
        head, anchor = topic_id.split("#", 1)
    else:
        head = topic_id
    if "/" in head:
        category, topic = head.split("/", 1)
    else:
        category, topic = head, head
    return TopicRef(category=category, topic=topic, anchor=anchor)


def _anchor_present(text: str, anchor: str) -> bool:
    if re.search(rf'<a\s+id=["\']?{re.escape(anchor)}["\']?\s*></a>', text):
        return True
    pat = re.compile(r"^#{1,6}\s+(.+)$", re.MULTILINE)
    for m in pat.finditer(text):
        slug = re.sub(r"[^a-z0-9]+", "-", m.group(1).lower()).strip("-")
        if slug == anchor:
            return True
    return False


def resolve_topic_path(
    *,
    nf_root: pathlib.Path,
    topic_id: str,
    category_layout: dict[str, str],
) -> PathResolution:
    ref = parse_topic_ref(topic_id)
    if ref.category not in category_layout:
        raise ValueError(f"unknown category {ref.category!r}")
    layout = category_layout[ref.category]
    if layout == "directory":
        path = nf_root / ref.category / f"{ref.topic}.md"
    elif layout == "single-file":
        path = nf_root / f"{ref.category}.md"
    else:
        raise ValueError(f"unknown layout {layout!r}")
    exists = path.is_file()
    anchor_found = True
    if ref.anchor and exists:
        anchor_found = _anchor_present(path.read_text(encoding="utf-8"), ref.anchor)
    return PathResolution(
        path=path, exists=exists, anchor=ref.anchor, anchor_found=anchor_found,
    )
