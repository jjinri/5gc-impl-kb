#!/usr/bin/env python3
# 본 repo 의 markdown doc 을 GitHub-style self-contained HTML 로 변환
"""
Usage:
    .venv/bin/python3 design/scripts/render-doc.py <md-path> [<html-path>]

기본 출력 — md 와 같은 디렉터리, 확장자만 .html.

설계 원칙.
- self-contained — 외부 CDN/script/font 의존 0. fresh-clone offline 동작.
- single source — md 가 1차, html 은 build artifact. 직접 편집 금지 (다음
  render 시 덮어쓰임).
- markdown extension — extra (tables/fenced_code/attr_list) + toc +
  codehilite + sane_lists + smarty.
- pygments — 코드 블록 syntax highlight (CSS embed).
- two-column layout — sticky TOC 좌측 + 본문 우측. mobile 에서 stack.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

try:
    import markdown
    from pygments.formatters import HtmlFormatter
except ModuleNotFoundError as e:
    sys.exit(
        f"[render-doc] 필요한 패키지 누락 ({e.name}). "
        f"`.venv/bin/python3 -m pip install -r requirements.txt` 실행."
    )


THEME_CSS = """
:root {
  --fg: #1f2328;
  --fg-muted: #57606a;
  --bg: #ffffff;
  --bg-alt: #f6f8fa;
  --border: #d8dee4;
  --border-muted: #eaeef2;
  --accent: #0969da;
  --accent-emphasis: #0550ae;
  --hover-bg: #ddf4ff;
  --success: #1a7f37;
  --danger: #cf222e;
  --neutral: #59636e;
  --shadow-sm: 0 1px 0 rgba(31,35,40,.04);
  --radius: 6px;
  --font-body: -apple-system, BlinkMacSystemFont, "Segoe UI", "Noto Sans KR",
    "Apple SD Gothic Neo", "Malgun Gothic", sans-serif;
  --font-mono: ui-monospace, "SF Mono", "Cascadia Code", "Roboto Mono",
    "D2Coding", monospace;
}

* { box-sizing: border-box; }
html { scroll-behavior: smooth; scroll-padding-top: 1rem; }
body {
  margin: 0;
  font-family: var(--font-body);
  font-size: 15px;
  line-height: 1.65;
  color: var(--fg);
  background: var(--bg);
}

.wrap {
  max-width: 1400px;
  margin: 0 auto;
  display: grid;
  grid-template-columns: 280px minmax(0, 1fr);
  gap: 3rem;
  padding: 2rem 1.5rem 4rem;
}

aside.toc {
  position: sticky;
  top: 1rem;
  align-self: start;
  max-height: calc(100vh - 2rem);
  overflow-y: auto;
  padding-right: 1.5rem;
  border-right: 1px solid var(--border-muted);
  font-size: .87rem;
}
aside.toc::before {
  content: "Contents";
  display: block;
  font-size: .72rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: .08em;
  color: var(--fg-muted);
  margin-bottom: .65rem;
  padding-bottom: .5rem;
  border-bottom: 1px solid var(--border-muted);
}
aside.toc ul { list-style: none; padding-left: 0; margin: 0; }
aside.toc ul ul { padding-left: 1rem; margin-top: .15rem; }
aside.toc li { margin: .1rem 0; }
aside.toc a {
  display: block;
  padding: .18rem .4rem;
  color: var(--fg-muted);
  text-decoration: none;
  border-radius: 4px;
  transition: background .1s, color .1s;
}
aside.toc a:hover { background: var(--bg-alt); color: var(--fg); }

main { max-width: 980px; min-width: 0; }

h1, h2, h3, h4, h5, h6 {
  position: relative;
  margin: 2rem 0 .65rem;
  font-weight: 700;
  scroll-margin-top: 1rem;
}
h1 {
  font-size: 2rem;
  border-bottom: 1px solid var(--border);
  padding-bottom: .35rem;
  margin-top: 0;
}
h2 {
  font-size: 1.45rem;
  border-bottom: 1px solid var(--border-muted);
  padding-bottom: .3rem;
  margin-top: 2.5rem;
}
h3 { font-size: 1.18rem; margin-top: 1.75rem; }
h4 {
  font-size: .94rem;
  margin-top: 1.25rem;
  color: var(--fg-muted);
  text-transform: uppercase;
  letter-spacing: .04em;
}

/* Anchor link on heading hover */
h1 .headerlink, h2 .headerlink, h3 .headerlink, h4 .headerlink {
  position: absolute;
  left: -1.4em;
  width: 1.25em;
  text-align: center;
  color: var(--accent);
  opacity: 0;
  text-decoration: none;
  font-weight: 400;
  transition: opacity .1s;
}
h1:hover .headerlink, h2:hover .headerlink, h3:hover .headerlink,
h4:hover .headerlink { opacity: 1; }
.headerlink::before { content: "#"; }

p { margin: .65rem 0; }
ul, ol { padding-left: 1.5rem; margin: .5rem 0; }
li { margin: .2rem 0; }
li > p { margin: .25rem 0; }

a { color: var(--accent); text-decoration: none; }
a:hover { text-decoration: underline; }

code {
  font-family: var(--font-mono);
  font-size: .87em;
  background: var(--bg-alt);
  padding: .12em .35em;
  border-radius: 3px;
}
pre {
  background: var(--bg-alt);
  border: 1px solid var(--border-muted);
  border-radius: var(--radius);
  padding: .95rem 1.1rem;
  overflow-x: auto;
  font-size: .86rem;
  line-height: 1.5;
  margin: .8rem 0;
  box-shadow: var(--shadow-sm);
}
pre code {
  background: none;
  padding: 0;
  font-size: inherit;
  border-radius: 0;
}
.codehilite pre { margin: 0; border: none; box-shadow: none; }
.codehilite {
  border: 1px solid var(--border-muted);
  border-radius: var(--radius);
  overflow: hidden;
  margin: .8rem 0;
  box-shadow: var(--shadow-sm);
}

table {
  border-collapse: collapse;
  width: 100%;
  margin: 1rem 0 1.5rem;
  font-size: .9rem;
  background: var(--bg);
  border: 1px solid var(--border-muted);
  border-radius: var(--radius);
  overflow: hidden;
  box-shadow: var(--shadow-sm);
}
thead { background: var(--bg-alt); }
thead th {
  text-align: left;
  padding: .55rem .8rem;
  border-bottom: 2px solid var(--border);
  font-weight: 600;
  font-size: .8rem;
  color: var(--fg-muted);
  text-transform: uppercase;
  letter-spacing: .03em;
  white-space: nowrap;
}
tbody td {
  padding: .55rem .8rem;
  border-bottom: 1px solid var(--border-muted);
  vertical-align: top;
}
tbody tr:last-child td { border-bottom: none; }
tbody tr:hover td { background: var(--hover-bg); transition: background .1s; }
td code { font-size: .82em; }

blockquote {
  border-left: 4px solid var(--accent);
  margin: 1rem 0;
  padding: .35rem 1rem;
  background: var(--bg-alt);
  color: var(--fg-muted);
}
blockquote p:first-child { margin-top: 0; }
blockquote p:last-child { margin-bottom: 0; }

hr {
  border: none;
  border-top: 1px solid var(--border-muted);
  margin: 2.5rem 0;
}

strong { font-weight: 600; }
em { font-style: italic; color: var(--fg-muted); }

/* Generated by render-doc.py footer */
.render-footer {
  margin-top: 4rem;
  padding-top: 1rem;
  border-top: 1px solid var(--border-muted);
  font-size: .78rem;
  color: var(--fg-muted);
  text-align: center;
}
.render-footer code { font-size: .85em; }

@media (max-width: 920px) {
  .wrap {
    grid-template-columns: 1fr;
    gap: 1.5rem;
    padding: 1rem 1rem 2.5rem;
  }
  aside.toc {
    position: static;
    max-height: none;
    border-right: none;
    border-bottom: 1px solid var(--border-muted);
    padding-right: 0;
    padding-bottom: 1rem;
  }
  main { max-width: 100%; }
  table { font-size: .82rem; }
}

@media print {
  aside.toc { display: none; }
  .wrap { display: block; max-width: 100%; padding: 0; }
  main { max-width: 100%; }
  pre, table, .codehilite { box-shadow: none; border-color: #ccc; }
  a { color: var(--fg); text-decoration: none; }
  h1, h2 { page-break-after: avoid; }
  table, pre, blockquote { page-break-inside: avoid; }
}
"""


def _badge_blocking(html: str) -> str:
    """Render *(blocking)* italic markers as colored badge spans.

    Markdown *(blocking)* → <em>(blocking)</em>. CSS :has-text selector
    가 표준이 아니라 작동 불가 → post-process 로 안정 변환.
    """
    return re.sub(
        r"<em>\(blocking\)</em>",
        '<span class="badge-blocking">(blocking)</span>',
        html,
    )


BADGE_CSS = """
.badge-blocking {
  display: inline-block;
  padding: .05em .5em;
  font-size: .78em;
  font-weight: 600;
  color: var(--danger);
  background: #ffebe9;
  border: 1px solid #ffc1ba;
  border-radius: 999px;
  font-style: normal;
  letter-spacing: .02em;
}
"""


def render(md_path: pathlib.Path, html_path: pathlib.Path) -> None:
    text = md_path.read_text(encoding="utf-8")
    md_engine = markdown.Markdown(extensions=[
        "extra",         # tables, fenced_code, attr_list, def_list
        "toc",           # auto TOC + permalink anchors
        "codehilite",    # pygments syntax highlight
        "sane_lists",
        "smarty",
    ], extension_configs={
        "toc": {
            "permalink": True,
            "permalink_title": "Link to this section",
        },
        "codehilite": {
            "css_class": "codehilite",
            "guess_lang": False,
        },
    })
    body_html = md_engine.convert(text)
    body_html = _badge_blocking(body_html)
    toc_html = md_engine.toc

    code_css = HtmlFormatter(style="default").get_style_defs(".codehilite")
    title = md_path.stem

    return _emit_full_html(
        html_path=html_path,
        title=title,
        toc=toc_html,
        body=body_html,
        code_css=code_css,
        source_md=md_path,
    )


def _emit_full_html(
    *,
    html_path: pathlib.Path,
    title: str,
    toc: str,
    body: str,
    code_css: str,
    source_md: pathlib.Path,
) -> None:
    page = f"""<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title} — 5gc-impl-kb</title>
<style>
{THEME_CSS.strip()}

{BADGE_CSS.strip()}

{code_css}
</style>
</head>
<body>
<div class="wrap">
<aside class="toc" aria-label="목차">
{toc}
</aside>
<main>
{body}
<footer class="render-footer">
Generated from <code>{source_md.as_posix()}</code> by <code>design/scripts/render-doc.py</code>. Source MD 가 진실 출처 — 본 HTML 은 build artifact, 직접 편집 금지.
</footer>
</main>
</div>
</body>
</html>
"""
    html_path.write_text(page, encoding="utf-8")
    print(
        f"[render-doc] wrote {html_path} "
        f"(source {source_md}, body {len(body):,} chars)",
        file=sys.stderr,
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Markdown → self-contained GitHub-style HTML",
    )
    parser.add_argument("md_path", help="입력 markdown 파일")
    parser.add_argument(
        "html_path", nargs="?", default=None,
        help="출력 HTML 경로 (default: md_path 와 같은 디렉터리, 확장자 .html)",
    )
    args = parser.parse_args()

    md_path = pathlib.Path(args.md_path).resolve()
    if not md_path.is_file():
        sys.exit(f"[render-doc] {md_path} 없음")

    html_path = (
        pathlib.Path(args.html_path).resolve()
        if args.html_path
        else md_path.with_suffix(".html")
    )
    render(md_path, html_path)


if __name__ == "__main__":
    main()
