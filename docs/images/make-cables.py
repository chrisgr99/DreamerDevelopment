#!/usr/bin/env python3
"""Draws the cable pictures used in the manuals.

  python3 docs/images/make-cables.py

Three things: a swatch of each family's cable for the colour table, an animation of a cable
being re-plugged and taking its new destination's colour, and an animation of the dashes
crawling from an output to an input.

The two animations exist because neither shows a state — one is a change and the other is a
direction, and a still picture of either is a picture of nothing happening.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import rackart as art


HERE = Path(__file__).parent


def swatches():
    """A short lead in each family's colour, with that family's dash rhythm on it."""
    w, h = 300, 70
    p0, p1 = (28.0, 26.0), (272.0, 26.0)
    ctrl = ((p0[0] + p1[0]) / 2.0, p0[1] + 34.0)     # A shallow curve; see cable().
    for name, (colour, _title, dash_units) in art.FAMILIES.items():
        body = (art.cable(p0, p1, colour, ctrl=ctrl) + "\n  "
                + art.flow_dashes(p0, p1, dash_units, 0.0, ctrl=ctrl) + "\n  "
                + art.plug(p0[0], p0[1], colour) + "\n  "
                + art.plug(p1[0], p1[1], colour))
        art.render(art.svg(w, h, body), HERE / f"cable-{name}.png", width=300)


def replug():
    """The pointer takes a cable off a yellow audio input and drops it on a blue gate input,
    and the cable turns blue: the colour belongs to where a cable GOES."""
    w, h = 460, 300
    fps = 12
    src = (70.0, 72.0)          # The audio output it comes from.
    old = (230.0, 72.0)         # The audio input it starts in.
    new = (390.0, 72.0)         # The gate input it ends in.
    audio = art.FAMILIES["audio"][0]
    gate = art.FAMILIES["trigger"][0]

    frames = HERE / "_replug"
    frames.mkdir(exist_ok=True)

    def scene(end, colour, held, click=False, pointer_at=None):
        parts = [
            art.jack(src[0], src[1], 22, audio, True),
            art.label(src[0], src[1] - 32, "Audio out", 13),
            art.jack(old[0], old[1], 22, audio, False),
            art.label(old[0], old[1] - 32, "Audio in", 13),
            art.jack(new[0], new[1], 22, gate, False),
            art.label(new[0], new[1] - 32, "Gate in", 13),
            art.cable(src, end, colour),
            art.plug(src[0], src[1], colour),
            art.plug(end[0], end[1], colour),
        ]
        if pointer_at:
            parts.append(art.pointer(pointer_at[0], pointer_at[1], held=held, click=click))
        return art.svg(w, h, "\n  ".join(parts))

    shots = []
    # Sitting there, plugged into the audio input.
    shots += [scene(old, audio, False, pointer_at=(old[0] - 40, old[1] + 30))] * 6
    # The pointer arrives and takes the end.
    shots += [scene(old, audio, True, pointer_at=(old[0], old[1]))] * 3
    # Carried across. In flight a cable keeps the colour of the port it LEFT.
    steps = 10
    for i in range(1, steps + 1):
        t = i / steps
        x = old[0] + (new[0] - old[0]) * t
        y = old[1] + 26 * (t * (1 - t) * 4)     # A slight arc, so it reads as carried.
        shots.append(scene((x, y), audio, True, pointer_at=(x, y)))
    # Dropped: the colour changes to the destination's family.
    shots += [scene(new, gate, False, click=True, pointer_at=(new[0], new[1]))] * 3
    shots += [scene(new, gate, False, pointer_at=(new[0] + 40, new[1] + 30))] * 10

    for i, shot in enumerate(shots, start=1):
        art.render(shot, frames / f"{i:03d}.png", width=w)
    art.animate(frames, HERE / "cable-replug.gif", fps)


def flow():
    """The dashes crawling from an output to an input. The frame count is chosen so the crawl
    travels exactly one dash period over the loop, which makes it seamless."""
    w, h = 460, 300
    fps = 12
    p0, p1 = (90.0, 72.0), (370.0, 72.0)
    colour, _title, dash_units = art.FAMILIES["cv"]
    period = dash_units * art.CABLE_W + art.DASH_GAP * art.CABLE_W
    count = max(8, round(period / (art.CRAWL_PX_PER_S / fps)))

    frames = HERE / "_flow"
    frames.mkdir(exist_ok=True)
    for i in range(count):
        t = (i / count) * (period / art.CRAWL_PX_PER_S)
        body = "\n  ".join([
            art.jack(p0[0], p0[1], 22, colour, True),
            art.label(p0[0], p0[1] - 32, "Output", 13),
            art.jack(p1[0], p1[1], 22, colour, False),
            art.label(p1[0], p1[1] - 32, "Input", 13),
            art.cable(p0, p1, colour),
            art.flow_dashes(p0, p1, dash_units, t),
            art.plug(p0[0], p0[1], colour),
            art.plug(p1[0], p1[1], colour),
        ])
        art.render(art.svg(w, h, body), frames / f"{i + 1:03d}.png", width=w)
    art.animate(frames, HERE / "cable-flow.gif", fps)


if __name__ == "__main__":
    swatches()
    replug()
    flow()
    print("wrote the cable swatches, cable-replug.gif and cable-flow.gif")
