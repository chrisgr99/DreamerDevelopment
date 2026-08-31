#!/usr/bin/env python3
"""Renders the manual's animations from the scenes in clips.py.

  python3 docs/images/make-clips.py

The cable swatches for the colour table are here too, since they are stills of the same shapes.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import rackart as art
import clips


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
        art.render(art.svg(w, h, body), HERE / f"cable-{name}.png", width=w)


def gif(clip):
    frames = HERE / f"_{clip.name}"
    frames.mkdir(exist_ok=True)
    for i, scene in enumerate(clip.scenes, start=1):
        art.render(scene, frames / f"{i:03d}.png", width=clip.width)
    art.animate(frames, HERE / f"{clip.name}.gif", clip.fps)


if __name__ == "__main__":
    swatches()
    for build in (clips.flow, clips.replug, clips.trace, clips.knobs,
                  clips.pull, clips.new_cable, clips.pinch,
                  clips.pick_two, clips.new_from_two):
        gif(build())
    print("wrote the swatches and every animation")
