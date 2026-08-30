# Test Gear

Test gear for your rack: instruments you clip onto a terminal, where the signal is, instead of patching a lead across the rack to a module that shows you one.

Place one Test Gear module anywhere. Its panel lists what is on offer and carries the one jack these need, MONITOR OUT.

## Adding a widget

**Right-click any terminal and select "Widgets"** — the first entry on the menu — then choose from the list. The widget follows your pointer until you click to place it, so the first thing you do is decide where it goes rather than finding it landed on something you were looking at.

Option-clicking a terminal opens the same chooser without going through the menu.

Every widget behaves the same way once attached:

- A **loop** at the jack, joined to the face by a line, shows what it is attached to. Drag the small tab on that loop to another jack to move it there; drop it away from any jack to remove it. The face stays where you put it.
- The **red X** at the top-left corner removes it.
- **Drag the face** to move it. It is anchored to the port, not the screen, so it travels with the module and scrolls and zooms with the rack.
- Dragging the loop's tab to a terminal **off the edge of the view** scrolls the rack, exactly as carrying a cable does.
- Everything is **saved with the patch**, including where it sits and how it is set up.

---

# Viewers

## Scope

Captures at the engine's sample rate rather than the frame rate — a 440 Hz tone sampled thirty times a second is not a small picture of a waveform, it is noise. About eleven seconds of history is kept, so a paused trace can be scrolled back through.

A scope arrives set up: it frames the signal for you the moment it has a full window to judge, which is exactly what pressing **A** does later. Moving it to another terminal does the same again. A scope restored from a patch does not — the scales you saved are the settings.

**Controls**, along the bottom and revealed on hover: transport (run and pause), **F** follow, **<** home, **A** autoset, **AC** coupling, **G** grid. Autoset frames the signal for you, and runs once when a scope opens.

**Scrolling** the face moves the trace: sideways to pan through history when paused, vertically to move it up and down. One axis at a time, claimed from the way the gesture starts. Over the readout below the face, scrolling changes the scales instead — volts per division on the left, time base on the right.

**Resizing**: drag any edge or corner.

### Triggering

The left ten pixels of the face are the trigger strip. Click it to switch triggering on and off. Drag up and down in it to set the level, marked by an amber triangle; click the triangle to flip between rising and falling edges.

**External triggering**: drag from the strip to any other jack in the rack and drop. An amber loop attaches there, and the trace is then triggered by that signal rising through 1 V — the standard gate threshold, so a gate or clock does what you expect. Its triangle becomes a cross, since the level is fixed. Right-click the strip to drop the external trigger; the loop's own tab moves or removes it, like any other.

## Analyser

A spectrum on a logarithmic frequency axis, so octaves are equal distances and a harmonic series has a shape you can learn. The window is 8192 samples, taken from the same history the scopes use, which is why its resolution does not depend on what any scope on the same signal is set to.

- **The peak is read as a note** as well as a frequency — "A4 440.2" — which makes it a tuner. The peak is refined between bins, since a bin is about six hertz wide.
- **Amber ticks** mark the harmonics of the peak, so a filter removing them or a waveshaper adding them is visible at a glance.
- **Averaged** by default. A raw spectrum flickers too hard to read; the average settles in a few frames and still follows a change. Switch it off in the right-click menu.

**W** switches on the **waterfall**: the same spectrum with time as the second axis, newest row at the top. Where a spectrum says what is in a sound, a waterfall says what it is doing — a filter sweeping, an envelope opening, a sequence moving. It draws from the raw spectrum rather than the average, so events stay crisp.

**Pinch** on the face to zoom the frequency axis about the pointer, and **scroll sideways** to pan it, so a region of interest can fill the face instead of being a sliver beside a top octave that carries nothing. The waterfall keeps its history through this: rows are stored across the whole range and the view is drawn as a crop of them. **Full range** in the right-click menu goes back out.

## Audio monitor

Hear any point in the patch. Patch **MONITOR OUT** to your audio interface once; every monitor you attach after that is audible through it.

- **Click the face** to mute. **Scroll** to set the level, in decibel-and-a-half steps, from -60 dB to +6 dB.
- **They sum.** The jack is a mixing bus, not a switch, so several at once lets you hear a modulator under the sound it is shaping rather than flipping between them.
- **It listens to outputs as readily as inputs**, and nothing is inserted into the signal path — a monitor cannot change what it is listening to.
- The bus is DC-blocked at about 20 Hz, so CV is safe to monitor: an envelope resting at five volts would otherwise be a DC offset on a loudspeaker.

---

# Generators

Generators attach to **inputs**. They work on a terminal that already has a cable in it: the engine sums several cables into one input, so a generator adds to what is there rather than replacing it, and nothing has to be unplugged to try something.

Each has a **readout** you scroll to change — coarse to the left of the decimal point, fine to the right, at a tenth the rate.

| Widget | What it sends |
| --- | --- |
| **Gate button** | 10 V while the button is held |
| **Pulse button** | One 1 ms pulse per press |
| **Clock** | A stream of pulses, set in beats per minute |
| **DC level** | A steady voltage |
| **LFO** | A repeating waveform, 0.01 to 100 Hz. Right-click for the shape and for unipolar or bipolar |
| **VCO** | The same at audio rates, 1 Hz to 8 kHz, set in hertz. Defaults to A4 |
| **Note** | A VCO set by note name rather than by frequency |
| **Volt/oct** | A pitch as a control voltage, one volt per octave, shown as a note |
| **Noise** | White, pink, brown, blue or violet, chosen from the right-click menu |
| **Attenuverter** | Scales and inverts what the cable is already delivering to that port |

Switching one on or off ramps over a few milliseconds rather than stepping, because a step is a click.

---

## Notes

**Watching a bare output wakes it up.** Nearly every module skips computing an output that has no cable in it, so a viewer on an unpatched jack would see nothing. Attaching one lays a hidden cable from that output into this module, which is what makes the source compute; it goes when the viewer goes, or when you patch a real cable there. The cost is that a module which behaves differently while its output is empty — one that normals the signal elsewhere — will behave as though it has been patched, because it has.

**One Test Gear module runs everything.** A second is harmless but does nothing; if you have several, the first one does the work and the saving.

**Bypassing it silences the widgets** — the generators and the monitor both ride in its process — while everything drawn goes on looking alive. A generator says WIDGETS BYPASSED on its face when that happens.

**Eight generators and sixteen monitors** can exist at once, along with thirty-two taps shared by the scopes, analysers and monitors.

**The hidden outputs.** A generator is a real cable from one of this module's outputs into the port it drives, which is what makes Rack responsible for cleaning it up. The cable and its plugs are hidden, because what you should see is the loop at the terminal rather than a lead running across the rack.

