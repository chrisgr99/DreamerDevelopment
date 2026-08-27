# Dreamer Development — VCV Rack modules

## DRUI

Makes a whole rack clearer to read, whoever wrote the modules in it.

- **Jacks coloured by signal family** — audio, CV, gate and pitch each get their own colour.
- **Input or output at a glance** — a dashed ring hugs the outer edge of an output and the
  hole of an input. Shape carries direction, colour carries family: a jack told apart only by
  hue cannot be told apart in peripheral vision, under magnification, or by anyone whose
  colour vision differs.
- **Bipolar marking** — ports carrying a signal that swings negative are marked.
- **Cables coloured by their destination**, so where a cable is going is visible from either
  end.
- **Signal flow along cables** — dashes crawl from source to destination, their length keyed
  to the destination's signal family.
- **Drawn knobs** — one consistent, high-contrast knob across every plugin.
- **Pinch to zoom** on a trackpad, with no modifier held.

Everything is optional and switched from the DRUI module's right-click menu.

Drop one DRUI module anywhere in your patch to switch it on. It has no inputs or outputs; it
exists because a Rack plugin cannot run until a module of its own is placed.

## Building

Needs the Rack SDK:

    RACK_DIR=../Rack-SDK make
    RACK_DIR=../Rack-SDK make install

Built against the official SDK rather than any modified Rack, so it runs on stock VCV Rack
Free and Pro.

## Licence

GPL-3.0-or-later. Not affiliated with VCV.
