# Brief for an agent writing one plugin's help entries

You are writing `research/help/<Plugin>.json` for ONE plugin. Everything you need is in this repository. Work from the project root, `~/ProgrammingProjects/DreamerDevelopment`.

## Read first, in this order

1. `research/help/STYLE.md` — the binding rules for the writing. Every rule there came from a real failure; none is negotiable. Read all of it before writing a line.
2. `research/help/census/<Plugin>.json` — your plugin, as the modules actually are inside Rack: every param, input and output by index, with the maker's own tooltip name, the maker's `description` second line where one exists, and **the x/y position of every control on the panel**.
3. `research/help/Venom.json` — a finished entry in the exact output format. Copy its shape.

## Sources, best first

Use **all** of these, not the first one that answers. Each has been the only source for something real on a previous run.

- **The maker's source code**, where it is on GitHub. It carries ranges, thresholds, defaults and normalling that manuals leave out — and it is the only place that says what a control does when the panel and the manual both just name it. **Match the installed version, not `main`** — check the installed version in `~/Library/Application Support/Rack2/plugins-mac-arm64/<Plugin>/plugin.json`.
- **The panel itself.** The panel beats the manual, and beats the maker's own `configInput` names, whenever they disagree. Three ways to get at it, in order:
  1. Render the SVG from the installed plugin folder: `rsvg-convert -w 400 panel.svg -o /tmp/x.png`. **Then draw the census param/input/output indices on top of the image** — that settles the index-to-jack map beyond doubt, and has already caught controls numbered bottom-up on two plugins in this batch.
  2. **Some makers draw their panels in code, not SVG.** Then `rsvg-convert` has nothing to work on, and the panel lettering is inside each widget's `render()`/`draw()` as text calls. Read it there.
  3. The VCV library screenshots, which are at width 400 and 200, not 800.

  **A plugin's `res/` may hold more than one panel per module, and only one of them is the running one.** OrangeLine ships `*Bright.svg` and `*Work.svg`, and the Work files are an OLDER layout whose control positions do not match the module in the rack. Where there is a choice, check the positions against the census before trusting a panel, and read the widget source to see which file it actually loads.
- **The maker's own tooltips**, which are already in your census slice: the `name` for every control, and the `description` second line where one exists. Only about 800 controls in the whole library have a description, and where one exists it usually says the thing no other source does — normalling, ranges, and what a control does that its name does not convey. **Where we write our own line, it must agree with the maker's tooltip, or improve on it. Never contradict it without a reason you can point to.**
- **Files shipped inside the installed plugin folder.** Look at everything there, not just the binary: `README`, `CHANGELOG`, `docs/`, `manual/`, `res/` text. NYSTHI ships a 204KB `CHANGELOG.md` with control-by-control legends for 17 modules that exist nowhere else — under the maker's OLD module names, so match them up by panel, not by slug.
- `strings` on the plugin binary, for text that is in the code and nowhere else — menu item labels, mode names, error text.
- **The maker's manual or wiki**, which is the weakest source: usually written for a different version, and it omits the numbers. Useful for what a module is FOR; not to be trusted for what a control does.
- Release notes, forum threads and issue trackers, where a control's behaviour is disputed or changed.

**A port or copy of commercial hardware is the hardest case.** The maker's own page is usually marketing, and the hardware manual describes a revision the plugin does not implement. Trust the code, then the panel; quote a range only where the code gives it. Where the plugin plainly differs from the hardware it is named after, that is a fact about this module, not an error to correct.

Where two sources disagree, say so in the module's `notes` field rather than picking silently.

**QUOTE A NUMBER ONLY WHERE YOU HAVE SEEN IT.** On a plugin with published source that means in the source; on a closed one it means in the binary, as a constant you found. A figure from a hardware manual, a product page or a review is not a figure about this module and must never be promoted into a line. Say what the control does without the number, and record in `notes` what you left out and why — a thin line that is true beats a full one that is invented. This is not hypothetical: one entry in this project was written with a list of DSP constants that appear nowhere in the plugin, and it had to be thrown away and rewritten. `research/check_numbers.py` now tests every quoted number against the binary; `research/help/NUMBER-CHECK.md` records what it found.

**One maker's bug is common enough to check for by name.** `configOutput` (or `configInput`, or `configParam`) called twice on the same id, or called inside a loop with the index left out, leaves one control wearing its neighbour's tooltip and another with no name at all. Five plugins in this project have it. When a tooltip looks like it belongs to the control next door, go and read the config calls before believing it.

## The one error no validator can catch

**Index order is not panel order.** A tag says "input 7 is described by line 12", and if you derive that from the numbering you will sometimes get it exactly backwards — NYSTHI's Model277 numbers its output jacks from the bottom up. Use the `inputPos` / `outputPos` / `paramPos` maps in your census slice to work out which jack is which on the panel, every time. y increases downwards.

## What to produce

`research/help/<Plugin>.json`, one object per model:

- `lines` — the first line says what the module is; then one line per control. **No cap on the number of lines.** A module with fifteen controls gets fifteen lines. Menu options last, on lines starting `Menu — `.
- `param`, `in`, `out` — index to line number. A tag points at a line describing THAT control, or at nothing.
- `family.in`, `family.out` — index to `audio`, `cv`, `trigger` or `pitch`, for every jack you are sure about. These colour the ports in Clarity, so a wrong one is worse than a missing one.
- Top level: `plugin`, `source` (the URL you read), `read` (today's date).

## Rules that get broken most often

- A line never names where a control is, and never recites a row. The reader clicked ONE control.
- Every line stands alone — no "also", "still", "as above", "the same as".
- Nothing does what only a person does. No personification.
- Never say a relationship sideways. If the knob attenuates the CV, write that.
- **Leave it out rather than guess** — but only where you are guessing. Two different cases:
  - **You cannot tell what it does.** No line, no tag. Silence beats invention. Record it in `notes`.
  - **You have PROVED it does nothing.** That is knowledge, and the reader needs it more than anything else on that panel. Write the line and tag it: `Nothing in the module reads this control; the boost CV is applied at a fixed 20dB per volt`. This applies to any control that is ON THE PANEL and can be clicked. A control declared in code with no widget, or hidden under another control at the same position, can never be clicked and still gets no line.
- **Say it in the control's own line when we contradict the maker.** Where the panel, the tooltip or the manual disagrees with the code, the line describes the code AND ends with a short clause saying so: `Sets the decay of the snare body, despite the panel printing NOISE`. The reader clicked that control and is looking at that label; they have to learn it there, not in a note they will never reach. Keep it to one clause — the full account of the disagreement still goes in `notes`.

Both of these matter because of what happens when a control has NO line: the help falls back to the maker's own tooltip, marked as theirs. So an untagged dead control tells the reader exactly the wrong thing — the very text you proved was false.

## Before you finish

```
python3 research/validate_help.py <Plugin>
```

Run it until it is clean. Do not edit `src/HelpText.cpp` — it is generated.

## Report back

How many modules you wrote, how many ports you gave a family, anything about the plugin that needs a person to settle, and any module you deliberately left partial.
