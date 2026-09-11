# Fundamental 2.6.4 — what can be read without running Rack

39 modules. For each: its manifest entry, the labels drawn on its panel with their positions in panel pixels, the strings its code carries (candidate parameter and port names), and whatever NYSTHI's own changelog says about it.


## VCO

- slug `VCO`, 9 HP, tags: VCO, Polyphonic
- Voltage-controlled oscillator
- panel: VCO.svg

No live text on the panel — outlined, or unlabelled. Read `panels/VCO.png` to see it.

Strings in its code:

  - `VCMixer.cpp`
  - `Exponential channel VCAs`
  - `Exponential mix VCA`
  - `VCO`
  - `FM mode`
  - `1V/octave`
  - `Sync mode`

## Wavetable VCO

- slug `VCO2`, tags: VCO, Polyphonic
- Voltage-controlled wavetable oscillator
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## VCF

- slug `VCF`, 7 HP, tags: VCF, Polyphonic
- Voltage-controlled filter
- panel: VCF.svg

No live text on the panel — outlined, or unlabelled. Read `panels/VCF.png` to see it.

Strings in its code:

  - `VCF`
  - `Cutoff frequency`
  - `Resonance`
  - `Resonance CV`
  - `Cutoff frequency CV`
  - `Drive`
  - `Lowpass filter`
  - `Highpass filter`

## VCA

- slug `VCA-1`, 3 HP, tags: VCA, Polyphonic
- Voltage-controlled amplifier
- panel: VCA-1.svg

No live text on the panel — outlined, or unlabelled. Read `panels/VCA-1.png` to see it.

Strings in its code:

  - `Unity.cpp`
  - `Merge channels 1 & 2`
  - `VCA-1`
  - `Response mode`
  - `Channel`

## VCA-2

- slug `VCA`, 0 HP, tags: VCA, Dual, Polyphonic
- 2-channel voltage-controlled amplifier
- panel: VCA.svg

No live text on the panel — outlined, or unlabelled. Read `panels/VCA.png` to see it.

Strings in its code:

  - `VCA-1.cpp`
  - `Exponential response`
  - `VCA`
  - `Channel 1 level`
  - `Channel 2 level`
  - `Channel 1 exponential CV`
  - `Channel 2 exponential CV`
  - `Channel 1 linear CV`
  - `Channel 2 linear CV`
  - `Channel 1`
  - `Channel 2`

## LFO

- slug `LFO`, 9 HP, tags: LFO, Polyphonic
- Low-frequency oscillator
- panel: LFO.svg

No live text on the panel — outlined, or unlabelled. Read `panels/LFO.png` to see it.

Strings in its code:

  - `LFO`
  - `Offset`
  - `Bipolar`
  - `Invert`
  - `Frequency`
  - `Frequency modulation`
  - `Pulse width`
  - `Pulse width modulation`
  - `Reset`
  - `Square`
  - `Phase`
  - `configLight`

## Wavetable LFO

- slug `LFO2`, tags: LFO, Polyphonic
- Low-frequency wavetable oscillator
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## Delay

- slug `Delay`, 9 HP, tags: Delay
- panel: Delay.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Delay.png` to see it.

Strings in its code:

  - `Delay`
  - `Time CV`
  - `Feedback CV`
  - `Tone CV`
  - `Mix CV`
  - `1V/octave when Time CV is 100%`
  - `Audio`
  - `Clock`
  - `Wet`
  - `configBypass`
  - `br.outputId != outputId`

## ADSR EG

- slug `ADSR`, 9 HP, tags: Envelope Generator, Polyphonic
- Generates an envelope with Attack/Decay/Sustain/Release
- panel: ADSR.svg

No live text on the panel — outlined, or unlabelled. Read `panels/ADSR.png` to see it.

Strings in its code:

  - `Attack`
  - `Decay`
  - `Sustain`
  - `Release`
  - `Attack CV`
  - `Sustain CV`
  - `Release CV`
  - `Retrigger`

## Mix

- slug `Mixer`, 3 HP, tags: Mixer, Polyphonic
- Mixes 6 signals
- panel: Mixer.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Mixer.png` to see it.

Strings in its code:

  - `Mixer`
  - `Level`
  - `average`
  - `invert`

## VCA Mix

- slug `VCMixer`, 9 HP, tags: Mixer, VCA, Polyphonic
- Mixes 4 signals with built-in VCAs
- panel: VCMixer.svg

No live text on the panel — outlined, or unlabelled. Read `panels/VCMixer.png` to see it.

Strings in its code:

  - `VCMixer`
  - `Mix level`
  - `Channel 3 level`
  - `Channel 4 level`
  - `Channel %d CV`
  - `chExp`
  - `mixExp`

## 8vert

- slug `8vert`, 8 HP, tags: Attenuator, Polyphonic
- Attenuverts 8 signals or creates constant voltages
- panel: 8vert.svg

No live text on the panel — outlined, or unlabelled. Read `panels/8vert.png` to see it.

Strings in its code:

  - `]pBw`
  - `]pB,`
  - `]pB`
  - `]pBJ`
  - `]pBY`
  - `]pBm`
  - `]pB`
  - `CY,`
  - `BY,`
  - `zB"`
  - `Aku`
  - `Ci4VBku`
  - `ALfGC].VBLfGC`
  - `ItC].VB`
  - `ItC`
  - `C].VB`
  - `C].VB`
  - `mC~`
  - `A*u`
  - `ALfGC`
  - `ItC`
  - `RVBku`
  - `IiC=`
  - `BB+`
  - `VBA`
  - `VB!`
  - `[BB`
  - `A)T`
  - `VB)T`
  - `nC,`
  - `O*C`
  - `O*C,`
  - `oBw`
  - `z8C`
  - `bVC`
  - `ItC`
  - `oB8`
  - `Cu:`
  - `Bu:`
  - `Bu:`
  - `Bu:`
  - `Cu:`
  - `z8Cu:`
  - `bVCu:`
  - `ItCu:`
  - `Cu:`
  - `Cu:`
  - `z8C`
  - `bVC`
  - `ItC`
  - `A P`
  - `Aku`
  - `E4B`
  - `E4B`
  - `E4B`
  - `VBLfGC].VBD`
  - `z8C`
  - `bVC`
  - `ItC`
  - `oBw`
  - `z8C`
  - `bVC`
  - `ItC`
  - `oB8`
  - `z8C`
  - `bVC`
  - `ItC`
  - `XnB`
  - `L&A`
  - `L&A`
  - `L&A`
  - `L&A`
  - `L&A`
  - `LBA`
  - `LBp`
  - `z8C`
  - `bVC`
  - `ItC`
  - `Bc1)C0`
  - `%CT*`
  - `z)CE`
  - `mCT*`
  - `z)C`
  - `CT*`
  - `z)C`
  - `B]n`
  - `BS)`
  - `2C088C*`
  - `iLB088C`
  - `iLB`
  - `C088C`
  - `C088C`
  - `MCC`
  - `XwB`
  - `Cu:`
  - `hCu:`
  - `Cu:`
  - `Cu:`
  - `XwB`
  - `CE"`
  - `:CE"`
  - `VCE"`
  - `tCE"`
  - `RB7[`
  - `ACA`
  - `RBCA`
  - `AQ(8C1`
  - `RBQ(8C`
  - `VC1`
  - `RB!`
  - `sC1`
  - `RB/`
  - `BCA`
  - `BQ(8C`
  - `AN'`
  - `X&A`
  - `cCB`
  - `RC-`
  - `rC-`
  - `kBI`
  - `kBR`
  - `Bbi,B`
  - `Cbi,B`
  - `Cbi,B`
  - `Cbi,Bz`
  - `&Cqx`
  - `nCA`
  - `MBp`
  - `Ch3`
  - `A:]`
  - `AF#~C`
  - `.Cg`
  - `Cow+C`
  - `N;C`
  - `;jC`
  - `&C:]`
  - `kQB`
  - `WAV:wav,WAV`
  - `&C5Z`
  - `tQB`
  - `tQB`
  - `qQB`
  - `WAV:wav,WAV`
  - `data`
  - `wave`
  - `fmt`
  - `data`
  - `fact`
  - `Row %d gain`
  - `Row %d`
  - `configParam`
  - `Module.hpp`
  - `configInput`
  - `configOutput`
  - `createModuleWidget`

## Unity

- slug `Unity`, 0 HP, tags: Mixer, Utility, Dual
- Mixes or averages signals with unity gain
- panel: Unity.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Unity.png` to see it.

Strings in its code:

  - `res/fonts/Nunito-Bold.ttf`
  - `Unity`
  - `Channel 1 mode`
  - `Average`
  - `Channel 2 mode`
  - `Channel %d #%d`
  - `Channel 1 mix`
  - `Channel 1 inverse mix`
  - `Channel 2 mix`
  - `Channel 2 inverse mix`
  - `merge`

## Mutes

- slug `Mutes`, 8 HP, tags: Switch, Polyphonic
- Toggles up to 10 signals
- panel: Mutes.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Mutes.png` to see it.

Strings in its code:

  - `Mutes`
  - `Row %d mute`
  - `states`

## Pulses

- slug `Pulses`, 8 HP, tags: Switch
- Generates up to 10 trigger and gate signals
- panel: Pulses.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Pulses.png` to see it.

Strings in its code:

  - `Pulses`
  - `Row %d push`
  - `Row %d trigger`
  - `Row %d gate`

## Scope

- slug `Scope`, 13 HP, tags: Visual, Polyphonic
- Inspect waveforms with an oscilloscope
- panel: Scope.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Scope.png` to see it.

Strings in its code:

  - `Gain multiplier`
  - `Reflect at maximum`
  - `Reflect at minimum`
  - `Scope`
  - `Gain 1`
  - `V/screen`
  - `Gain 2`
  - `Scope mode`
  - `1 x 2`
  - `Trigger threshold`
  - `Enabled`
  - `External trigger`
  - `lissajous`
  - `external`

## SEQ 3

- slug `SEQ3`, 22 HP, tags: Sequencer
- 3-channel 8-step sequencer also with gate outputs
- panel: SEQ3.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SEQ3.png` to see it.

Strings in its code:

  - `res/fonts/ShareTechMono-Regular.ttf`
  - `% 6.2f`
  - `max`
  - `min`
  - `Tempo`
  - `Run`
  - `Steps`
  - `CV %d step %d`
  - `Step %d trigger`
  - `Step %d`
  - `running`
  - `gates`
  - `clockPassthrough`

## Sequential Switch 1 to 4

- slug `SequentialSwitch1`, 3 HP, tags: Switch, Utility, Polyphonic
- Routes 1 input to one of 4 outputs
- panel: SequentialSwitch1.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SequentialSwitch1.png` to see it.

Strings in its code:

  - `SEQ3.cpp`
  - `Clock passthrough`
  - `Rotate left`
  - `Rotate right`
  - `SequentialSwitch1`
  - `SequentialSwitch2`
  - `declick`

## Sequential Switch 4 to 1

- slug `SequentialSwitch2`, 3 HP, tags: Switch, Utility, Polyphonic
- Routes one of 4 inputs to 1 output
- panel: SequentialSwitch2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SequentialSwitch2.png` to see it.

## Octave

- slug `Octave`, 3 HP, tags: Utility, Polyphonic
- Shifts 1V/oct pitch CV by octaves
- panel: Octave.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Octave.png` to see it.

Strings in its code:

  - `Octave`
  - `Shift`
  - `1V/octave pitch`
  - `Octave shift CV`
  - `Pitch`
  - `octave`

## Quantizer

- slug `Quantizer`, 3 HP, tags: Quantizer, Polyphonic
- 12-note quantizer and scale selector
- panel: Quantizer.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Quantizer.png` to see it.

Strings in its code:

  - `Quantizer`
  - `Pre-offset`
  - `semitones`
  - `enabledNotes`

## Split

- slug `Split`, 5 HP, tags: Polyphonic, Utility
- Splits a polyphonic cable into multiple monophonic cables
- panel: Split.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Split.png` to see it.

Strings in its code:

  - `Split`

## Merge

- slug `Merge`, 5 HP, tags: Polyphonic, Utility
- Combines multiple monophonic cables into a polyphonic cable
- panel: Merge.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Merge.png` to see it.

Strings in its code:

  - `Merge`
  - `Channel %d`
  - `Polyphonic`
  - `channels`

## Sum

- slug `Sum`, 3 HP, tags: Polyphonic, Utility
- Sums all channels of a polyphonic cable
- panel: Sum.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Sum.png` to see it.

Strings in its code:

  - `Sum`
  - `Monophonic`

## Viz

- slug `Viz`, 3 HP, tags: Polyphonic, Visual
- Visualizes all channels of a polyphonic cable
- panel: Viz.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Viz.png` to see it.

Strings in its code:

  - `Viz`

## Mid/Side

- slug `MidSide`, 5 HP, tags: Utility, Polyphonic
- Encodes/decodes between mid/side and left/right stereo signals
- panel: MidSide.svg

No live text on the panel — outlined, or unlabelled. Read `panels/MidSide.png` to see it.

Strings in its code:

  - `res/fonts/DSEG7ClassicMini-BoldItalic.ttf`
  - `Automatic (%d)`
  - `MidSide`
  - `Encoder width`
  - `Decoder width`
  - `Encoder left`
  - `Encoder right`
  - `Decoder mid`
  - `Decoder side`
  - `Encoder mid`
  - `Encoder side`
  - `Decoder left`
  - `Decoder right`

## Noise

- slug `Noise`, 3 HP, tags: Noise
- Multicolored noise generator
- panel: Noise.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Noise.png` to see it.

Strings in its code:

  - `Mutes.cpp`
  - `Invert mutes`
  - `Noise`
  - `White noise`
  - `0 dB/octave power density`
  - `Pink noise`
  - `-3 dB/octave power density`
  - `Red noise`
  - `-6 dB/octave power density`
  - `Violet noise`
  - `+6 dB/octave power density`
  - `Blue noise`
  - `+3 dB/octave power density`
  - `Gray noise`
  - `Psychoacoustic equal loudness`
  - `Black noise`
  - `Uniform random numbers`

## Random

- slug `Random`, 9 HP, tags: Random, Sample and hold, Polyphonic
- Random CV generator
- panel: Random.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Random.png` to see it.

Strings in its code:

  - `Shift notes up`
  - `Shift notes down`
  - `Random`
  - `Internal trigger rate`
  - `Trigger probability`
  - `Random spread`
  - `Shape`
  - `Internal trigger rate CV`
  - `Trigger probability CV`
  - `Random spread CV`
  - `Stepped`
  - `Linear`
  - `Exponential`
  - `Smooth`
  - `version`

## CV Mix

- slug `CVMix`, 3 HP, tags: Mixer, Utility, Polyphonic
- Mixes 3 CV signals with attenuverters
- panel: CVMix.svg

No live text on the panel — outlined, or unlabelled. Read `panels/CVMix.png` to see it.

Strings in its code:

  - `CVMix`
  - `Level %d`
  - `CV %d`
  - `Normalled to 10 V`
  - `Mix`

## Fade

- slug `Fade`, 3 HP, tags: Panning, Voltage-controlled amplifier, Utility, Polyphonic
- Crossfades audio or CV
- panel: Fade.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Fade.png` to see it.

Strings in its code:

  - `Crossfade`
  - `Crossfade CV`
  - `panLaw`

## Logic

- slug `Logic`, 5 HP, tags: Logic, Polyphonic
- Gate logic processor
- panel: Logic.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Logic.png` to see it.

Strings in its code:

  - `Logic`
  - `NOT A`
  - `NOT B`
  - `NOR`
  - `AND`
  - `XOR`

## Compare

- slug `Compare`, 5 HP, tags: Utility, Polyphonic
- Compares two voltages
- panel: Compare.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Compare.png` to see it.

Strings in its code:

  - `Compare`
  - `Maximum`
  - `Minimum`
  - `Limit`
  - `Clip gate`
  - `Limit gate`

## Gates

- slug `Gates`, 5 HP, tags: Polyphonic
- Gate processor
- panel: Gates.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Gates.png` to see it.

Strings in its code:

  - `Pan law`
  - `-6 dB (linear)`
  - `-3 dB`
  - `Gates`
  - `Gate length`
  - `Reset flip/flop`
  - `Rising edge trigger`
  - `Falling edge trigger`
  - `Gate delay`

## Process

- slug `Process`, 5 HP, tags: Sample and hold, Slew limiter, Polyphonic
- CV processor
- panel: Process.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Process.png` to see it.

Strings in its code:

  - `drawLayer`
  - `Octave.cpp`
  - `paramWidget`
  - `onDragEnter`
  - `%gV to %gV`
  - `Process`
  - `ms/V`
  - `Voltage`
  - `Sample & hold`
  - `Sample & hold 2`
  - `Track & hold`
  - `Hold & track`
  - `Glide`

## Mult

- slug `Mult`, 3 HP, tags: Multiple, Utility, Polyphonic
- Copies a signal to 8 outputs
- panel: Mult.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Mult.png` to see it.

Strings in its code:

  - `appendContextMenu`
  - `Mixer.cpp`
  - `module`
  - `Invert output`
  - `Average voltages`
  - `Mult %d`

## Rescale

- slug `Rescale`, 3 HP, tags: Attenuator, Utility, Polyphonic
- Rescales voltages with gain, offset, and range limiting
- panel: Rescale.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Rescale.png` to see it.

Strings in its code:

  - `Random range`
  - `Rescale`
  - `Signal`
  - `multiplier`
  - `reflectMin`
  - `reflectMax`

## Random Values

- slug `RandomValues`, 3 HP, tags: Random, Utility, Polyphonic
- Generates 7 random fixed voltages
- panel: RandomValues.svg

No live text on the panel — outlined, or unlabelled. Read `panels/RandomValues.png` to see it.

Strings in its code:

  - `RandomValues`
  - `Random %d`
  - `values`
  - `randomGain`
  - `randomOffset`

## Push

- slug `Push`, 3 HP, tags: Utility
- Button with gate/trigger outputs and hold switch
- panel: Push.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Push.png` to see it.

Strings in its code:

  - `Trigger`
  - `hold`

## Sample & Hold Analog Shift Register

- slug `SHASR`, 7 HP, tags: Sample and hold
- 8 channel sample & hold combined with a shift register
- panel: SHASR.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SHASR.png` to see it.

Strings in its code:

  - `SHASR`
  - `Randomize`
  - `Normalizes "Sample 1 input" to a random signal`
  - `Clear`
  - `Sample %d`
  - `Trigger %d`

