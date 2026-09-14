# DreamerMPX — what the census already says

Installed version 2.0.0. Source: https://github.com/chrisgr99/MPX

Across the plugin: 0 unnamed inputs, 0 inputs with no panel jack.

10 modules. For each: how many controls, then anything the census flags.

## toMPX — toMPX

Maker's own summary: Gathers ordinary control voltages into voice cables. Gate, pitch, level, duration, pan, pressure and timbre go in as polyphonic signals, and four voice cables come out, each carrying one instrument's notes. Pitch, level, duration and pan are read at the gate's rising edge and held for the note's life; bend, pressure and timbre are followed while it sounds. The notes-per-voice knob decides how the sixteen channels divide among the four cables.

Tags: Polyphonic, Utility

7 in, 1 out, 5 controls

## fromMPX — fromMPX

Maker's own summary: Takes one voice cable apart into ordinary polyphonic control voltages: gate, pitch, level, bend as a control signal and as volts per octave, pressure, timbre, pan and duration. It allocates the notes among its voices, with a choice of what gives when none is free: take the oldest, take the quietest, ignore the note, glide, or legato.

Tags: Polyphonic, Utility

2 in, 7 out, 3 controls

## mpxEuclid — mpxEuclid

Maker's own summary: Four Euclidean voices on one MPX cable. Each has its own steps, pulses and offset, and its own clock divider. Every note's level and length come from a slowly wandering value that is correlated with its own past and repeats after a set number of beats, so each voice reads as a separate player rather than as a machine being random. Harder hits ring longer by however much you ask.

Tags: Sequencer, Polyphonic, Random

2 in, 1 out, 23 controls

## mpxProgression — mpxProgression

Maker's own summary: A dozen chord progressions everybody knows — two five one, a turnaround, twelve bar blues, the four chords, a Dorian vamp — chosen with one knob and shown as a chart in the key you set. Publishes them as the harmony on an MPX cable, so everything downstream is played against something. Not a chart module: no repeats, no sections, no import.

Tags: Sequencer, Utility

2 in, 1 out, 3 controls

## mpxMonitor — mpxMonitor

Maker's own summary: Shows what is on an MPX cable and passes it on unchanged: the harmony being played against, and the notes going by with their pitch, level and duration. An MPX cable carries no voltage, so a scope says nothing about it and this is the way to see one.

Tags: Visual, Utility

1 in, 1 out, 1 controls

## mpxChart — mpxChart

Maker's own summary: Reads iReal Pro playlists and plays their charts onto an MPX cable. Import an HTML export once and the songs are yours to choose from; the chosen one is stored in the patch itself, so it opens on a machine that has never seen your playlists. Repeats, endings and similes are written out and the chart is shown as it is played, four bars to a line, with the sounding bar picked out. Transposes with one knob, since chords are held as degrees of the key.

Tags: Sequencer, Utility

2 in, 4 out, 7 controls

## mpxComp — mpxComp

Maker's own summary: Chordal accompaniment: voices a chord and leads the voices between changes

Tags: Polyphonic, Utility, Arpeggiator

1 in, 1 out, 16 controls

## polyToStereo — polyToStereo

Maker's own summary: A channel strip per voice: two level stages multiplied, a pan, and a stereo pair

Tags: Polyphonic, Mixer, Panning, VCA

4 in, 2 out, 3 controls

## mpxArp — mpxArp

Maker's own summary: An arpeggiator on the chord from the cable, with a length that can be pushed into overlap

Tags: Arpeggiator, Polyphonic, Sequencer

1 in, 1 out, 8 controls

## mpxRand — mpxRand

Maker's own summary: Varies one attribute of an MPX cable's notes — duration, level, pan, timing or tuning — by white, walk or Perlin randomness

Tags: Random, Polyphonic, Utility

1 in, 1 out, 5 controls

