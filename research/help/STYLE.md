# How a help entry is written

Every entry is read aloud, one line at a time, by somebody who clicked that line. That is the whole of the design brief, and every rule below follows from it.

## Shape

An entry is a list of lines. The first line says what the module is. Every line after it is one port, one knob, one switch or one group of them, led by the name printed on the panel.

Menu options go last, on a line starting `Menu — `, and only where they matter to using the module.

One line per control. There is no target length: a module with four jacks gets four lines and a module with fifteen controls gets fifteen. Length is not a cost here, because nobody reads the entry straight through — each line is clicked separately, and a line that covers two controls cannot be clicked for one of them. Group only where controls are genuinely identical, such as eight outputs that differ by number alone.

## A line is about the one control that was clicked

The reader clicked one jack, not the row it is in. So the line answers for that one: `The delayed signal at half the time set by TIME`, never `Three jacks carrying the delayed signal at a half, a third and a quarter`.

This is the most common way an entry goes wrong, because a manual describes a row in one sentence and copying its shape is the path of least resistance. Model277 has four output jacks — t, t/2, t/3 and t/4 — and an entry that gave t its own line and grouped the other three recited the group to somebody who had clicked one of them, and gave the wrong count as well.

So: no counting the controls, and no reciting a row. Where several controls really do the same thing, write the line as though about the one that was clicked and let the tags point all of them at it: `An attenuverter, -1 to +1, setting the level of the input on that row`, `One of the four inputs into the delay line`.

Counts are fine where they are a fact about the module rather than a list of what was clicked — "all four outputs carry the same shape" on a wave selector, "the sum, difference, maximum and minimum of its two inputs" on an arithmetic section.

## Every line stands alone

A line is heard on its own, with nothing before it and nothing after it. So:

- No word pointing at something the reader cannot see. Not "still", not "also", not "again", not "as VCO does", not "the same as above". If a module is a cut-down version of another, say what this one has, not what it lacks.
- No "up to the knob position" or any other sideways way of saying a thing. If the knob attenuates the CV, write that the knob attenuates the CV.
- Name the control outright and put its panel name first: `PHS — one per output, moving that output away from the fundamental`.

A line describing a group of identical controls — one per stage, one per band — is written as though about the one that was clicked: `PHS — where that wave sits in the mix`, not `PHS — four knobs, one per wave`.

## Never say where a control is

A line is reached by clicking the control it describes, so the reader is already pointing at it. "The large knob at the top", "the small knob under TIME", "the four columns down the right" — all of that is spent words, and on a spoken line the words are the whole cost.

Say what it does, not where it sits. `Sets the pitch, marked in volts; 0V gives C4 at 261.63Hz`, not `Large knob at the top, marked in volts — sets the pitch`.

The exception is a module's position in the rack, which is not about finding a control: an expander does say that it goes immediately to the right of the module it expands.

## Controls with no label, and labels used twice

Some controls have no name printed on them. Those lines simply start with what the control does, with no label and no location: `Sets the pitch, marked in volts`, `Modulates the width of the square wave`.

Some panels print the same word on two or three different things — GATE as a knob, an input and an output. Add the one word that separates them: `GATE knob`, `GATE input`, `GATE output`. The stand-alone rule wins over the bare-label rule, always: a line heard on its own must say which control it is about.

## A line for something that is not a control

A fact worth knowing that is not attached to any control goes on a line starting `Note — `. Use it rarely, and only for something that changes how the module is used — for example, that right-clicking a knob lets an exact value be typed, which is how a sample-accurate delay is set.

## Never name the control either

A line is reached by clicking the control, and the note shows that control's name as its heading. So the line itself does not repeat it: write `Saturation on the saw wave`, not `SAT — saturation on the saw wave`.

The first line of an entry is the exception, because it is about the module rather than a control. So are lines starting `Menu — ` and `Note — `, where the prefix says where the thing lives rather than which control it is.

Where a label genuinely belongs inside a line — because the line mentions a different control, as in "V/OCT is added to it" — leave it. The rule is about the lead, not about the word.

## The panel is the authority on names

You still need the panel names, for two reasons: they tell you what the control is, and they are what a line refers to when it mentions a control other than the one being described.

Use the label actually printed on the panel, in the case it is printed in. The manual often calls a control something longer than the panel does; the person listening is looking at the panel.

Panel lettering is outlines, not text, so it cannot be read from the SVG. Render the panel and look at it:

    rsvg-convert -w 600 "<plugin res dir>/<Panel>.svg" -o /tmp/panel.png

`census/<Plugin>.json` in this directory has every model's parameter, input and output names as the maker configured them, which is usually enough to know what the controls are; the render settles what they are called.

RENDER EVERY PANEL. It is not a check on the manual, it is a source in its own right: manuals go stale, and the panel is what the listener is sitting in front of. On Bogaudio — a careful maker with a good manual — the manual said ATACK and DECAY where the panel says ATTACK and RELEASE, and DECTECT where the panel says DETECT. Where they disagree the panel wins.

## Voice

Plain, factual, second person where a person is addressed at all. No enthusiasm, no "simply" or "powerful". Concrete verbs. One idea per line.

**Nothing does what only a person does.** A control does not sit, live, listen, want, know, decide, watch or wait. This is not only a matter of taste: the personified verb is almost always standing in for the fact that has not been worked out. "PHS — where that wave sits in the mix" was hiding that PHS sets the phase of that wave, and somebody reading it learned nothing.

When a line resists, it is usually because the source did not say what the control does. Go back to the manual or the panel and find out, or leave the control out. Do not reach for a verb that sounds like an explanation.

Write in our own words. Do not copy the maker's sentences; read the manual, then say what it means.

## Units, because the lines are spoken

Write units exactly like this, whatever case the panel prints them in: `0-10V`, `3HP`, `-36dB`, `100Hz`, `1ms`, `2 seconds`. The plugin rewrites these for the synthesiser, and it matches that spelling. Panel casing governs the NAMES of controls, not the units.

## An expander names the module it expands

An expander's first line says which module it expands and that it sits immediately to the right of it. If one of its controls is completed by a knob on the base module, say so. That is not leaning on another entry — it is what the module actually is.

## When the manual does not say

Leave the control out rather than guess. An entry that covers eight of ten controls is useful; one that invents the other two is not.

## Tagging: the index is not the position

A tag says which line covers which jack or knob, by index. **Index order is not panel order, and assuming it is will reverse an entry without looking wrong.** NYSTHI's Model277 numbers its four output jacks from the bottom up: index 3 is the top jack, index 0 the bottom. An entry tagged from the numbering alone told somebody clicking the top jack about the bottom one.

`census/<Plugin>.json` carries `inputPos`, `outputPos` and `paramPos` beside the names — the centre of each control on the panel, as x and y, with y increasing downwards. Work out the order from those, then check it against the rendered panel. Never from the index.

Two habits that catch the rest: after tagging, read the tags back as sentences ("the top output is the full delay time — is it?"), and be suspicious whenever a tag map looks like a tidy run of 0, 1, 2, 3.

## Format

`research/help/<Plugin>.json`:

```json
{
  "plugin": "<plugin slug>",
  "source": "<the manual URL that was read>",
  "read": "<YYYY-MM-DD>",
  "modules": {
    "<model slug>": ["first line", "LABEL — what it does", "..."]
  }
}
```

Every model slug in the installed `plugin.json` must be present. Run `python3 research/validate_help.py` before finishing; it checks coverage and the rules above that can be checked mechanically.
