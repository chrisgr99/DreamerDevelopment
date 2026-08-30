# Clarity

User interface clarity: this module changes how the rack is drawn and handled. It does nothing to the sound and nothing to your patch: every feature is about reading what is in front of you and getting hold of it.

<img src="images/clarity-knobs.jpg" width="760" alt="A rack holding Test Gear, Clarity, two Fundamental modules, an Instruo tona and an audio interface. Every knob across all of them wears the same face, the jacks carry their family colours, and a cable runs across with dashes crawling along it.">

Place one anywhere in the rack. There is no need for a second — one module drives the whole rack, and its panel is the feature list under the heading **Features**. Each row is a switch, lit when the feature is on.

---

## Reading the rack

### Colour code jacks

Every jack in the rack gets a coloured ring by signal family, guessed from the port's own name:

| Family | Input | Output | Ports named like |
| --- | :---: | :---: | --- |
| Audio | <img src="images/jack-audio-in.png" width="44"> | <img src="images/jack-audio-out.png" width="44"> | anything not matched below |
| CV | <img src="images/jack-cv-in.png" width="44"> | <img src="images/jack-cv-out.png" width="44"> | CV, MOD, FM |
| Gate and trigger | <img src="images/jack-trigger-in.png" width="44"> | <img src="images/jack-trigger-out.png" width="44"> | GATE, TRIG, CLOCK, CLK, RESET, SYNC |
| Pitch | <img src="images/jack-pitch-in.png" width="44"> | <img src="images/jack-pitch-out.png" width="44"> | V/OCT, PITCH, NOTE |

The same ring says which way the signal goes: it hugs the **outer edge** of an output and the **hole** of an input. Shape carries direction and colour carries family, so the two readings never have to compete for the same cue.

The pictures above are drawn by `docs/images/make-jacks.py`, from the same geometry the plugin uses, so they can be redrawn when the drawing changes rather than being screenshots that quietly go out of date.

Because the naming is read from the port itself, this works on modules nobody has described by hand, including plugins released after this one.

### Colour code cables

A cable takes the colour of the family of the port it **arrives** at, so where it is going is visible from either end. Re-plug it somewhere else and it takes the new colour. A cable in flight is coloured from the port it left, so it reads as that signal from the moment it leaves the jack.

| Family | Cable |
| --- | --- |
| Audio | <img src="images/cable-audio.png" width="240"> |
| CV | <img src="images/cable-cv.png" width="240"> |
| Gate and trigger | <img src="images/cable-trigger.png" width="240"> |
| Pitch | <img src="images/cable-pitch.png" width="240"> |

Pulling an audio cable onto a gate input, and the colour following it:

<img src="images/cable-replug.gif" width="420" alt="A cable is pulled from an audio input to a gate input; it stays yellow while it is carried, and turns blue when it is dropped.">

Rack saves cable colours in the patch. Change one by hand and it is left alone from then on.

### Consistent knob style

<img src="images/knob-turn.gif" width="300" alt="A knob with its pointer and grip ticks turning back and forth.">

Draws one knob face over every knob in the rack, whatever the plugin. Two Fundamental modules and an Instruo, all reading the same way:

<img src="images/clarity-knobs-crop.jpg" width="620" alt="Close up on the knobs of two Fundamental modules and an Instruo tona, every one of them wearing the same face."> Knobs are drawn on top of each module, so a module that draws its own light-emitting ring around a knob will have it covered. If that matters to you, switch this off; the note is repeated in the right-click menu.

### Animate cable directions

Dashes crawl along every cable from source to destination. Dash length is keyed to the destination's family — fine for audio, coarse for gates — so a glance tells you both the direction and roughly what is travelling.

<img src="images/cable-flow.gif" width="420" alt="Dashes crawling along a cable from a port labelled Output to a port labelled Input.">

The crawl states direction only. It is not synchronised with the signal, and says nothing about what the signal is doing.

---

## Handling cables

### Cable trace assist

Hover either end of a cable and a small pill appears on it. Click the pill and that cable stays bright while every other cable in the rack is hidden, which is how you follow one lead through a tangle. Click again to put it back.

<img src="images/cable-trace.gif" width="420" alt="Six cables cross each other. The pointer reaches a cable end, a pill appears on it, and a click leaves that one cable on screen alone. A second click brings the others back.">

Where several cables converge on one jack, clicking the pill steps through them one at a time.

**Right-click the pill** to lift that particular cable off its jack — useful when the cable you want is the one underneath four others.

### Add cables without dragging

You no longer have to hold the button down to move a cable.

<img src="images/cable-new.gif" width="440" alt="A click on an empty terminal starts a cable, which follows the pointer across with no button held and is connected with a second click.">

Moving one that is already patched works the same way:

<img src="images/cable-pull.gif" width="440" alt="A cable is picked up from one input with a single click, follows the pointer across with no button held, and is dropped on another input with a second click.">

Click a jack to pick up its cable, move, and click another jack to drop it. Or press, drag and release as you always have — a release after any movement lands the cable, connecting it if it is over a jack. Both gestures are live at once, so whichever you do is right, and you can start one way and finish the other. Right-click cancels either way, and the rack scrolls itself when a carried cable reaches the edge of the view.

A note appears beside the jack each time a cable comes off one, because nobody would guess that letting go of the button is allowed. It stays until you close it, it moves to the new jack if you pick up another cable, and it keeps appearing until you tick **Don't show this again** and press OK. **Show tips again** in the right-click menu brings it back.

This is the one feature that ships **on** and still has a switch, which is the opposite of how it started. There is nothing to choose between the two gestures, so the switch is not really a choice — it is there because one behaviour does differ from stock Rack, and because a gesture this fundamental should have a visible way out. The difference: a click on a jack that never moves picks the cable up, where Rack would do nothing.

### Pinch to zoom

Pinch on a trackpad to zoom the rack. The rack is photographed once when the gesture starts and that picture is scaled while you pinch, with the real zoom applied when you let go — so nothing re-rasterises during the gesture and it stays smooth in a large patch. The picture softens as you zoom in and sharpens the moment you release.

Pinching over an analyser zooms **its** frequency axis instead, since that is the thing under your fingers.

---

## The right-click menu

- **Draw pointer (for screen recordings)** — draws a pointer into the rack, with clicks, drags and scrolling shown. Screen recorders capture the window and not the system cursor, so a recording of a patch being worked on otherwise shows things happening with nothing touching them.
- **Show tips again** — forgets every "Don't show this again". The answers are kept beside Rack's own settings rather than in the patch, since whether you want to be told something is about you and not about the work.

---

## Notes

**One module is enough.** A second changes nothing; the last one you take out switches the features off.

**Nothing here touches audio.** Clarity has no ports and does no processing. Bypassing it stops nothing, because there is nothing in the signal path to stop.

**The features are params**, so they are saved with the patch, they can be mapped to a controller, and they show up in Rack's own right-click menu for each switch.

