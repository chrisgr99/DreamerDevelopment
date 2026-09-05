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

Captures at the engine's sample rate rather than the frame rate. About eleven seconds of history is retained, so a paused trace can be scrolled back through.

A scope sets its scales from the signal when it is attached, once it has a full window to measure, which is what pressing **A** does subsequently. Moving it to another port repeats this. A scope restored from a patch does not: the saved scales are the settings.

**Controls**, along the bottom and displayed on hover: transport (run and pause), **F** follow, **<** home, **A** autoset, **AC** coupling, **G** grid.

**Scrolling** the face moves the trace: sideways to pan through the history when paused, vertically to move it up and down. One axis at a time, determined by the direction the gesture begins in. Over the readout below the face, scrolling changes the scales instead — volts per division on the left, time base on the right.

**Resizing**: drag any edge or corner.

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

- **One number, and a word above it saying which.** **VOLTS** is the voltage at this moment; **PEAK** is the largest reading of the last quarter of a second, held so it can be read. **Click the face** to change over; the right-click menu offers the same choice by name.
- **The peak is found at the engine's rate**, not at the frame rate. A meter that looked once a frame would catch one sample in eight hundred of an audio signal and report whatever it happened to land on. It is the largest reading *by size*, shown with its sign, so a signal swinging to minus eight reads −8.
- **Always the same width**: a sign, two digits, a point and two decimals. A reading past ninety-nine volts is held there rather than taking a third digit, since a number that changes width as it moves is one the eye cannot rest on.
- **Polyphonic cables** are read on the first channel, and the word says so — `VOLTS 1/4` on a cable of four.

---

# Generators

Generators attach to **inputs**. They can be attached to a port that already has a cable connected: the engine sums several cables into one input, so a generator adds to what is there rather than replacing it.

Each has a **readout** which scrolling changes — coarse to the left of the decimal point, fine to the right, at a tenth of the rate.

| Widget | Output |
| --- | --- |
| **Gate button** | 10 V while the button is held |
| **Pulse button** | One 1 ms pulse per press |
| **Clock** | A stream of pulses, set in beats per minute |
| **DC level** | A constant voltage |
| **LFO** | A repeating waveform, 0.01 to 100 Hz. **Click the shape** on the readout to change it, and the **B** or **U** beside it for bipolar or unipolar |
| **VCO** | The same at audio rates, 1 Hz to 8 kHz, set in hertz. Defaults to A4. The same two marks change its shape and polarity |
| **Note** | A VCO set by note name rather than by frequency |
| **Volt/oct** | A pitch as a control voltage, one volt per octave, displayed as a note |
| **Noise** | White, pink, brown, blue or violet, selected from the right-click menu |
| **Attenuverter** | Scales and inverts the signal already arriving at that port |
| **Switch** | Breaks and remakes the connection into that port |

Switching a generator on or off ramps its level over a few milliseconds rather than stepping, since a step produces an audible click.

## Switch

Not a generator, though it lives among them: it turns the connection into a port on and off.

- **The light is on when the switch is on**, and on means the signal is getting through. A mute is the wrong idea for most of what travels down a cable — nobody mutes a gate, they switch it off — so one word and one polarity mean the same thing whatever the signal is.
- **It takes the cables out and holds them**, and puts them back when it goes on again. Nothing is altered, so switching it back on leaves the patch exactly as it was.
- **A cable that is held is drawn as a short stub** leaving the port in its own colour, at the angle the cable left at, so you can see what is waiting on the other side and which of several cables they are.
- **A cable patched into a port while its switch is off** is taken as well. The button means "this port", not "whatever was here when you pressed it".
- **It survives saving.** Rack writes the cables it can see, and a switched-off port's are not among them, so they are written into the switch's own state and put back from there.

**Why it does not cancel the signal instead.** The obvious way is the attenuverter's: the engine sums everything arriving at an input, so sending the exact opposite of what a cable delivers leaves nothing. It works for a control voltage and it cannot work for anything else. Rack decides the order it processes modules in and a plugin has no say, so the value read from the source may be the one it produced a sample ago while the destination reads the one it produces now — and the difference of a signal with itself one sample back is a high-pass filter. Audio comes through thinner and quieter rather than stopping, and a gate, flat except at its edges, comes through as a spike at every rise and fall.

---

## Notes

**A viewer on an unconnected output.** Most modules do not compute an output that has no cable connected to it, so a viewer attached to one would receive nothing. Attaching a viewer connects a hidden cable from that output to this module, which causes the source to compute; the cable is removed when the viewer is removed, or when a cable is connected to that output. A module that behaves differently while its output is unconnected — one that normals the signal elsewhere — will behave as though it has been connected, because it has.

**One Test Gear module is sufficient.** A second has no effect; where there are several, the first performs the processing and saves the widgets.

**Bypassing it silences the widgets.** The generators and the monitor are processed by this module, so bypassing it stops them, while everything drawn continues to be displayed. A generator displays WIDGETS BYPASSED on its face in that state.

**Eight generators, sixteen monitors and sixteen voltmeters** can exist at once, with thirty-two signal taps shared between the scopes, analysers, monitors and voltmeters.

**The hidden outputs.** A generator is a real cable from one of this module's outputs to the port it drives, which is what makes Rack responsible for removing it when a module is deleted. The cable and its plugs are hidden.
