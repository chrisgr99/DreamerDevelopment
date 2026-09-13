# Are the numbers real?

An agent writing an entry for a plugin with no published source has to read the compiled binary. One of them did not. The first draft of the EricaCopies entry carried a confident list of DSP constants — a 19490Hz bucket-brigade clock, a 352.5Hz filter corner, a 0.2675Hz LFO floor, an LFO rate law, a 0.03 dead band, 0.8 waves per volt, and a hidden gesture where flipping a switch six times toggles unipolar mode. The agent that took the work over checked every value against the binary and found that **none of them exists in it**. It threw the file away and rewrote all four modules from ground it could verify.

Nothing else catches this. `validate_help.py` checks how a line reads and where its tag points; a fabricated number passes every rule and reads better than an honest one.

So `research/check_numbers.py` is the other half: for every number a line quotes, is that number in the plugin's binary at all?

## The tolerance is the whole test

The first version of this script cleared the very fabrications it was written to find. An 800KB binary holds about 75,000 distinct plausible constants, so a window of half a per cent contains something almost always. Measured against 4,000 random plausible values:

| tolerance | random numbers "found" |
|---|---|
| half a per cent | 98.6% |
| a tenth of a per cent | 63.9% |
| 1e-4 | 20.1% |
| 1e-5 | 2.9% |
| **1e-6** | **0.3%** |
| exact | 0.0% |

1e-6 is loose enough to survive a float32 round trip and tight enough that a number nobody wrote is very unlikely to be there. Each binary is also probed with random values to get its own chance rate, because a bigger file clears more numbers by luck and the rate means nothing without it.

## Result, 2026-09-12

Every plugin with no published source, checked against its own binary:

```
plugin                   nums  found   rate chance
Autodafe-DrumKit            8      8   100%      1%
MM_Tools                   19     15    79%      0%
Grayscale                  13     13   100%      0%
AlrightDevices              6      5    83%      0%
EricaCopies                 5      5   100%      0%
Blamsoft-XFXReverb         16     15    94%      9%
Blamsoft-XFXWave            7      6    86%      3%
Blamsoft-XFXF35            15     15   100%     11%
Instruo                    19     18    95%      0%
DanTModules                62     62   100%      1%
VultModulesFree            13     13   100%      7%
VultCompacts                9      9   100%      7%
FLAG-Free                   4      4   100%      1%
StellareModular             3      1    33%      0%
Hora-treasureFree           0      0      —      0%
Hora-Mixers                 0      0      —      0%
VultModules                 0      0      —      7%
```

**No evidence of fabrication anywhere except the draft already caught and rewritten.** Rates of 79 to 100 per cent against a chance rate of 0 to 11 per cent mean these numbers were read off the binaries.

The three entries quoting no numbers are not a gap: Hora and VultModules describe their controls without quoting figures, which is the conservative thing to do on a closed plugin. Hora's agent had figures in its report and kept them out of the lines.

**The eight remaining misses were each looked at and are all derived rather than stored:**

- `MM_Tools` TheCBlocker — a coefficient range of 0.9 to 1 and a corner "around 700Hz at 44.1kHz". The corner is computed from the coefficient and the sample rate; neither endpoint is a literal.
- `StellareModular` TuringMachine — a ±0.05V dead band, compared against rather than stored. Only three numbers in the whole entry, so the rate means nothing.
- `AlrightDevices` Chronoblob2 — a 2.73 second maximum, computed from a buffer length in samples.
- `Blamsoft-XFXReverb` — 44100 is Rack's constant, present in libRack and not in the plugin.
- `Blamsoft-XFXWave` — 261.63 is C4. It is in neither the plugin nor libRack, which stores the frequency to more places; a line quoting the familiar rounded value is right and the constant is simply written differently.
- `Instruo` cèis — small stage-time multipliers, computed.

## The rule this gives the writing

**Quote a number only where you have seen it.** On a plugin with published source that means in the source; on a closed one it means in the binary, as a float32 or float64 constant, at this tolerance. A figure from a hardware manual, a product page or a review is not a figure about this module and must not be promoted into a line — say what the control does without the number, and record in `notes` what was left out and why. The Erica entries are thinner than the rest for exactly this reason, and that is the correct trade.

Run it before generating the table:

    python3 research/check_numbers.py            # every source-less plugin
    python3 research/check_numbers.py Grayscale  # one of them
