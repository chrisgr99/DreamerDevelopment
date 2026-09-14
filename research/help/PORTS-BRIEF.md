# What a jack expects: the third pass

Read [STYLE.md](STYLE.md) first. This pass adds nothing to the prose. It answers three questions about **input ports**, as fields rather than sentences, and it answers them only where they can be established.

The questions come from the Rack forum, which has spent a year asking them and answering them with an oscilloscope, one module at a time:

1. **Does this input take polyphony?**
2. **What voltage range does it expect?**
3. **Is the signal continuous, or stepped?**
4. **Is it unipolar or bipolar?**

**And the same about outputs, where it can be had cheaply.** An output's polarity is the one a patch usually turns on — a 0-10V envelope into something expecting ±5V is the commonest silent mistake in a rack — and a module that writes `outputs[Y].setVoltage(x * 5.f)` has said so plainly. Do the inputs first and completely; take the outputs where the same function you are already reading gives them to you, and leave them otherwise.

## The one rule that matters

**Write nothing you cannot point at.** A blank field is the correct answer where nobody has established the fact. It is not a gap to be filled in with what is usual, or likely, or what the other jacks on the panel do.

This project has already shipped fabricated DSP constants once, produced by exactly the reasoning that feels like knowledge and is not — read [NUMBER-CHECK.md](NUMBER-CHECK.md) before you write a single figure. Every field you write carries a `why` saying where it came from, and the validator rejects a field without one. If the `why` would have to say "it seemed reasonable", do not write the field.

An earlier attempt at this pass measured the modules by running them. It was abandoned, and the reason is worth carrying: a sweep watches the **outputs**, and an output is the far end of the whole module. Feed a continuous CV into a comparator and its gate output changes once — so the measurement says "stepped", confidently and wrongly. Nothing you conclude from behaviour at the far end is a fact about the jack.

## Where to look, in order of authority

1. **The maker's source at the installed version.** Decisive for polyphony, and usually for the rest. Check out the tag matching the installed `plugin.json` version — see STYLE.md on matching the installed build.
2. **The compiled binary**, for closed plugins. `strings` and disassembly; see the rescan notes for what worked.
3. **The manual, the README, the module's library page.**
4. **The `Polyphonic` tag in the maker's `plugin.json`.** This is already shown in the help at module level and needs no work from you — but it is a cross-check: a module the maker calls polyphonic, all of whose inputs you concluded are mono, means one of the two is wrong.

## Polyphony: what to look for

These are unambiguous in C++ and they settle the question per port:

- `inputs[X].getPolyVoltage(c)` or a loop over `getChannels()` on that input — the module reads every channel. **Polyphonic.**
- `inputs[X].getVoltage()` alone, with no channel loop — it reads channel one and nothing else. **Not polyphonic.**
- `outputs[Y].setChannels(inputs[X].getChannels())` — that input sets the width. Polyphonic, and worth a `why` saying so.
- `getNormalVoltage`, `getPolyVoltageSimd`, `getVoltageSimd` — read the surrounding loop; SIMD usually means all sixteen.

Beware the input that is polyphonic **only in one mode**, and the module that reduces sixteen channels to one on purpose — a mixer reads all sixteen and sums them, which is still `poly: true`, because the question is whether the jack takes a polyphonic cable.

## Range and shape

- `range` only where the source or the manual states it, or where the code clamps or maps explicitly — `clamp(v, 0.f, 10.f)`, `rescale(v, 0.f, 10.f, ...)`. A knob's range is not the jack's range.
- `step: "stepped"` where the code quantises the input — `std::floor(v)`, `std::round(v * 12.f)`, an index into a table. `"continuous"` where it is used as a value.
- A threshold comparison, `v >= 1.f`, means the jack is a gate or trigger: **stepped**, and `"high above 1V"` is the range spelling for it.

## Three more facts, added after the first plugins were done

These came out of using the help: the four quick facts answer what to send a jack, and leave three obvious questions unanswered.

**`normal` — what an unpatched jack reads.** Rack gives an unpatched input 0V, but a great many makers normal one jack to another or to a constant, and it changes what the module does when you take a cable out. `getNormalVoltage(x)` states it outright; so does a manual read of the right channel when nothing is connected. Write the short phrase a person would say: `10V`, `the jack above it`, `the left channel`. Forty-eight characters at most, no full stop.

**`negative` — what a negative voltage does.** Three behaviours that look alike from outside, and the one people get wrong:
- `"ignored"` — the jack's own voltage is bounded below at zero before use, `clamp(v / 10.f, 0.f, 1.f)`. A negative voltage does nothing at all.
- `"subtracts"` — the jack is scaled and added to a knob and the **sum** is clamped at zero. A negative voltage is not ignored: it pulls the knob's value down until the total hits the floor. This is the commonest pattern.
- `"swings"` — nothing bounds it below, `clamp(v / 5.f, -1.f, 1.f)`. This also supplies the polarity, so do not write `polarity` as well.

The distinction is the clamp's low bound **and** whether the clamp wraps the jack alone or the knob-plus-jack sum. Both are on the same line you are already reading for the range. In a binary, a clamp is `fmaxnm`/`fminnm` against literals, or a compare and a conditional select; whether it comes before or after the `fadd` that mixes in the knob is what tells you which case you have.

**`sumRange` — the clamp on the total**, where the knob and the input are summed. Same four spellings as `range`. This is what makes a blank `range` useful instead of merely honest: `range` says nothing because the clamp does not belong to the jack, and `sumRange` says what the clamp actually is.

Where the destination is a Rack parameter, the census already holds that parameter's own minimum and maximum — a free corroboration for the floor.

## Polarity

`polarity` is `"unipolar"` or `"bipolar"`.

**Do not write it where you have written a range.** A range already says it — 0 to 10V is unipolar, ±5V is bipolar — and the generator works it out from the range on its own. Writing both is one fact twice, and the second copy is the one that will go stale.

Write it where the range could not be pinned down but the polarity is plain: `v * 0.1f` into a 0..1 destination is unipolar; `v * 0.2f` into a -1..1 destination is bipolar; a signal the code centres or offsets by half its span is bipolar. A volt-per-octave port is bipolar — C4 is 0V and every note below it is negative.

## What to write

In the module's entry in `research/help/<Plugin>.json`, beside `lines`, `in`, `out` and `family`:

```json
"props": {
 "in": {
  "0": {"poly": true, "range": "1V per octave", "step": "continuous",
        "why": "VCO.cpp:88 loops getChannels() on IN_PITCH and setChannels() on the output"},
  "3": {"step": "stepped", "why": "process() does std::floor(inputs[MODE].getVoltage())"}
 }
}
```

- `"in"` and `"out"` are both allowed; port numbers are the same indices the matching tag map uses.
- `poly` is `true` or `false` — never a string, never "maybe". Omit it if unsettled.
- `step` is exactly `continuous` or `stepped`.
- `polarity` is exactly `unipolar` or `bipolar`, and only where no range and no `negative` was established.
- `negative` is exactly `ignored`, `subtracts` or `swings`.
- `normal` is a short phrase, 48 characters at most, with no full stop.
- `sumRange` takes the same four shapes as `range`.
- `range` takes one of four **shapes**, with whatever number is the fact: `<lo> to <hi>V` (`0 to 10V`, `-5 to 10V`), `±<n>V`, `1V per octave`, or `high above <n>V` (`high above 2V`, `high above 3.6V`). The validator enforces the shape and not the number, because a column somebody scans for a match is worthless if the same fact is spelled three ways — but a threshold that really is 2V must say 2V rather than being rounded to a house figure or left blank.
- `why` is required on every port you touch. One clause. A file and line, a manual page, or the binary.
- Omit any field you did not establish. Omit the whole port. Omit `props` entirely if you established nothing — that is a real outcome and the honest one.

Anything you learn that will not fit these fields — a jack that is polyphonic only in one mode, a range that depends on a switch — goes in the module's `notes`, or as a `Note —` line if a reader needs it. Do not stretch a field to carry a sentence.

## Never run `git checkout`, `git restore` or `git stash`

The whole `research/help/` tree is uncommitted work in progress. An agent on an earlier wave ran `git checkout` on one file to undo a formatting change of its own and destroyed a previous pass over that plugin. If you make a mess of a file, fix it forwards — re-read it, correct it, validate it. Never reach for git to undo anything.

## Do not touch

The prose lines, the tag maps, the families. They were settled by two previous passes. If you find one is wrong, put it in your report; do not fix it here.

## Order of work

Do the modules the maker tags `Polyphonic` first — that is where the question actually bites. Then the rest of the plugin.

Finish one plugin completely and run `python3 research/validate_help.py <Plugin>` before starting the next, so progress is always on disk and always valid.

**Do the work yourself.** Do not delegate to sub-agents: a previous run did, burned 158,000 tokens and changed nothing.

## Report

Per plugin: how many input ports got `poly`, how many `range`, how many `step`; which sources you could read and which did not exist; anything a person should check; and anything you found that contradicts an existing line.
