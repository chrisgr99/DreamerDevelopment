# Corrections the help writing found in the hand-read port map

`src/PortMap.cpp` and `research/nysthi-portmap.json` were read off annotated panels by eye during
the port-colouring work. Writing the help entries meant reading the same panels again against the
maker's own text, and that turned up places where the map is wrong or was filled in by default.
These affect Clarity's port COLOURS, not just the help, so they want folding back into the map.

Found 2026-09-12, while writing NYSTHI:

| Module | Ports | Map says | Should be | Why |
|---|---|---|---|---|
| Flipper | all 30 | cv | trigger | The panel prints TRIG IN, GATE and TRIG OUT on those rows, and the changelog describes them as triggers and 10V gates. The map looks like a bulk fill. |
| AutoFader | in 13, in 14 | trigger | cv | The running module shows blue CV jacks beside the FADE IN TIME and FADE OUT TIME knobs, each with a seconds display. |
| NYECHOEcoeco | in 8–13 | cv | audio | The per-head TAP IN returns take processed audio back into the delay chain. |
| AttackSustainRelease 4/8/16 | out 0 | audio | cv | The unity mix of the envelopes. Every per-channel ENV OUT in the same module is already cv. |
| SimplerTapeControl | all 19 | trigger | mixed | CLICK OUT is documented as a sound output (audio); AUX OUT is a speed voltage (cv); MOD IN, WOW, INERTIAL and the note-based SELECT are voltages (cv). |
| Simpliciter, Simpliciter2, confusingSimpler, Sussudio | slice/selection START and END CV | trigger | cv | The changelog documents them as "CV IN and KNOB". Same for the 0-10V slice-select and keyframe-select inputs. |
| confusingSimpler | SOS CV-MIX | trigger | cv | A mix voltage. |
| complexSimpler | 1V/octave input | cv | pitch | Both its siblings are pitch; the style ruling makes anything tracking a volt per octave pitch. |

The help entries already carry the corrected families. The map itself is unchanged — do that as one
pass, with `make_portmap.py`, rather than piecemeal.

## Suspected but NOT overridden — needs somebody to look at the running module

Raised while writing NYSTHI group C. The agent followed the map as instructed and flagged these
rather than changing them, so the help entries still carry the map's colour:

| Module | Ports | Map says | Suspected | Why |
|---|---|---|---|---|
| RodentV2 | in 6–9 | cv | trigger | The four mod-switch jacks are drawn as orange pulse jacks, identical to the BYPASS trigger beside them. |
| RXG100ChanB | in 3 | cv | trigger | BOOST, drawn the same orange as the trigger jacks around it. |
| TIMEX | out 18, 19 | trigger | cv | Drawn blue, and the changelog describes them as carrying a CV ramp. |

Jack COLOUR on a NYSTHI panel is a reliable signal — the maker uses orange for pulses and blue for
voltages — so these three are probably right, but nobody has confirmed them against the module.

## One more, overridden with reason (NYSTHI group B)

| Module | Ports | Map says | Written as | Why |
|---|---|---|---|---|
| VectorMixer | out 6, 7, 8 | audio | cv, cv, trigger | X, Y and GATE. They are plainly position voltages and a touch gate, and QuadPanner's identical touch-gate is already mapped trigger in the same map. |
