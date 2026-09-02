#!/usr/bin/env python3
"""Draws the jack images used in the manuals.

The same geometry as drawJack() in src/Modules.cpp, kept here so the pictures can be redrawn
when the drawing changes rather than being screenshots that quietly go out of date. Run it
from the repository root: python3 docs/images/make-jacks.py
"""
import math
import subprocess
from pathlib import Path

FAMILIES = {
    "audio": "#c91847",
    "cv": "#0c8e15",
    "trigger": "#0986ad",
    "pitch": "#c9b70e",
}

R = 40.0                 # Jack radius, in a 100x100 viewBox.
RH = R * 0.53            # The hole.
BAND = R - RH
W = BAND / 3.0           # Dash width: a third of the coloured band.


def dashes(is_output):
    """The dashed ring: hugging the outer edge for an output, the hole for an input."""
    ring = (R * 0.95 - W / 2.0) if is_output else (RH + W / 2.0)
    n = max(6, round(2.0 * math.pi * ring / (W * 1.6)))
    step = 2.0 * math.pi / n
    out = []
    for i in range(n):
        a0, a1 = i * step, i * step + step / 2.0
        x0, y0 = 50 + ring * math.cos(a0), 50 + ring * math.sin(a0)
        x1, y1 = 50 + ring * math.cos(a1), 50 + ring * math.sin(a1)
        out.append(f'<path d="M {x0:.2f} {y0:.2f} A {ring:.2f} {ring:.2f} 0 0 1 {x1:.2f} {y1:.2f}" '
                   f'fill="none" stroke="#000" stroke-width="{W:.2f}" stroke-linecap="butt"/>')
    return "\n  ".join(out)


def svg(colour, is_output):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="100" height="100">
  <circle cx="50" cy="50" r="{R}" fill="{colour}" stroke="#000" stroke-opacity="0.78" stroke-width="{R * 0.1:.2f}"/>
  <circle cx="50" cy="50" r="{RH:.2f}" fill="#2f2f33"/>
  {dashes(is_output)}
</svg>
'''


def main():
    here = Path(__file__).parent
    for family, colour in FAMILIES.items():
        for is_output, kind in ((False, "in"), (True, "out")):
            stem = here / f"jack-{family}-{kind}"
            stem.with_suffix(".svg").write_text(svg(colour, is_output))
            subprocess.run(["rsvg-convert", "-w", "96", "-h", "96",
                            "-o", str(stem.with_suffix(".png")), str(stem.with_suffix(".svg"))],
                           check=True)
            stem.with_suffix(".svg").unlink()
    print("wrote", len(FAMILIES) * 2, "jack images")


if __name__ == "__main__":
    main()
