# What the rescan could not settle

Every agent in the rescan was asked to say what a person should look at. This is that list, gathered, with what the entry says now and what it would take to change it. **Nothing here is blocking**: each item already has a reading in the text, chosen deliberately and defensible from a source. Reading this file is optional; ignoring it leaves the entries as they are.

Ordered by how much a wrong reading would cost the person using the help.

## Settled without asking

Two rules were decided during the run rather than left open, because a run of forty agents cannot wait on an answer. Both are reversible in one pass.

**A cosmetic menu that every module in a plugin carries gets no line.** Themes, skins, panel art, light and dark. Written per module they are pure repetition — dBiz would carry three identical theme lines on all 32 of its modules — and for twenty of those they would be the only thing under the "Right-click the panel for:" heading. Two exceptions kept: where looking at the panel IS the module's function (alefsbits blank6hp, computerscare Custom Blank), and where a plugin-wide item changes behaviour rather than appearance (Venom's *Lock all parameters*, DanT's CV Param Mode). **Confirmed by Chris during the run.**

**A menu item's printed name is quoted, so the voice rules do not judge it.** Zilah prints *MSB waits for LSB*; rewording it would falsify the one thing the line exists to get right. `validate_help.py` now reads a menu line with the name taken out. Where a *choice* name is the problem, it goes in the same exempt part after a colon.

## Found by reading the compiled code, and worth telling the makers

These came out of the port pass, where NYSTHI's binary was disassembled because the maker's repository contains no code at all. Each is a fact about the installed build, cited to an instruction address in that module's `notes`.

**NYSTHI TheCage indexes its inputs and outputs at -1** when no row is selected, reading outside both arrays. **PolySevenSeas reads its input array at byte offset 1200**, past the end of its thirteen ports. Neither is ours to fix, and both are the kind of thing a maker wants to know.

### Jacks that are on the panel and are never read

The port pass turned up a class of thing no amount of manual-reading would have found: a jack a maker drew, configured and named, which nothing in the module's code ever looks at. Our line describes what it was meant to do, and it does nothing. Each of these is a candidate for a line saying so, in the way STYLE.md already provides for a control that does nothing.

- **Bidoo liMonADe input 3** — our line: "a trigger here switches the sync between soft and hard". `SYNCMODE_INPUT` appears only in the enum and the widget.
- **dBiz Bench input 8** — our line: "a rising edge above 2V restarts the LFO". Nothing reads it; the oscillator's reset reads a different constant that has no jack on the panel at all.
- **mscHack Osc_3Ch inputs 9-11** — our line describes them attenuating filter resonance. `IN_REZ` is named and added and never read; that description is true of the maker's *other* oscillator, OSC_WaveMorph_3.
- **NYSTHI PolyDelayAttackHoldDecay input 7** — our line: "a trigger switches the looping on and off". Never read; input 8, the hold toggle, is.

**And then it turned out not to be a handful but a hundred.** FrozenWasteland alone has 46 inputs no line of the module ever reads, dbRackSequencer 4. Most are not drawn on the panel and so can never be clicked, which is why nobody has ever noticed — but **31 of them ARE drawn and clickable**: ManicCompression's two curve CVs, ManicCompressionMB's ten band curve CVs, all sixteen of PortlandWeather's TAP LEVEL jacks, SliceOfLife's MIX, SeriouslySlowEG's RELEASE TIME, and P16B's OFFSET. Option-click any of those today and the help falls back to the maker's own tooltip, which describes a thing that does not happen.

STYLE.md already says a control proved to do nothing gets a line saying so. Writing those lines is a `lines` edit, which the port pass deliberately does not make — so it wants a small pass of its own, and it is the single most useful thing left on this page.

**FrozenWasteland's SeriouslySlowEG reads its delay-time jack twice** — once for the delay and once for the release — so the RELEASE TIME jack does nothing and the DELAY TIME jack moves both. A copy-paste in the maker's source.

**dbRackSequencer's SigMod writes sixteen channels to its CV output and never calls `setChannels`**, so only the first reaches a cable.

**Bidoo BAFIS** is the near miss in the same family: its type inputs compare the voltage against exactly 0 and exactly 1, so anything between or above them leaves the switch where it was. Our line says the voltage "steps that band's distortion on through the three curves".

**NYSTHI FixedVoltageSource compares every gate against 10V, not 1V**, so a 5V gate leaves the row silent. Now recorded as `high above 10V`.

**Fundamental's Random reads `inputs[RATE_PARAM]` where it means `inputs[RATE_INPUT]`.** It works only because both constants happen to be index 0.

**HetrickCV's PhasorEuclidean and PhasorToLFO read their Phasor input with `getVoltage`** where every sibling module uses `getPolyVoltage`, so a polyphonic patch silently loses every channel but the first.

**Bogaudio's MEGAGATE reads its VCA BIAS CV only while VCA ENV CV is also patched**, and **CMPDIST's DRY/WET CV scales the less-than mix rather than the dry/wet amount**.

**Venom's Slew gives its two shape CV jacks the same names as its two time CV jacks**, so Rack shows "Rise time CV" twice.

## Worth a minute each, because a wrong reading misleads

**Alikins ColorPanel — a reversal of what the first pass said.** The first pass said the ColorMode menu was dead. It is not: `ColorModeItem::onAction` sets it and `ColorPanelFrame::step()` reads it. The entry has been rewritten on that basis. To check: put a 0-10V ramp into the first input and switch ColorMode. RGB should go red-ish, HSL should sweep hues.

**DanT Kapow — does a fresh one make a sound?** The binary contains the literal `presets/Kapow/Kapow.kapow`, which suggests a factory analysis may be loaded on a new instance. Neither source nor disassembly settled it. The note no longer claims the module is silent out of the box; it says what the voice plays and how to load a sample. To check: add a Kapow and trigger it.

**Wavulike's `Frequency:` menu row.** The VCO/LFO chooser's label is read from the binary as `"Frequency: "`. The maker uses `"Label:"` rows elsewhere, so this is almost certainly the menu row and not display text. To check: right-click a Wavulike.

**OrangeLine Buckets was written without source.** The maker's repository is a single squashed commit at 2.4.15 with no 2.4.14 tag, and 2.4.14 is what is installed. The difference is real and visible: the repository's `Buckets.hpp` declares 3 inputs and 39 outputs, while the installed census has 2 inputs and 26 outputs. Buckets was therefore written from the census, the panel and the README alone. Nothing else that changed in 2.4.15 can be ruled out for this plugin.

**Entrian's two "selection input:" labels** are built at runtime as the module's word plus that phrase, so the capital cannot be read out of the binary. Written as *Phrase selection input:* and *Song selection input:*. One glance at a running module settles it; nothing else depends on it.

**Entrian's Run output** — gate or trigger is stated nowhere and is not readable from the binary. Both Reset and Run are deliberately left without a port family, so Clarity leaves them in Rack's own colour rather than colouring them wrongly.

**FLAG Prodigal-Son** — whether its amplitude envelope silences the outputs with nothing patched to GATE could not be established. Closed source, and the maker's site does not say. No prerequisite note was written, which is the safe way round: the entry claims nothing either way.

**The free Vult modules** could not be proved monophonic from the stripped binary, so no "mono only" note was written on any of them. Same shape as above — silence rather than a guess.

**Grayscale Variant resting at 0V** is inference from disassembly — `Variant::process` reading a zeroed expander message with nothing to its left — not from source. There is no source for this plugin.

**Grayscale Nexus** — whether a fresh one already has a keyframe at position zero was not established, so the note says only that AMP is inert between keyframes. That statement is true either way.

**Our own panel-layout menu, on all ten MPX modules.** `Edit panel`, `Save layout`, `Restore the built-in wording`, `Forget my layout`. It is plugin-wide, so the cosmetic rule leaves it out — but it is a panel *editor*, not a theme, and if you count it as behaviour it earns a line on every module. Left out for now. This is the one item on this page that is about our own work rather than somebody else's.

**Vult's arrow choice labels.** Freak-HW's VCA menu prints `NO VCA`, `DR->L&R` and `RES->L DR->R`. The item is named exactly; the two arrow labels are described in words rather than quoted, because they are unreadable aloud and nearly so on the page. That is the ruling for the project: quote a choice where quoting helps somebody match it against the screen, describe it where the printed form is a diagram rather than a name.

**VCV Pro Compressor's oversample range**, 1x to 16x, is the manual's. The binary holds only `Off` and a `%dx` format string, so the endpoints are not verified against code.

**BaconMusic's ContrastBNDEditor** is a theme editor, and the new cosmetic rule would remove its only menu line. It was kept under the "where the theme IS the module" exception, on the grounds that a module whose whole purpose is choosing a colour scheme has nothing else to say. Same shape as the blank-panel exception.

**Sha-Bang RandRoute's Latch and Toggle** were established by the first pass as swapped against the maker's own code. That finding was left standing, unchecked this pass.

## Judgement calls already made, listed so they are not invisible

**computerscare Custom Blank's keyboard list is wrong in the maker's own menu.** It prints "A,S,D,F: Translate image position"; the code reads **w**, not f, and handles eleven more keys the menu never mentions. Our notes describe the code. No contradiction clause was added, because the rule covers text on a panel or a control's tooltip and this conflicting text is on a menu. Extending the rule to menu text is a one-line change and a short pass.

**Sapphire Elastika's first line** still says it can self-oscillate with no input, which is the maker's own claim and is not true of the default settings. The line was left and a note added, rather than rewording a control line this pass was told not to touch.

**Amalgamated Harmonics PolyScope's colour maps** are file-scope, so loading a scheme changes every PolyScope in the patch. Left out of the line as too fine a point.

**Two menu items are quoted with the maker's own misspelling**, so they can be matched against what is on screen: RPJ's "Enable Self Oscilation" (one L) and PathSet GlassPane's "Low Peformance Mode". Both will sound like typos aloud. The alternative is an entry that reads correctly and names an item the menu does not have.

**Hora's menu items all print a trailing " : " before their checkmark.** Dropped, here and in Erica's, as punctuation belonging to the menu's layout rather than to the item's name.

**Our own census was stale, and is corrected.** It still put the help switch on Dark and gave Clarity ten parameters; the source has eleven, with the help switch as Clarity's own and off by default. Fixed by hand against `Modules.cpp` rather than by a rescan, because regenerating it means running Rack. Worth regenerating properly the next time Rack is open.

**Decibel spellings are ours, not the menu's.** Valley's Plateau prints `0 dB` and `-18 dB` with a space; the entries use the project's unit spelling, because the space is what makes a speech synthesiser read it as a letter pair rather than a word. The units rule wins over the quoting rule wherever the two meet.

**RPJ GenieExpander** saves only four of its five colour rows, so Color Node 4 is not kept in the patch. In `notes`, not in a line.

**QuantalAudio's eight `Create …` menu lines** on one module is a lot of lines. Each is named exactly as the source prints it; they can be merged if that granularity is unwanted.

**StudioSixPlusOne, two maker bugs** now in `notes`: Bascom calls `configOutput` twice on `MAIN_OUTPUT`, so both output tooltips read "Right" and the right input is never configured; Hula configures `DEFAULT_TUNING_PARAM` with the name "Over Sample Rate", so Rack shows that name twice.

**Two of our own tag maps were wrong, and are fixed.** `mpxComp` and `mpxChart` both tagged their parameters in index order while the lines are in panel order; because `P_RHYTHM`, `P_RATE` and the chart's later params were appended to the enum rather than inserted, nearly every control was opening its neighbour's line. Both remapped against the `ParamId` enum. Worth knowing because the same fault can only exist where a maker appended to an enum, which is the thing we ourselves do deliberately to keep saved patches loading.

**Alikins GateLength's shipped preset** is named `150ms` and sets 1.5 seconds. The author's mistake, not ours, and not put into a line.

**VCV Drums' hi-hat choke input** is printed MUTE on the panel. Our lines describe it without naming it, which is allowed, but the name is unusual enough to note.

**computerscare, ten figures not literal in the binary.** All ten were verified as correct derivations in source, not inventions — `0.625V` is `ch / 1.6f`, `221222` is a string, and so on. `check_numbers.py` flags derived figures by design; see NUMBER-CHECK.md.

## One source that was wrongly written off

An early run reported that Geodesics' ten per-module PDFs could not be read, because `pdftoppm`, `pdftotext` and `mutool` are all absent from this machine. That was wrong: **`pypdf` 6.18.1 is installed** and extracts their text cleanly, which a later run proved by fetching an Instruo manual and reading it. No `brew install` is needed for any of it, and `qpdf` and `rsvg-convert` are here too.

So Geodesics' PDFs are readable whenever somebody wants them, and every maker who publishes per-module manuals — the URLs are in the installed `plugin.json` — is now a source this project can use directly.

## Sources that did not exist

Several plugins ship no presets and no demo patches at all, so that source added nothing: Amalgamated Harmonics, RPJ, CharredDesert, LifeFormModular, QuantalAudio, FehlerFabrik. CharredDesert and LifeFormModular also define no context menus anywhere, so the "Right-click the panel for:" heading never appears for them.
