#!/usr/bin/env python3
"""Draws the remaining Clarity animations.

  python3 docs/images/make-features.py

Cable trace assist, where a tangle becomes one legible lead, and the knob face being turned.
Both are changes over time, which is the only reason to spend an animation on them.
"""
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import rackart as art


HERE = Path(__file__).parent
W, H = 460, 300


def trace():
    """Six cables crossing; the pointer reaches one end, a pill swells out of it, and a click
    leaves that cable alone on the screen. A second click brings the rest back."""
    fps = 12
    audio = art.FAMILIES["audio"][0]
    cv = art.FAMILIES["cv"][0]
    gate = art.FAMILIES["trigger"][0]
    pitch = art.FAMILIES["pitch"][0]

    # Six leads across two rows of jacks, deliberately crossing.
    outs = [(60.0, 60.0), (60.0, 120.0), (60.0, 180.0)]
    ins = [(400.0, 60.0), (400.0, 120.0), (400.0, 180.0)]
    cables = [
        (outs[0], ins[2], cv),
        (outs[1], ins[0], gate),
        (outs[2], ins[1], pitch),
        (outs[0], ins[1], audio),
        (outs[2], ins[0], cv),
        (outs[1], ins[2], audio),
    ]
    traced = 1          # The gate lead, which crosses the others twice.

    frames = HERE / "_trace"
    frames.mkdir(exist_ok=True)

    def scene(focus, show_pill, pointer_at, click=False):
        parts = []
        for p in outs:
            parts.append(art.jack(p[0], p[1], 18, "#8a909a", True))
        for p in ins:
            parts.append(art.jack(p[0], p[1], 18, "#8a909a", False))
        for i, (p0, p1, colour) in enumerate(cables):
            # Focused: every other cable is HIDDEN, not merely dimmed — the point is a screen
            # with one lead on it.
            if focus is not None and i != focus:
                continue
            # Rack draws cables at half opacity by default, and the pill at full — which is
            # what makes a pill stand out from the cable it belongs to. The lit cable goes to
            # full strength, as trace assist puts the global opacity up while it holds one.
            parts.append(art.cable(p0, p1, colour, opacity=1.0 if focus is not None else 0.55))
            parts.append(art.plug(p0[0], p0[1], colour))
            parts.append(art.plug(p1[0], p1[1], colour))
        if show_pill:
            p0, p1, colour = cables[traced]
            parts.append(art.pill(p0, p1, colour, lit=(focus is not None), at_input=True))
        if pointer_at:
            parts.append(art.pointer(pointer_at[0], pointer_at[1], click=click))
        return art.svg(W, H, "\n  ".join(parts))

    # Where the pill sits, so the pointer can arrive at it.
    p0, p1, _ = cables[traced]
    ctrl = art.slump(p0, p1)
    pts, cum = art.curve_points(p0, ctrl, p1)
    total = cum[-1]
    pill_at = art.point_on(pts, cum, total - 22.0)   # The middle of where the pill is drawn.

    # The timing is the whole difficulty here. A pill is a short fat piece of cable in the
    # cable's own colour, which is subtle by design — it has to be, since it appears under the
    # pointer while you are looking at something else. On a loop that means it needs LONGER
    # than feels necessary, or a reader watching the tangle misses it entirely and sees the
    # cables vanish for no reason.
    shots = []
    shots += [scene(None, False, (pill_at[0] - 110, pill_at[1] + 70))] * 6    # The tangle.
    steps = 8
    for i in range(1, steps + 1):                                             # Reaching in.
        t = i / steps
        shots.append(scene(None, False,
                           (pill_at[0] - 110 * (1 - t), pill_at[1] + 70 * (1 - t))))
    shots += [scene(None, True, pill_at)] * 12                                # The pill, held.
    shots += [scene(None, True, pill_at, click=True)] * 2                     # The click.
    shots += [scene(traced, True, pill_at, click=True)] * 2
    shots += [scene(traced, True, pill_at)] * 14                              # One lead.
    shots += [scene(None, True, pill_at, click=True)] * 3                     # Clicked again.
    shots += [scene(None, False, (pill_at[0] - 40, pill_at[1] + 30))] * 6     # Back.

    for i, shot in enumerate(shots, start=1):
        art.render(shot, frames / f"{i:03d}.png", width=W)
    art.animate(frames, HERE / "cable-trace.gif", fps)


def knobs():
    """One knob face, turned. Rack knobs are turned by dragging, so the pointer holds still
    with the button down while the knob moves under it — which is what a knob drag looks
    like, and would be a lie if the pointer swept round in a circle."""
    fps = 12
    w, h = 300, 190
    frames = HERE / "_knob"
    frames.mkdir(exist_ok=True)

    lo, hi = -135.0, 135.0
    shots = []
    for i in range(26):
        # Ease in and out, so it reads as a hand rather than a motor.
        t = i / 25
        eased = 0.5 - 0.5 * math.cos(math.pi * t)
        angle = lo + (hi - lo) * eased
        body = "\n  ".join([
            art.knob(150, 88, 46, angle),
            art.pointer(150 + 30, 88 + 18, held=True),
        ])
        shots.append(art.svg(w, h, body))
    shots += [shots[-1]] * 6

    for i, shot in enumerate(shots, start=1):
        art.render(shot, frames / f"{i:03d}.png", width=w)
    art.animate(frames, HERE / "knob-turn.gif", fps)


if __name__ == "__main__":
    trace()
    knobs()
    print("wrote cable-trace.gif and knob-turn.gif")
