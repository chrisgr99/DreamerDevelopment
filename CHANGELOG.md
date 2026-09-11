# Changelog

Newest first.

## 2.0.9 — unreleased

### Added
- **Dark** — a third module. Darkens the panels of makers who ship only light artwork, in memory, from the drawing Rack has already loaded. Nothing is written to disk and removing the module puts every panel back. Near-white areas become the ground, small dark shapes become light so lettering survives, and a panel that is not light all over is left alone. VCV's own panels keep their artwork; only the white patch behind their output jacks is toned down to a middle grey, which still says "these are the outputs" without glaring on a dark rack.
- Clarity knows what a port is on modules that never named one. NYSTHI has 3,104 ports and names 187 of them, so no rule could ever have coloured it; 2,976 of those ports were read off the panels themselves and now ship in the plugin. A user's own right-click override still wins, and a module whose port count has changed since it was read is ignored rather than half-trusted.
- Rules can ask about the module a port is on, not only the port's name: its maker, its model, its tags, and which way the port faces. That is what settles the ports whose names are a position rather than a description — "Cell 3", "Channel 2", "Row 5".

### Changed
- A port no rule recognises is now drawn off-white rather than being coloured as audio. Audio used to be the fallback, which meant the table could never be wrong: a port it had never heard of came back as audio, coloured with a confidence it had not earned. Off-white says what is true — this jack takes whatever you give it, or nobody has worked out what it is. A hundred and ten of VCV's own ports are genuinely of that kind.
- Where a cable's destination has no opinion, its colour is taken from its source instead. A mult, a merge, a scope and a sequential switch all accept anything, and asking them what a cable carries is asking the one end that does not know.
- Many more words are recognised, drawn from a census of every module installed rather than from guesswork: 840 ports across VCV's own plugins, then 27,362 across the whole library. Gates gained retrigger, run, start, stop, strobe, mute, hold, and a logic module's operations; control voltages gained aftertouch, tune, sweep, envelope, glide, threshold, tempo and the rest; and where a word means different things at the two ends of a module, the rule now faces one way only.

### Fixed
- A switch dragged from one port to another went on muting the port it had left. A switch does not mute a signal — it takes the cables arriving at its port out of the rack and remembers them — and moving it never put them back. Lifting a switch off a port now revives that port at once, so being carried counts as being switched on.
- Clicks on any window floating over the rack fell through to the modules behind it. Clarity asked only whether one of ITS OWN windows was in the way, by name, so anybody else's window was invisible to it — including the chart window of our own MPX plugin.
- A rule whose shape changed in the table left its old version behind in a user's settings, sitting in front of the replacement. IN, OUT, LEFT, RIGHT, L and R became whole words; a file written before that kept the substring versions, and a substring L matches any name with the letter L in it — so a port called "External trigger" was coloured as audio by the L in "External".
- Linux builds asked for a glibc newer than many distributions have, and would not load at all. Linux and Windows are both built in the VCV plugin toolchain now, which is what the library itself uses; every build prints the highest glibc it needs.

## 2.0.8 — 8 September 2026

### Fixed
- Rack could crash on quit. The module browser builds a preview widget for every module it shows, with no module behind it, so such a widget never counts itself into the rack. Test Gear's destructor did rack-wide cleanup whenever the count of them reached zero without asking whether it was itself one of the counted ones — and a preview is destroyed while the scene is being torn down, by which time the rack it walked had already been deleted. Both modules now do nothing at all from a preview, and the functions that walk the rack check it is still there. Anyone who opened the module browser could hit this, whether or not they used Test Gear.

## 2.0.7 — 8 September 2026

### Fixed
- Adding Test Gear applied Clarity's changes to the rack even with no Clarity present: jacks recoloured, knobs restyled, cables animated, and pinch zoom, slider scrolling, click-to-patch and tracing all switched on. Both modules install the same overlay, but the flags it reads are only written by Clarity and only cleared when the last Clarity leaves — so in a rack that never had one, the defaults stood, and every default was on. They now start off, and they are cleared while no Clarity is present.
- The same fault turned cable colouring on, which writes to the patch. A rack with only Test Gear in it had its saved cable colours overwritten.
- Deleting the last Clarity left the drawn recording pointer and its value readout switched on. Clarity sets ten flags and only eight were being cleared.

## 2.0.6 — 8 September 2026

### Added
- Test Gear: a diagnostics window, from the module's right-click menu. Reports what the module's per-sample work costs and, beside it, what a pair of clock reads costs with nothing between them — Rack brackets every module with such a pair when its CPU meter is on, so if the two figures are close the meter is largely reporting itself.
- The window also counts the widgets attached, and has six switches for taking the work out a piece at a time while Rack's own meter is watched. Everything is restored when it closes.
- A changelog, named in the plugin manifest so Rack shows it in the module menu.

### Fixed
- Test Gear: the widget list had grown past the panel and was overwriting the Monitor out jack and its label.
- Test Gear: the widgets are now removed when the last Test Gear leaves the rack, rather than being left attached to nothing. Undo brings them back.
- Clarity: a cable started by click-to-patch could be invisible — it was made without a colour, and an unset colour is black at zero opacity.
- Clarity: a port whose name contains "level" is now coloured as CV rather than audio.

### Changed
- Test Gear: less work when idle. One shared counter instead of five, the sample rate published only when it changes, and a hidden widget no longer captures its port.

## 2.0.5 — 6 September 2026

### Added
- A voltmeter, clipping onto a terminal like the scope: current value and peak.
- A frequency counter, reading in hertz, as a note, or as volts per octave.
- A switch, and a pair of marks that can be pressed.

### Fixed
- Taps survive a patch being loaded.
- A tap can be asked for any channel of a polyphonic port, not only the first.
- A tap destroyed twice no longer misbehaves.

## 2.0.4 — 3 September 2026

### Fixed
- Idle Test Gear cost whole percents of a core. Deciding which Test Gear was in charge walked every module widget in the rack, with a cast on each, once per sample — and reached into the widget tree from the audio thread. The modules say when they arrive and leave instead.

### Changed
- The menu was reorganised.

## 2.0.3 — 3 September 2026

### Fixed
- A cable picked up and put back on the same port kept the colour of the port it came from. What is remembered about a cable is now the colour we gave it, not merely where it goes, so a cable repainted by any route is put right on the next frame.

Released because the forum fixes went out as 2.0.2 and this landed nine hours after that tag, so no build had both.

## 2.0.2 — 2 September 2026

Seven people replied to the announcement thread and between them found six defects and asked for four things.

### Fixed
- Drawing over hidden widgets: a control inside a hidden container still reported itself visible. The whole chain up to the module is asked now. Reported by DaveVenom.
- The cursor never came back, sticking as a resize arrow over the rack. A leave event is not delivered when the widget under the pointer is deleted, hidden or scrolled away, so a shape is asked for every frame it is wanted instead. Reported by contemporaryinsanity on Linux.
- The trace handle could not be grabbed: the hit test measured to its centre with an eight-pixel reach, inside something drawn twenty-three by eleven. It also drifted far along a long cable, because the walk returned the first sample past the mark rather than the mark.
- The trace refused a press near a port, because any press touching a port's square box went to the port. Whichever is nearer wins now.
- The value readout drew over Rack's own menu. Reported by DaveVenom on Windows.
- The drawn pointer stuck as held: Rack locks the cursor while a knob is turned and dispatches no button events while locked, so the release never arrived. The button state is read from the window each frame now.

### Changed
- Cable colouring is no longer destructive. Every cable's colour is remembered and put back when the switch goes off or the module leaves, there is a menu item for putting them back on demand, and the switch now starts off. Reported by technochitlin, whose patch went yellow on load.
- The default colour scheme is now Omri Cohen's — red audio, blue gates, yellow volt per octave, green modulation. Ours meant the opposite of his on yellow, so anyone who had learned the common scheme read every patch backwards. The original is kept as a scheme that can be chosen. Asked for by Ohmer.

## 2.0.1 — 1 September 2026

### Added
- The four signal-family colours can be chosen: right-click a Clarity panel and choose "Jack and cable colours". Saved beside Rack's own settings rather than in the patch, since Clarity colours every module in the rack including other plugins'.
- Clarity knows the MPX family, the note cables from the Dreamer MPX plugin.

### Fixed
- The demonstration video stopped partway through on some players. The encoder was declaring H.264 level 5.0, above what several hardware decoders accept; every encode now names its profile and level explicitly.

## 2.0.0 — 31 August 2026

First release.

- **Clarity** — colour-coded jacks and cables, a consistent knob style drawn over other people's modules, cable direction animation, cable tracing, click-to-patch, and pinch zoom.
- **Test Gear** — instruments that attach to any terminal in the rack: a scope, a spectrum analyser, an audio monitor, and signal injectors.
