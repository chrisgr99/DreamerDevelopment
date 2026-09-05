# Dreamer Development — VCV Rack modules

## A note from the author

I made these features for my own use in VCV Rack. As I have got older my eyesight is not as good, and I have difficulty reading faceplates and following cables across a patch. I work on a trackpad, where dragging a cable means keeping the pad pressed from beginning to end. Also, I wanted to pinch to zoom.

Having built them, I have put them in a plugin in case anyone else finds them useful. They are not offered as a better way to use Rack. They are the way I use it, and every one of them is a switch you can turn off.

---

**[Clarity](docs/clarity.md)** changes how the whole rack is drawn and how you interact with it. **[Test Gear](docs/test-gear.md)** attaches instruments to ports, and takes no rack space to do it: oscilloscopes, a frequency analyser, an audio monitor, a voltmeter, a switch, and a set of signal generators.

They are separate modules because they do different things. Clarity applies to the whole rack and is set once. Test Gear is used a widget at a time. Each works without the other.

---

## Clarity

Clarity shows what kind of signal a port carries, whether it is an input or an output, where a cable goes, and which way the signal is flowing. It applies to every module in the rack, whichever plugin drew it.

Each feature is a switch on the panel and can be turned off.

### Reading the rack

- **Ports coloured by signal family** — red for audio, green for control voltage, blue for gates and triggers, yellow for pitch as volt per octave, following the convention used in Omri Cohen's tutorials. The family is determined from the port's name. The colours can be chosen, a ready-made scheme can be used instead, and a port whose name does not say what it carries can be corrected on its own right-click menu.
- **Input or output shown by the same ring** — it is drawn against the outer edge on an output and against the hole on an input. Colour indicates the family; position indicates the direction.
- **Cables coloured by their destination**, so a cable's colour indicates what it is being used for rather than what produced the signal.
- **Animated cable directions** — dashes drift from source to destination, so the direction can be read from any part of the cable.
- **One knob face across every plugin.**

### Handling cables

- **Cable trace assist** — hovering a cable end displays a handle on it. Clicking the handle leaves that cable at full opacity and hides every other cable in the rack; clicking any module panel restores them. Where several cables meet at one port, repeated clicks select each in turn. Right-clicking the handle disconnects that cable.
- **Add and move cables without dragging** — click a port to take its cable and click another to connect it, or press, drag and release as before; a release over a port connects it. Both gestures are active at the same time. The rack scrolls when a carried cable reaches the edge of the view.
- **Pinch to zoom** the rack on a trackpad.

---

## Test Gear

*The Test Gear manual is incomplete: the module is released and working, but its documentation is still being written.*

Right-click any port and choose **Widgets**, the first entry on the menu. The widget follows the pointer until a click places it. Each one is anchored to its port, is saved with the patch, and can be moved to another port by dragging the handle at its port.

A viewer attached to an output with nothing connected to it still receives a signal: a hidden cable is connected to that output, because a module does not compute an output that nothing is connected to.

### Viewers

- **Scope** — captures at the engine's sample rate rather than the frame rate, retains about eleven seconds of history, and sets its scales from the signal when it is attached. Triggering is controlled by a strip down its left edge; dragging a link from that strip to another port triggers from that signal instead.
- **Analyser** — a spectrum on a logarithmic frequency axis, with the peak reported as a note as well as a frequency, and ticks marking its harmonics. **W** switches to a waterfall: the same spectrum with time as the second axis. Pinch zooms the frequency axis and scrolling sideways pans it.
- **Audio monitor** — connect Monitor out to an audio interface once, and every monitor attached after that is audible through it, each with its own level and mute. They are summed, so several can be listened to at once.
- **Voltmeter** — the voltage on a terminal, or the peak of the last quarter second, in one readout that a click turns over. The peak is taken at the engine's rate rather than the frame rate, so it catches what an eye cannot.

### Generators

A switch, an LFO, a VCO, gate and pulse buttons, a clock, a DC level, a note, a volt per octave, noise and an attenuverter. They attach to a port that already has a cable connected: the engine sums several cables into one input, so a generator adds to what is there rather than replacing it.

On the LFO and the VCO, the shape drawn on the readout is the control — click it for the next waveform, and the letter beside it for bipolar or unipolar.

The **switch** is the exception among them, since it turns something off rather than adding something. Its light is on when the signal is getting through. It works by taking the cables out of the port and holding them, drawing each as a short stub in its own colour so you can see what is waiting, and putting them back when it goes on again — which is exact for audio, gates and polyphony alike, where cancelling the signal is not.

---

## Installing

Download the file for your platform from the [latest release](https://github.com/chrisgr99/DreamerDevelopment/releases/latest):

| Your computer | File |
| --- | --- |
| Mac with Apple silicon | `DreamerDevelopment-2.0.1-mac-arm64.vcvplugin` |
| Mac with an Intel processor | `DreamerDevelopment-2.0.1-mac-x64.vcvplugin` |
| Windows | `DreamerDevelopment-2.0.1-win-x64.vcvplugin` |
| Linux | `DreamerDevelopment-2.0.1-lin-x64.vcvplugin` |

Put the downloaded file in Rack's plugins folder:

| | |
| --- | --- |
| Mac | `~/Library/Application Support/Rack2/plugins-mac-arm64/` (or `plugins-mac-x64/` on an Intel Mac) |
| Windows | `%USERPROFILE%\Documents\Rack2\plugins-win-x64\` |
| Linux | `~/.local/share/Rack2/plugins-lin-x64/` |

Then start Rack. It unpacks the file on startup and the two modules appear in the browser under
Dreamer Development. The downloaded file is replaced by a folder of the same name, which is
what unpacking looks like.

To remove the plugin, delete that folder and restart Rack.

## Building from source

For development, or for a platform with no package. Set `RACK_DIR` to your Rack SDK and run
`make`. Requires a C++11 compiler.

```
RACK_DIR=/path/to/Rack-SDK make          # build
RACK_DIR=/path/to/Rack-SDK make install  # build and copy into the plugins folder
```

## Licence

GPL-3.0-or-later. No artwork ships with the plugin — every panel and every face is drawn in code.

