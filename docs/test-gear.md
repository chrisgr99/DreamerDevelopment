# Test Gear

Test Gear attaches instruments to ports: instruments are placed at the point in the patch being measured, rather than being patched to a module elsewhere in the rack.

*This manual is incomplete. The module is finished and released; the illustrations and several sections have still to be written.*

Place one Test Gear module anywhere in the rack. Its panel lists the widgets and carries the module's one port, Monitor out.

## Adding a widget

**Right-click any port and select "Widgets"**, the first entry on the menu, then choose from the list. The widget follows the pointer until a click places it.

Option-clicking a port opens the same list without going through the menu.

Every widget behaves the same way once attached:

- A **loop** at the port, joined to the face by a line, shows which port it is attached to. Drag the small tab on that loop to another port to move it there, or away from any port to remove it. The face stays where it was placed.
- The **red cross** at the top-left corner removes it.
- **Drag the face** to move it. It is anchored to the port rather than to the screen, so it moves with the module and scrolls and zooms with the rack.
- Dragging the loop's tab towards a port **off the edge of the view** scrolls the rack, as carrying a cable does.
- The widget is **saved with the patch**, including its position and its settings.

---

# Viewers

## Scope

Captures at the engine's sample rate rather than the frame rate, and keeps about twenty-two seconds of history, so a paused trace can be scrolled back through. Above 48 kHz the history is kept at 48 kHz or near it — at 96 kHz each stored sample is the mean of two — so it holds the same length of time at any engine rate.

The time base runs from 20 microseconds to 5 seconds per division. Slow time bases are triggered like fast ones: the trace stays still and is redrawn at each trigger edge, so a slow LFO updates once a cycle. A long window is drawn one column per pixel, from each column's lowest value to its highest, so a slow view costs no more than a fast one and a fast signal inside it shows as the band it fills.

**AUTO on a slow signal** looks back through up to eight seconds of history, and waits until two cycles have been captured before setting the time base.

A scope sets its scales from the signal when it is attached, once it has a full window to measure, which is what pressing **AUTO** does subsequently. Moving it to another port repeats this. A scope restored from a patch does not: the saved scales are the settings.

**Controls**, along the bottom and displayed on hover: transport (run and pause), **F** follow, **<** home, **AUTO** autoset, **AC** coupling, **G** grid.

**Scrolling** the face moves the trace: sideways to pan through the history when paused, vertically to move it up and down. One axis at a time, determined by the direction the gesture begins in. Over the readout below the face, scrolling changes the scales instead — volts per division on the left, time base on the right.

**Resizing**: hovering the face brings up seven handles just outside it, in the frame's colour — one in the middle of each edge and one at each corner except the top left, where the close button is. Drag one to resize. They stay for a second after the pointer leaves the face, so there is time to reach them, and they are the same size on screen at any zoom. Nothing inside the face resizes, so the whole face is free for dragging and scrolling.

### Triggering

The left ten pixels of the face are the trigger strip. Click it to enable and disable triggering. Drag up and down within it to set the level, which is marked by an amber triangle; click the triangle to switch between the rising and falling edge.

**External triggering**: drag from the strip to any other port and release. An amber loop attaches there, and the trace is then triggered when that signal rises through 1 V, the standard gate threshold. The triangle becomes a cross, since the level is fixed. Right-click the strip to remove the external trigger; the loop's own tab moves or removes it, as with any other attachment.

## Analyser

A spectrum on a logarithmic frequency axis, so that octaves occupy equal distances. The window is 8192 samples, taken from the same history the scopes use, so its resolution does not depend on the settings of any scope on the same signal.

- **The peak is reported as a note** as well as a frequency — "A4 440.2". The peak is interpolated between bins, which are about six hertz apart.
- **Amber ticks** mark the harmonics of the peak.
- **Averaged** by default. An unaveraged spectrum changes too rapidly to read; the average settles within a few frames and still follows a change. It can be disabled in the right-click menu.

**W** switches on the **waterfall**: the same spectrum with time as the second axis, the newest row at the top. It is drawn from the unaveraged spectrum, so short events are not smeared across several rows.

**Pinch** on the face zooms the frequency axis about the pointer, and **scrolling sideways** pans it. The waterfall retains its history through this: rows are stored across the whole range and the view is drawn as a crop of them. **Full range** in the right-click menu returns to the full span.

The **transport** in the lower left holds the display: the averaged spectrum is left as it is and the waterfall stops adding rows.

## Audio monitor

Connect **Monitor out** to an audio interface once; every monitor attached after that is audible through it.

- **Click the face** to mute. **Scroll** to set the level, in steps of one and a half decibels, from -60 dB to +6 dB.
- **Monitors are summed.** The port is a mixing bus rather than a switch, so several points in a patch can be listened to at the same time.
- **A monitor reads the port's voltage directly**, so it can be attached to an output as well as an input, and nothing is inserted into the signal path.
- The bus is DC-blocked at about 20 Hz, so control voltages can be monitored: an envelope resting at five volts would otherwise be a constant offset at the interface.

## Voltmeter

Reads the voltage on a terminal, on an input or an output alike, and inserts nothing into the signal.

- **The voltage now, large, and the lowest and highest beneath it**, small. Each extreme is held for a second after the signal last reached it, so it can be read, then let go to what is there now.
- **The extremes are found at the engine's rate**, not at the frame rate. A meter that looked once a frame would catch one sample in eight hundred of an audio signal and report whatever it happened to land on.
- **Always the same width**: a sign, two digits, a point and two decimals. A reading past ninety-nine volts is held there rather than taking a third digit, since a number that changes width as it moves is one the eye cannot rest on.
- **Polyphonic cables** are read on the first channel, and the word says so — `METER 1/4` on a cable of four.

## Frequency meter

Reads the pitch on a terminal, on an input or an output alike, and inserts nothing into the signal.

- **One reading, and a chip above it saying which unit.** **Click the chip** to change unit; the right-click menu offers the same three by name. Clicking anywhere else only picks the widget up.
- **Hz** is the measured pitch — `440.00`, or `1.234k` above a thousand.
- **NOTE** is that same pitch as a note, with its distance from equal temperament in cents: `A4 +07`, `F#2 -13`.
- **V/OCT** is not a measurement of pitch but of intent: the note a steady control voltage is *asking* for, with nought volts as C4. Clip one on a volt-per-octave cable while tuning and it reads the note being requested rather than the one being made.
- **The pitch is the time between crossings of the signal's own slow mean**, measured at the engine's rate with a threshold either side of the line so that a wobble is not counted as several cycles. It reads an oscillator or an LFO down to a fraction of a hertz.
- **Dashes are a reading.** A signal too small to be anything, a signal that has stopped, or one whose cycles disagree with each other — a chord, noise, a heavily folded wave — has no single frequency, so none is shown.
- **The tuning standard** is set from the right-click menu, from 415 Hz to 444 Hz. It moves the note and the cents; the hertz reading is what it is.
- **Polyphonic cables** are read on the first channel, and the chip says so — `Hz 1/4` on a cable of four.

---

# Generators

Generators attach to **inputs**. They can be attached to a port that already has a cable connected: the engine sums several cables into one input, so a generator adds to what is there rather than replacing it.

Each has a **readout** which scrolling changes — coarse to the left of the decimal point, fine to the right, at a tenth of the rate.

| Widget | Output |
| --- | --- |
| **Gate button** | 10 V while the button is held |
| **Pulse button** | One 1 ms pulse per press |
| **Clock** | A stream of pulses, set in beats per minute |
| **Constant voltage** | A steady voltage, shown in volts or as a note name, nought volts being C4, chosen from the right-click menu under **Show as**. Changing to a note name snaps it to the nearest note |
| **LFO** | A repeating waveform, 0.01 to 100 Hz. **Click the shape** on the readout to change it, and the **B** or **U** beside it for bipolar or unipolar |
| **Oscillator** | The same at audio rates, 1 Hz to 8 kHz. Defaults to A4. Dialled by frequency or by note name, chosen from the right-click menu under **Dial by**. The same two marks change its shape and polarity |
| **Noise** | White, pink, brown, blue or violet, selected from the right-click menu |
| **Attenuverter** | Scales and inverts the signal already arriving at that port. On an output, it scales what arrives at every input that output feeds |
| **Mute** | Takes the connection into that port out of the rack and puts it back, or on an output, every connection leaving it |

Only the mute and the attenuverter can be clipped onto an output: nothing is injected into an output, which its own module drives. An attenuverter there reads the output and lays a hidden cable of its own into each input the output feeds, kept in step as cables are added and removed.

**A widget goes when its jack does.** A module that shows a different set of controls as it is configured hides the ones it is not using, and a widget clipped to one of those jacks is removed rather than left reading a port that is no longer there. A mute removed this way puts back the cables it was holding.

**A port with a widget on it starts a cable as usual.** A generator reaches its port through a hidden cable of its own, and a drag from the port never picks that cable up.

Switching a generator on or off ramps its level over a few milliseconds rather than stepping, since a step produces an audible click.

## Mute

Not a generator, though it lives among them: it stops the connection into a port and restores it.

- **A click throws it; a drag only moves it.** Which it was is decided when the button is released, by whether the pointer travelled — so nudging the mute to a tidier place on the panel does not break the connection under it. Every widget with a setting behaves this way.
- **The light is on when it is muting**, as a mixer's mute is. It takes the cables out rather than silencing a signal, and nobody mutes a gate, but mute is the word everybody already has for a control that stops one point in a patch while the rest plays.
- **It takes the cables out and holds them**, and puts them back when it is unmuted. Nothing is altered, so unmuting leaves the patch exactly as it was.
- **A cable that is held is drawn as a short stub** leaving the port in its own colour, at the angle the cable left at, so you can see what is waiting on the other side and which of several cables they are.
- **A cable patched into a muted port** is taken as well. The button means "this port", not "whatever was here when you pressed it".
- **It survives saving.** Rack writes the cables it can see, and a muted port's are not among them, so they are written into the mute's own state and put back from there.

**Why it does not cancel the signal instead.** The obvious way is the attenuverter's: the engine sums everything arriving at an input, so sending the exact opposite of what a cable delivers leaves nothing. It works for a control voltage and it cannot work for anything else. Rack decides the order it processes modules in and a plugin has no say, so the value read from the source may be the one it produced a sample ago while the destination reads the one it produces now — and the difference of a signal with itself one sample back is a high-pass filter. Audio comes through thinner and quieter rather than stopping, and a gate, flat except at its edges, comes through as a spike at every rise and fall.

---

## Notes

**A viewer on an unconnected output.** Most modules do not compute an output that has no cable connected to it, so a viewer attached to one would receive nothing. Attaching a viewer connects a hidden cable from that output to this module, which causes the source to compute; the cable is removed when the viewer is removed, or when a cable is connected to that output. A module that behaves differently while its output is unconnected — one that normals the signal elsewhere — will behave as though it has been connected, because it has.

**One Test Gear module is sufficient.** A second has no effect; where there are several, the first performs the processing and saves the widgets.

**Bypassing it hides the widgets.** A bypassed Test Gear does nothing, as though it had been removed: every widget is hidden, the generators and the monitor stop, and Widgets… is no longer offered on a port's right-click menu. Unbypassing brings every widget back as it was. Deleting the last Test Gear module removes the widgets for good. Widgets… is offered only while a Test Gear module is in the rack.

**Eight generators, sixteen monitors and sixteen voltmeters** can exist at once, with thirty-two signal taps shared between the scopes, analysers, monitors and voltmeters.

**The hidden outputs.** A generator is a real cable from one of this module's outputs to the port it drives, which is what makes Rack responsible for removing it when a module is deleted. The cable and its plugs are hidden.
