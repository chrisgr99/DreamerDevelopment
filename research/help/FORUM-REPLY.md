Coincidentally I have been working on a superset of this and only just noticed the thread. Some of it may be useful here.

What I have is in-rack help for every module installed on my machine: 2,115 modules across 107 plugins. Option-click on a Mac, or the equivalent on Windows and Linux, on a port, a control or a module's faceplate, and a note appears above the pointer describing that one thing. The text was written by reading each maker's manual, panels and source rather than generated from the module, because a generated sheet can only repeat the port names already on the panel, and on the modules that most need explaining those names are numbers.

Over the last few days I extended it to the properties this thread is about. An input's note now carries the voltage range it expects, whether the signal is continuous or stepped, whether it is unipolar or bipolar, and whether it takes a polyphonic cable. A module's note carries POLY or MONO after its name.

Current state of the analysis:

- 15,083 input ports read so far
- 11,367 of them say whether they take a polyphonic cable
- 11,725 say whether the signal is continuous or stepped
- 2,726 give a voltage range
- 6,207 output ports carry the same fields
- 1,265 of the 2,115 modules answer the polyphony question from a reading of their own code

About 3,700 ports remain unread. That is days of work, not weeks.

**How each fact was established.** For open plugins, the maker's source at the version actually installed: `getPolyVoltage`, or a loop bounded by `getChannels`, settles polyphony outright, and an explicit `clamp` or `rescale` settles a range. For plugins with no published source, the binary — every one I have looked at is unstripped, so each module's `process` can be disassembled and each port access attributed and classified.

Every field carries a `why` recording where it came from: a file and a line, or an instruction address. A validator rejects any field without one. This is a response to a specific failure earlier in the project, where a scan produced DSP constants that were plausible and appeared nowhere in the binary. There is now also a checker that tests quoted numbers against the compiled code, calibrated against random probes so that it reports its own false-positive rate.

Cella and Alphagem-O, on the two problems you raised with the extracted output — that it was too verbose, and that it needed verifying.

Verification is the `why` field above: nothing may be written that cannot be pointed at, and the validator enforces it. Three derivations were withdrawn after testing on exactly that basis — that a CV input is continuous, that a trigger input is stepped, that a pitch input is one volt per octave. Each held for most ports and failed for enough to matter.

Length is handled by generating the entry rather than writing it. Each port gets one short phrase and no more: at most four facts, always in the same order, drawn from a fixed vocabulary. `0 to 10V · continuous · polyphonic`. A range takes one of four shapes and nothing else, so a column of them can be scanned rather than read. The description of what the jack is *for* stays separate, above it. Nothing is summarised, so nothing is lost in the summarising.

Fewer facts are recorded than you might expect. Across 15,083 input ports, polyphony is answered for 75% and continuous-or-stepped for 78%, but a voltage range for only 18%. Part of that is real: some jacks change range with a menu setting, where no one figure is true. Most of it is a choice I have not finished making. A range is currently claimed only where something clamps or scales that jack on its own — Bogaudio's XCO clamps each CV input separately, which is why fifteen of its sixteen inputs have one. Where a knob and its CV are summed and the *total* is clamped, which is the commoner pattern, nothing is recorded at all. That clamp is still worth stating, along with whether a negative voltage is ignored, subtracts from the knob, or swings the value negative — and whether an unpatched jack is normalled to something. Those are the next fields going in.

Alphagem-O, reading the code rather than the documentation produces findings a table built from manuals cannot contain. About sixty jacks in this library are drawn on a panel, named, configured, and never read by the module's code; thirty-one of those are clickable. Five plugins' manifests disagree with their own source about polyphony — one tags nothing polyphonic although ten of its modules are, another omits the tag from three effects whose code allows it. One module's release-time jack does nothing because its delay-time jack is read twice. Another's four inputs are documented as an OR but are summed and compared against a threshold, so two inputs at 5V turn the output on and one at 5V does not.

stoermelder, on tooltips: that is the right place for this information, and your modules are among the few where it is already there — 356 of the 817 port descriptions in my installed library are yours. I am not writing into other makers' tooltips. It would mean claiming space on their panels and getting 107 makers to agree on a format. The note appears beside the jack instead, on option-click, which nothing else in Rack claims.

**Status.** None of this is released. It will go into my Clarity module when the analysis is finished, which could be as soon as tomorrow, as one more option there — a switch to turn it on, and the same switch to turn it off. Clarity is off by default for everything it does, and this will be too.

Rather than finish in private, I would put a pre-release build up now, so it can be used against modules people know well and the errors can be reported. Corrections from a module's own maker are worth more to me than another hundred ports scanned.

If that is useful, say so and I will post a build.

I will also publish the tables the help was built from, on my GitHub page, so the findings can be read and checked directly rather than only through the notes in Rack. I will post here when they are up.
