# Dreamer Development — VCV Rack modules

**[Clarity](docs/clarity.md)** changes how the whole rack is drawn and handled. **[Test Gear](docs/test-gear.md)** puts instruments on your terminals: scopes, a frequency analyser, a monitor to listen through, and a set of signal generators.

They are separate because they are different kinds of thing. Clarity is ambient — it has many tricks to make things clearer and less effortful. Test Gear is what you reach for. Each works without the other.

---

## Clarity

A patch you can read at a glance: what kind of signal a jack carries, whether it is an input or an output, where a cable goes, and which way the signal is flowing. It reads the same way on every module in the rack, whoever wrote it. Less of your attention on working out what is connected to what, and more of it left for the patch itself.

Each feature is a switch on the panel, so how much the module does is up to you.

### Reading the rack

- **Jacks coloured by signal family** — yellow for audio, orange for control voltage, blue for gates and triggers, and green for pitch as volt per octave.
- **Input or output at a glance** — a dashed ring hugs the outer edge of an output and the hole of an input. Shape carries direction, colour carries family.
- **Cables coloured by their destination**, so you know at a glance what a cable is being used for, regardless of the type of source it came from.
- **Animated cable directions** — dashes slowly crawl from source to destination, so you know which way the signal is flowing even if you can only see the middle of the cable.
- **One consistent knob** across every plugin. Your rack looks and feels more coherent.

### Handling cables

- **Cable trace assist** — hover a cable end and a pill appears on it; click the pill to hold that one cable bright and hide every other cable in the rack. Where several cables converge, clicking steps through them; right-click the pill to lift that particular cable off. Click on any module's panel to bring every cable back.
- **Add cables without dragging** — you no longer have to hold the button down. Click a jack to pick up its cable and click another to drop it, or drag and release exactly as you always have; releasing over a jack connects it. Both gestures are live at once, so there is nothing to learn, and it can be restful over a long distance. The rack scrolls itself when a carried cable reaches the edge of the view.
- **Pinch to zoom** the rack on a trackpad.

---

## Test Gear

Right-click any terminal, choose **Widgets** at the top of the menu, and clip an instrument onto it. It follows the pointer until you click to place it. Everything you attach follows the module it is attached to, is saved with the patch, and can be moved to another terminal by dragging the loop at its jack. A viewer on an output with nothing patched to it still works: the output is woken with a hidden cable, since a module does not compute an output that nothing is connected to.

### Viewers

- **Scope** — captures at the engine's sample rate, not the frame rate, keeps about eleven seconds of history to pan back through, and frames the signal for you when it opens. Triggering is a strip down its left edge; drag a link from that strip to another jack to trigger from somewhere else.
- **Analyser** — a spectrum on a logarithmic frequency axis, with the peak read out as a note as well as a frequency, and ticks on its harmonics. Press **W** for a waterfall: the same spectrum with time as the second axis. Pinch to zoom the frequency axis, scroll sideways to pan it.
- **Audio monitor** — hear any point in the patch. Patch Monitor out to your interface once, and every monitor you attach after that is audible through it, each with its own level and mute. They sum, so you can hear a modulator under the sound it is shaping.

### Generators

Gate, pulse, clock, DC level, LFO, VCO, note, volt per octave, noise and an attenuverter. They work on a terminal that already has a cable in it — a generator adds to what is there rather than replacing it, so nothing has to be unplugged to try something.

---

## Building

Set `RACK_DIR` to your Rack SDK and run `make`. Requires a C++11 compiler.

```
RACK_DIR=/path/to/Rack-SDK make
```

## Licence

GPL-3.0-or-later. No artwork ships with the plugin — every panel and every face is drawn in code.

