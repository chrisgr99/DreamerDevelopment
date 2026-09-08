# Changelog

Newest first.

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
