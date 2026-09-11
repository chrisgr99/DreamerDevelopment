# NYSTHI port research

What can be learned about NYSTHI's 149 modules without running Rack, gathered so that port colouring rules can be written against real data rather than guesses. Nothing here is committed to the plugin; it is working material.

## What is here

- `nysthi-ports.md` — the readable report, one section per module.
- `nysthi-ports.json` — the same, as data.
- `panels/` — every panel rendered to PNG at 300 px wide, 137 of them. This is the source of truth for anything positional.
- The scanner that produced them is `scan_nysthi.py`, kept beside this file.

## What each module's entry holds

- Its manifest entry: slug, name, description, tags, panel width in HP.
- Every live `<text>` label on its panel. 39 of the 137 panels still carry text; the rest have it converted to outlines, which is why the renders exist.
- The string literals its code carries, which are candidate parameter and port names. 125 modules have some.
- Whatever NYSTHI's own CHANGELOG.md says about it. 53 modules are mentioned.

## What is NOT here, and why

**The ports themselves.** A port's name lives in `PortInfo`, which exists only once a module is instantiated inside Rack, and the same is true of every port's position on the panel. So this material can say what words a module uses and where its labels sit, but it cannot say which jack is port 3.

That link is what a rule needs, and it needs the in-Rack census: walk the plugin registry, create each model, record every port's index, name, type and position, and every parameter's index, name and position. Until that exists, this is half the map.

**Trustworthy coordinates.** The label positions in the report are computed through each SVG's transform chain, and spot checks against the renders show them ordered correctly but placed wrongly — these are Inkscape documents with deeply nested transforms and off-canvas leftovers. Treat the coordinates as a rough reading order. When a position matters, look at the PNG.

## What the two modules that started this look like

- **Nudger** names no ports. Its panel render shows the layout plainly: five nudge groups, each with one jack and two buttons flanked by `<<` and `>>`, then a `CV ADD` section with five jacks, four of them labelled A, B, C and D. Ten jacks, ten buttons — and the patch file confirms ten parameters.
- **Programmer** names no ports either, but names all its parameters: `Stage select trigger`, `Stage pulse active trigger`, `Channel A Voltage` and so on. That is the pattern the parameter-proximity idea is built on.
