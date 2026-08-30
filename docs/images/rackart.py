"""The shapes the plugin draws, in SVG, for the pictures in the manuals.

The geometry here is a copy of what src/Modules.cpp draws: the jack's proportions, the cable's
sag, the dash rhythm and the crawl speed. A copy can drift, so keep the constants together at
the top and check them against the source when the drawing changes — that is cheaper than
screenshots, which drift silently and cannot be redrawn at all.
"""
import math
import subprocess
from pathlib import Path

# ---- The signal families, from familyColor() ----
FAMILIES = {
    "audio": ("#f3c40b", "Audio", 1.6),
    "cv": ("#ff7300", "CV", 3.4),
    "trigger": ("#5aa0e6", "Gate and trigger", 5.6),
    "pitch": ("#39a85a", "Pitch", 3.4),
}

PANEL_BG = "#161a20"
PLUG_DARK = "#2f2f33"
POINTER_GREEN = "#3de07a"

# ---- Cables, from drawFlowDashes() and cableSlump() ----
CABLE_W = 6.0            # The thickness the dashes are keyed to.
DASH_GAP = 2.6           # Gap, in cable widths.
CRAWL_PX_PER_S = 5.5 * (75.0 / 25.4)    # 5.5 mm/s at Rack's 75 DPI.
TENSION = 0.5            # settings::cableTension, Rack's default.

# ---- Jacks, from drawJack() ----
HOLE = 0.53              # Hole radius as a fraction of the jack's.


def jack(cx, cy, r, colour, is_output):
    """One jack: coloured ring, dark hole, and a dashed ring hugging the edge that says which
    way the signal goes — the outer edge for an output, the hole for an input."""
    rh = r * HOLE
    band = r - rh
    w = band / 3.0
    ring = (r * 0.95 - w / 2.0) if is_output else (rh + w / 2.0)
    n = max(6, round(2.0 * math.pi * ring / (w * 1.6)))
    step = 2.0 * math.pi / n

    parts = [
        f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{colour}" stroke="#000" '
        f'stroke-opacity="0.78" stroke-width="{r * 0.1:.2f}"/>',
        f'<circle cx="{cx}" cy="{cy}" r="{rh:.2f}" fill="{PLUG_DARK}"/>',
    ]
    for i in range(n):
        a0, a1 = i * step, i * step + step / 2.0
        x0, y0 = cx + ring * math.cos(a0), cy + ring * math.sin(a0)
        x1, y1 = cx + ring * math.cos(a1), cy + ring * math.sin(a1)
        parts.append(f'<path d="M {x0:.2f} {y0:.2f} A {ring:.2f} {ring:.2f} 0 0 1 '
                     f'{x1:.2f} {y1:.2f}" fill="none" stroke="#000" '
                     f'stroke-width="{w:.2f}" stroke-linecap="butt"/>')
    return "\n  ".join(parts)


def slump(p0, p1):
    """Where a cable hangs, from cableSlump(): the midpoint, dropped by an amount that grows
    with the distance between the ends."""
    dist = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
    return ((p0[0] + p1[0]) / 2.0,
            (p0[1] + p1[1]) / 2.0 + (1.0 - TENSION) * (150.0 + dist))


def curve_points(p0, ctrl, p1, samples=60):
    pts, cum = [p0], [0.0]
    for i in range(1, samples + 1):
        t = i / samples
        u = 1.0 - t
        x = p0[0] * u * u + ctrl[0] * 2 * u * t + p1[0] * t * t
        y = p0[1] * u * u + ctrl[1] * 2 * u * t + p1[1] * t * t
        pts.append((x, y))
        cum.append(cum[-1] + math.hypot(x - pts[-2][0], y - pts[-2][1]))
    return pts, cum


def cable(p0, p1, colour, thickness=CABLE_W, ctrl=None):
    """The lead itself: a dark outline with the coloured core inside it.

    `ctrl` overrides where it hangs. The swatches use a shallow curve rather than the real
    sag: a swatch is there to show a colour and a dash rhythm, and at swatch size the true
    hang is a deep U that leaves the interesting part off the bottom of the picture."""
    if ctrl is None:
        ctrl = slump(p0, p1)
    d = (f'M {p0[0]:.1f} {p0[1]:.1f} Q {ctrl[0]:.1f} {ctrl[1]:.1f} {p1[0]:.1f} {p1[1]:.1f}')
    return (f'<path d="{d}" fill="none" stroke="#0b0d10" stroke-width="{thickness + 3:.1f}" '
            f'stroke-linecap="round"/>\n  '
            f'<path d="{d}" fill="none" stroke="{colour}" stroke-width="{thickness:.1f}" '
            f'stroke-linecap="round"/>')


def flow_dashes(p0, p1, dash_units, time_s, thickness=CABLE_W, ctrl=None):
    """The marching ants, from drawFlowDashes(): black dashes crawling source to destination,
    their length keyed to the destination's family."""
    if ctrl is None:
        ctrl = slump(p0, p1)
    pts, cum = curve_points(p0, ctrl, p1)
    total = cum[-1]
    dash = dash_units * thickness
    gap = DASH_GAP * thickness
    period = dash + gap
    phase = math.fmod(time_s * CRAWL_PX_PER_S, period)

    def point_at(s):
        if s <= 0:
            return pts[0]
        if s >= total:
            return pts[-1]
        i = 1
        while i < len(cum) - 1 and cum[i] < s:
            i += 1
        seg = cum[i] - cum[i - 1]
        f = (s - cum[i - 1]) / seg if seg > 0 else 0.0
        return (pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * f,
                pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * f)

    out = []
    start = phase - period
    while start < total:
        a, b = max(0.0, start), min(total, start + dash)
        start += period
        if b <= a:
            continue
        d = [f'M {point_at(a)[0]:.1f} {point_at(a)[1]:.1f}']
        for i in range(1, len(pts)):
            if a < cum[i] < b:
                d.append(f'L {pts[i][0]:.1f} {pts[i][1]:.1f}')
        d.append(f'L {point_at(b)[0]:.1f} {point_at(b)[1]:.1f}')
        out.append(f'<path d="{" ".join(d)}" fill="none" stroke="#000" '
                   f'stroke-width="{thickness / 2:.1f}" stroke-linecap="butt"/>')
    return "\n  ".join(out)


def plug(cx, cy, colour, r=9.0):
    return (f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r}" fill="{colour}" '
            f'stroke="#0b0d10" stroke-width="2"/>')


def pointer(x, y, held=False, click=False):
    """The drawn pointer, as the recording aid draws it: an arrow, a halo while the button is
    down, and a ring at the moment of a click."""
    parts = []
    if held:
        parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="13" fill="{POINTER_GREEN}" '
                     f'fill-opacity="0.28" stroke="{POINTER_GREEN}" stroke-opacity="0.9" '
                     f'stroke-width="1.6"/>')
    if click:
        parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="20" fill="none" '
                     f'stroke="{POINTER_GREEN}" stroke-opacity="0.8" stroke-width="2.5"/>')
    arrow = (f'M {x:.1f} {y:.1f} L {x:.1f} {y + 20:.1f} L {x + 5.5:.1f} {y + 14.5:.1f} '
             f'L {x + 9:.1f} {y + 22:.1f} L {x + 13:.1f} {y + 20:.1f} '
             f'L {x + 9.5:.1f} {y + 12.5:.1f} L {x + 17:.1f} {y + 12:.1f} Z')
    parts.append(f'<path d="{arrow}" fill="#fff" stroke="#000" stroke-width="1.4" '
                 f'stroke-linejoin="round"/>')
    return "\n  ".join(parts)


def pill(p0, p1, colour, lit=False, at_input=True):
    """The pill that swells out of a cable end, from CableFocus.cpp: a short fat round-capped
    stroke in the cable's colour, with a white ring on the one being traced."""
    ctrl = slump(p0, p1)
    pts, cum = curve_points(p0, ctrl, p1)
    total = cum[-1]

    def along(s):
        s = total - s if at_input else s
        i = 1
        while i < len(cum) - 1 and cum[i] < s:
            i += 1
        seg = cum[i] - cum[i - 1]
        f = (s - cum[i - 1]) / seg if seg > 0 else 0.0
        return (pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * f,
                pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * f)

    a, b = along(16.0), along(28.0)
    out = [f'<path d="M {a[0]:.1f} {a[1]:.1f} L {b[0]:.1f} {b[1]:.1f}" fill="none" '
           f'stroke="{colour}" stroke-width="11" stroke-linecap="round"/>',
           f'<path d="M {a[0]:.1f} {a[1]:.1f} L {b[0]:.1f} {b[1]:.1f}" fill="none" '
           f'stroke="#000" stroke-opacity="0.47" stroke-width="1.2" stroke-linecap="round"/>']
    if lit:
        out.append(f'<circle cx="{(a[0] + b[0]) / 2:.1f}" cy="{(a[1] + b[1]) / 2:.1f}" '
                   f'r="8.25" fill="none" stroke="#fff" stroke-width="1.4"/>')
    return "\n  ".join(out)


def knob(cx, cy, r, angle_deg, ticks=7):
    """The knob face, from druiDrawKnob(): a blue shaded rim, a brushed cap, and a pointer
    with its grip ticks, all turning together."""
    cap = r * 0.72
    gid = f"k{int(cx)}{int(cy)}"
    parts = [
        f'<defs>'
        f'<radialGradient id="{gid}rim" cx="50%" cy="50%" r="50%">'
        f'<stop offset="55%" stop-color="#006da8"/><stop offset="100%" stop-color="#003d62"/>'
        f'</radialGradient>'
        f'<radialGradient id="{gid}core" cx="50%" cy="50%" r="50%">'
        f'<stop offset="0%" stop-color="#1688cc"/><stop offset="100%" stop-color="#006da8"/>'
        f'</radialGradient>'
        f'<radialGradient id="{gid}cap" cx="50%" cy="50%" r="50%">'
        f'<stop offset="40%" stop-color="#4c5058"/><stop offset="62%" stop-color="#5a5f67"/>'
        f'<stop offset="100%" stop-color="#6b7079"/>'
        f'</radialGradient></defs>',
        f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="url(#{gid}rim)"/>',
        f'<circle cx="{cx}" cy="{cy}" r="{r * 0.55:.1f}" fill="url(#{gid}core)"/>',
        f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="none" stroke="#6fa8d6" '
        f'stroke-width="{r * 0.077:.2f}"/>',
        f'<circle cx="{cx}" cy="{cy}" r="{cap:.1f}" fill="url(#{gid}cap)"/>',
        f'<circle cx="{cx}" cy="{cy}" r="{cap:.1f}" fill="none" stroke="#b8b8bc" '
        f'stroke-width="{r * 0.06:.2f}"/>',
    ]
    inner = [f'<line x1="0" y1="0" x2="0" y2="{-cap:.1f}" stroke="#b8b8bc" '
             f'stroke-width="{r * 0.12:.2f}" stroke-linecap="round"/>']
    for k in range(ticks):
        frac = 0.5 if ticks == 1 else k / (ticks - 1)
        a = (frac - 0.5) * 2.0 * math.radians(150.0)
        inner.append(f'<line x1="{math.sin(a) * r * 0.78:.2f}" y1="{-math.cos(a) * r * 0.78:.2f}" '
                     f'x2="{math.sin(a) * r * 1.04:.2f}" y2="{-math.cos(a) * r * 1.04:.2f}" '
                     f'stroke="#fff" stroke-width="{r * 0.1:.2f}"/>')
    parts.append(f'<g transform="translate({cx} {cy}) rotate({angle_deg:.1f})">'
                 + "".join(inner) + '</g>')
    return "\n  ".join(parts)


def label(x, y, text, size=15, colour="#e6e8ec", anchor="middle"):
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="Helvetica, Arial, sans-serif" '
            f'font-size="{size}" fill="{colour}" text-anchor="{anchor}">{text}</text>')


def svg(width, height, body, background=PANEL_BG):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" '
            f'width="{width}" height="{height}">\n'
            f'  <rect width="{width}" height="{height}" fill="{background}"/>\n'
            f'  {body}\n</svg>\n')


def render(svg_text, out_png, width=None):
    tmp = Path(out_png).with_suffix(".tmp.svg")
    tmp.write_text(svg_text)
    cmd = ["rsvg-convert", "-o", str(out_png)]
    if width:
        cmd += ["-w", str(width)]
    subprocess.run(cmd + [str(tmp)], check=True)
    tmp.unlink()


def animate(frames_dir, out_gif, fps):
    """Frames to GIF, through a palette generated from the frames themselves — the default
    palette turns these few flat colours into mud."""
    palette = frames_dir / "palette.png"
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", str(fps),
                    "-i", str(frames_dir / "%03d.png"),
                    "-vf", "palettegen=stats_mode=diff", str(palette)], check=True)
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", str(fps),
                    "-i", str(frames_dir / "%03d.png"), "-i", str(palette),
                    "-lavfi", "paletteuse=dither=none", "-loop", "0", str(out_gif)], check=True)
    for f in frames_dir.glob("*.png"):
        f.unlink()
    frames_dir.rmdir()
