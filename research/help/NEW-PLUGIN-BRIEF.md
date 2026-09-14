# A plugin nobody has written about yet

Read [STYLE.md](STYLE.md) first — it is the rules, and this brief does not repeat them. Then read [NUMBER-CHECK.md](NUMBER-CHECK.md), which is the incident that produced the one rule that matters: **nothing may be written that cannot be pointed at.**

Your plugin has a skeleton already — `research/help/<Plugin>.json` with one stub per model, and `research/help/scaffold/<Plugin>.md`, a worksheet saying what the census already knows. Read the worksheet before you start. It tells you which ports the maker never named, which have no jack on the panel at all, and what the maker's own one-line summary of each module says.

This is **one visit, everything at once**. The first library was written in two passes months apart and the second pass cost a full re-read. You are inside the source already; do not leave anything for later.

## What a finished module has

**`lines`** — the prose. One line per control, plus:
- a **first line** saying what the module is, in your words, not the maker's marketing.
- **`Note — `** lines for anything true of the module rather than of one control. Above all: **if the module does nothing until you do something, say what.** A file to load, a clock to patch, a device to choose, an expander to place, a switch that comes up off. This is the single most useful thing in the whole dataset.
- **`Menu — `** lines, one per item, named exactly as the menu prints it, then an em dash, then what it does. Say where the menu is if it is not the module's own panel menu — Rack has four kinds.

**`in`, `out`, `param`** — the tag maps, port index to line index. A port with no widget position (the worksheet lists them) gets no tag and no line: no click can ever reach it.

**`family`** — what each jack carries: `audio`, `cv`, `trigger`, `pitch`. This is what Clarity colours ports from.

**`props`** — the port fields, for inputs and, where the same function gives them cheaply, outputs:
- `poly` — true or false. `getPolyVoltage`, or a loop bounded by `getChannels`, settles it.
- `range` — four shapes only: `0 to 10V`, `±5V`, `1V per octave`, `high above 1V`, with whatever number is the fact.
- `step` — `continuous` or `stepped`.
- `negative` — `ignored`, `subtracts` or `swings`. The clamp's low bound, and whether the clamp wraps the jack alone or the knob-plus-jack sum.
- `normal` — what an unpatched jack reads. A short phrase: `10V`, `the jack above it`.
- `sumRange` — the clamp on the knob-plus-input total, same spellings as `range`.
- **`why` on every port you touch** — a file and a line, or an instruction address. The validator rejects a field without one.

**`notes`** — anything that will not fit a field: mode-dependent behaviour, a maker's bug, why something was left blank.

Then **remove `"partial": true`** from the top of the file. That is what says the plugin is finished.

## Where to look

1. **The maker's source at the installed version.** The skeleton's `source` field has the URL. Most makers do not tag the release that shipped — find the commit whose `plugin.json` carries the installed version, and say in your report which commit you used.

   **When several commits carry the same version, the installed binary settles it.** Makers often leave the version string alone while they keep working: 4ms's ProducerPack gained polyphony and bypass across a dozen commits all labelled 2.0.1. Pick something those commits changed, look for it in the installed `plugin.dylib` — a channel loop, a `setChannels`, a menu string — and you know which side of the change shipped. `nm` and `strings` answer most of these without a full disassembly. Say in your report how you settled it.
2. **The compiled binary**, where there is no source. Every one examined so far has been unstripped: `otool -tvV`, one `Class::process` per model. Rack's ABI is the same in every plugin — `inputs` at Module+0x38, `outputs` at +0x50, port stride 0x50, the `channels` byte at Port+0x40. **The decisive test**: a read of the channels byte followed by a compare against zero is `isConnected()` and means nothing; a read that becomes a loop bound, or any read of `voltages[1]` and beyond, is real polyphony.
3. **Manuals, READMEs, library pages.** `pypdf` is installed and reads PDF manuals; `pdftotext` is not.
4. **The census slice**, `research/help/census/<Plugin>.json` — port names, knob names, positions.

Where a manual and the code disagree, **the code wins**, and the disagreement goes in `notes`.

### When the binary is stripped and the manual is all there is

Most binaries carry full symbols. One did not — Ambivalent-Instruments exports `_init` and nothing else, so no function can be tied to a model and `process()` cannot be read at all. The fallback is the maker's manual, and the question then is whether that manual describes the build you have or an older one.

**`check_numbers.py` answers it.** Quote the manual's figures, run the check, and see how many are present in the binary as literals. That run came back 48 of 48 against a 0% chance rate — strong evidence the manual matches this build — and three further figures it gives were absent, so those were left out of the lines.

This inverts the tool: it was written to catch numbers nobody could point at, and here it points at a document and asks whether to believe it. Use it that way whenever the code cannot be read, and say in your report what the rate was.

Where the manual is silent, **claim nothing**. A stripped binary cannot settle polyphony, and a blank is the correct answer.

## Never run `git checkout`, `git restore` or `git stash`

The tree holds uncommitted work. If you make a mess of a file, fix it forwards. Never reach for git to undo anything.

## Do the work yourself

Do not delegate to sub-agents. A previous run did, burned 158,000 tokens and changed nothing.

## Finish one plugin at a time

Run `python3 research/validate_help.py <Plugin>` before starting the next, so progress is always on disk and always valid. If you run short, stop cleanly and say exactly which plugins you finished and which you did not start.

## Report

Per plugin: modules covered; how many ports got each field; which sources you could read and which did not exist; anything a person should check; and anything you found that contradicts what the maker documents.
