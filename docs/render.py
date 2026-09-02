#!/usr/bin/env python3
"""Renders the manuals to HTML so they can be read in a browser.

  python3 docs/render.py

Writes clarity.html and test-gear.html beside their markdown, so the relative image paths in
them keep working. Open the file directly; no server is needed.

A small converter rather than a dependency: the manuals use headings, paragraphs, rules,
tables, lists, bold, italics, code spans, fenced blocks and raw HTML image tags, and that is
the whole of it.
"""
import html
import re
from pathlib import Path


HERE = Path(__file__).parent

STYLE = """
:root { color-scheme: light dark; }
body {
  margin: 0 auto; padding: 3rem 1.5rem 6rem; max-width: 54rem;
  font: 18px/1.65 -apple-system, "Helvetica Neue", Helvetica, Arial, sans-serif;
  color: #1b1f24; background: #fff;
}
h1 { font-size: 2.2rem; margin: 0 0 1.5rem; }
h2 { font-size: 1.5rem; margin: 3rem 0 1rem; }
h3 { font-size: 1.15rem; margin: 2.25rem 0 .75rem; }
p { margin: 0 0 1.1rem; }
hr { border: 0; border-top: 1px solid #d8dce1; margin: 2.5rem 0; }
img { max-width: 100%; height: auto; display: block; margin: 1rem 0; }
em { color: #5b636d; font-style: italic; }
pre { background: #f2f4f6; padding: .8em 1em; border-radius: 4px; overflow-x: auto; }
pre code { background: none; padding: 0; }
code { font: .9em ui-monospace, Menlo, monospace; background: #f2f4f6;
       padding: .1em .35em; border-radius: 3px; }
table { border-collapse: collapse; margin: 1.25rem 0; }
th, td { border: 1px solid #d8dce1; padding: .5rem .75rem; vertical-align: middle; }
th { background: #f2f4f6; text-align: left; }
td img { margin: 0 auto; }
ul { margin: 0 0 1.1rem; padding-left: 1.4rem; }
li { margin: 0 0 .5rem; }
@media (prefers-color-scheme: dark) {
  body { color: #e6e8ec; background: #16191d; }
  hr { border-top-color: #2c313a; }
  em { color: #9aa1ac; }
  code { background: #232830; }
  pre { background: #232830; }
  th, td { border-color: #2c313a; }
  th { background: #1d2127; }
}
"""


def inline(text):
    """Bold, italics and code spans. Raw HTML in the line is left alone."""
    parts = re.split(r'(<[^>]+>)', text)
    out = []
    for i, part in enumerate(parts):
        if i % 2:                      # A tag: passed through untouched.
            out.append(part)
            continue
        part = html.escape(part, quote=False)
        part = re.sub(r'`([^`]+)`', r'<code>\1</code>', part)
        part = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', part)
        part = re.sub(r'(?<!\*)\*([^*]+)\*(?!\*)', r'<em>\1</em>', part)
        out.append(part)
    return "".join(out)


def convert(md):
    lines = md.split("\n")
    out = []
    i = 0
    while i < len(lines):
        line = lines[i].rstrip()

        if not line.strip():
            i += 1
            continue

        if line.startswith("#"):
            level = len(line) - len(line.lstrip("#"))
            out.append(f"<h{level}>{inline(line[level:].strip())}</h{level}>")
            i += 1
            continue

        # A fenced block, held verbatim. Everything inside is escaped and nothing in it is
        # read as markup: it is there to be copied into a file, so it has to arrive as typed.
        if line.startswith("```"):
            i += 1
            block = []
            while i < len(lines) and not lines[i].startswith("```"):
                block.append(html.escape(lines[i]))
                i += 1
            i += 1
            out.append("<pre><code>" + "\n".join(block) + "</code></pre>")
            continue

        if re.fullmatch(r'-{3,}', line.strip()):
            out.append("<hr>")
            i += 1
            continue

        # A table: a header row, a divider, then rows until a blank line.
        if line.startswith("|") and i + 1 < len(lines) and set(lines[i + 1]) <= set("|-: "):
            header = [c.strip() for c in line.strip("|").split("|")]
            out.append("<table><thead><tr>"
                       + "".join(f"<th>{inline(c)}</th>" for c in header)
                       + "</tr></thead><tbody>")
            i += 2
            while i < len(lines) and lines[i].startswith("|"):
                cells = [c.strip() for c in lines[i].strip("|").split("|")]
                out.append("<tr>" + "".join(f"<td>{inline(c)}</td>" for c in cells) + "</tr>")
                i += 1
            out.append("</tbody></table>")
            continue

        if line.lstrip().startswith("- "):
            out.append("<ul>")
            while i < len(lines) and lines[i].lstrip().startswith("- "):
                out.append(f"<li>{inline(lines[i].lstrip()[2:])}</li>")
                i += 1
            out.append("</ul>")
            continue

        # A line that is only a tag stands on its own; anything else is a paragraph.
        if line.lstrip().startswith("<"):
            out.append(line)
        else:
            out.append(f"<p>{inline(line)}</p>")
        i += 1
    return "\n".join(out)


def render(name, source=None):
    md = (source or HERE / f"{name}.md").read_text()
    title = md.split("\n", 1)[0].lstrip("# ").strip()
    page = (f"<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
            f"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
            f"<title>{html.escape(title)}</title>\n<style>{STYLE}</style>\n</head>\n<body>\n"
            f"{convert(md)}\n</body>\n</html>\n")
    out = HERE / f"{name}.html"
    out.write_text(page)
    print(out)


if __name__ == "__main__":
    for name in ("clarity", "test-gear"):
        render(name)
    # The README lives at the repository root; its page is written here with the manuals so
    # that one folder holds everything readable in a browser.
    render("readme", HERE.parent / "README.md")
