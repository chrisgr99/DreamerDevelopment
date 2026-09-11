# NYSTHI port map — batch 1

NYSTHI 2.4.23. 19 modules, 334 ports decided.

Read from the panel with each port index drawn on its own jack, cross-checked against the module's parameter names, its panel text, and the community manual at github.com/patman023/nysthimanual where it covers the module.

| module | ports | decided | notes |
| --- | --- | --- | --- |
| DelayAttackHoldDecay | 19 | 19 | Times and scale CV; trig, looped, hanged gates; the two VCA ins and outs audio; envelope chain CV; four end-of-stage outs gates. |
| FixedVoltageSource | 34 | 34 | Every input a gate, every output the fixed voltage — CV. |
| Model277 | 13 | 13 | Four audio in, four audio out, five CV. Confirmed by the manual. |
| Nudger | 20 | 20 | All inputs. The double-arrow pairs are nudge gates; the diamond at each group head and the CV ADD block are CV. |
| PolyDelayAttackHoldDecay | 19 | 19 | The same panel as DAHD, port for port. |
| PolyLPG | 3 | 3 | In and out audio, level CV. |
| Programmer | 41 | 41 | Sixteen stage-select gate inputs, sixteen stage pulse outputs; ADDR in CV; forward and back in gates; A to D out CV; trig and push out gates. |
| QuadPanner | 14 | 14 | Input, the four quadrant outs and the four chain ins are audio; X, Y, azimuth, magnitude CV; one gate out. |
| SOU-UTILS | 24 | 24 | Scalers, octave folders and two shift registers, all CV; the two ASR pulse inputs are gates. |
| Simpliciter | 32 | 31 | Stereo sampler. L and R in and out audio; rec, start, pause, gate, loop, the slice and grid buttons and the engine clock and sync are gates; varispeed CV, oct as volt per octave; ramp and peak out CV; eoc and click out gates. |
| Simpliciter2 | 30 | 29 | As Simpliciter, mono, with its own numbering. |
| SlimDualFixedVoltageSource | 4 | 4 | Two gate in, two CV out. |
| SlimFixedVoltageSource | 2 | 2 | One gate in, one CV out. |
| SoyModelSOU | 29 | 29 | Pulse in and out are gates; smooth, hard, time, probability, quantized and stored voltages CV; three flip-flops gate both ways. |
| b208_5steps | 8 | 8 | Dual: trig and reset in are gates, pulse out gate, CV out control voltage. |
| b208_dualLPG | 12 | 12 | Four gates: in and out audio, mod CV. |
| b208_envelope | 12 | 12 | Gate in; attack, duration, decay in CV; EOC out gate, Env out CV. |
| b208_pulser | 6 | 6 | Trig in gate, period modulation in CV, pulse out gate. |
| b208_random | 14 | 14 | Trig in gate; the four random outs and the inverter are CV. |

## Left undecided

- **PolyVoltageMeter** input 0 — a meter takes anything. Off-white is right.
- **Simpliciter / Simpliciter2** the jack labelled "mix" — the panel does not say what it takes and the manual does not cover it.
- **SOU Utils** octave folders: marked CV rather than volt per octave. They fold a voltage into an octave range, which is usually a pitch, but they are fed random voltages as often as not.

## What was used

- `census.json` — port and parameter names, from instantiating every module
- `census-positions.json` — where every jack and knob sits
- `annotate.py` — draws each port index onto its own jack; this is what made the reading possible
- the rendered panels, the module code strings, and NYSTHI's changelog
