# Harvesting what the citations already say

Read `research/help/STYLE.md` and `research/help/PORTS-BRIEF.md` first. PORTS-BRIEF defines the fields; STYLE defines the spellings, and `validate_help.py` enforces both.

## The job

Every port in this library carries a `why` — a sentence saying what the code does with that jack. Over a thousand of those sentences name a bound while the port's own `range` field is empty. The evidence was gathered and never harvested. Your list holds those ports, with the citation printed beside each one.

For each: read the citation, decide which field it settles, and fill that field.

## The distinction that decides everything

A clamp on the **jack** is a `range`. A clamp on the **knob-plus-jack total** is a `sumRange`, and the jack's own range may be genuinely unbounded.

> "adds it to the coarse knob, the sum clamped to −5 and 5 octaves"

That is a `sumRange`. Writing it into `range` would be a false statement about what the jack accepts, replacing an honest blank with a wrong figure — worse than the gap it fills. Read every citation for *what* is being clamped before you write anything.

## Three traps in this particular list

- **A citation that says there is no bound.** "writes the total to one channel with no clamp" contains the word clamp and a number and means the opposite. Leave those alone.
- **A bound in the wrong units.** "clamps the sum with the knob to −0.1 and 0.1" is a bound on a parameter in its own units, not a voltage. A range field is volts. If the citation does not settle volts, leave it blank and say why.
- **A bound that is a count, an index or a time** — steps, semitones, milliseconds, bpm. Same rule: not a voltage, not a range.

## What you may not do

Do not open the source to settle anything. **This pass reads only what is already written.** If a citation does not settle the field on its own terms, leave the field blank — that is a correct outcome and it is most of the value of doing this carefully. Where a citation is so vague that it cannot be acted on, say so in your report; that is a finding about the citation, not about the port.

Do not touch prose lines, tags, families, or any field other than `range` and `sumRange`.

## Report

How many `range` fields you filled, how many `sumRange`, how many you deliberately left blank and the commonest reasons, and any citation that turned out to be self-contradictory. Run `python3 research/validate_help.py <Plugin>` after each plugin you touch.

Do not delegate to sub-agents.
