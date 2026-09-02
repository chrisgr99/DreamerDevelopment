"""The scenes, as frames.

Both outputs come from here: the manual's animations, rendered small, and the video, rendered
at whatever size it wants. A builder returns its frames rather than rendering them, so the two
cannot drift apart — there is one description of what happens and two ways of drawing it.
"""
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import rackart as art


HERE = Path(__file__).parent
W, H = 460, 300


class Clip:
    """A named run of frames, with the rate they were timed for and the canvas they were drawn
    on, so a renderer can work out how to fit them."""

    def __init__(self, name, scenes, fps, width, height):
        self.name = name
        self.scenes = scenes
        self.fps = fps
        self.width = width
        self.height = height

    @property
    def still(self):
        """Whether nothing moves. A still is held for exactly as long as the words take; a
        moving clip can only be lengthened a whole pass at a time, and rounding a five second
        loop up to cover a six second sentence wastes four seconds saying nothing."""
        return len(set(self.scenes)) <= 1


def trace_parts():
    """Six cables crossing; the pointer reaches one end, a handle swells out of it, a click
    leaves that cable alone on the screen, and a click on a panel brings the rest back.

    Returned as three clips, one per moment — see the note beside the frames below."""
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


    # A module face behind each column. The way out of a trace is a click on a panel, and
    # without panels the picture had nowhere to make that click.
    panels = [
        f'<rect x="18" y="24" width="84" height="196" rx="4" fill="#20242c" '
        f'stroke="#2c313a" stroke-width="1.5"/>',
        f'<rect x="358" y="24" width="84" height="196" rx="4" fill="#20242c" '
        f'stroke="#2c313a" stroke-width="1.5"/>',
    ]

    def scene(focus, show_pill, pointer_at, click=False, caption=False):
        parts = list(panels)
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
        if caption == "restore":
            parts.append(art.label(W / 2.0, 34, "Click a module panel to bring", 15))
            parts.append(art.label(W / 2.0, 53, "the other cables back", 15))
        elif caption:
            # Named for what it does rather than what it looks like: someone meeting this for
            # the first time has no reason to call a short fat piece of cable a pill.
            parts.append(art.label(330, 30, 'Click the "cable view"', 15, anchor="end"))
            parts.append(art.label(330, 49, 'button that appears', 15, anchor="end"))
            parts.append(art.arrow(340, 44, pill_at[0] - 10, pill_at[1] - 12))
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
    # THREE MOMENTS, kept apart. Reaching the handle, isolating the cable, and putting the
    # others back are three things to say, and a single run of frames could only be repeated
    # whole — so each sentence played the entire animation again. Split, each sentence lands
    # on the part it describes.
    panel_at = (62.0, 244.0)

    reach = [scene(None, False, (pill_at[0] - 110, pill_at[1] + 70))] * 6     # The tangle.
    steps = 8
    for i in range(1, steps + 1):                                             # Reaching in.
        t = i / steps
        reach.append(scene(None, False,
                           (pill_at[0] - 110 * (1 - t), pill_at[1] + 70 * (1 - t))))
    reach += [scene(None, True, pill_at, caption=True)] * 12                  # The handle.

    isolate = [scene(None, True, pill_at, caption=True)] * 6
    isolate += [scene(None, True, pill_at, click=True, caption=True)] * 2      # The click.
    isolate += [scene(traced, True, pill_at, click=True)] * 2
    isolate += [scene(traced, True, pill_at)] * 16                             # One lead.

    shots = []
    shots += [scene(traced, True, pill_at, caption="restore")] * 20           # The way back.
    for i in range(1, 7):
        t = i / 6
        shots.append(scene(traced, True,
                           (pill_at[0] + (panel_at[0] - pill_at[0]) * t,
                            pill_at[1] + (panel_at[1] - pill_at[1]) * t), caption="restore"))
    shots += [scene(traced, True, panel_at, caption="restore")] * 8
    shots += [scene(traced, True, panel_at, click=True, caption="restore")] * 2
    shots += [scene(None, False, panel_at, click=True)] * 2                   # Clicked.
    shots += [scene(None, False, panel_at)] * 8                               # Back.

    return [Clip("cable-trace-reach", reach, fps, W, H),
            Clip("cable-trace-isolate", isolate, fps, W, H),
            Clip("cable-trace-restore", shots, fps, W, H)]


def knobs():
    """One knob face, turned. Rack knobs are turned by dragging, so the pointer holds still
    with the button down while the knob moves under it — which is what a knob drag looks
    like, and would be a lie if the pointer swept round in a circle."""
    fps = 12
    w, h = 300, 190

    # BACK AND FORTH, twice. One sweep from end to end reads as a demonstration of the travel;
    # a knob worked to and fro reads as somebody using it, which is what the pointer resting on
    # it with the button down is meant to say.
    lo, hi = -135.0, 135.0
    shots = []
    legs = [(lo, hi), (hi, lo), (lo, hi), (hi, lo)]
    for start, end in legs:
        for i in range(14):
            # Eased at both ends, so it reads as a hand rather than a motor.
            t = i / 13
            eased = 0.5 - 0.5 * math.cos(math.pi * t)
            body = "\n  ".join([
                art.knob(150, 88, 46, start + (end - start) * eased),
                art.pointer(150 + 30, 88 + 18, held=True),
            ])
            shots.append(art.svg(w, h, body))

    return Clip("knob-turn", shots, fps, w, h)


def pull():
    """A cable moved from one input to another with two clicks and nothing held down.

    THE POINTER HAS NO HALO while the cable is in flight, and that is the whole picture: the
    halo is what the recording aid draws while a button is down, so its absence is the claim
    being made. Three terminals rather than two, because "the cable" has to be a cable that
    exists — one already patched somewhere, which is the case anyone will actually be in."""
    fps = 12
    w, h = 470, 250
    src = (70.0, 70.0)          # Where the cable comes from.
    old = (250.0, 70.0)         # Where it starts.
    new = (400.0, 70.0)         # Where it ends up.
    colour = art.FAMILIES["audio"][0]


    def scene(end, pointer_at, click=False, caption=None, caption_at=None):
        parts = [
            art.jack(src[0], src[1], 20, colour, True),
            art.jack(old[0], old[1], 20, colour, False),
            art.jack(new[0], new[1], 20, colour, False),
            art.cable(src, end, colour),
            art.plug(src[0], src[1], colour),
            art.plug(end[0], end[1], colour),
        ]
        if caption:
            parts.append(art.label(caption_at[0], caption_at[1], caption, 15))
        parts.append(art.pointer(pointer_at[0], pointer_at[1], click=click))
        return art.svg(w, h, "\n  ".join(parts))

    pick = "Click to pick up the cable"
    drop = "Click to deposit the cable on a terminal"
    pick_at = (old[0] - 10, old[1] - 36)
    drop_at = (w / 2.0 + 20, new[1] - 36)

    shots = []
    # Approaching, with the first instruction up long enough to read.
    for i in range(10):
        t = i / 9
        shots.append(scene(old, (old[0] - 90 * (1 - t), old[1] + 60 * (1 - t)),
                           caption=pick, caption_at=pick_at))
    shots += [scene(old, old, caption=pick, caption_at=pick_at)] * 14
    shots += [scene(old, old, click=True, caption=pick, caption_at=pick_at)] * 3

    # Carried, with nothing held: the pointer has no halo, and the cable follows anyway.
    steps = 10
    for i in range(1, steps + 1):
        t = i / steps
        x = old[0] + (new[0] - old[0]) * t
        y = old[1] + 30 * (t * (1 - t) * 4)
        shots.append(scene((x, y), (x, y), caption=drop, caption_at=drop_at))
    shots += [scene(new, new, caption=drop, caption_at=drop_at)] * 14
    shots += [scene(new, new, click=True, caption=drop, caption_at=drop_at)] * 3
    shots += [scene(new, (new[0] + 40, new[1] + 40))] * 8

    return Clip("cable-pull", shots, fps, w, h)


def new_cable():
    """Starting a cable at an empty terminal, carrying it, and connecting it — the other half
    of the gesture, where there is no cable to pick up until you make one."""
    fps = 12
    w, h = 470, 250
    out = (90.0, 70.0)
    inp = (380.0, 70.0)
    colour = art.FAMILIES["audio"][0]


    def scene(end, pointer_at, click=False, caption=None, caption_at=None):
        parts = [
            art.jack(out[0], out[1], 20, colour, True),
            art.jack(inp[0], inp[1], 20, colour, False),
        ]
        if end is not None:
            parts.append(art.cable(out, end, colour))
            parts.append(art.plug(out[0], out[1], colour))
            parts.append(art.plug(end[0], end[1], colour))
        if caption:
            parts.append(art.label(caption_at[0], caption_at[1], caption, 15))
        parts.append(art.pointer(pointer_at[0], pointer_at[1], click=click))
        return art.svg(w, h, "\n  ".join(parts))

    start = "Click an empty terminal to start a cable"
    finish = "Click another terminal to connect it"
    start_at = (w / 2.0 - 40, 30)
    finish_at = (w / 2.0 + 10, 30)

    shots = []
    for i in range(10):                                   # Approaching an empty jack.
        t = i / 9
        shots.append(scene(None, (out[0] - 70 * (1 - t), out[1] + 60 * (1 - t)),
                           caption=start, caption_at=start_at))
    shots += [scene(None, out, caption=start, caption_at=start_at)] * 16
    shots += [scene(None, out, click=True, caption=start, caption_at=start_at)] * 3

    steps = 10                                            # The new cable follows the pointer.
    for i in range(1, steps + 1):
        t = i / steps
        x = out[0] + (inp[0] - out[0]) * t
        y = out[1] + 34 * (t * (1 - t) * 4)
        shots.append(scene((x, y), (x, y), caption=finish, caption_at=finish_at))
    shots += [scene(inp, inp, caption=finish, caption_at=finish_at)] * 16
    shots += [scene(inp, inp, click=True, caption=finish, caption_at=finish_at)] * 3
    shots += [scene(inp, (inp[0] + 40, inp[1] + 40))] * 8

    return Clip("cable-new", shots, fps, w, h)


def cycle():
    """Clicking the same jack twice: the cable on top, then the one under it, then away.

    Two colours, because the point is WHICH cable is in your hand and two of one colour cannot
    say that. The states tell themselves apart without labels: the cable being held is at full
    strength while the other drops to half, and it runs from its far end to the pointer.
    """
    fps = 12
    w, h = 470, 330
    out = (110.0, 70.0)
    in_top = (400.0, 70.0)
    in_low = (400.0, 190.0)
    audio = art.FAMILIES["audio"][0]
    gate = art.FAMILIES["trigger"][0]


    def scene(held, caption, at=None, click=False):
        """held: None, "top" or "low"."""
        at = at or out
        parts = [
            art.jack(out[0], out[1], 20, audio, True),
            art.jack(in_top[0], in_top[1], 20, audio, False),
            art.jack(in_low[0], in_low[1], 20, gate, False),
        ]
        for name, far, colour in (("top", in_top, audio), ("low", in_low, gate)):
            if held == name:
                # In the hand: it runs from its far end to the pointer, at full strength and
                # with no plug on the end being carried.
                parts.append(art.cable(far, at, colour))
                parts.append(art.plug(far[0], far[1], colour))
            else:
                parts.append(art.cable(out, far, colour, opacity=0.5 if held else 1.0))
                parts.append(art.plug(out[0], out[1], colour))
                parts.append(art.plug(far[0], far[1], colour))
        parts.append(art.label(w / 2.0, 28, caption, 15))
        parts.append(art.pointer(at[0], at[1], click=click))
        return art.svg(w, h, "\n  ".join(parts))

    first = "Click to pick up the top cable"
    second = "Click again to pick up the cable below it"
    away = "Move away and you are carrying it"

    shots = []
    shots += [scene(None, first)] * 26                    # Both cables, nothing held.
    shots += [scene(None, first, click=True)] * 3
    shots += [scene("top", second)] * 26                  # The audio one, still on the port.
    shots += [scene("top", second, click=True)] * 3
    shots += [scene("low", second)] * 8                   # Swapped for the gate one.
    shots += [scene("low", away)] * 18

    # Carried off the jack, which is what ends the cycle: from here the next click puts it
    # down somewhere.
    steps = 10
    for i in range(1, steps + 1):
        t = i / steps
        pos = (out[0] + 150.0 * t, out[1] + 90.0 * t)
        shots.append(scene("low", away, at=pos))
    shots += [scene("low", away, at=(out[0] + 150.0, out[1] + 90.0))] * 12

    return Clip("cable-cycle", shots, fps, w, h)


def stack():
    """Starting a SECOND cable at an output that already has one.

    This is the case the old gesture could not reach at all: a click took the cable that was
    there and there was no way to ask for another. One more click is the whole answer.
    """
    fps = 12
    w, h = 470, 330
    out = (110.0, 70.0)
    in_top = (400.0, 70.0)
    in_low = (400.0, 200.0)
    colour = art.FAMILIES["audio"][0]


    def scene(state, caption, at=None, click=False):
        """state: None, "held" or "new"."""
        # Low on the jack while a new cable is being made: its loose end is wherever the
        # pointer is, so at the jack's centre it would have no length and nothing to see.
        at = at or ((out[0], out[1] + 15.0) if state == "new" else out)
        parts = [
            art.jack(out[0], out[1], 20, colour, True),
            art.jack(in_top[0], in_top[1], 20, colour, False),
            art.jack(in_low[0], in_low[1], 20, colour, False),
        ]
        if state == "held":
            parts.append(art.cable(in_top, at, colour))
            parts.append(art.plug(in_top[0], in_top[1], colour))
        else:
            parts.append(art.cable(out, in_top, colour, opacity=0.5 if state else 1.0))
            parts.append(art.plug(out[0], out[1], colour))
            parts.append(art.plug(in_top[0], in_top[1], colour))
        if state == "new":
            parts.append(art.cable(out, at, colour))
            parts.append(art.plug(out[0], out[1], colour))
        parts.append(art.label(w / 2.0, 28, caption, 15))
        parts.append(art.pointer(at[0], at[1], click=click))
        return art.svg(w, h, "\n  ".join(parts))

    first = "Click to pick up the cable"
    second = "Click again to get a new cable"
    third = "Carry it and click to connect it"

    shots = []
    shots += [scene(None, first)] * 26
    shots += [scene(None, first, click=True)] * 3
    shots += [scene("held", second)] * 26
    shots += [scene("held", second, click=True)] * 3
    shots += [scene("new", third)] * 18                   # A new cable, hanging from the jack.

    steps = 10                                            # Carried down to the free input.
    start = (out[0], out[1] + 15.0)
    for i in range(1, steps + 1):
        t = i / steps
        shots.append(scene("new", third,
                           at=(start[0] + (in_low[0] - start[0]) * t,
                               start[1] + (in_low[1] - start[1]) * t)))
    shots += [scene("new", third, at=in_low, click=True)] * 3

    # Connected: two cables out of the one jack, which is what this was for.
    def done(caption):
        parts = [
            art.jack(out[0], out[1], 20, colour, True),
            art.jack(in_top[0], in_top[1], 20, colour, False),
            art.jack(in_low[0], in_low[1], 20, colour, False),
            art.cable(out, in_top, colour), art.plug(out[0], out[1], colour),
            art.plug(in_top[0], in_top[1], colour),
            art.cable(out, in_low, colour), art.plug(in_low[0], in_low[1], colour),
            art.label(w / 2.0, 28, caption, 15),
            art.pointer(in_low[0] + 30, in_low[1] + 30),
        ]
        return art.svg(w, h, "\n  ".join(parts))

    shots += [done("Two cables out of the one jack")] * 16

    return Clip("cable-stack", shots, fps, w, h)


def replug():
    """The pointer takes a cable off an audio input and drops it on a gate input, and the cable
    changes to the gate colour: the colour belongs to where a cable GOES."""
    w, h = 460, 300
    fps = 12
    src = (70.0, 72.0)          # The audio output it comes from.
    old = (230.0, 72.0)         # The audio input it starts in.
    new = (390.0, 72.0)         # The gate input it ends in.
    audio = art.FAMILIES["audio"][0]
    gate = art.FAMILIES["trigger"][0]


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

    return Clip("cable-replug", shots, fps, w, h)


def flow():
    """The dashes crawling from an output to an input. The frame count is chosen so the crawl
    travels exactly one dash period over the loop, which makes it seamless."""
    w, h = 460, 300
    fps = 12
    p0, p1 = (90.0, 72.0), (370.0, 72.0)
    colour, _title, dash_units = art.FAMILIES["cv"]
    period = dash_units * art.CABLE_W + art.DASH_GAP * art.CABLE_W
    count = max(8, round(period / (art.CRAWL_PX_PER_S / fps)))

    shots = []
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
        shots.append(art.svg(w, h, body))
    return Clip("cable-flow", shots, fps, w, h)


def pinch():
    """Two fingers on the rack, and the rack growing between them.

    The picture here is a real screenshot rather than drawn shapes, because the thing being
    demonstrated happens to whatever is on screen — a drawing of a rack zooming would be a
    drawing of our own artwork, which proves nothing.
    """
    fps = 12
    w, h = 470, 264
    shot = art.embedded(HERE / "clarity-knobs.jpg")
    # The rack sits still while the view grows around the point between the fingers.
    centre = (w / 2.0, h / 2.0)

    def scene(scale, spread, caption):
        # The image is drawn at the canvas size and scaled about the midpoint between the
        # fingers, which is what a pinch zooms about.
        tx = centre[0] - centre[0] * scale
        ty = centre[1] - centre[1] * scale
        parts = [
            f'<g transform="translate({tx:.1f} {ty:.1f}) scale({scale:.3f})">'
            f'<image href="{shot}" x="0" y="0" width="{w}" height="{h}" '
            f'preserveAspectRatio="xMidYMid slice"/></g>',
        ]
        for sign in (-1.0, 1.0):
            # SIDE BY SIDE. Nobody pinches on a diagonal; two fingers on a trackpad move apart
            # roughly horizontally, and drawing it otherwise makes the gesture look awkward.
            fx = centre[0] + sign * spread
            fy = centre[1]
            parts.append(f'<circle cx="{fx:.1f}" cy="{fy:.1f}" r="17" fill="#ffffff" '
                         f'fill-opacity="0.22" stroke="#ffffff" stroke-opacity="0.9" '
                         f'stroke-width="2"/>')
        parts.append(f'<rect x="0" y="0" width="{w}" height="34" fill="#0e1116" '
                     f'fill-opacity="0.82"/>')
        parts.append(art.label(w / 2.0, 20, caption, 15))
        return art.svg(w, h, "\n  ".join(parts))

    apart = "Pinch to zoom the rack"
    together = "And pinch back to see more of it"

    shots = []
    shots += [scene(1.0, 46, apart)] * 12
    for i in range(1, 13):                       # Fingers apart, the rack grows.
        t = i / 12
        eased = 0.5 - 0.5 * math.cos(math.pi * t)
        shots.append(scene(1.0 + 0.55 * eased, 46 + 44 * eased, apart))
    shots += [scene(1.55, 90, apart)] * 10
    shots += [scene(1.55, 90, together)] * 8
    for i in range(1, 13):                       # And back together.
        t = i / 12
        eased = 0.5 - 0.5 * math.cos(math.pi * t)
        shots.append(scene(1.55 - 0.75 * eased, 90 - 56 * eased, together))
    shots += [scene(0.8, 34, together)] * 12
    return Clip("pinch-zoom", shots, fps, w, h)


def jack_table():
    """The four families, as an input and an output apiece — the manual's table, moving only
    in the sense that it is held long enough to read."""
    w, h = 470, 300
    rows = [("Audio", "audio"), ("CV", "cv"), ("Gate and trigger", "trigger"), ("Pitch", "pitch")]
    parts = [
        art.label(255, 42, "Input", 15, "#9aa1ac"),
        art.label(350, 42, "Output", 15, "#9aa1ac"),
    ]
    y = 88
    for title, family in rows:
        colour = art.FAMILIES[family][0]
        parts.append(art.label(215, y + 5, title, 15, anchor="end"))
        parts.append(art.jack(255, y, 20, colour, False))
        parts.append(art.jack(350, y, 20, colour, True))
        y += 56
    scene = art.svg(w, h, "\n  ".join(parts))
    return Clip("jack-table", [scene] * 60, 12, w, h)


def cable_swatches():
    """The same four colours as leads, each with its own dash rhythm."""
    w, h = 470, 300
    parts = []
    y = 80
    for family, (colour, title, dash_units) in art.FAMILIES.items():
        p0, p1 = (170.0, float(y)), (430.0, float(y))
        ctrl = ((p0[0] + p1[0]) / 2.0, p0[1] + 26.0)
        parts.append(art.label(150, y + 5, title, 15, anchor="end"))
        parts.append(art.cable(p0, p1, colour, ctrl=ctrl))
        parts.append(art.flow_dashes(p0, p1, dash_units, 0.0, ctrl=ctrl))
        parts.append(art.plug(p0[0], p0[1], colour))
        parts.append(art.plug(p1[0], p1[1], colour))
        y += 58
    scene = art.svg(w, h, "\n  ".join(parts))
    return Clip("cable-swatches", [scene] * 48, 12, w, h)


def rack_shot(name, caption, seconds=6.0):
    """The rack itself, held with a caption over it.

    A photograph rather than drawn shapes, because the claim being made is about what happens
    to OTHER people's modules, which our own drawings cannot demonstrate.
    """
    w, h = 470, 259
    shot = art.embedded(HERE / "clarity-knobs.jpg")
    parts = [
        f'<image href="{shot}" x="0" y="0" width="{w}" height="{h}" '
        f'preserveAspectRatio="xMidYMid meet"/>',
        f'<rect x="0" y="0" width="{w}" height="30" fill="#0e1116" fill-opacity="0.82"/>',
        art.label(w / 2.0, 18, caption, 14),
    ]
    scene = art.svg(w, h, "\n  ".join(parts))
    return Clip(name, [scene] * int(seconds * 12), 12, w, h)


def overview():
    return rack_shot("rack-overview", "A rack running Clarity", 7.0)


def knob_rack():
    return rack_shot("rack-knobs", "One knob face, across every plugin", 6.0)


WIDGETS = ["Scope", "Analyser", "Audio monitor", "Gate", "Pulse", "Clock", "DC level",
           "LFO", "VCO", "Note", "Volt/oct", "Noise", "Attenuverter"]


def widget_card(named):
    """The list of widgets beside a rack with several of them on it.

    One card per name, so the list can be read out with each name arriving as it is said — a
    list that appears whole and is then talked through leaves the eye wandering ahead of the
    voice. The photograph is there because a list of names is only a list of names: beside it,
    the analyser's waterfall, a clock, a pitch source, a monitor and a frozen trace say what
    the words mean.
    """
    w, h = 470, 264
    shot = art.embedded(HERE / "test-gear-widgets.jpg")
    img_w = 300.0
    img_h = img_w * 1134.0 / 1938.0
    parts = [
        f'<image href="{shot}" x="{w - img_w - 14:.0f}" y="{(h - img_h) / 2:.0f}" '
        f'width="{img_w:.0f}" height="{img_h:.0f}"/>',
        art.label(78, 30, "Widgets", 17, "#e6e8ec"),
    ]
    y = 54
    for i, name in enumerate(WIDGETS):
        if i < named - 1:
            colour = "#e6e8ec"
        elif i == named - 1:
            colour = "#3de07a"
        else:
            colour = "#3a4048"
        parts.append(art.label(78, y, name, 13, colour))
        y += 16
    scene = art.svg(w, h, "\n  ".join(parts))
    return Clip(f"widgets-{named}", [scene] * 6, 12, w, h)


def widget_cards():
    """A builder per name, and one at the end with the whole list lit."""
    builders = [(lambda n=i: widget_card(n)) for i in range(1, len(WIDGETS) + 1)]
    builders.append(lambda: widget_card(len(WIDGETS) + 1))
    return builders


def trace():
    """The three moments as one run of frames, for the manual's animation."""
    parts = trace_parts()
    scenes = [s for part in parts for s in part.scenes]
    return Clip("cable-trace", scenes, parts[0].fps, parts[0].width, parts[0].height)


def trace_parts_builders():
    """A builder per moment, so a section can give each one its own sentence."""
    return [(lambda n=i: trace_parts()[n]) for i in range(3)]


def _two_cable_jack():
    """The scene shared by both halves of the several-cables section.

    Both start from the SAME patch — an audio lead and a gate one on one output, and a free
    input below them. Showing the second half on a jack with only one cable made it look as
    though something had to be unplugged before a new cable could be started, which is the
    opposite of what the feature does.
    """
    w, h = 470, 330
    out = (110.0, 60.0)
    in_top = (400.0, 60.0)
    in_low = (400.0, 150.0)
    in_free = (400.0, 250.0)
    audio = art.FAMILIES["audio"][0]
    gate = art.FAMILIES["trigger"][0]

    def scene(held, caption, at=None, click=False, new_end=None):
        """held: None, "top", "low"; new_end: where a newly made cable is being carried."""
        at = at or out
        parts = [
            art.jack(out[0], out[1], 20, audio, True),
            art.jack(in_top[0], in_top[1], 20, audio, False),
            art.jack(in_low[0], in_low[1], 20, gate, False),
            art.jack(in_free[0], in_free[1], 20, audio, False),
        ]
        busy = held is not None or new_end is not None
        for name, far, colour in (("top", in_top, audio), ("low", in_low, gate)):
            if held == name:
                parts.append(art.cable(far, at, colour))
                parts.append(art.plug(far[0], far[1], colour))
            else:
                parts.append(art.cable(out, far, colour, opacity=0.5 if busy else 1.0))
                parts.append(art.plug(out[0], out[1], colour))
                parts.append(art.plug(far[0], far[1], colour))
        if new_end is not None:
            parts.append(art.cable(out, new_end, audio))
            parts.append(art.plug(out[0], out[1], audio))
        parts.append(art.label(w / 2.0, 26, caption, 15))
        parts.append(art.pointer(at[0], at[1], click=click))
        return art.svg(w, h, "\n  ".join(parts))

    return scene, (w, h), out, in_free


def pick_two():
    """Click for the cable on top, click again for the one under it."""
    scene, (w, h), out, _ = _two_cable_jack()
    first = "Click for the cable on top"
    second = "Click again for the one below it"

    shots = [scene(None, first)] * 16
    shots += [scene(None, first, click=True)] * 3
    shots += [scene("top", first)] * 14
    shots += [scene("top", second, click=True)] * 3
    shots += [scene("low", second)] * 18
    return Clip("jack-pick-two", shots, 12, w, h)


def new_from_two():
    """The same jack, clicked past both cables to a new one, which is then connected."""
    scene, (w, h), out, in_free = _two_cable_jack()
    low = "Click again for a new cable"
    carry = "Nothing had to be unplugged"

    # Low on the jack while the new cable exists: its loose end is at the pointer, so at the
    # jack's centre it would have no length to see.
    born = (out[0], out[1] + 15.0)
    shots = [scene(None, low)] * 10
    shots += [scene(None, low, click=True)] * 2
    shots += [scene("top", low)] * 8
    shots += [scene("top", low, click=True)] * 2
    shots += [scene("low", low)] * 8
    shots += [scene("low", low, click=True)] * 2
    shots += [scene(None, low, at=born, new_end=born)] * 12

    steps = 10                                            # Carried down to the free input.
    for i in range(1, steps + 1):
        t = i / steps
        at = (born[0] + (in_free[0] - born[0]) * t, born[1] + (in_free[1] - born[1]) * t)
        shots.append(scene(None, carry, at=at, new_end=at))
    shots += [scene(None, carry, at=in_free, new_end=in_free, click=True)] * 3
    shots += [scene(None, carry, at=(in_free[0] + 34, in_free[1] + 30), new_end=in_free)] * 12
    return Clip("jack-new-from-two", shots, 12, w, h)


def download_card():
    """Where to get it. A still: an address is read, not watched."""
    w, h = 470, 264
    parts = [
        art.label(w / 2.0, 96, "github.com/chrisgr99", 20, "#3de07a"),
        art.label(w / 2.0, 124, "/DreamerDevelopment", 20, "#3de07a"),
        art.label(w / 2.0, 168, "Free, and open source", 14, "#9aa1ac"),
    ]
    scene = art.svg(w, h, "\n  ".join(parts))
    return Clip("download", [scene] * 24, 12, w, h)
