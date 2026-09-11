# NYSTHI port map — batch 2

19 more modules. Running total: 38 modules, 406 ports decided.

| module | ports | decided | notes |
| --- | --- | --- | --- |
| Spectre | 1 | 1 | Analyser: one audio input. |
| VUMeterSlim | 1 | 1 | Meter input, audio. |
| VUMeterSingle | 2 | 2 | Audio in and through. |
| VUMeterDual | 4 | 4 | Two audio channels in and through, each with a VCA. |
| GranTunismo | 2 | 2 | Granular: audio in, audio out. |
| BZENVELOPE | 3 | 3 | Trig in gate, EOC out gate, envelope out CV. |
| NYEnvFollower | 3 | 3 | Signal in and out audio; envelope out CV. |
| PitchVoltager | 3 | 3 | Signal in and out audio; the voltage out is volt per octave. |
| Dica33 | 4 | 4 | 303 filter: in and out audio; resonance and cutoff CV. |
| MicrotonalHostHelper | 4 | 4 | Quantized in and the 1/12-volt out are pitch; the two bend voltages CV. |
| SimplerFileControlExpander | 4 | 4 | Previous, next and random are gates; the sample selector is CV. |
| SlopeDetector | 4 | 4 | Input CV; rising, steady and falling outs are gates. |
| TUNATHOR | 4 | 4 | Calibration signal in is audio; the calibration voltage and the in and out voltages are pitch. |
| AttackDecay | 5 | 5 | Attack and decay CV, trig gate, EOC out gate, envelope out CV. |
| PolyAttackDecay | 5 | 5 | The same panel, polyphonic. |
| Ratchet | 5 | 5 | Clock and tap in gates; range min and max CV; pulse out gate. |
| BIGNUMBER | 6 | 6 | A counter: active, reset and pulse in are gates, and so are its outputs. |
| CONVOLVZILLA | 6 | 6 | In and out stereo audio; bypass gate; dry-wet CV. |
| DualSignalDelayer | 6 | 6 | Two channels: signal in and out audio, delay CV. |

## Left undecided

- **UNNYSTHIPLEASURESGRAPHER** — a grapher plots whatever it is given.
- **PolyVoltageMeter** — as before, a meter takes anything.

## What this batch showed

The small modules go about four times faster than the west-coast set: twenty of them in roughly five minutes against twenty minutes for the first batch. Most were decidable from the panel alone, and several from the dossier without opening the picture at all — a module whose parameters are named "Delay" twice and whose jacks alternate in, knob, out is not ambiguous.
