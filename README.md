# Dreamer Development — VCV Rack modules

## DreamRack

DreamRack is a standalone modular synthesizer, and for the last while it has doubled as a test
bed for modular interface design — trying alternatives to conventions every rack has inherited,
keeping what turns out to work and discarding what does not. This plugin brings the results to
VCV Rack.

What they add up to is clarity. A patch you can read at a glance: what kind of signal a jack
carries, whether it is an input or an output, where a cable goes, what a control is set to —
and never a moment spent wondering which way a signal is flowing. You can tell an input from
an output without reading a word of the panel, and it reads the same way on every module in
the rack, whoever wrote it. Less of your attention on working out what is connected to what,
and more of it left for the patch itself.

You choose which of the features you want, with switches on the panel.

### Reading the rack

- **Jacks coloured by signal family** — audio, CV, gate and pitch each get their own colour.
- **Input or output at a glance** — a dashed ring hugs the outer edge of an output and the
  hole of an input. Shape carries direction, colour carries family: a jack told apart only by
  hue cannot be told apart in peripheral vision, under magnification, or by anyone whose
  colour vision differs.
- **Cables coloured by their destination**, so where a cable is going is visible from either
  end, and recoloured when you re-plug them.
- **Animated cable directions** — dashes crawl from source to destination, their length keyed
  to the destination's signal family.
- **One consistent knob** across every plugin.

### Handling cables

- **Cable trace assist** — hover a cable end for a pill, click it to hold that one cable
  bright and hide every other cable in the rack. Where several cables converge, clicking steps
  through them; right-click the pill to lift that particular cable off.
- **Click to pull cables** — click a jack to pick up its cable and click another to drop it,
  with no button held in between. The rack scrolls itself when a carried cable reaches the
  edge of the view. Off by default, since it changes Rack's most basic gesture.

### Instruments that clip onto a terminal

- **Oscilloscopes on terminals** — Option-click a jack and clip on a scope. It captures at the
  engine's sample rate, not the frame rate, keeps about eleven seconds of history to pan back
  through, and frames the signal for you when it opens. It follows the module it is attached
  to, and saves with the patch.
- **Signal widgets on terminals** — clip a gate button, a trigger button, a DC level, an LFO,
  an audio oscillator dialled by frequency or by note, a note source in volts per octave, or
  an attenuverter onto any input. They sum with whatever is already patched there, so nothing
  has to be unplugged to try something.

### Elsewhere

- **Pinch to zoom** on a trackpad, with no modifier held.
- **Scroll wheel adjusts sliders**, in menus and anywhere else they appear — in the module's
  right-click menu.

Place one DreamRack module anywhere in your patch to switch it on. It exists because a Rack
plugin cannot run until a module of its own is placed; its process() is also where the scope
capture and the signal widgets run, so bypassing it silences both.

## Building

Needs the Rack SDK:

    RACK_DIR=../Rack-SDK make
    RACK_DIR=../Rack-SDK make install

Built against the official SDK rather than any modified Rack, so it runs on stock VCV Rack
Free and Pro.

## Licence

GPL-3.0-or-later. Not affiliated with VCV.

---

## Draft wording — to choose between (delete once settled)

**A. Shorter, leads with what you can read at a glance.**

> What they add up to is clarity. A patch you can read at a glance: what kind of signal a jack
> carries, where a cable goes, what a control is set to — and never a moment spent wondering
> which way a signal is flowing. Less of your attention on working out what is connected to
> what, and more of it left for the patch itself.

**B. The same, plus the input/output and consistency points.**

> What they add up to is clarity. A patch you can read at a glance: what kind of signal a jack
> carries, whether it is an input or an output, where a cable goes, what a control is set to —
> and never a moment spent wondering which way a signal is flowing. You can tell an input from
> an output without reading a word of the panel, and it reads the same way on every module in
> the rack, whoever wrote it. Less of your attention on working out what is connected to what,
> and more of it left for the patch itself.

Both say the same things; B adds that direction and port type are legible without panel text,
and that the reading is the same across every developer's modules. B is longer and repeats
"at a glance" against "without reading a word", which is the part worth cutting.
