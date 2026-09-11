# NYSTHI 2.4.23 — what can be read without running Rack

149 modules. For each: its manifest entry, the labels drawn on its panel with their positions in panel pixels, the strings its code carries (candidate parameter and port names), and whatever NYSTHI's own changelog says about it.


## Logic AND, OR, XOR, NOT

- slug `Logic`, 5 HP, tags: AND, OR, XOR, NOT, LOGIC
- Boolean module
- panel: LOGIC.svg

No live text on the panel — outlined, or unlabelled. Read `panels/LOGIC.png` to see it.

Strings in its code:

  - `CHOOSE destination file...`
  - `ARMED FILE:`
  - `AUTO ENUMERATE SAVES`
  - `LOGGING SEPARATOR`
  - `Logic`

## Model277 Buchla 277 imitation

- slug `Model277`, 7 HP, tags: DELAY, ECHO
- Imitation of the Buchla 277 feedback delay with some twists
- panel: Model277.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Model277.png` to see it.

Strings in its code:

  - `Model277`
  - `Gain 1 Dry-Wet -1..+1`
  - `Gain 2 Dry-Wet -1..+1`
  - `Gain 3 Dry-Wet -1..+1`
  - `Gain 4 Dry-Wet -1..+1`
  - `Time 0.005..0.2`
  - `Mod Depth -0.5..+0.5`
  - `Time multiplier 0..1`

## Surveillance

- slug `Surveillance`, 6 HP, tags: Utility, Controller, Attenuator
- One knob to drive all! (10 CV knob)
- panel: Surveillance.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Surveillance.png` to see it.

Strings in its code:

  - `Surveillance`
  - `Global Attenuverter -1..1`
  - `Global Range 0..1 (-5v-+5 or 0-10v)`
  - `Attenuverter -1..1`

## StereoPhaser Phaser

- slug `NYStereoPhaser`, 7 HP, tags: Effect, Phaser
- 96-48 cells 2nd-4th order phaser
- panel: NYStereoPhaser.svg

No live text on the panel — outlined, or unlabelled. Read `panels/NYStereoPhaser.png` to see it.

Strings in its code:

  - `NYStereoPhaser`
  - `Stages 0..1 (12 - 24)`
  - `Order 0..1 (2 - 4)`
  - `Mod speed 0..1`
  - `Mod depth -1..1`
  - `Reso 0..1`
  - `Depth 0..0.8`
  - `Bandwidth 0..1`
  - `Frequency 0..1`

## StereoChorus Chorus

- slug `NYStereoChorus`, 7 HP, tags: Effect, Chorus
- Stereo Chorus with added nysthi movements modulators
- panel: NYStereoChorus.svg

No live text on the panel — outlined, or unlabelled. Read `panels/NYStereoChorus.png` to see it.

Strings in its code:

  - `vcvx`
  - `vcv`
  - `VCVX`
  - `THEMES`
  - `Use DEFAULT NYSTHI THEME`
  - `USERS THEMES`
  - `NYStereoChorus`
  - `VCO type 0..3`
  - `Mod Speed 0.01..5.0`
  - `Mod Depth -1.0..1.0`
  - `Movement Depth -1.0..1.0`
  - `Movement Depth 0.0..1.0`
  - `Mix 0.0..1.0`
  - `Mix 0.0..5.0`
  - `Bypass 0..1`

## DualSignalDelayer Dual Signal Delay

- slug `DualSignalDelayer`, 2 HP, tags: Effect, Delay
- Dual Signal Delay
- panel: DualSignalDelayer.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DualSignalDelayer.png` to see it.

Strings in its code:

  - `Voltage mode`
  - `DualSignalDelayer`
  - `Delay 0.0..1.0`

## EnvelopeFollower Envelope Follower

- slug `NYEnvFollower`, 6 HP, tags: VCA, Envelope follower
- Envelope Follower
- panel: NYEnvFollower.svg

No live text on the panel — outlined, or unlabelled. Read `panels/NYEnvFollower.png` to see it.

Strings in its code:

  - `BINSONDISC`
  - `NYEnvFollower`
  - `Input 0.0..5.0`
  - `Smoothness 1.0..0.0`
  - `Attack 0.0..1.0`
  - `Release 0.0..1.0`
  - `Scale -5.0..5.0`
  - `Offset -5.0..5.0`
  - `Delay Envelope 0.0..0.2`
  - `Delay Signal 0.0..0.2`
  - `Scope ON-OFF 0..1`
  - `Scope Time Scale`
  - `Scope Vert Scale -2.0..8.0`

## Stereo Recorder vers 2

- slug `MasterRecorder2`, 9 HP, tags: Recording
- Stereo wave recorder
- panel: MasterRecorder2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/MasterRecorder2.png` to see it.

Strings in its code:

  - `Saving WAV Format:`
  - `Switch to`
  - `PCM 16bit`
  - `PCM 24bit`
  - `INPUT ATTENUATION`
  - `0 dB`
  - `-6 dB`
  - `-12 dB`
  - `-24 dB`
  - `MasterRecorder2`
  - `pcm`

## Polyphonic Recorder

- slug `PolyRecorder`, tags: Recording, Polyphonic
- from 1 to 16 channels recorder
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## PolyRecorder64 64 tracks

- slug `PolyRecorder64`, 8 HP, tags: Recording, Polyphonic
- 64 tracks recorder
- panel: polyrecorder64.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `LEFT` at -217, 261
  - `SIGNATURE` at -220, 280
  - `NYSTHI` at -7, 381

Strings in its code:

  - `PolyRecorder64`

## Dual VU Meter with VCA

- slug `VUMeterDual`, 2 HP, tags: Visual
- panel: VUMeterDual.svg

No live text on the panel — outlined, or unlabelled. Read `panels/VUMeterDual.png` to see it.

Strings in its code:

  - `VUMeterDual`

## Single VU Meter

- slug `VUMeterSingle`, 2 HP, tags: Visual
- panel: VUMeterSingle.svg

No live text on the panel — outlined, or unlabelled. Read `panels/VUMeterSingle.png` to see it.

Strings in its code:

  - `Voltage Meter Mode Selection`
  - `0 to 1v voltages`
  - `0 to 5v voltages`
  - `0 to 10v voltages`
  - `0 to 20v voltages`
  - `0 to 50v voltages`
  - `0 to 100v voltages`
  - `-1 to 1v voltages`
  - `-5 to 5v voltages`
  - `-10 to 10v voltages`
  - `VU Meter Mode Selection`
  - `VU Meter mode`
  - `VU meter mode RMS timing`
  - `VU meter mode PEAK HOLD timing`
  - `VUMeterSingle`
  - `PolyVoltageMeter`

## Single Slim VU Meter

- slug `VUMeterSlim`, 1 HP, tags: Visual
- panel: VUMeterSlim.svg

No live text on the panel — outlined, or unlabelled. Read `panels/VUMeterSlim.png` to see it.

Strings in its code:

  - `VUMeterSlim`

## AttackDecay AD

- slug `AttackDecay`, 2 HP, tags: Envelope generator, Function generator
- Attack Decay, AD, Envelope EG
- panel: AttackDecay.svg

No live text on the panel — outlined, or unlabelled. Read `panels/AttackDecay.png` to see it.

Strings in its code:

  - `OPTIONS`
  - `Subtract Original Signal From WET`
  - `PRESETS`
  - `AttackDecay`

## PolyAttackDecay Polyphonic AD

- slug `PolyAttackDecay`, 2 HP, tags: Envelope generator, Function generator, Polyphonic
- Polyphonic Attack Decay, AD, ADSR, Envelope Generator EG
- panel: PolyAttackDecay.svg

No live text on the panel — outlined, or unlabelled. Read `panels/PolyAttackDecay.png` to see it.

Strings in its code:

  - `PolyAttackDecay`

## DelayAttackHoldDecay DAHD

- slug `DelayAttackHoldDecay`, 6 HP, tags: Envelope generator, Function generator
- DAHD Delay Attack Hold Decay Envelope like 281e, Serge USG, ems trapezoid
- panel: DelayAttackHoldDecay.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DelayAttackHoldDecay.png` to see it.

Strings in its code:

  - `DelayAttackHoldDecay`

## PolyDelayAttackHoldDecay PolyDAHD

- slug `PolyDelayAttackHoldDecay`, 6 HP, tags: Envelope generator, Function generator, Polyphonic
- PolyDAHD Polyphonic Delay Attack Hold Decay Envelope, like 281e, Serge USG, ems trapezoid  EG DAHD
- panel: PolyDelayAttackHoldDecay.svg

No live text on the panel — outlined, or unlabelled. Read `panels/PolyDelayAttackHoldDecay.png` to see it.

Strings in its code:

  - `PolyDelayAttackHoldDecay`

## PolyAttackDecaySustainRelease PolyADSR

- slug `PolyAttackDecaySustainRelease`, 6 HP, tags: Envelope generator, Function generator, Polyphonic
- Polyphonic Attack Decay Sustain Release Envelope
- panel: PolyAttackDecaySustainRelease.svg

No live text on the panel — outlined, or unlabelled. Read `panels/PolyAttackDecaySustainRelease.png` to see it.

Strings in its code:

  - `Paraphonic mode`
  - `PolyAttackDecaySustainRelease`
  - `TZEN`

From the changelog:

  * correct number of VCA1 and VCA2 input (can be poly and in different number from GATERs and/or TRIGGERs)

## SimplerFileControlExpander

- slug `SimplerFileControlExpander`, 1 HP, tags: Utility, Controller, Expander
- Controller for files imported in complex Simpler and slim Simpler
- panel: simplerFileControlExpander.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `SI` at 40, 12
  - `FM-CV` at 40, 21
  - `SPEED` at 40, 60
  - `CYCLE` at 41, 84
  - `GATED` at 40, 103
  - `TRIG-IN` at 40, 122
  - `TRIG-OUT` at 40, 157
  - `START-SA` at 40, 192
  - `MODINDEX` at 124, 218
  - `VCA` at 44, 276
  - `EOC` at 44, 315
  - `OUT-L+R` at 40, 339

Strings in its code:

  - `Play next file trigger`
  - `Play previous file trigger`
  - `Play Random file trigger`

## slimSimpler Sample Player

- slug `slimSimpler`, 1 HP, tags: Sample player, VCO
- Sample player VCO
- panel: slimSimpler.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `SI` at 40, 12
  - `FM-CV` at 40, 21
  - `SPEED` at 40, 60
  - `CYCLE` at 41, 84
  - `GATED` at 40, 103
  - `TRIG-IN` at 40, 122
  - `TRIG-OUT` at 40, 157
  - `START-SA` at 40, 192
  - `MODINDEX` at 124, 218
  - `VCA` at 44, 276
  - `EOC` at 44, 315
  - `OUT-L+R` at 40, 339

Strings in its code:

  - `MAX IMPORTABLE FILE SIZE`
  - `Speed CV input reducer`
  - `No reduction`
  - `Divide by 2`
  - `Divide by 5`
  - `Divide by 10`
  - `Reset Playhead When Changing Sample`
  - `Any file size`
  - `Max 50 Mbytes`
  - `Max 20 Mbytes`
  - `Max 10 Mbytes`
  - `Max 5 Mbytes`
  - `Max 2 Mbytes`
  - `Max 1 Mbyte`

## complexSimpler Sampler

- slug `complexSimpler`, 7 HP, tags: Recording, Sampler, VCO
- Sampler VCO
- panel: complexSimpler.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `MODINDEX` at 124, 218

Strings in its code:

  - `complexSimpler`
  - `slimSimpler`
  - `SimplerFileControlExpander`
  - `Speed IN CV VCA`

## Dica33 303 filter

- slug `Dica33`, 6 HP, tags: filter
- Distorted Acid filter 303 wannabe
- panel: Dica33.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Dica33.png` to see it.

Strings in its code:

  - `Dica33`

## HotTuna TUNER

- slug `HotTuna`, 6 HP, tags: Utility, Tuner
- Chromatic TUNER
- panel: HotTuna.svg

No live text on the panel — outlined, or unlabelled. Read `panels/HotTuna.png` to see it.

## SoyModelSOU

- slug `SoyModelSOU`, 12 HP, tags: Random, Sample and hold
- Source of uncertainty Buchla266/Doepfer149 wannabe wannabe
- panel: SoyModelSOU.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SoyModelSOU.png` to see it.

Strings in its code:

  - `SoyModelSOU`

From the changelog:

  * optimizations
  *  debug halt when changing sample rate

## SOUUtils

- slug `SOU-UTILS`, 8 HP, tags: Random, Sample and hold, Utility, VCA, Polyphonic
- SoyModelSOU little helper, scaler, offsetter, voltage folders, analogue shifter
- panel: SOUUtils.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SOUUtils.png` to see it.

Strings in its code:

  - `SOU-UTILS`

## SQUONK sequencer

- slug `SQUONK`, 23 HP, tags: Sequencer
- sequencer, keyboard, memory, gater, bridger, ratcheter
- panel: SQUONK.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `START` at 66, 399
  - `STOP` at 130, 399
  - `RESET` at 182, 399

Strings in its code:

  - `QUARTERS LENGTH:`
  - `TIME LENGTH:`
  - `SHOW RUNNING STAGE`
  - `CONTINUOUS EDIT`
  - `TRANSPOSE`
  - `TRANSPOSE +12`
  - `TRANSPOSE +1`
  - `TRANSPOSE -1`
  - `TRANSPOSE -12`
  - `TRANSPOSE PATTERN +12`
  - `TRANSPOSE PATTERN +1`
  - `TRANSPOSE PATTERN -1`
  - `TRANSPOSE PATTERN -12`
  - `EDIT`
  - `COPY SEQUENCE`
  - `CUT SEQUENCE`
  - `PASTE SEQUENCE`
  - `DELETE SEQUENCE (option/window key down)`
  - `DELETE SEQUENCE`
  - `COPY CURR PATTERN`
  - `CUT CURR PATTERN`
  - `PASTE PATTERN`
  - `DELETE CURR PATTERN`
  - `DELETE ALL PATTERNS (option/window key down)`
  - `DELETE ALL PATTERNS`
  - `STAGE COMMANDS`
  - `ADD STAGE`
  - `UPDATE CURR STAGE FROM SCREEN`
  - `INSERT STAGE`
  - `LOAD CURR STAGE TO SCREEN`
  - `DELETE STAGE`
  - `PATTERN COMMANDS`
  - `ADD PATTERN`
  - `UPDATE CURR PATTERN FROM SEQUENCE`
  - `INSERT PATTERN`
  - `LOAD PATTERN TO SEQUENCE`
  - `SQ2 LEGENDA`
  - `START/STOP TAP - START/STOP PULSE OUT`
  - `RESET TAP - RESET PULSE OUT`
  - `LFO FREQ - CLK PULSE OUT`
  - `CLK PULSE IN - SYNC PULSE IN`
  - `STEPS COUNT`
  - `CURRENT STEP`
  - `CURRENT STEP NOTE`
  - `CURRENT STEP CV2`
  - `CURRENT STEP GATE %`
  - `CURRENT STEP DURATION`
  - `PATTERN SEQ ON/OFF`
  - `PATTERNS COUNT`
  - `CURRENT PATTERN`
  - `TRIG PULSE OUT - GATE OUT`
  - `CV1 OUT (NOTE) - CV2 OUT`
  - `END OF PATTERN PULSE OUT`
  - `SQUONK`
  - `Select Stage`
  - `Multiplier For Stage`
  - `CV Line A Value For Stage`
  - `CV Line B Value For Stage`
  - `CV Line C Value For Stage`
  - `CV Line D Value For Stage`
  - `CV Line E Value For Stage`
  - `Mode For Stage`
  - `Ratchet Repetitions For Stage`
  - `Rotation Steps Per CLOCK IN`
  - `Select Stage`
  - `Advance Direction`
  - `Randomize Stage Mode`
  - `START`
  - `STOP`
  - `RESET`
  - `Randomize All Filtered (Use SHIFT KEY)`
  - `Rotation Steps CV In VCA`
  - `CV Stage Select (1V/oct)`
  - `TRIG UP/DOWN MODE`
  - `TRIG START`
  - `TRIG STOP`
  - `TRIG RESET`
  - `TRIG RANDOM ALL`
  - `TRIG Select Stage`
  - `CV Line A For Chaining`
  - `CV Line B For Chaining`
  - `CV Line C For Chaining`
  - `CV Line D For Chaining`
  - `CV Line E For Chaining`
  - `TRIG For Chaining`
  - `CV Rotation Steps`
  - `AUDIO Or CV To Be Bridged When Active Stage`
  - `CV To Override Line A, Stage`
  - `CV Line A`
  - `CV Line B`
  - `CV Line C`
  - `CV Line D`
  - `CV Line E`
  - `TRIG Common`
  - `TRIG Last Step`
  - `TRIG Incoming Clock`
  - `TRIG Selected Stage`
  - `GATE Selected Stage`
  - `BRIDGEd AUDIO Or CV Selected Stage`
  - `Selected Stage`
  - `Mutiplied By 5 Selected Stage`
  - `Mode For Selected Stage`
  - `Direction UP If Light ON`
  - `RANDOM STAGE SELECT If Light ON`
  - `GATE If Light ON For Selected Stage`

## NYSTHIOMETER

- slug `NYSTHIOMETER`, 1 HP, tags: Blank
- panel: NYSTHIOMETER.svg

No live text on the panel — outlined, or unlabelled. Read `panels/NYSTHIOMETER.png` to see it.

Strings in its code:

  - `NYSTHIOMETER`

From the changelog:

  * add ZSTD decompress utility on drag and drop of files for type "vcvx" :D
  * add ZSTD compress utility on drag and drop of directories with type "vcvx"

## QuadPanner

- slug `QuadPanner`, 10 HP, tags: Mixer, Panning
- Buchla 227e single channel emulation: quadraphonic audio source placer
- panel: QuadPanner.svg

No live text on the panel — outlined, or unlabelled. Read `panels/QuadPanner.png` to see it.

Strings in its code:

  - `POINTS in WAVE`
  - `WAVES in BANK`
  - `256 points`
  - `512 points`
  - `1024 points`
  - `2048 points`
  - `4096 points`
  - `8192 points`
  - `8 waves`
  - `16 waves`
  - `32 waves`
  - `64 waves`
  - `QuadPanner`
  - `SCALE X`
  - `OFFSET X`
  - `volts`
  - `SCALE Y`
  - `OFFSET Y`
  - `AZIMUTH SCALE`
  - `AZIMUTH DEG OFFSET`
  - `MAGNITUDE SCALE`
  - `MAGNITUDE OFFSET`
  - `SWIRL ON-OFF`
  - `SWIRL RATE`
  - `SWIRL MAGNITUDE`
  - `EPP ON-OFF`
  - `IDIC ON-OFF`

From the changelog:

  *   added CV source mode (from contextual menu "uses 10V if no input")
  *   feature request: added GATE output when touching the area: to be used as controller

## DX7Envelope

- slug `DX7Envelope`, 8 HP, tags: Envelope generator, Polyphonic
- DX7 style envelope
- panel: DX7Envelope.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DX7Envelope.png` to see it.

Strings in its code:

  - `Path length`
  - `Speed`
  - `Km/h`
  - `Traveled`
  - `Distance sound source`
  - `Speed toward listener`
  - `Vector settings`
  - `Add Point`
  - `Remove Point`
  - `Clear All`
  - `Km/h`
  - `DX7Envelope`

From the changelog:

  *   [BUG] Repair the POLY behaviour: correct initialization
  *   emulation of the 4 rates 4 levels DX7 operator envelop
  *   values are like in DX7 from 0 to 1
  *   rates are from super SLOW to hyper FAST
  *   levels are from 0 to MAX
  *   added SCALE (from -1 to 1) and OFFSET (-10 to +10)
  *   2 inner VCA to control Audio Stereo signals
  *   TRIG (Attack Release) mode or GATED (level 1, level 2 level3SUSTAIN, level 4)
  *   TRIG CHAIN to control more DXEnvelopes with same trig or gate signal
  *   TRIG GATE button
  *   LED to signal the trig or gate status
  *   fully POLY based on the GATE input number of channels
  *   fully poly for the 2 VCA 1 and 2 (32 vcas)
  *   added INVERTED OUTPUT (-SIGNAL)

## DualFeedbackEcho

- slug `DualFeedbackEcho`, 2 HP, tags: Effect, Delay
- experimental delay driven by VCO
- panel: DualFeedbackEcho.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DualFeedbackEcho.png` to see it.

Strings in its code:

  - `DualFeedbackEcho`

## Quad DC Block

- slug `4DCBlock`, 2 HP, tags: VCF, Quad
- High pass filter, selectable, to avoid Direct currents
- panel: 4DCBlock.svg

No live text on the panel — outlined, or unlabelled. Read `panels/4DCBlock.png` to see it.

Strings in its code:

  - `value`
  - `fonts/LEDCalculator.ttf`
  - `COMMANDS`
  - `OPEN a LOGAN CSV`
  - `LOGGING IMPORT SEPARATOR`
  - `TAB`
  - `CSV files:csv`
  - `File`
  - `is empty`
  - `configBypass`
  - `br.outputId != outputId`

## PlateVerb: Plate Reverb

- slug `PlateVerb`, 7 HP, tags: Reverb, Effect
- Plate Reverb based on dattorro paper (beware, sometimes scares you!)
- panel: PlateVerb.svg

No live text on the panel — outlined, or unlabelled. Read `panels/PlateVerb.png` to see it.

Strings in its code:

  - `DETECTION PRECISION ON LOW & SPEED`
  - `Detect from  30Hz, delay 70 msecs`
  - `Detect from  45Hz, delay 50 msecs`
  - `Detect from  60Hz, delay 35 msecs`
  - `Detect from  90Hz, delay 25 msecs`
  - `Detect from 120Hz, delay 18 msecs`
  - `Detect from 180Hz, delay 13 msecs`
  - `Detect from 240Hz, delay  9 msecs`
  - `Detect from 360Hz, delay  6 msecs`
  - `Detect from 480Hz, delay  4 msecs`
  - `Detect from 720Hz, delay  3 msecs`
  - `PlateVerb`

## DissonantVerb: Plate Reverb

- slug `DissonantVerb`, 7 HP, tags: Reverb, Effect
- Plate Reverb with dual pitch shifter (with different tuning) in the feedback loop
- panel: DissonantVerb.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DissonantVerb.png` to see it.

Strings in its code:

  - `DissonantVerb`

## ScalaQuantizer

- slug `ScalaQuantizer`, 10 HP, tags: Quantizer, Polyphonic
- Quantizer based on .scl SCALA files
- panel: ScalaQuantizer.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `SCALE BASE` at 200, 380

Strings in its code:

  - `logoblack`
  - `bgndwhite`
  - `logowhite`
  - `bgndblack`
  - `ScalaQuantizer`

From the changelog:

  *  debug: was not loading the selected scale reopening a file at RACK relaunch
  *  DEBUG: in midimap mode, the root base (from contextual menu) must be applied to
     extra outputs too (#333)
  *  debug: was not loading the selected scale reopening a file at RACK relaunch
  *  DEBUG: in midimap mode, the root base (from contextual menu) must be applied to
     extra outputs too (#333)
  *	FEATURE REQUEST: add root frequency for the applied, scala.   (#313)
  	The base it's always C4 (261.626 Hz)
  	The field is editable and value is in Hz
  	You can write also in note form : "C3 -> B4"
  	the field will autotranslate in correct frequency
  	The frequency can be modulate via CV where
  	0 -> 10v are mapped from C3 to C5 range
  	Added a Reset scale root command (to C4) via contextual menu

## EqualDivisionQuantizer

- slug `EqualDivisionQuantizer`, 7 HP, tags: Quantizer, Polyphonic
- Quantizer based on the formula A-th root of B
- panel: EqualDivisionQuantizer.svg

No live text on the panel — outlined, or unlabelled. Read `panels/EqualDivisionQuantizer.png` to see it.

Strings in its code:

  - `EXPONENTIAL MODE (on - off):`
  - `OFF`
  - `Envelope Goes To Zero when TRIGGED/GATED`
  - `Envelope Triggerable Only When Stable`
  - `Time Ranges`
  - `Time Range: 1 second`
  - `Time Range: 10 second`
  - `Time Range: 100 second`
  - `EN status:`
  - `LVL`
  - `SCL`
  - `EN LEGENDA, an AR or ADSR style envelope`
  - `AR mode ON OFF`
  - `LOOP mode ON OFF (only if AR)`
  - `TAP and IN GATE TRIG`
  - `ATTACK + ATTACK CV`
  - `DECAY + DECAY CV`
  - `SUSTAIN + SUSTAIN CV`
  - `RELEASE + RELEASE CV`
  - `BASE LEVEL (from 0 to 1)`
  - `SCALE (from -2 to +2: negative inverts)`
  - `ENV OUT`
  - `VCA IN`
  - `VCA OUT`
  - `End Of Cycle OUT`
  - `EqualDivisionQuantizer`

From the changelog:

  * debug not saving status in v2

## XattoDelayer

- slug `XattoTime`, 7 HP, tags: Effect, Delay, Quad
- Clockable delayer, with precise microseconds settings
- panel: XattoTime.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `DRY-WET` at 207, 29
  - `FB` at -32, 62
  - `MSEC DELAY` at 153, 63
  - `CLK` at -117, 98
  - `OUT` at -51, 98
  - `CLOCK-IN` at 131, 101

Strings in its code:

  - `#version 120`
  - `varying vec3 fragmentColor;`
  - `// Output color = color specified in the vertex shader,`
  - `// interpolated between all 3 surrounding vertices`
  - `gl_FragColor = vec4(fragmentColor, 1);`
  - `#version 120`
  - `attribute vec3 vertexPosition_modelspace;`
  - `attribute vec3 vertexColor;`
  - `// Output data ; will be interpolated for each fragment.`
  - `varying vec3 fragmentColor;`
  - `// Values that stay constant for the whole mesh.`
  - `uniform mat4 MVP;`
  - `gl_Position =  MVP * vec4(vertexPosition_modelspace,1);`
  - `// The color of each vertex will be interpolated`
  - `// to produce the color of each fragment`
  - `fragmentColor = vertexColor;`
  - `MVP`
  - `vertexColor`
  - `History Fades Exponentially`
  - `Show Axis`
  - `Show Grid`
  - `Faster And Less Resolution`
  - `Show Vectors`
  - `Show Points`
  - `Use Beam Size CV IN as Z channel`
  - `Burn-in Luminosity`
  - `Blue Beam`
  - `Green Beam`
  - `White Beam`
  - `Linking program`
  - `XattoTime`
  - `input 2 gain`
  - `dry-wet mix`
  - `Feedback`

## ConstAddMult

- slug `ConstAddMult`, 7 HP, tags: Utility, Attenuator, VCA, Quad, Polyphonic
- Utility to Add, Sub, Div and Mult
- panel: ConstAddMult.svg

No live text on the panel — outlined, or unlabelled. Read `panels/ConstAddMult.png` to see it.

Strings in its code:

  - `Delay CV Input Mapping Modes`
  - `Old style mode`
  - `Exponential mode`
  - `Inverse Exponential mode`
  - `Logaritmic mode`
  - `Linear mode`
  - `ConstAddMult`

## 8 AD

- slug `8AttackDecay`, 18 HP, tags: Function generator, Envelope generator, Quad
- 8 Attack Decay EG with VCA and unity mixer
- panel: 8AttackDecay.svg

No live text on the panel — outlined, or unlabelled. Read `panels/8AttackDecay.png` to see it.

Strings in its code:

  - `Use Old Portamento Scaling`

## QuadSimplerSlicerQuantizer

- slug `QuadSimplerSlicerQuantizer`, 4 HP, tags: Quantizer, Utility, Quad
- Quad Equal voltage division quantizer
- panel: QuadSimplerSlicerQuantizer.svg

No live text on the panel — outlined, or unlabelled. Read `panels/QuadSimplerSlicerQuantizer.png` to see it.

Strings in its code:

  - `DECLICK ON START`
  - `Can Use Negative FM CV`
  - `Exclude Connected SAMPLES from MIX`
  - `Autoplay on LOAD`
  - `Open WAV or AIFF File SLOT 1 ...`
  - `Open WAV or AIFF File SLOT 2 ...`
  - `Open WAV or AIFF File SLOT 3 ...`
  - `Open WAV or AIFF File SLOT 4 ...`
  - `FILE`
  - `Declick on start DISABLED`
  - `Declick on start 5 samples`
  - `Declick on start 10 samples`
  - `Declick on start 20 samples`
  - `Declick on start 50 samples`
  - `Declick on start 100 samples`
  - `Declick on start 200 samples`
  - `Declick on start 500 samples`
  - `Declick on start 1000 samples`
  - `WAV files:wav;AIFF files:aif,aiff`
  - `QuadSimplerSlicerQuantizer`

From the changelog:

  * simd speed up, poly mode, support for 10v, 5v and 1v subdivision
  * added rounding modes (ceil, floor, round)
  * support for negative voltages

## Spectre

- slug `Spectre`, 18 HP, tags: Visual
- Real time spectrogram
- panel: Spectre.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Spectre.png` to see it.

Strings in its code:

  - `Spectre`

## DeepNote

- slug `DeepNote`, 7 HP, tags: VCO, Utility
- THX effect simulator
- panel: DeepNote.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DeepNote.png` to see it.

Strings in its code:

  - `Attack ms`
  - `Release ms`
  - `Ratio`
  - `Threshold dB`
  - `Knee`
  - `Lookahead ms`
  - `Pre dB`
  - `Post dB`
  - `A ms`
  - `R ms`
  - `Thre dB`
  - `LA ms`
  - `DeepNote`
  - `error: first time must be always 0.0`
  - `error: time not accepted must be a growing time`

## DOPPLAB Doppler simulator

- slug `DOPPLAB`, 30 HP, tags: Filter
- Doppler simulator
- panel: DOPPLAB.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DOPPLAB.png` to see it.

Strings in its code:

  - `DOPPLAB`

## StereoPhaser2 phaser

- slug `StereoPhaser2`, 7 HP, tags: Effect, Phaser
- 12-24 cells 1st-2nd order phaser
- panel: StereoPhaser2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/StereoPhaser2.png` to see it.

Strings in its code:

  - `StereoPhaser2`

## Stereo Chorus and Tremolo

- slug `StereoChorus2`, 7 HP, tags: Effect, Chorus
- Stereo Chorus + Tremolo with added nysthi movements modulators
- panel: StereoChorus2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/StereoChorus2.png` to see it.

Strings in its code:

  - `Window function`
  - `None`
  - `Hamming`
  - `Hann`
  - `Bartlett`
  - `Blackman`
  - `Color mode Jet`
  - `Color mode Jet 2`
  - `Color mode 2`
  - `Color mode 3`
  - `Color mode BLACK`
  - `Color mode WHITE`
  - `StereoChorus2`

## Flipper

- slug `Flipper`, 10 HP, tags: Logic, Utility
- FlipFlops and Trigs to generate GATES and TRIGS on demand 
- panel: Flipper.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Flipper.png` to see it.

Strings in its code:

  - `Flipper`

## DYNAMO

- slug `DYNAMO`, 10 HP, tags: Effect, Compressor
- Stereo compressor, hard limiter
- panel: DYNAMO.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DYNAMO.png` to see it.

Strings in its code:

  - `DYNAMO`

## Strummer

- slug `Strummer`, 7 HP, tags: Utility
- String strummer device
- panel: Strummer.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Strummer.png` to see it.

Strings in its code:

  - `Strummer`

## AutoFader

- slug `AutoFader`, 10 HP, tags: Utility, recording, VCA
- Automated fader and cross fader
- panel: AutoFader.svg

No live text on the panel — outlined, or unlabelled. Read `panels/AutoFader.png` to see it.

Strings in its code:

  - `AutoFader`

## VectorMixer

- slug `VectorMixer`, 17 HP, tags: Mixer, Utility, VCA
- Bidimensional mixer with 4 sources and sequencer for animations
- panel: VectorMixerB.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `GATE` at -62, 300

Strings in its code:

  - `VectorMixer`

From the changelog:

  *   moved lowpass output to context menu
  *   feature request: added GATE output when touching the area: to be used as controller

## metaAARDVARK

- slug `metaAARDVARK`, 6 HP, tags: Noise, Utility, Clock, S&H, LFO
- Warren Burt noise s&h lfo Aardvark IV RVG cell, clockable, cvable, syncable
- panel: metaAARDVARK.svg

No live text on the panel — outlined, or unlabelled. Read `panels/metaAARDVARK.png` to see it.

Strings in its code:

  - `GRID`
  - `GRID Active`
  - `DELETE Current Slice`
  - `LOAD Current Slice`
  - `ADD Slice`
  - `UPDATE Current Slice`
  - `Long gate on REC delete`
  - `metaAARDVARK`

## Jooper 8 channel

- slug `Jooper`, 18 HP, tags: Switch, Utility, Mixer, Multiple
- 8 channel, with scene manager, UNITYMIXER, MULTIPLEXER, IN-OUT SWITCH
- panel: Jooper.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Jooper.png` to see it.

Strings in its code:

  - `Jooper`

## ClockableDelay

- slug `ClockableDelay`, 7 HP, tags: Effect, Delay
-  stereo echo delay, clockable, synchronizable, DLD imitation
- panel: ClockableDelay.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `PING-PONG MODE` at 112, 46
  - `IN-GAIN` at 123, 58

Strings in its code:

  - `BPM`
  - `No CLOCK OUT If No CLOCK IN Connected`
  - `ClockableDelay`

From the changelog:

  * bigger resolution to feedback
  * add ping pong mode
  * bigger resolution to feedback
  * add ping pong mode

## STKPitchShifter

- slug `STKPitchShifter`, 7 HP, tags: Effect, filter
- pitch shifter from STK library
- panel: STKPitchShifter.svg

No live text on the panel — outlined, or unlabelled. Read `panels/STKPitchShifter.png` to see it.

Strings in its code:

  - `SCALES`
  - `RANDOMIZE RANGE`
  - `RANDOMIZE FULL`
  - `RANDOMIZE 20%`
  - `RANDOMIZE 10%`
  - `RANDOMIZE 5%`
  - `RANDOMIZE CV LINE A`
  - `RANDOMIZE CV LINE B`
  - `RANDOMIZE CV LINE C`
  - `RANDOMIZE CV LINE D`
  - `RANDOMIZE CV LINE E`
  - `RANDOMIZE REP(etitions)`
  - `RANDOMIZE DIRECTION`
  - `RANDOMIZE x5 multiplier`
  - `RANDOMIZE MODE`
  - `STKPitchShifter`

## 4Hands

- slug `4Hands`, 18 HP, tags: Controller, Utility
- multi value sequencer
- panel: 4Hands.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `OCTAVE` at -120, 297

Strings in its code:

  - `Set to 5Hz`
  - `Set to 10Hz`
  - `Set to 15Hz`
  - `Set to 20Hz`
  - `Set to 25Hz`
  - `Set to 30Hz`

From the changelog:

  *  feature request: added INSERT scene command, via TAP and TRIG IN
     the insert mode is BEFORE current scene
  *  feature request: added INSERT scene command, via TAP and TRIG IN
     the insert mode is BEFORE current scene

## LFOMultiPhase

- slug `LFOMultiPhase`, 7 HP, tags: LFO, VCO
- LFO with 8 fixed phases and 1 variable, with precise frequency setting
- panel: LFOMultiPhase.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `CV-VCA-FREE-PHASE` at -70, 314

Strings in its code:

  - `EXCLUSIVE`
  - `MULTIPLEXER`
  - `DUAL EXCL A-B`
  - `Special Modes`
  - `Multiplex EXCLUSIVE mode (no summing)`
  - `PREV-NEXT SCENE AUTOLOAD`
  - `USE OLD STYLE CV SELECT (all range)`
  - `LFOMultiPhase`
  - `REALFREQUENCY`
  - `LFO wave shape`
  - `UNI-BI polar`
  - `Tap Tempo`
  - `Frequency multiplier`
  - `Frequency fine`
  - `FM CV VCA`
  - `Pulse Width`
  - `Pulse with modulation CV VCA`
  - `Free Phase Degrees`
  - `Free Phase Degrees CV VCA`
  - `Morph LFO wave shape`
  - `Morph LFO wave shape CV VCA`
  - `Tap Sync`

## mix4

- slug `mix4`, 8 HP, tags: Mixer, VCA, Polyphonic
- 4mix 4 channels stero mixer. Channel 1 LEFT is POLY in INPUT
- panel: mix4.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `mix4` at 160, 20

Strings in its code:

  - `Centered`
  - `Left`
  - `Right`
  - `SHOW POST OUTPUTS`
  - `HIDE POST OUTPUTS`
  - `POST OUT won't SOLO`
  - `Polyphony channels POST OUT Channel 1`
  - `LIMITER`
  - `No limiter`
  - `Hard limit +0dB`
  - `Hard limit +3dB`
  - `Hard limit +6dB`
  - `Soft limit +0dB`
  - `Soft limit +3dB`

## mix8

- slug `mix8`, 13 HP, tags: Mixer, VCA, Polyphonic
- 8mix 8 channels stero mixer. Channel 1 LEFT is POLY in INPUT
- panel: mix8.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `mix8` at 263, 20

## mix16

- slug `mix16`, 24 HP, tags: Mixer, VCA, Polyphonic
- 16mix 16 channels stero mixer. Channel 1 LEFT is POLY in INPUT
- panel: mix16.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `mix16` at 400, 20

From the changelog:

  * 	visually renamed with real name (before was 4MIX 8MIX and 16MIX)
  * 	visually renamed with real name (before was 4MIX 8MIX and 16MIX)

## UNNYSTHIPLEASURESGRAPHER

- slug `UNNYSTHIPLEASURESGRAPHER`, 20 HP, tags: Visual
- TOY Waterfall Spectrograph
- panel: UNNYSTHIPLEASURESGRAPHER.svg

No live text on the panel — outlined, or unlabelled. Read `panels/UNNYSTHIPLEASURESGRAPHER.png` to see it.

Strings in its code:

  - `UNNYSTHIPLEASURESGRAPHER`

## Phasor

- slug `Phasor`, 39 HP, tags: VCO
- Graphic Harmonic VCO
- panel: Phasor.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Phasor.png` to see it.

Strings in its code:

  - `Phasor`

## BIGNUMBER

- slug `BIGNUMBER`, 7 HP, tags: Visual, Utility
- panel: BIGNUMBER.svg

No live text on the panel — outlined, or unlabelled. Read `panels/BIGNUMBER.png` to see it.

Strings in its code:

  - `BUTTONS MODE (GATE OR TRIG)`
  - `BUTTON 1 AS GATE`
  - `BUTTON 2 AS GATE`
  - `BIGNUMBER`

## BIGBUTTON

- slug `BIGBUTTON`, 7 HP, tags: Utility
- panel: BIGBUTTON.svg

No live text on the panel — outlined, or unlabelled. Read `panels/BIGBUTTON.png` to see it.

Strings in its code:

  - `BIGBUTTON`

## µOPERATOR

- slug `OP`, 1 HP, tags: VCO, LFO, FM, Polyphonic, Utility
- FM Operator dx7 style, Sinusoid VCO
- panel: OP.svg

No live text on the panel — outlined, or unlabelled. Read `panels/OP.png` to see it.

Strings in its code:

  - `Modulation index`
  - `FM Feedback`
  - `FM Ratio`

## µENVELOPE

- slug `EN`, 1 HP, tags: Envelope generator, Function generator, FM
- FM Envelope generator AR or ADSR style
- panel: EN.svg

No live text on the panel — outlined, or unlabelled. Read `panels/EN.png` to see it.

From the changelog:

  *  DEBUG: current NODEs status not highlighting
  *  DEBUG: current NODEs status not highlighting
  *	bug: in some situations with very aligned controlpoints and nodes, we were hitting some bizarre math limits... :D (thanks Mark Sanders for the #324)
  *	it's a simple Envelope with no sustain, brother of BZ-MAPPER
  * 	can use the expanders as the BZ-MAPPER (BZ-XPAND and BZ-XPANDXPAND)
  *  there are 2 to 6 nodes (and 1 to 5 Control Points)
  *  nodes are persistent (when switching, module will remember previous settings)
  *  from contextual menu it's possible to set 4 durations
  	*	from 0.001 to 0.100 seconds
  	*	from 0.100 to 1.000 seconds
  	*	from 1.000 to 10.00 seconds
  	*	from 10.00 to 60.00 seconds
  * the knob "duration" will set the precise value between the min and max of the range
  * the duration knob acts in exponential mode
  * the BZ-ENVELOPE can be looped
  * BZ-ENVELOPE is fully POLY
  * the TRIG IN accepts from 1 to 16 channels
  * the TAP TRIG let you activate on demand the ENVELOPE
  * the EOC (End of Cycle) is a pulse emitted at the end of every cycle (in loop mode too)
  * the ENV-OUT is the poly out and can output in 2 ranges
  	* -5 to 5V range
  	* 0 TO 10V range
  * Voltage range is changed using the contextual menu on the Display (where the range is presented)
  * the Zoom of vertical of the screen is set using the contextual menu on the left in the ZOOM area
  * dragging is quantized if SHIFT key is pressed before dragging: nodes will be quantized in 0.5 volts steps
  *  dragging with COMMAND/WINDOW key means "precise/slow dragging"
  *   [BUG] Repair the POLY behaviour: correct initialization
  *   emulation of the 4 rates 4 levels DX7 operator envelop
  *   values are like in DX7 from 0 to 1
  *   rates are from super SLOW to hyper FAST
  *   levels are from 0 to MAX
  *   added SCALE (from -1 to 1) and OFFSET (-10 to +10)
  *   2 inner VCA to control Audio Stereo signals
  *   TRIG (Attack Release) mode or GATED (level 1, level 2 level3SUSTAIN, level 4)
  *   TRIG CHAIN to control more DXEnvelopes with same trig or gate signal
  *   TRIG GATE button
  *   LED to signal the trig or gate status
  *   fully POLY based on the GATE input number of channels
  *   fully poly for the 2 VCA 1 and 2 (32 vcas)
  *   added INVERTED OUTPUT (-SIGNAL)

## µMIXER

- slug `M1`, 1 HP, tags: Mixer
- FM Mixer helper, Stereo mixer, 3 IN channels
- panel: M1.svg

No live text on the panel — outlined, or unlabelled. Read `panels/M1.png` to see it.

Strings in its code:

  - `BAR PRECOUNT 0`
  - `BAR PRECOUNT 1`
  - `BAR PRECOUNT 2`

From the changelog:

  *   moved lowpass output to context menu
  *   feature request: added GATE output when touching the area: to be used as controller

## µMIXER2

- slug `M2`, 1 HP, tags: Mixer, Quad, VCA, FM
- FM Mixer helper, Mixer, 4 IN mono channels
- panel: M2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/M2.png` to see it.

Strings in its code:

  - `M1 LEGENDA, a 3 chans mixer with VCAs and PANs`
  - `for every channel:`
  - `IN SIGNAL`
  - `SIGNAL LEVEL`
  - `SIGNAL LEVEL CV`
  - `PAN CONTROL`
  - `MUTE ON-OFF`
  - `IN CHAIN LEFT`
  - `IN CHAIN RIGHT`
  - `OUT LEVEL`
  - `OUT LEFT`
  - `OUT RIGHT`

## µSEQUENCER1

- slug `SQ1`, 2 HP, tags: Sequencer
- 16 steps analog style sequencer
- panel: SQ1.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SQ1.png` to see it.

Strings in its code:

  - `SQ1`

## µSEQUENCER2

- slug `SQ2`, 2 HP, tags: Sequencer
- Programmable sequencer (Roland MC-4 style)
- panel: SQ2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SQ2.png` to see it.

Strings in its code:

  - `CLOCK FREQ:`
  - `BPM:`
  - `OCTAVE OFFSET:`
  - `OCTAVE + 1`
  - `OCTAVE - 1`
  - `SEQUENCER MODE`
  - `NORMAL`
  - `REVERSE`
  - `PENDULUM 1`
  - `PENDULUM 2`
  - `RANDOM`
  - `SQ1 LEGENDA, 16 stages analog style sequencer`
  - `ACTIVE ON-OFF + TRIG`
  - `RESET TAP and TRIG`
  - `INNER CLOCK LFO, and CV CONTROL`
  - `EXT CLOCK TIME COMPUTER and RESET INNER LFO`
  - `GATE % + CV`
  - `NUM STEPS + CV`
  - `16 stages with lights for current and last`
  - `OFFSET CV OUT + CV`
  - `TRIG to STEP STAGE - CV to SELECT STAGE`
  - `OUT TRIG and OUT GATE`
  - `OUT CURRENT STAGE CV and OUT LAST STEP PULSE`
  - `SQ2`
  - `pattern`
  - `REST`

## µSLEW

- slug `SL`, 1 HP, tags: Dual, Slew Limiter, Envelope Follower, Filter
- Dual slew with shaper
- panel: SL.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SL.png` to see it.

Strings in its code:

  - `fonts/CommodoreRoundedv1.2.ttf`
  - `Applying rule:`
  - `%3i      %c%c      %3i       %i`
  - `%3i      ??      %3i       %i`
  - `%d: %c%c`
  - `insert WX following dipthong NOT ending in IY sound`
  - `insert YX following dipthong ending in IY sound`
  - `RULE: %s`
  - `phoneme %d (%c%c) length %d`
  - `Error reading from tables`
  - `%5i %5i %5i %5i %5i %5i %5i %5i`
  - `Random Sentence Mode`
  - `Internal Phoneme presentation:`
  - `idx    phoneme  length  stress`
  - `PRE`
  - `POST`
  - `Final data for speech output:`
  - `flags ampl1 freq1 ampl2 freq2 ampl3 freq3 pitch`
  - `Error writing to tables`

## µDELAY

- slug `DL`, 3 HP, tags: Effect, Dual, Delay
- Dual signal delay with display
- panel: DL.svg

No live text on the panel — outlined, or unlabelled. Read `panels/DL.png` to see it.

From the changelog:

  * bigger resolution to feedback
  * add ping pong mode
  * bigger resolution to feedback
  * add ping pong mode

## confusingSimpler

- slug `confusingSimpler`, 7 HP, tags: Recording, Sampler, VCO
- Really confused sampler oscillator
- panel: confusingSimpler.svg

No live text on the panel — outlined, or unlabelled. Read `panels/confusingSimpler.png` to see it.

Strings in its code:

  - `confusingSimpler`

From the changelog:

  *  add a RND input for the GRID SLICE mode (insted of using the RND input in SLICE sequencer)
  *  add a grid on off microbutton (in GRID area, close to GRID title)
  *  add a RND on EOC mode for the SLICE sequencer, is valid if GRID MODE is OFF
  *  add: if in GRID mode the RANDOM select input in the slice area will select a RANDOM GRID selection
  *  feature request: add capability to import "cue " chunks from WAV files (we can import now Morphagene splice points)
     rules are:
     1) As in Morphagene we have splices and in SIMPLICITER slices, we import them as ranges
       slice 1 is  splice1 to splice 2
       slice 2 is splice 2 to splice 3
       etc
     2) If SIMPLICITER contains already slices, no splices will be imported
     3) if 2 or more contiguous splices are of the same value, only one will be considered
     4) Last splice will always create a slice going from "last splice" to "end of sample" (only special case if last splice IS last sample)
  *  new export functionality, the Save as WAV command will export slices as CUE points
     1) A list of slices must exist
     2) Only the START of every slice will be exported as CUE POINT
  *	DEBUG: "STOP recording with append" was inverting channels (#338)
  *  FEATURE REQ: press REC with SHIFT to activate automatic APPEND for current REC
  *  add a RND input for the GRID SLICE mode (insted of using the RND input in SLICE sequencer)
  *  add a grid on off microbutton (in GRID area, close to GRID title)
  *  add a RND on EOC mode for the SLICE sequencer, is valid if GRID MODE is OFF
  *  add: if in GRID mode the RANDOM select input in the slice area will select a RANDOM GRID selection
  *  feature request: add capability to import "cue " chunks from WAV files (we can import now Morphagene splice points)
     rules are:
     1) As in Morphagene we have splices and in SIMPLICITER slices, we import them as ranges
       slice 1 is  splice1 to splice 2
       slice 2 is splice 2 to splice 3
       etc
     2) If SIMPLICITER contains already slices, no splices will be imported
     3) if 2 or more contiguous splices are of the same value, only one will be considered
     4) Last splice will always create a slice going from "last splice" to "end of sample" (only special case if last splice IS last sample)
  *  new export functionality, the Save as WAV command will export slices as CUE points
     1) A list of slices must exist
     2) Only the START of every slice will be exported as CUE POINT
  *	DEBUG: "STOP recording with append" was inverting channels (#338)
  *  FEATURE REQ: press REC with SHIFT to activate automatic APPEND for current REC
  *	bug: start was delayed because the anticlick was not inited
  * removed the use of AUTOMATIC cache
    *  loaded files are always load from their current position (if they still exist)
    *  if a recording or an append is done, saving the file will connect the sampler to the saved file

## WORMHOLIZER

- slug `WORMHOLIZER`, 7 HP, tags: Delay, Effect, Reverb
- Wormhole effect, delay reverb
- panel: WORMHOLIZER.svg

No live text on the panel — outlined, or unlabelled. Read `panels/WORMHOLIZER.png` to see it.

Strings in its code:

  - `Voltages`
  - `Use input range -5V to +5V (normally 0V to 10V)`
  - `Trails`
  - `X-Y modes (inputs + animator)`
  - `PRIORITY to the X-Y POSITIONS inputs`
  - `MIX X-Y POSITIONS inputs with KF ANIMATOR if running`
  - `PRIORITY to the KF ANIMATOR if running`
  - `Lowpass output`
  - `WORMHOLIZER`

## CONVOLVZILLA reverb

- slug `CONVOLVZILLA`, 21 HP, tags: Delay, Effect, Reverb
- Real time impulse convolver, convolution reverb
- panel: CONVOLVZILLA.svg

No live text on the panel — outlined, or unlabelled. Read `panels/CONVOLVZILLA.png` to see it.

Strings in its code:

  - `Invert bridge mode`
  - `Return last value`
  - `CONVOLVZILLA`
  - `wav`
  - `aif`
  - `aiff`
  - `WAV`
  - `AIF`
  - `AIFF`
  - `RTimpulse`

## Ratchet

- slug `Ratchet`, 3 HP, tags: Utility
- Ratcheting helper, Timed pulse repeater
- panel: Ratchet.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Ratchet.png` to see it.

Strings in its code:

  - `Ratchet`

## Bridges

- slug `Bridges`, 5 HP, tags: Utility
- Gate with probabilty and locks
- panel: Bridges.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Bridges.png` to see it.

Strings in its code:

  - `COMMANDS`
  - `Unselected Voltage Output`
  - `Unselected Outuputs 0 Volts`
  - `Unselected Outuputs Last Voltage`
  - `Probability mode`
  - `USE A/B switching probability mode`
  - `USE A/B routing probability mode`
  - `Bridges`

## Bivio

- slug `Bivio`, 6 HP, tags: Utility
- 3 path randomizers (with probabilty and locks)
- panel: Bivio.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Bivio.png` to see it.

Strings in its code:

  - `Clean All Bits`
  - `Set All Bits`
  - `Poly channels from output 1`
  - `Bivio`

## Janneker

- slug `Janneker`, 5 HP, tags: Utility, Clock
- Pulse sequencer and sequencers director
- panel: Janneker.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Janneker.png` to see it.

Strings in its code:

  - `Multiply delay time by 10x`
  - `Janneker`

From the changelog:

  * was not saving the time scale: solved
  *	FEATURE REQUEST: autodeactivate if hitting END of LOOP and not in LOOP mode (to have the ONE SHOT mode) (#320)
  	the feature is in contextual menu

## JannekerTimed

- slug `JannekerTimed`, 8 HP, tags: Utility, Clock
- Timed pulse sequencer and sequencers director
- panel: JannekerTimed.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `GATE` at -60, 105

Strings in its code:

  - `Auto Deactivate on End of Loop`
  - `JannekerTimed`

From the changelog:

  * was not saving the time scale: solved

## MagisterFuzz

- slug `MagisterFuzz`, 6 HP, tags: Effect, Distortion
- Maestro Fuzz emulator
- panel: MagisterFuzz.svg

No live text on the panel — outlined, or unlabelled. Read `panels/MagisterFuzz.png` to see it.

Strings in its code:

  - `MagisterFuzz`
  - `Gain`
  - `Filter`
  - `Volume`

## RodentV2

- slug `RodentV2`, 14 HP, tags: Effect, Distortion
- ProCo RAT distortion emulator
- panel: RodentV2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/RodentV2.png` to see it.

Strings in its code:

  - `NYSTHI`
  - `VCV RACK`
  - `RodentV2`
  - `Ruetz Mod`
  - `Turbo`
  - `Tight`
  - `Overdrive`

## SmashMaster

- slug `SmashMaster`, 14 HP, tags: Effect, Distortion
- SmashMaster distortion emulator
- panel: SmashMaster.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SmashMaster.png` to see it.

Strings in its code:

  - `SmashMaster`

## RXG100ChanA

- slug `RXG100ChanA`, 14 HP, tags: Effect, Distortion
- RXG100ChanA distortion emulator
- panel: RXG100ChanA.svg

No live text on the panel — outlined, or unlabelled. Read `panels/RXG100ChanA.png` to see it.

Strings in its code:

  - `Use Text Fields`
  - `WONKINESS RECOVER COMMANDS`
  - `WONKINESS Recover`
  - `WONKINESS Recover LOOP count`
  - `LOOP count to Integrality`
  - `2 Loops`
  - `4 Loops`
  - `8 Loops`
  - `16 Loops`
  - `32 Loops`
  - `RXG100ChanA`

## RXG100ChanB

- slug `RXG100ChanB`, 14 HP, tags: Effect, Distortion
- RXG100ChanB distortion emulator
- panel: RXG100ChanB.svg

No live text on the panel — outlined, or unlabelled. Read `panels/RXG100ChanB.png` to see it.

Strings in its code:

  - `RXG100ChanB`

## PolySevenSeas

- slug `PolySevenSeas`, 18 HP, tags: VCO, Polyphonic
- Polyphonic Wavetable VCO, with bidimensional navigator
- panel: PolySevenSeas.svg

No live text on the panel — outlined, or unlabelled. Read `panels/PolySevenSeas.png` to see it.

Strings in its code:

  - `chan`
  - `Use START button as TOGGLE (Start-Stop)`
  - `Consolidate Tracks after Recording`
  - `Save Tracks in Separated WAV Files`
  - `Save In Stereo Files`
  - `Group Separated WAV Files in Directory`
  - `1 channel WAVE files`
  - `1 channel RAW PCM 32bit IEEE FLOAT files`
  - `ENCODING pcm`
  - `ENCODING wav`
  - `PolySevenSeas`

From the changelog:

  * fixed a crash with Drag and Drop
  * fixed saved BANKS: correctly reloaded now
  * variable geometry of the sample cube
  * supporting till 8192 points waves (contextual menu from 256 to 8192)
  * from 8 to 64 waves per bank (contextual menu)
  * at startup is the same as the POLYSEVENSEAS standard, loading the base table of 64 x 64 x 256
  but you can do a CLEAN all LOAD banks, change the geometries (the dependent variable is the BANK number, for example if you set 64 waves of 8192 points, there will be only 2 banks (2 x 64 x 8192 = 1048576 sample points)
  or you have a classic geometry of 32 * 16 * 2048 (hosting 32 SERUM banks of 16 waves)
  when you fill it, you can save as GLOBAL bank and reuse in the defaults
  *  removed multithreading in WIN and LINUX builds
  *  Now multithreading on MAC is by default OFF (you can switch ON using the contextual menu)
  *  removed multithreading in WIN and LINUX builds
  *  Now multithreading on MAC is by default OFF (you can switch ON using the contextual menu)

## PolySevenSeas2

- slug `PolySevenSeas2`, 18 HP, tags: VCO, Polyphonic
- Polyphonic Wavetable VCO, with bidimensional navigator
- panel: PolySevenSeas2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/PolySevenSeas2.png` to see it.

Strings in its code:

  - `Visual scale:`
  - `POWER MODE`
  - `LOW POWER`
  - `MEDIUM POWER`
  - `HCF MODE`
  - `MULTITHREADED`
  - `PROCESSING BLOCK SIZES`
  - `SWARMING VCOs COUNT`
  - `STOP GRAPHIC ANIMATIONS`
  - `REMOVE ALL BANKS`
  - `LOAD GLOBAL BANK`
  - `default bank`
  - `NO BIG BANK LOADED`
  - `SAVE ALL BANKS`
  - `BANKS`
  - `16 samples`
  - `32 samples`
  - `64 samples`
  - `128 samples`
  - `256 samples`
  - `512 samples`
  - `1024 samples`
  - `2048 samples`
  - `4 swarmers`
  - `6 swarmers`
  - `8 swarmers`
  - `12 swarmers`
  - `16 swarmers`
  - `20 swarmers`
  - `24 swarmers`
  - `LOAD BANK`
  - `LOAD BANK`
  - `- LOADED:`
  - `PolySevenSeas2`

From the changelog:

  * fixed a crash with Drag and Drop
  * fixed saved BANKS: correctly reloaded now
  * variable geometry of the sample cube
  * supporting till 8192 points waves (contextual menu from 256 to 8192)
  * from 8 to 64 waves per bank (contextual menu)
  * at startup is the same as the POLYSEVENSEAS standard, loading the base table of 64 x 64 x 256
  but you can do a CLEAN all LOAD banks, change the geometries (the dependent variable is the BANK number, for example if you set 64 waves of 8192 points, there will be only 2 banks (2 x 64 x 8192 = 1048576 sample points)
  or you have a classic geometry of 32 * 16 * 2048 (hosting 32 SERUM banks of 16 waves)
  when you fill it, you can save as GLOBAL bank and reuse in the defaults

## SevenSeas

- slug `SevenSeas`, 18 HP, tags: VCO
- Wavetable VCO, with bidimensional navigator
- panel: SevenSeas.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SevenSeas.png` to see it.

Strings in its code:

  - `Snap values to 0.1`
  - `SevenSeas`

From the changelog:

  * fixed a crash with Drag and Drop
  * fixed saved BANKS: correctly reloaded now
  * variable geometry of the sample cube
  * supporting till 8192 points waves (contextual menu from 256 to 8192)
  * from 8 to 64 waves per bank (contextual menu)
  * at startup is the same as the POLYSEVENSEAS standard, loading the base table of 64 x 64 x 256
  but you can do a CLEAN all LOAD banks, change the geometries (the dependent variable is the BANK number, for example if you set 64 waves of 8192 points, there will be only 2 banks (2 x 64 x 8192 = 1048576 sample points)
  or you have a classic geometry of 32 * 16 * 2048 (hosting 32 SERUM banks of 16 waves)
  when you fill it, you can save as GLOBAL bank and reuse in the defaults
  *  removed multithreading in WIN and LINUX builds
  *  Now multithreading on MAC is by default OFF (you can switch ON using the contextual menu)
  *  removed multithreading in WIN and LINUX builds
  *  Now multithreading on MAC is by default OFF (you can switch ON using the contextual menu)

## TIMEX

- slug `TIMEX`, 8 HP, tags: CLOCK
- Timer and time utilities
- panel: TIMEX.svg

No live text on the panel — outlined, or unlabelled. Read `panels/TIMEX.png` to see it.

Strings in its code:

  - `Don't reset playhead on keyframe change (if possible)`
  - `TIMEX`

## SAM

- slug `SAM`, 14 HP, tags: VCO, VOCODER
- CBM64 voice synthesizer
- panel: SAM.svg

No live text on the panel — outlined, or unlabelled. Read `panels/SAM.png` to see it.

Strings in its code:

  - `SAM`
  - `Hello, my name is SAM.`

## AttackSustainRelease16

- slug `AttackSustainRelease16`, 24 HP, tags: Utility, Quad, Envelope generator, Function generator
- 16 Attack (Sustain) Release Envelopes
- panel: AttackSustainRelease16.svg

No live text on the panel — outlined, or unlabelled. Read `panels/AttackSustainRelease16.png` to see it.

Strings in its code:

  - `AttackSustainRelease16`
  - `AttackSustainRelease8`
  - `AttackSustainRelease4`

From the changelog:

  * added a menu command to transform first channel in poly 4-8-116 ASR

## AttackSustainRelease8

- slug `AttackSustainRelease8`, 14 HP, tags: Utility, Quad, Envelope generator, Function generator
- 8 Attack (Sustain) Release Envelopes
- panel: AttackSustainRelease8.svg

No live text on the panel — outlined, or unlabelled. Read `panels/AttackSustainRelease8.png` to see it.

From the changelog:

  * added a menu command to transform first channel in poly 4-8-116 ASR

## AttackSustainRelease4

- slug `AttackSustainRelease4`, 9 HP, tags: Utility, Quad, Envelope generator, Function generator
- 4 Attack (Sustain) Release Envelopes
- panel: AttackSustainRelease4.svg

No live text on the panel — outlined, or unlabelled. Read `panels/AttackSustainRelease4.png` to see it.

## LOGAN20 logger

- slug `LOGAN20`, 12 HP, tags: Utility
- Logs signal to a CSV file
- panel: LOGAN20.svg

No live text on the panel — outlined, or unlabelled. Read `panels/LOGAN20.png` to see it.

Strings in its code:

  - `LOGGING FREQUENCY`
  - `LOG Every sample`
  - `LOG Every 10 samples`
  - `LOG Every 100 samples`
  - `LOG Every 1000 samples`
  - `LOG Every 10000 samples`
  - `LOGAN20`
  - `%Y%m%d%H%M`

## 02NAGOL UN-logger

- slug `02NAGOL`, 12 HP, tags: Utility
- Read CSV files generated by LOGAN20 and replay it
- panel: 02NAGOL.svg

No live text on the panel — outlined, or unlabelled. Read `panels/02NAGOL.png` to see it.

Strings in its code:

  - `D,G`
  - `D,G`
  - `D,G`
  - `D,G`
  - `D,G`
  - `N14DCBlocksWidget15dcBlockFreqItemE`
  - `D,G`
  - `D,G`
  - `!Dp12ACXELdisplay`
  - `-DT`
  - `D,G`
  - `a?S.`
  - `L9u`
  - `C14AmbuanceModule`
  - `N14AmbuanceModule9roomsizeQE`
  - `N14AmbuanceModule11reverbtimeQE`
  - `C2AD`
  - `#r2528AttackSustainReleaseMultiplo`
  - `D,G`
  - `D,G`
  - `A15AutoFaderModule`
  - `N15BIGBUTTONWidget11bt1ModeItemE`
  - `N15BIGBUTTONWidget11bt2ModeItemE`
  - `C9BIGNUMBER`
  - `=YKm`
  - `=YKm`
  - `MbP?`
  - `!Dp`
  - `D,G`
  - `D,G`
  - `D,G`
  - `C15CVSpreadControl`
  - `C15ClockMultiplier`
  - `&.719PingableStereoDelay`
  - `C12ConstAddMult`
  - `PCG`
  - `C9SlimDelay`
  - `D,G`
  - `C7Doppler`
  - `D,G`
  - `A11DX7Envelope`
  - `D,G`
  - `C22StereoCompressorModule`
  - `MbP?`
  - `MbP?`
  - `C3THX`
  - `C4DAHD`
  - `D,G`
  - `?6Dica33`
  - `C13DissonantVerb`
  - `D,G`
  - `?16DualFeedbackEcho`
  - `?17DualDelayerModule`
  - `.IIIIIIE7SlimEnv`
  - `HC;`
  - `C22EqualDivisionQuantizer`
  - `A11ExpiredTime`
  - `C23TrigStartFlipFlopModule`
  - `D,G`
  - `?Zd;`
  - `D,H`
  - `D,H`
  - `C12HiVerbModule`
  - `B8ECHOREC2`
  - `CUU`
  - `pB14ECHOREC2Widget`
  - `L=33s?`
  - `C/;`
  - `=BD?`
  - `LQU8Janneker`
  - `yDo`
  - `A13JannekerTimed`
  - `"13LFOMultiPhase`
  - `"14LFOMultiPhase2`
  - `B5LOGAN`
  - `C5LOGIC`
  - `!DpN8lupitone8LuPitoneE`
  - `N8lupitone14LuPitoneWidgetE`
  - `N8lupitone15looperBigKnob90E`
  - `N8lupitone15looperPULSEIN24E`
  - `N8lupitone16looperPULSEOUT24E`
  - `N8lupitone16looperTapperGrayE`
  - `N8lupitone20looperBigKnobSnappedE`
  - `N8lupitone18looperTapperRecordE`
  - `N8lupitone16looperTapperStopE`
  - `MbP?`
  - `MbP?`
  - `C7SlimMix`
  - `C8SlimMix2`
  - `?X9`
  - `?11MVerbModule`
  - `N8audiolib7modules8transferIdEE`
  - `N8audiolib7modules9highpass1IdEE`
  - `N8audiolib7modules8lowpass1IdEE`
  - `N22NYMasterRecorderWidget13MRSetBitsItemE`
  - `D,G`
  - `C20MicrotonalHostHelper`
  - `C14Model277Module`
  - `C18ModuloMagicControl`
  - `C10MultiTrack`
  - `N16MultiTrackWidget13MTSetBitsItemE`
  - `MbP?`
  - `xA17MultiTriggerDelay`
  - `2"15MultiVoltometer`
  - `!DpFF`
  - `D,G`
  - `!Dp`
  - `!DpFF`
  - `B7ECHOREC`
  - `CUU`
  - `pB13ECHORECWidget`
  - `D,G`
  - `pA6Pepper`
  - `=%I`
  - `%Fp`
  - `*S]`
  - `)96Phasor`
  - `C9PlateVerb`
  - `#r2529PolyAttackDecaySustainRelease`
  - `N;?`
  - `=Uo`
  - `+?-C`
  - `QTWZ`
  - `]12PolyRecorder`
  - `!Dp`
  - `MbP?`
  - `!Dp`
  - `"3DU`
  - `xxxfp14PolySevenSeas2`
  - `C12NYQuadPanner`
  - `!Dp`
  - `3;CKS[ck11QuadSimpler`
  - `N17QuadSimplerWidget17NYQuadOpenWavItemE`
  - `A26QuadSimplerSlicerQuantizer`
  - `N8audiolib7modules9tonestackIdEE`
  - `A7Ratchet`
  - `TIC?(,`
  - `S.6`
  - `LL`
  - `R.6V6CIO`
  - `BI%3B(/OOBOn`
  - `BBBnnnTTT`
  - `BBBmVmTTT`
  - `J"I`
  - `IQ2`
  - `F!(`
  - `fY,S`
  - `Lb)`
  - `PBTJ`
  - `Jc9`
  - `Hf1`
  - `xpp`
  - `%68X9`
  - `pD&`
  - `X1qf`
  - `(A.)=EH4Y.`
  - `(A) =A`
  - `(ARE) =AA`
  - `(AR)O=AX`
  - `(AR)#=EH4`
  - `(A)WA=A`
  - `(AW)=AO`
  - `:(ANY)=EH4NI`
  - `#:(ALLY)=ULI`
  - `(AL)#=U`
  - `(AGAIN)=AXGEH4`
  - `#:(AG)E=IH`
  - `(ARR)=AX`
  - `(ARR)=AE4`
  - `(AR)=AA5`
  - `(AIR)=EH4`
  - `(AI)=EY`
  - `(AY)=EY`
  - `(AU)=AO`
  - `#:(AL) =U`
  - `#:(ALS) =UL`
  - `(ALK)=AO4`
  - `:(ABLE)=EY4BU`
  - `(ABLE)=AXBU`
  - `(A)VO=EY`
  - `(ANG)+=EY4N`
  - `(ATARI)=AHTAA4RI`
  - `(A)TOM=A`
  - `(A)TTI=A`
  - `(AT) =AE`
  - `(A)T=A`
  - `(A)=A`
  - `(B) =BIY`
  - `(BEING)=BIY4IHN`
  - `(BOTH) =BOW4T`
  - `(BUS)#=BIH4`
  - `(BREAK)=BREY5`
  - `(BUIL)=BIH4`
  - `(B)=`
  - `(C) =SIY`
  - `(CHA)R#=KEH`
  - `(CH)=C`
  - `S(CI)#=SAY`
  - `(CI)A=S`
  - `(CI)O=S`
  - `(CI)EN=S`
  - `(CITY)=SIHTI`
  - `(C)+=`
  - `(CK)=`
  - `(COMMODORE)=KAA4MAHDOH`
  - `(COM)=KAH`
  - `(CUIT)=KIH`
  - `(CREA)=KRIYE`
  - `(C)=`
  - `(D) =DIY`
  - `(DR.) =DAA4KTE`
  - `#:(DED) =DIH`
  - `.E(D) =`
  - `(DO) =DU`
  - `(DOES)=DAH`
  - `(DONE) =DAH5`
  - `(DOING)=DUW4IHN`
  - `(DOW)=DA`
  - `#(DU)A=JU`
  - `(D)=`
  - `(E) =IYIY`
  - `#:(E)`
  - `:(E) =I`
  - `#(ED) =`
  - `#:(E)D`
  - `(EV)ER=EH4`
  - `(ERI)#=IY4RI`
  - `(ERI)=EH4RI`
  - `#:(ER)#=E`
  - `(ERROR)=EH4ROH`
  - `(ERASE)=IHREY5`
  - `(ER)#=EH`
  - `(ER)=E`
  - `(EVEN)=IYVEH`
  - `#:(E)W`
  - `(EW)=YU`
  - `(E)O=I`
  - `#:&(ES) =IH`
  - `#:(E)S`
  - `#:(ELY) =LI`
  - `#:(EMENT)=MEHN`
  - `(EFUL)=FUH`
  - `(EE)=IY`
  - `(EARN)=ER5`
  - `(EAD)=EH`
  - `#:(EA) =IYA`
  - `(EA)SU=EH`
  - `(EA)=IY`
  - `(EIGH)=EY`
  - `(EI)=IY`
  - `(EYE)=AY`
  - `(EY)=I`
  - `(EU)=YUW`
  - `(EQUAL)=IY4KWU`
  - `(E)=E`
  - `(F) =EH4`
  - `(FUL)=FUH`
  - `(FRIEND)=FREH5N`
  - `(FATHER)=FAA4DHE`
  - `(F)F`
  - `(F)=`
  - `(G) =JIY`
  - `(GIV)=GIH5`
  - `(GE)T=GEH`
  - `SU(GGES)=GJEH4`
  - `(GG)=`
  - `B#(G)=`
  - `(G)+=`
  - `(GREAT)=GREY4`
  - `(GON)E=GAO5`
  - `#(GH)`
  - `(GN)=`
  - `(G)=`
  - `(H) =EY4C`
  - `(HAV)=/HAE6`
  - `(HERE)=/HIY`
  - `(HOUR)=AW5E`
  - `(HOW)=/HA`
  - `(H)#=/`
  - `(H)`
  - `(IN)=IH`
  - `(I) =AY`
  - `(I) =A`
  - `(IN)D=AY5`
  - `SEM(I)=I`
  - `ANT(I)=A`
  - `(IER)=IYE`
  - `#:R(IED) =IY`
  - `(IED) =AY5`
  - `(IEN)=IYEH`
  - `(IE)T=AY4E`
  - `(I')=AY`
  - `:(IE) =AY`
  - `(I)%=I`
  - `(IE)=IY`
  - `(IDEA)=AYDIY5A`
  - `(IR)#=AY`
  - `(IZ)%=AY`
  - `(IS)%=AY`
  - `(IR)=E`
  - `(IGH)=AY`
  - `(ILD)=AY5L`
  - `(IGN)=IHG`
  - `(IGN) =AY4`
  - `(IGN)%=AY4`
  - `(ICRO)=AY4KRO`
  - `(IQUE)=IY4`
  - `(I)=I`
  - `(J) =JEY`
  - `(J)=`
  - `(K) =KEY`
  - `(K)N`
  - `(K)=`
  - `(L) =EH4`
  - `(LO)C#=LO`
  - `L(L)`
  - `(LEAD)=LIY`
  - `(LAUGH)=LAE4`
  - `(L)=`
  - `(M) =EH4`
  - `(MR.) =MIH4STE`
  - `(MS.)=MIH5`
  - `(MRS.) =MIH4SIX`
  - `(MOV)=MUW4`
  - `(MACHIN)=MAHSHIY5`
  - `M(M)`
  - `(M)=`
  - `(N) =EH4`
  - `E(NG)+=N`
  - `(NG)R=NX`
  - `(NG)#=NX`
  - `(NGL)%=NXGU`
  - `(NG)=N`
  - `(NK)=NX`
  - `(NOW) =NAW`
  - `N(N)`
  - `(NON)E=NAH4`
  - `(N)=`
  - `(O) =OH4`
  - `(OF) =AH`
  - `(OH) =OW`
  - `(OROUGH)=ER4O`
  - `#:(OR) =E`
  - `#:(ORS) =ER`
  - `(OR)=AO`
  - `(ONE)=WAH`
  - `#(ONE) =WAH`
  - `(OW)=O`
  - `(OVER)=OW5VE`
  - `PR(O)V=UW`
  - `(OV)=AH4`
  - `(OL)D=OW4`
  - `(OUGHT)=AO5`
  - `(OUGH)=AH5`
  - `(OU)=A`
  - `H(OU)S#=AW`
  - `(OUS)=AX`
  - `(OUR)=OH`
  - `(OULD)=UH5`
  - `(OUP)=UW5`
  - `(OU)=A`
  - `(OY)=O`
  - `(OING)=OW4IHN`
  - `(OI)=OY`
  - `(OOR)=OH5`
  - `(OOK)=UH5`
  - `F(OOD)=UW5`
  - `L(OOD)=AH5`
  - `M(OOD)=UW5`
  - `(OOD)=UH5`
  - `F(OOT)=UH5`
  - `(OO)=UW`
  - `(O')=O`
  - `(O)E=O`
  - `(O) =O`
  - `(OA)=OW`
  - `(ONLY)=OW4NLI`
  - `(ONCE)=WAH4N`
  - `(ON'T)=OW4N`
  - `C(O)N=A`
  - `(O)NG=A`
  - `I(ON)=U`
  - `#:(ON)=U`
  - `(O)ST=O`
  - `(OTHER)=AH5DHE`
  - `R(O)B=RA`
  - `(OSS) =AO5`
  - `(O)=A`
  - `(P) =PIY`
  - `(PH)=`
  - `(PEOPL)=PIY5PU`
  - `(POW)=PAW`
  - `(PUT) =PUH`
  - `(P)P`
  - `(P)S`
  - `(P)N`
  - `(PROF.)=PROHFEH4SE`
  - `(P)=`
  - `(Q) =KYUW`
  - `(QUAR)=KWOH5`
  - `(QU)=K`
  - `(Q)=`
  - `(R) =AA5`
  - `(R)R`
  - `(R)=`
  - `(S) =EH4`
  - `(SH)=S`
  - `#(SION)=ZHU`
  - `(SOME)=SAH`
  - `#(SUR)#=ZHE`
  - `(SUR)#=SHE`
  - `#(SU)#=ZHU`
  - `#(SSU)#=SHU`
  - `#(SED)=Z`
  - `#(S)#=`
  - `(SAID)=SEH`
  - `(S)S`
  - `.(S) =`
  - `#:.E(S) =`
  - `U(S) =`
  - `:#(S) =`
  - `##(S) =`
  - `(SCH)=S`
  - `(S)C+`
  - `#(SM)=ZU`
  - `#(SN)'=ZU`
  - `(STLE)=SU`
  - `(S)=`
  - `(T) =TIY`
  - `(THE) #=DHI`
  - `(THE) =DHA`
  - `(TO) =TU`
  - `(THAT)=DHAE`
  - `(THIS) =DHIH`
  - `(THEY)=DHE`
  - `(THERE)=DHEH`
  - `(THER)=DHE`
  - `(THEIR)=DHEH`
  - `(THAN) =DHAE`
  - `(THEM) =DHAE`
  - `(THESE) =DHIY`
  - `(THEN)=DHEH`
  - `(THROUGH)=THRUW`
  - `(THOSE)=DHOH`
  - `(THOUGH) =DHO`
  - `(TODAY)=TUXDE`
  - `(TOMO)RROW=TUMAA`
  - `(TO)TAL=TOW`
  - `(THUS)=DHAH4`
  - `(TH)=T`
  - `#:(TED)=TIX`
  - `S(TI)#N=C`
  - `(TI)O=S`
  - `(TI)A=S`
  - `(TIEN)=SHU`
  - `(TUR)#=CHE`
  - `(TU)A=CHU`
  - `(TWO)=TU`
  - `&(T)EN`
  - `(T)=`
  - `(U) =YUW`
  - `(UN)I=YUW`
  - `(UN)=AH`
  - `(UPON)=AXPAO`
  - `(UR)#=YUH4`
  - `(UR)=E`
  - `(UY)=AY`
  - `G(U)#`
  - `G(U)%`
  - `G(U)#=`
  - `#N(U)=YU`
  - `(U)=YU`
  - `(V) =VIY`
  - `(VIEW)=VYUW`
  - `(V)=`
  - `(W) =DAH4BULYU`
  - `(WERE)=WE`
  - `(WA)SH=WA`
  - `(WA)ST=WE`
  - `(WA)S=WA`
  - `(WA)T=WA`
  - `(WHERE)=WHEH`
  - `(WHAT)=WHAH`
  - `(WHOL)=/HOW`
  - `(WHO)=/HU`
  - `(WH)=W`
  - `(WAR)#=WEH`
  - `(WAR)=WAO`
  - `(WR)=`
  - `(WOM)A=WUH`
  - `(WOM)E=WIH`
  - `(WEA)R=WE`
  - `(WANT)=WAA5N`
  - `ANS(WER)=E`
  - `(W)=`
  - `(X) =EH4K`
  - `(X)=`
  - `(X)=K`
  - `(Y) =WAY`
  - `(YOUNG)=YAHN`
  - `(YOUR)=YOH`
  - `(YOU)=YU`
  - `(YES)=YEH`
  - `(Y)=`
  - `F(Y)=A`
  - `PS(YCH)=AY`
  - `:(Y) =A`
  - `:(Y)#=A`
  - `(Y)=I`
  - `(Z) =ZIY`
  - `(Z)=`
  - `(A)`
  - `(") =-AH5NKWOWT`
  - `(")=KWOW4T`
  - `(#)= NAH4MBE`
  - `(%)= PERSEH4N`
  - `(&)= AEN`
  - `(*)= AE4STERIHS`
  - `(+)= PLAH4`
  - `(.)= POYN`
  - `(/)= SLAE4S`
  - `(0)= ZIY4RO`
  - `(1ST)=FER4S`
  - `(10TH)=TEH4NT`
  - `(1)= WAH4`
  - `(2ND)=SEH4KUN`
  - `(2)= TUW`
  - `(3RD)=THER4`
  - `(3)= THRIY`
  - `(4)= FOH4`
  - `(5TH)=FIH4FT`
  - `(5)= FAY4`
  - `(64) =SIH4KSTIY FOH`
  - `(6)= SIH4K`
  - `(7)= SEH4VU`
  - `(8TH)=EY4T`
  - `(8)= EY4`
  - `(9)= NAY4`
  - `(=)= IY4KWUL`
  - `&7N`
  - `6S.G`
  - `N8samspace3samE`
  - `TIC?(,`
  - `S.6V12SAMIAMWidget`
  - `C8SlimSlew`
  - `C8SOUUtils`
  - `N22SergeProgrammerControl7voltageE`
  - `&5C7SlimSeq`
  - `RJUM`
  - `D,G`
  - `C12PitchMangler`
  - `aCw`
  - `B14ScalaQuantizer`
  - `N20ScalaQuantizerWidget17NYOpenSCLFileItemE`
  - `N20ScalaQuantizerWidget15selectScaleItemE`
  - `A11ScaleOffset`
  - `!Dp`
  - `#)3P`
  - `N23simplertapecontrolmeter18SimplerTapeControlE`
  - `N23simplertapecontrolmeter24SimplerTapeControlWidgetE`
  - `N23simplertapecontrolmeter24tapecontrolflangiashadowE`
  - `N23simplertapecontrolmeter18tapecontrolflangiaE`
  - `pA+`
  - `L=33s?`
  - `!Dp`
  - `Y?fff?33s?`
  - `!Dp`
  - `Y?fff?33s?`
  - `C20SlopeDetectorControl`
  - `!Dp`
  - `?UU`
  - `?UU`
  - `?UU`
  - `?UU`
  - `?UU`
  - `N13SpectreWidget12wfSetterItemE`
  - `N13SpectreWidget23spectralColorSetterItemE`
  - `D,G`
  - `A14StrummerModule`
  - `C18SurveillanceModule`
  - `!DpFF`
  - `C5TIMEX`
  - `CN9tzmxmixer4TZMXE`
  - `N9tzmxmixer7levelDBE`
  - `N9tzmxmixer10TZMXWidgetE`
  - `/Dn`
  - `?Gc`
  - `=N9tzvumeter4TZVUE`
  - `N9tzvumeter10TZVUWidgetE`
  - `C17TwistedVerbModule`
  - `C9WFSpectre`
  - `N15WFSpectreWidget12wfSetterItemE`
  - `N15NY2MetersWidget8modeItemE`
  - `N14NY1MeterWidget8modeItemE`
  - `N22PolyVoltageMeterWidget8modeItemE`
  - `N15SlimMeterWidget8modeItemE`
  - `C6YUMMER`
  - `=Nb`
  - `KW]`
  - `?18XattoDelayerWidget`
  - `A333?`
  - `A333?`
  - `6 13b208_envelope`
  - `!Dp`
  - `N20complexSimplerWidget17NYOpenSaveWavItemE`
  - `CgfVB`
  - `C43`
  - `QCgfTB`
  - `!Dp`
  - `!)19A15confusedSimpler`
  - `,Bff`
  - `,Bff`
  - `QTWZ`
  - `d~E`
  - `xEqArEq`
  - `gEq`
  - `J[EqaPE`
  - `D~E`
  - `xEq`
  - `rEqAjE=z`
  - `=zQ`
  - `KEqaAE&p`
  - `D,G`
  - `-DT`
  - `-DT`
  - `A0"`
  - `M.!`
  - `O!W`
  - `~xL`
  - `LM'~1`
  - `F~X`
  - `R3Z`
  - `WGD`
  - `s,J`
  - `r%Lb`
  - `tjx`
  - `fEy`
  - `zAU`
  - `vGk`
  - `Nyt#`
  - `O7P`
  - `bHN&n`
  - `Ex W`
  - `rWe`
  - `.[FE`
  - `C5%Y`
  - `ZhB`
  - `UG3`
  - `Gdi`
  - `hKgq`
  - `JEc`
  - `E-(g`
  - `#V5`
  - `kH7`
  - `Q&Z`
  - `T4V`
  - `s?L`
  - `bWI`
  - `j6E6F`
  - `Bql`
  - `F0D`
  - `JyR`
  - `U i&`
  - `~eJ`
  - `Ve:`
  - `:Z0`
  - `I"E.`
  - `+IC2`
  - `DkE`
  - `F&;`
  - `.W?6`
  - `]xoV`
  - `bV9`
  - `P~A`
  - `Oxd`
  - `XtF`
  - `oJy`
  - `fF?`
  - `Zv-`
  - `YI+`
  - `mDA:`
  - `nLOX`
  - `nyv`
  - `Ti0`
  - `(0S`
  - `R.CR`
  - `)]UQ`
  - `W:D`
  - `,THC`
  - `lEQVB`
  - `c&Q/`
  - `FM*`
  - `=yX(`
  - `ncXjb&`
  - `/Ie`
  - `P-l`
  - `KqU`
  - `F[:`
  - `rM&`
  - `lK.`
  - `tpU~`
  - `NQ?`
  - `sWe`
  - `/.Ui`
  - `yHBlz`
  - `FJi`
  - `'G&`
  - `'eRC`
  - `!6X`
  - `pzr`
  - `zUg`
  - `z*L`
  - `%Tpx`
  - `I.Z`
  - `)lPE`
  - `JQx`
  - `bSB`
  - `;C':`
  - `IEc`
  - `Z8Z`
  - `RAE`
  - `]+F`
  - `WM!`
  - `~Y5c`
  - `)Dv`
  - `Bk4o`
  - `IyR`
  - `UfJ`
  - `K"V`
  - `cC7`
  - `LI o`
  - `TrN`
  - `K+t`
  - `uih`
  - `X32`
  - `UY%`
  - `cqI`
  - `P29`
  - `UC,`
  - `/LKQ`
  - `T3k`
  - `Zv-`
  - `eLfa`
  - `orl`
  - `%v+S`
  - `?(P`
  - `gpB`
  - `2J~`
  - `?nT`
  - `yEd`
  - `?"DC`
  - `?Rp`
  - `?"R`
  - `Iw!`
  - `%mO5`
  - `?]V`
  - `= u`
  - `?ADj`
  - `U2[`
  - `M9p`
  - `XaD`
  - `)~B,`
  - `?0WPb!`
  - `?Pb!`
  - `?Y9`
  - `eiL-`
  - `?=qJ`
  - `?NI1`
  - `?Z]`
  - `?fZ`
  - `nJy`
  - `?5YHe/`
  - `?aJ`
  - `Wd*`
  - `Kd0`
  - `?K*`
  - `~Hd`
  - `= u`
  - `?F"4`
  - `?P1s&`
  - `?,X`
  - `:C'`
  - `?=LV`
  - `?lzPP`
  - `lscz`
  - `?w'Deh`
  - `?k:LEt`
  - `? W4`
  - `??B`
  - `gxX`
  - `kFa`
  - `?.T`
  - `?Y*`
  - `?!yB`
  - `?%iH&`
  - `?!gP#`
  - `?[DoL`
  - `?uJ`
  - `?Na`
  - `-NF`
  - `oYO`
  - `!!%Q`
  - `?Mg'`
  - `?4B`
  - `?M%M`
  - `?Ju`
  - `U6b`
  - `?Z=qJ`
  - `?24COo`
  - `?GER`
  - `?SB`
  - `Q50`
  - `CJX`
  - `?*A5`
  - `B1C`
  - `h"G`
  - `?VQr=`
  - `?N(D`
  - `?jH`
  - `*Zo`
  - `?X%G`
  - `? W4`
  - `?s 57`
  - `?Z"a)`
  - `?rGIY`
  - `?,E`
  - `?Fe`
  - `?I=U`
  - `?P)e`
  - `?V3!`
  - `Ghp`
  - `?gG`
  - `bV9`
  - `hUK:`
  - `?"nN%`
  - `?Gey`
  - `?hD`
  - `(cW`
  - `uNX`
  - `Lsa`
  - `?COo`
  - `L/1`
  - `?Od`
  - `Nbk`
  - `Zjs`
  - `M~0`
  - `?Pb!`
  - `Slq`
  - `ClU`
  - `=Ab`
  - `?[D`
  - `?Y[g2`
  - `?P#`
  - `?Yj`
  - `.5B?`
  - `2B.'`
  - `?tPx`
  - `(Kc`
  - `'X-`
  - `?T0`
  - `! _B`
  - `?As`
  - `?R:`
  - `lK.`
  - `?UJt;`
  - `?A)Z`
  - `?lW`
  - `?IO`
  - `Nyt#`
  - `GX/`
  - `?#5H`
  - `?6Dp`
  - `ss;E`
  - `?BY`
  - `?MeQ`
  - `?xL`
  - `?%C`
  - `?N`
  - `l ]`
  - `qpi`
  - `?XQ(`
  - `Pzp`
  - `TJt`
  - `?jRes0v`
  - `?Z]`
  - `'Ls`
  - `?t4X`
  - `pN6/`
  - `?H(`
  - `?T=`
  - `?Lr`
  - `M~0`
  - `RRq-`
  - `?YX&`
  - `Ws%`
  - `?C?`
  - `?Wx`
  - `?/Q`
  - `I"E`
  - `?Tk`
  - `?Ku`
  - `?Tb`
  - `?6k:LE`
  - `?eF`
  - `?,J`
  - `?gC`
  - `?2J`
  - `?zO`
  - `Wy]`
  - `?C#3rq/`
  - `Ed3`
  - `HPX`
  - `yFNV`
  - `H;V`
  - `4F;`
  - `=Ug`
  - `.=U`
  - `u?B`
  - `v?E+n?n`
  - `'6BI`
  - `.DX`
  - `Aff`
  - `Aff`
  - `Aff`
  - `Aff`
  - `Aff`
  - `A33`
  - `~?tG:`
  - `4+!N`
  - `+!N`
  - `4+!N`
  - `+!N`
  - `oD?`
  - `tG:`
  - `e1DJ`
  - `ER-`
  - `GhL`
  - `/7DqV;Fe`
  - `P"B`
  - `Ct)`
  - `Zq?x`
  - `YJF?`
  - `?7J`
  - `Cth`
  - `[z&B`
  - `qCX`
  - `A(RF`
  - `C/fyE`
  - `zXC`
  - `6?P`
  - `FEI`
  - `B#f`
  - `!CQ`
  - `C3P`
  - `BjB`
  - `KFB`
  - `DF7`
  - `Cpf`
  - `#fB`
  - `%FD`
  - `qhDE36D`
  - `A"?`
  - `RRRRRRRRR!RRRRRRRRR`
  - `RRRRRRRRRfRRRRRRRRR`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `C33s?`
  - `Qx?`
  - `Qx?ff`
  - `Aff&`
  - `Aff`
  - `Bff`
  - `Qx?`
  - `Bfff`
  - `Cff`
  - `C333`
  - `?6YK?`
  - `:B33MB`
  - `fB33`
  - `B33`
  - `B33`
  - `AAfffA`
  - `pA33`
  - `A33`
  - `B33;B`
  - `Bff`
  - `B33`
  - `LIGA`
  - `UUUUUU`
  - `UUUUUU`
  - `V?;`
  - `I?I`
  - `A/b`
  - `A/b`
  - `pGr`
  - `u?G`
  - `?wT`
  - `?Y7`
  - `?W3`
  - `Lx`
  - `vector`
  - `configParam`
  - `Module.hpp`
  - `createModuleWidget`

## 208 5 steps sequencer

- slug `b208_5steps`, 9 HP, tags: Dual, Sequencer
- Imitation of the Buchla 208 5 steps sequencer from the Buchla Easel
- panel: b208_5steps.svg

No live text on the panel — outlined, or unlabelled. Read `panels/b208_5steps.png` to see it.

Strings in its code:

  - `BLACK and WHITE`

## 208 pulser

- slug `b208_pulser`, 4 HP, tags: Dual, Clock
- Imitation of the Buchla 208 pulser from the Buchla Easel
- panel: b208_pulser.svg

No live text on the panel — outlined, or unlabelled. Read `panels/b208_pulser.png` to see it.

Strings in its code:

  - `Restart from ZERO on Retrig`

## 208 envelope

- slug `b208_envelope`, 6 HP, tags: Dual, LFO, Envelope generator
- Imitation of the Buchla 208 envelope from the Buchla Easel
- panel: b208_envelope.svg

No live text on the panel — outlined, or unlabelled. Read `panels/b208_envelope.png` to see it.

Strings in its code:

  - `LPG A`
  - `Vactrol FAST`
  - `Vactrol NORMAL`
  - `Vactrol SLOW`
  - `Non linear mode`
  - `LPG B`
  - `LPG C`
  - `LPG D`

## 208 random + inverter

- slug `b208_random`, 2 HP, tags: Quad, Random, Noise
- Imitation of the Buchla 208 noise generators from the Buchla Easel
- panel: b208_random.svg

No live text on the panel — outlined, or unlabelled. Read `panels/b208_random.png` to see it.

## 208 LPG

- slug `b208_dualLPG`, 9 HP, tags: Dual, VCA, VCF, Lowpass gate
- Imitation of the Buchla 208 dual LPG from the Buchla Easel
- panel: b208_dualLPG.svg

No live text on the panel — outlined, or unlabelled. Read `panels/b208_dualLPG.png` to see it.

## QuadSimpler

- slug `QuadSimpler`, 8 HP, tags: Quad, Sampler, VCO, Drum
- Quad Sample player with mixer
- panel: QuadSimpler.svg

No live text on the panel — outlined, or unlabelled. Read `panels/QuadSimpler.png` to see it.

Strings in its code:

  - `HIDE History Trail`
  - `History Fades exponentially`
  - `Commands`
  - `If No input use 10V as input`
  - `QuadSimpler`

From the changelog:

  * changed strategy for the LAST sample in table (should remove the click for samples starting with DC offsets)
  * removed the use of AUTOMATIC cache
    *  loaded files are always load from their current position (if they still exist)
  * simd speed up, poly mode, support for 10v, 5v and 1v subdivision
  * added rounding modes (ceil, floor, round)
  * support for negative voltages
  *	bug: start was delayed because the anticlick was not inited

## ClockMultiplier

- slug `ClockMultiplier`, 7 HP, tags: Quad, Clock modulator
- Clock Multiplier-Divider
- panel: ClockMultiplier.svg

No live text on the panel — outlined, or unlabelled. Read `panels/ClockMultiplier.png` to see it.

Strings in its code:

  - `ClockMultiplier`

From the changelog:

  *	FEATURE REQUEST: add voltage outputs for the divided/multiplied clocks (#322)

## Bitshifter BOH!NGLER

- slug `Bitshifter`, 7 HP, tags: Noise, S&H, Utility
- 256 bits bitshifter with S&H and noise and inner LFO and VCO
- panel: Bitshifter.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `BITSHIFTER` at 171, 20
  - `CLOCK` at 196, 30
  - `CLK IN` at 156, 41
  - `OCTAVE` at -114, 48
  - `RAEL` at -164, 66
  - `BITSHIFTER` at -164, 108
  - `USE RND` at 156, 111
  - `USE VCO` at 211, 111
  - `BITSHIFT` at 156, 130
  - `INNER VCO` at 208, 130
  - `XOR` at 156, 187
  - `EXT IN` at 215, 187
  - `OCTAVE` at -178, 262
  - `OCTAVE` at -178, 262
  - `SCALE-OFFSET` at 156, 365
  - `UNIPOLAR` at 208, 365

Strings in its code:

  - `Duration:`
  - `seconds`
  - `ENVELOPE TIME RANGES`
  - `From 0.001 seconds to 0.100 seconds`
  - `From 0.100 seconds to 1 second`
  - `From 1 second to 10 seconds`
  - `From 10 seconds to 60 seconds`
  - `Bitshifter`
  - `mean`
  - `bitset to_ulong overflow error`
  - `Scale control for CV Frequency`
  - `Use random generator`
  - `Use VCO generator`
  - `Scale control CV Frequency for inner VCO generator`
  - `XOR mode ON-OFF`
  - `Scale outputs`
  - `Offset outputs`
  - `Unipolar mode ON-OFF`
  - `External clock`
  - `Inner clock CV control`
  - `Pulse in to switch between RND or VCO generators`
  - `External signal (overrides inner generators)`
  - `Inner VCO generator CV control`
  - `Clock`
  - `Clock Square`
  - `Clock ramp up`
  - `Clock ramp down`
  - `Inner VCO`
  - `Column`
  - `Voltage`
  - `configInput`
  - `configOutput`

## GraphicMeter

- slug `GraphicMeter`, 13 HP, tags: Utility, Polyphonic
- Voltage Visualizer-Voltmeter
- panel: GraphicMeter.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `LOG VALUE 8` at -78, 400

Strings in its code:

  - `Sol`
  - `GraphicMeter`

## MultiVoltimetro

- slug `MultiVoltimetro`, 13 HP, tags: Utility, Polyphonic
- Voltage Visualizer
- panel: MultiVoltimetro.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `LOG VALUE 8` at -80, 400

Strings in its code:

  - `MultiVoltimetro`

From the changelog:

  * add VOLTAGES SORT for polyphonic incoming voltages (ascending - descending)
  * add poly voltage merging
  * poly voltage are positional and overridable (if you insert a 4 chan in position position 10
  channel 10 11 12 and 13 will be used

## FixedVoltageSource

- slug `FixedVoltageSource`, 12 HP, tags: Utility, Polyphonic
- Fixed Voltage Source with GATE and SUM
- panel: FixedVoltageSource.svg

No live text on the panel — outlined, or unlabelled. Read `panels/FixedVoltageSource.png` to see it.

Strings in its code:

  - `Restart Timer on open`
  - `FixedVoltageSource`
  - `SlimFixedVoltageSource`
  - `SlimDualFixedVoltageSource`
  - `Label`
  - `LabelSlim`

## SlimFixedVoltageSource

- slug `SlimFixedVoltageSource`, tags: Utility
- Single Fixed Voltage Source with GATE
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## SlimDualFixedVoltageSource

- slug `SlimDualFixedVoltageSource`, tags: Utility
- Dual Fixed Voltage Source with GATE
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## Label

- slug `Label`, tags: Utility
- Vertical Label Utility
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

From the changelog:

  * a small LABEL (by request)
  * a single slim editable inline LABEL, multiline

## LabelSlim

- slug `LabelSlim`, 1 HP, tags: Utility
- Vertical Label Utility 1 unit
- panel: VerticalLabelSlim.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `V2` at 23, 18

Strings in its code:

  - `Label multiline`

From the changelog:

  * a small LABEL (by request)

## TUNATHOR

- slug `TUNATHOR`, 10 HP, tags: Utility, Tuner
- hardware calibration tool for external CV devices
- panel: TUNATHOR.svg

No live text on the panel — outlined, or unlabelled. Read `panels/TUNATHOR.png` to see it.

Strings in its code:

  - `TUNATHOR`

From the changelog:

  *	[FEATURE REQ] add tuning ranges via contextual menu

## GranTunismo

- slug `GranTunismo`, 30 HP, tags: Utility, Tuner
- Chromatic TUNER
- panel: GranTunismo.svg

No live text on the panel — outlined, or unlabelled. Read `panels/GranTunismo.png` to see it.

From the changelog:

  * 	feature request: added midi note + cents deviation

## Pitch2Voltage

- slug `PitchVoltager`, 6 HP, tags: Utility, Tuner
- Pitch to voltage device
- panel: PitchVoltager.svg

No live text on the panel — outlined, or unlabelled. Read `panels/PitchVoltager.png` to see it.

Strings in its code:

  - `Phasor settings`
  - `Full Reset`
  - `Set Harmonics ratio to standard tuning`
  - `Set Harmonics ratio to square of position`
  - `Set Harmonics ratio to cube of position`
  - `Set Harmonics ratio to square root of position`
  - `Set Harmonics ratio to Hammond B3 wheels`
  - `Low Power Mode`
  - `Frequency Trimmers as FM inputs`
  - `Zoom Reset`
  - `Zoom IN`
  - `Zoom OUT`
  - `Show Phasor`
  - `Show Path`
  - `Show Wave`
  - `Reset to SAW`
  - `Reset to SQUARE`
  - `Reset to TRIANGLE`
  - `Load Current Frame To Screen`
  - `Save Screen To Current Frame`
  - `Add 100 Random KFs`
  - `Add 10 Random KFs`
  - `Add 1 Random KF`
  - `Randomize All Magnitudes`
  - `Randomize All Phases`
  - `Randomize All Frequency Trimmers`
  - `Randomize All Frequency Sign`
  - `Zero All Magnitudes`
  - `Zero All Phases`
  - `Zero All Frequency Trimmers`
  - `Reset All Frequency Sign`
  - `Max AMP Harmonic`
  - `Half AMP Harmonic`
  - `Zero AMP Harmonic`
  - `Invert Frequency Sign`
  - `Phase Harmonic`
  - `Phase Harmonic`
  - `Phase Harmonic`
  - `Phase Harmonic`
  - `PitchVoltager`

## ExpiredTime

- slug `ExpiredTime`, 56 HP, tags: Utility, Timer
- Timer Utility to count time an create temporal events
- panel: ExpiredTime.svg

No live text on the panel — outlined, or unlabelled. Read `panels/ExpiredTime.png` to see it.

Strings in its code:

  - `INDEX`
  - `MIDINOTE MAP Voltage Offset:`
  - `USE Old Quantizing Mode`
  - `Voltage Offset`
  - `+0.0V`
  - `+1.0V`
  - `+2.0V`
  - `+3.0V`
  - `+4.0V`
  - `+5.0V`
  - `ExpiredTime`

## ELSKER

- slug `MultiTriggerDelayer`, 21 HP, tags: Utility, Delay
- Multi TRIG/GATE with delay and settable lenght, clockable
- panel: MultiTriggerDelayer.svg

No live text on the panel — outlined, or unlabelled. Read `panels/MultiTriggerDelayer.png` to see it.

Strings in its code:

  - `MultiTriggerDelayer`

From the changelog:

  * optimizations

## NYECHOEcoeco

- slug `NYECHOEcoeco`, 18 HP, tags: Effect, Delay
- Analog delay based on magnetic disk
- panel: NYECHOEcoeco.svg

No live text on the panel — outlined, or unlabelled. Read `panels/NYECHOEcoeco.png` to see it.

Strings in its code:

  - `NYECHOEcoeco`

## JIRAJIRAECHO

- slug `JIRAJIRAECHO`, 18 HP, tags: Effect, Delay
- Analog delay based on magnetic disk, Binson Echorec imitation
- panel: JIRAJIRAECHO2.svg

No live text on the panel — outlined, or unlabelled. Read `panels/JIRAJIRAECHO2.png` to see it.

Strings in its code:

  - `JIRAJIRAECHO`
  - `Frequency`

From the changelog:

  * add 10x time multiplier (via contextual menu) (current max delay 20 minutes, WOW!)
  * removed the soft clipper form input (too much harmonic distorsion)
  * BEWARE repetitions + SWELL must be well controlled! (otherwise huge levels!) (clamp to -20 +20)
  * the Panic button now acts on the 2 dc blocker (left right) too

## MicrotonalHostHelper

- slug `MicrotonalHostHelper`, 6 HP, tags: Utility, Microtonal, Polyphonic
- Helper to bring microtonality to VST Host
- panel: MicrotonalHostHelper.svg

No live text on the panel — outlined, or unlabelled. Read `panels/MicrotonalHostHelper.png` to see it.

Strings in its code:

  - `PCM (.pcm):pcm`
  - `MP3 (.mp3):mp3`
  - `SAVE FORMAT`
  - `SAVE`
  - `channels WAVE files`
  - `channels RAW PCM 32bit IEEE FLOAT files`
  - `SAVE stereo MP3 files`
  - `ENCODING`
  - `PCM 32bit IEEE FLOAT`
  - `Bit rate`
  - `VU Meters setting`
  - `VU Meters disabled`
  - `RMS timing`
  - `RMS time 0.1ms`
  - `RMS time 1ms`
  - `RMS time 10ms`
  - `RMS time 100ms`
  - `RMS time 500ms`
  - `PEAK HOLD timing`
  - `PEAK HOLD time 50ms`
  - `PEAK HOLD time 100ms`
  - `PEAK HOLD time 250ms`
  - `PEAK HOLD time 500ms`
  - `PEAK HOLD time 1000ms`
  - `PEAK HOLD time 2000ms`
  - `PEAK HOLD time 5000ms`
  - `%d kbps`
  - `MicrotonalHostHelper`

## Polyphonic LPG

- slug `PolyLPG`, 4 HP, tags: VCF, VCA, Polyphonic, Lowpass gate
- Polyphonic Buchla 208 LPG
- panel: polylpg_bg.svg

No live text on the panel — outlined, or unlabelled. Read `panels/polylpg_bg.png` to see it.

Strings in its code:

  - `PolyLPG`

## Simpliciter

- slug `Simpliciter`, tags: Recording, Sampler, VCO
- Sample oscillator also known as confusingSimpler
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

From the changelog:

  * add REC GATE MODE (from contextual menu)
  * add LONG PRESS (3 secs) delete sample MODE, if in TRIG REC MODE
  * add a menu command "DRAG & DROP files are appended".
    If the flag is OFF incoming sample will substitute the current one. If it's ON samples will be appended
  * added 2nd mode to draw the waves
  * add the use of the SIMPLER TAPE CONTROL as expander
  * debug visuals when recording CV in PRE-RECORD MODE
  * moved the INTERPOL(ation) flag on the "Output" side (it's the correct place)
  * a multi track (from 1 to 16 tracks) sample player/recorder
  * (recording you are on your own! depends on your memory)
  * can do prerecord mode (til 120 secs) or standard record mode, Direct to Disk
  * Playing is always (like in all simpliciter/sampler) using memory
  * can record up to 16 tracks of CV data and save in a WAV file at 100hz (the feature is activated using the "CV" button in INPUT zone
  *  when importing via Drag&Drop (and only via D&D) the imported files will be always in append mode
  *  when importing via Drag&Drop(and only via D&D) automatically a slice will be generated for new appended section
  * starts the crossfade time between REC and PLAY to ZERO
  * implement the pause RECORD: pressing on the RED button REC, with the COMMANDKEY down, the REC timer will be stopped and RECORDING will stay in pause until a new TAP (or PULSE) on the REC button for a CONTINUE
  * debug a crash in headless mode
  * add a new PLAY MODE: PING PONG MODE (via contextual menu)
  * bug: if in "output 0V if stopped" mode, transition between loop - no-loop - loop will mute (corrected)
  * debug missing PEAK menus !
  * debug a crash deleting all SLICEs whne playing using slices
  * add menu for equal division of current selection (new slices will be added to the slice sequencer)
  *  add a RND on EOC mode for the SLICE sequencer, is valid if GRID MODE is OFF
  *  add: if in GRID mode the RANDOM select input in the slice area will select a RANDOM GRID selection
  *  debug: if in GRID mode other action from SLICE area are disabled
  *  DEBUG: "STOP recording with append" was inverting channels (#338)
  *  FEATURE REQ: press REC with SHIFT to activate automatic APPEND for current REC
  *  add a RND on EOC mode for the SLICE sequencer, is valid if GRID MODE is OFF
  *  add: if in GRID mode the RANDOM select input in the slice area will select a RANDOM GRID selection
  *  debug: if in GRID mode other action from SLICE area are disabled
  *  DEBUG: "STOP recording with append" was inverting channels (#338)
  *  FEATURE REQ: press REC with SHIFT to activate automatic APPEND for current REC
  *	bug: missing SYNC for the internal LFO
  *	bug: START and LENGHT mode was not correctly reloaded (#285)
  *  	add a led to visualize the CV out of the env follower
  *	feature request: add RAMP OUT representing the position of the playhead
  	can be relative to slice or relative to the full sample (#325)
  *	bug: crash caused by peak slice mode (issue #309)
  *	add some new EDIT contextual menu

## Simpliciter Multi

- slug `Simpliciter2`, tags: Recording, Sampler, VCO
- the Simpliciter MultiTrack version, from 1 to 16 tracks
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

From the changelog:

  * add a menu command "DRAG & DROP files are appended".
    If the flag is OFF incoming sample will substitute the current one. If it's ON samples will be appended
  * added 2nd mode to draw the waves
  * add the use of the SIMPLER TAPE CONTROL as expander
  * debug visuals when recording CV in PRE-RECORD MODE
  * moved the INTERPOL(ation) flag on the "Output" side (it's the correct place)
  * a multi track (from 1 to 16 tracks) sample player/recorder
  * (recording you are on your own! depends on your memory)
  * can do prerecord mode (til 120 secs) or standard record mode, Direct to Disk
  * Playing is always (like in all simpliciter/sampler) using memory
  * can record up to 16 tracks of CV data and save in a WAV file at 100hz (the feature is activated using the "CV" button in INPUT zone

## Pepper

- slug `Pepper`, 18 HP, tags: Utility
- Notes module to be used with Jooper
- panel: Pepper.svg

No live text on the panel — outlined, or unlabelled. Read `panels/Pepper.png` to see it.

Strings in its code:

  - `Reset to C4`
  - `Reset to C3`
  - `Reset to C2`
  - `Reset to C1`
  - `Detach Tuning from Left Expander`
  - `OP status:`
  - `OCTAVE`
  - `TUNING`
  - `BASE FREQUENCY`
  - `FREQUENCY after OCTAVE-TUNING-EXPFM`
  - `EXP FM VCA`
  - `FM RATIO`
  - `LIN FM MOD INDEX`
  - `FEEDBACK`
  - `OP LEGENDA`
  - `OCTAVE from -8 to +8`
  - `TUNING from -1 to +1`
  - `SYNC IN to reset phase with an incoming PULSE`
  - `TIME IN to drive base freq VCO with another VCO`
  - `CV IN pitch`
  - `CV OUT pitch (the cv is copied out to chain MODulators)`
  - `EXP FM IN VCA`
  - `EXP FM IN`
  - `LIN FM IN CV for VCA`
  - `LIN FM IN VCA`
  - `LIN FM IN 1`
  - `LIN FM IN 2`
  - `LIN FM IN 3`
  - `FEEDBACK from 0 to 4`
  - `FM RATIO`
  - `OUTPUT`
  - `Pepper`

From the changelog:

  * crash opening v1 files

## TheCage

- slug `TheCage`, 22 HP, tags: Quantizer, Switch, Sequencer, Bridge, Polyphonic, Utility
- Keyboard, Switch, Bridge, Sequencer, Quantizer with editable comparators
- panel: TheCage.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `1` at 475, 80
  - `2` at 473, 102
  - `3` at 473, 124
  - `4` at 472, 146
  - `5` at 472, 168
  - `6` at 472, 190
  - `7` at 473, 212
  - `the selecters` at 361, 227
  - `the selecters` at 361, 227
  - `8` at 472, 234
  - `9` at 472, 256
  - `10` at 470, 278
  - `11` at 472, 300
  - `12` at 470, 322
  - `Bridge` at 342, 338
  - `MOD` at 253, 440
  - `The grid` at 144, 448

Strings in its code:

  - `Reset Base Phase for all voices`
  - `Randomize Base Phase for voices 10%`
  - `Randomize Base Phase for voices 20%`
  - `Use Correlated Feedback`
  - `Detach is a FULL Detach from CARRIER/MODULATOR`
  - `TZVU`
  - `10 volts mode trigger`
  - `sensibility`
  - `msecs`
  - `Show Big Value in Meters`
  - `Remove Glass from Meters`
  - `Draw needle in white`
  - `Forward`
  - `Backward`
  - `Random`
  - `TheCage`

From the changelog:

  * solved the case of missing saved scales :D (always same problem of inverted startup v2 sequence)
  * not saving data in fields, in v2

## Sussudio

- slug `Sussudio`, 48 HP, tags: Sampler, Quad, VCO
- Multi head sample based oscillator
- panel: Sussudio.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `prev` at 720, 390
  - `next` at 691, 391
  - `NYSTHI & DISSOCIATES - SUSSUDIO` at 442, 402
  - `NYSTHI & DISSOCIATES - SUSSUDIO` at 242, 416
  - `SPEED` at 95, 427
  - `CUE IN` at 229, 433
  - `CUE IN` at 466, 436

Strings in its code:

  - `Sussudio`
  - `CV-IN VCA`
  - `Add Keyframe Trigger`
  - `Load Keyframe Trigger`
  - `Update Keyframe Trigger`
  - `Delete Keyframe Trigger`
  - `Delete All Keyframes Trigger`
  - `Next Keyframe Trigger`
  - `Prev Keyframe Trigger`
  - `Go Keyframe 1 Trigger`
  - `Select Keyframe CV`
  - `Select Keyframe Random Trigger`

From the changelog:

  *	add contextual menu command to output 0V if sample is stopped/paused
  *	add contextual menu command to output 0V if sample is stopped/paused
  *	FEATURE REQUEST: add contextual menu to avoid reset of playhead on keyframe change (#315)
  *	added drag playhead pressing ALT left (hard move of playhead with granular play)
  * 	added drag playhead pressing ALT right (smooth move of playhead with granular play)
  *  added drag playhead pressing SHIFT LEFT (scratch move of playhead, with pitch accelerations)
  *  added drag playhead pressing SHIFT RIGHT (scratch move of ALL the playheads, with pitch accelerations)
  *  Fade In-Out smoothers, max 2000 samples, controlled by anticlick knob
  *  Fade In can be ON or OFF, with linear or exp functions
  *  Fade Out can be ON or OFF, with linear or exp functions
  *	added XFADE between start and Reversed samples at the end, max 1 sec or 44100 samples
  * XFADE can be ON or OFF, it's controlled by the anticlick knob

## LFOMultiPhase2 

- slug `LFOMultiPhase2`, 7 HP, tags: LFO, VCO
- LFO with 6 fixed phases and 1 variable, phases 0 60 120 180 240 300, with precise frequency setting
- panel: LFOMultiPhase2.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `CV-VCA-FREE-PHASE` at -70, 314

Strings in its code:

  - `bpm`
  - `bpm`
  - `qpm`
  - `qpm`
  - `epm`
  - `epm`
  - `spm`
  - `spm`
  - `Invalid argument:`
  - `USE -10v to 10V BIPOLAR mode`
  - `Set Output 1 to Poly 8`
  - `LFOMultiPhase2`

## MusicalBox

- slug `MusicalBox`, 36 HP, tags: Sampler, Quad, VCO
- Multi sampler with 8 oscillators
- panel: MusicalBox.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `SPEED` at 257, 400

Strings in its code:

  - `PLAY MODE`
  - `BUSY mode`
  - `DISPLAY`
  - `SLICE SEQUENCER`
  - `DELETE ALL Slices (option key down!)`
  - `SELECTION`
  - `PLAYHEAD`
  - `SELECT ALL`
  - `Import Audio File`
  - `Open WAV or AIFF File...`
  - `CURRENT FILES`
  - `something happenend parsing dir`
  - `FILE:`
  - `MusicalBox`
  - `Active Mode`
  - `Gated Mode`
  - `Start Trigger`
  - `Stop Trigger`
  - `Start Slice Jog`
  - `End Slice Jog`
  - `Speed`
  - `Speed CV VCA`
  - `Octave`
  - `CV-IN Volume VCA`
  - `Pan`
  - `Next Sample Trigger`
  - `Prev Sample Trigger`
  - `Vertical Scale Wave View`
  - `Transient Smoother`
  - `Global Speed CV-IN VCA`
  - `Global Speed`
  - `Global Volume`

From the changelog:

  * bug: when re-initialized, forget about connected files
  * change anticlick strategy
  *
  * removed offscreens (maybe offending in VST ?)
  * some tweaking about the tails in samplers
  * add exclusive play mode (activable using contextual menu). When ON, only one sampler at time can play
  *	add contextual menu command to output 0V if sample is stopped/paused
  *	add contextual menu command to output 0V if sample is stopped/paused

## GateTrigMerger

- slug `GateTrigMerger`, 6 HP, tags: Gate, Quad, LOGIC, ADSR, Utility
- GATE or TRIG mixer/merger
- panel: GateTrigMerger.svg

No live text on the panel — outlined, or unlabelled. Read `panels/GateTrigMerger.png` to see it.

Strings in its code:

  - `GateTrigMerger`

## ETCHASKETCHOSCOPE

- slug `XYdisplay`, 26 HP, tags: Visual, Utility
- XY visualizer
- panel: XYdisplay.svg

No live text on the panel — outlined, or unlabelled. Read `panels/XYdisplay.png` to see it.

Strings in its code:

  - `XYdisplay`
  - `X Scale`
  - `X Offset`
  - `Y Scale`
  - `Y Offset`
  - `Fade time`
  - `Seconds`
  - `Beam focus`
  - `Pixels`
  - `X SCALE VCA`
  - `X OFFSET VCA`
  - `Y SCALE VCA`
  - `Y OFFSET VCA`
  - `Beam focus VCA`
  - `Max drawable vector length`
  - `pixels`

From the changelog:

  * bug: wrong max drawable behaviour
  * add history to Beam Size CV in (so can act as Z channel)
  *  feature request: add filter for "Max drawable vector length"

## Scale and Offset

- slug `ScaleOffset`, 8 HP, tags: VCA, Utility, Polyphonic
- Scale and offset voltages
- panel: ScaleOffset.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `nysthiLOOPER` at 280, -40
  - `BAR out` at 360, -0
  - `SIGNATURE` at 320, 40

Strings in its code:

  - `sample`
  - `Reset Root to C4`
  - `Midi Map offsets`
  - `SCALA FILES`
  - `Add New SCL File To The Library...`
  - `SELECTED SCALE:`
  - `SCALA files (.scl):scl`
  - `ScaleOffset`
  - `SCALE in CV VCA`
  - `OFFSET in CV VCA`

## Interleaver

- slug `Interleaver`, 8 HP, tags: Utility, Polyphonic
- Poly channels (de)interleaver
- panel: Interleaver.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `nysthiLOOPER` at 280, -40
  - `BAR out` at 360, -0
  - `SIGNATURE` at 320, 40
  - `LEFT` at 320, 80
  - `RIGHT` at 360, 120
  - `INTERLEAVED` at 400, 120
  - `POLY IN` at 200, 160

Strings in its code:

  - `Interleaver`

## YYdisplay

- slug `YYdisplay`, tags: Visual, Utility
- YY visualizer
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## BZ-MAPPER

- slug `BZ-MAPPER`, tags: Filter, Polyphonic, Utility
- function mapper
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

From the changelog:

  *  DEBUG: current NODEs status not highlighting
  *
  *  DEBUG: current NODEs status not highlighting
  *
  *  added dragging quantized (if press SHIFT key before dragging, nodes will be quantized in 0.5 volts steps)
  *  remember that dragging with COMMAND/WINDOW key means "precise dragging"
  *  debug poly mode

## BZ-XPAND

- slug `BZ-XPAND`, tags: Expander, Utility
- function mapper expander
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## BZ-XPANDXPAND

- slug `BZ-XPANDXPAND`, tags: Expander, Utility
- function mapper expander expander
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## BZ-ENVELOPE

- slug `BZENVELOPE`, 24 HP, tags: Filter, Envelope, Shaper, Polyphonic, Utility
- bezier envelope
- panel: BZENVELOPE.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `BZ-MAPPER` at 208, -42
  - `BZ-MAPPER` at 209, -41
  - `TRIG_GATE` at 402, 1
  - `TRIG-GATE` at 400, 80
  - `TRIG IN` at -202, 358
  - `TRIG IN` at -200, 360
  - `TRIG` at 83, 409
  - `NODES` at -55, 440
  - `2` at 40, 440
  - `3` at 78, 440
  - `4` at 105, 440
  - `5` at 145, 440
  - `6` at 185, 440
  - `NODES` at -56, 441
  - `LOOP` at -20, 476
  - `DURATION` at 40, 476
  - `TRIG IN` at 120, 480
  - `ENV-OUT` at 237, 480

Strings in its code:

  - `BZENVELOPE`
  - `Manual Trigger`
  - `Loop Mode`
  - `Envelope Time`

From the changelog:

  *  DEBUG: current NODEs status not highlighting
  *  DEBUG: current NODEs status not highlighting
  *	bug: in some situations with very aligned controlpoints and nodes, we were hitting some bizarre math limits... :D (thanks Mark Sanders for the #324)
  *	it's a simple Envelope with no sustain, brother of BZ-MAPPER
  * 	can use the expanders as the BZ-MAPPER (BZ-XPAND and BZ-XPANDXPAND)
  *  there are 2 to 6 nodes (and 1 to 5 Control Points)
  *  nodes are persistent (when switching, module will remember previous settings)
  *  from contextual menu it's possible to set 4 durations
  	*	from 0.001 to 0.100 seconds
  	*	from 0.100 to 1.000 seconds
  	*	from 1.000 to 10.00 seconds
  	*	from 10.00 to 60.00 seconds
  * the knob "duration" will set the precise value between the min and max of the range
  * the duration knob acts in exponential mode
  * the BZ-ENVELOPE can be looped
  * BZ-ENVELOPE is fully POLY
  * the TRIG IN accepts from 1 to 16 channels
  * the TAP TRIG let you activate on demand the ENVELOPE
  * the EOC (End of Cycle) is a pulse emitted at the end of every cycle (in loop mode too)
  * the ENV-OUT is the poly out and can output in 2 ranges
  	* -5 to 5V range
  	* 0 TO 10V range
  * Voltage range is changed using the contextual menu on the Display (where the range is presented)
  * the Zoom of vertical of the screen is set using the contextual menu on the left in the ZOOM area
  * dragging is quantized if SHIFT key is pressed before dragging: nodes will be quantized in 0.5 volts steps
  *  dragging with COMMAND/WINDOW key means "precise/slow dragging"

## TZOP

- slug `TZOP`, 10 HP, tags: VCO, LFO, FM, Polyphonic, Utility
- FM Operator dx7 style, Sinusoid VCO
- panel: TZOP.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `LIN FM 1-2-3` at 201, 226
  - `MOD IND CV` at 173, 328
  - `TRIG-GATE` at -121, 439
  - `TRIG-GATE` at -12, 455
  - `TRIG-GATE` at -134, 473

Strings in its code:

  - `TZOP`
  - `Detach expanders`
  - `Semitone`
  - `Fine tune`
  - `Phase input VCA`
  - `Phase Degrees`
  - `EXP FM1 input VCA`
  - `Modulation index input VCA`
  - `FM Feedback input VCA`
  - `Ouput Level input VCA`

From the changelog:

  *  debug FINE tuning (now correctly doing -1 -> +1 semitone)
  *	is the µOP big brother with all feature exposed
  * 	it's a pure SINE VCO to be used for FM
  *  full POLY 16 voices
  	PARAMETERS
  	YELLOW DISPLAY: Set base frequency (can be reset using contextual menu)
  		same rules as in LFOMultiphase, you can write `120 bpm` for example
  	CV-IN: poly CV in 1V/Octave
  	RATIO: the RATIO applied to the tuning system
  	DETACH: if used as expander/carrier, to detach the INCOMING TUNING
  	OCT: set the Octave
  	SEMI: Semitone
  	FINE: +- 1 semitone
  	SYNC: input to reset the phase. The phase will be reset to the curret PHASE ctrl value
  	CLKIN: applying a clock it's possible to set a frequency using an external device
  	PHASE: to set start phase
  	(via contextual menu it's possbile to add some
  	PRE-PHASE variation to the 16 voice to give more movement)
  	PHASE CV IN + PHASE CV VCA: to control phase using external CV
  	EXP FM IN + EXP FM IN VCA: to modulate pitch, exponential way
  	MOD INDEX: modulation index base
  	MOD INDEX CV + VCA: to modulate the MOD INDEX
  	LINEAR FM INPUTS (1 to 5): lin fm inputs... 5 because dx7 op have 5 IN...
  	FEEDBACK: feedback level; there are 2 types of feedback, switchable using contextual menu
  	FEEDBACK CV + VCA: to modulate the FEEDBACK
  	OUT CV + VCA + LEVEL: it's an output level controlled via CV + VCA
  	OUT: the OUTPUT
  *  it's an EXPANDER, you can build fast dx7 algorithms
  	when is carrier a PURPLE light is ON at the TOPRIGHT
  	when is modulator a BLUE light is ON at the TOPLEFT
  	when connected as expander,
  	the modulator receives CV voltage from the carrier
  	(the voltage can be overridden via DETACH in context menu, yellow light on)
  	the modulator sends the output to LIN FM IN to the carrier

## Ambuance Reverb

- slug `Ambuance`, 10 HP, tags: REVERB, EFFECT
- Reverb based on the Juhana Sadeharju GigaVerb
- panel: Ambuance.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `AMBUANCE` at -1, -129
  - `AMBUANCE` at 0, -120
  - `INPUT` at -70, -5
  - `INPUT GAIN` at -70, 30
  - `NYSTHI` at -105, 399
  - `NYSTHI` at -0, 400
  - `NYSTHI` at -104, 400
  - `NYSTHI` at 0, 400

Strings in its code:

  - `DETUNE:`
  - `Range`
  - `semitone`
  - `octave`
  - `PHASE:`
  - `Range`
  - `AMPLITUDE:`
  - `VOLUME:`
  - `Range [0..1]`
  - `Range [-60dB..0dB]`
  - `MODES`
  - `DETUNE RANGE +- 1 semitone`
  - `DETUNE RANGE +- 1 octave`
  - `Harmonics LEVEL mode`
  - `Use AMPLITUDE`
  - `Use VOLUME`
  - `Harmonics ratios`
  - `Set ratio to 2`
  - `Set ratio to 3/2`
  - `Set ratio to 1`
  - `Set ratio to 2/3`
  - `Set ratio to 1/2`
  - `Set ratio to 1/3`
  - `Set ratio to 1/4`
  - `Waver mode`
  - `Waver bump to end`
  - `Ambuance`
  - `version`
  - `presets`
  - `title`
  - `roomsize`
  - `reverbtime`
  - `damping`
  - `inputbw`
  - `taillevel`
  - `earlylevel`
  - `mix`
  - `Bypass Toggle`
  - `Mix Dry-Wet`
  - `Roomsize`
  - `Reverb Time`
  - `Damp`
  - `Input bandwidth`
  - `Tail Level`
  - `Early Reflections Level`
  - `Input gain`
  - `Output gain`
  - `Input gain CV VCA`
  - `Mix Dry-Wet CV VCA`
  - `Roomsize CV VCA`
  - `Reverb Time CV VCA`
  - `Damp CV VCA`
  - `Input bandwidth CV VCA`
  - `Tail Level CV VCA`
  - `Early Reflections Level CV VCA`
  - `Output gain CV VCA`

## TZEN Envelope

- slug `TZEN`, 8 HP, tags: Envelope generator, Function generator, Expander, Polyphonic
- Polyphonic ADSR linear and exponential, to use as expander for TZOP
- panel: TZEN.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `LIN FM 1-2-3` at 201, 226
  - `MOD IND CV` at 173, 328
  - `TRIG-GATE` at -121, 439
  - `TRIG-GATE` at -12, 455
  - `TRIG-GATE` at -134, 473
  - `TZEN` at -6, 516

## TZVU VU-meter

- slug `TZVU`, tags: Visual, Utility
- monophonic VU-meter EMS style
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

## TZMX FM Operators mixers

- slug `TZMX`, 5 HP, tags: Polyphonic, Mixer, Utility
- polyphonic FM mixers for DX7 style algorithms composition
- panel: TZMX.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `TRIG-GATE` at 116, -33
  - `LIN FM 1-2-3` at 145, 37

Strings in its code:

  - `ERR RANGE`
  - `Use OCTAVE Mapping (faster)`
  - `Use SEMITONES Mapping`
  - `RANGES`
  - `TZMX`
  - `Input Level`
  - `Ouput Level`

## SimplerTapeControl

- slug `SimplerTapeControl`, tags: Utility, Controller, Expander, Effect
- Reel to reel tape style controller for samplers; expander for Simpliciter and Confusing Simpler
- panel: NOT FOUND

No live text on the panel — outlined, or unlabelled. Read `panels/?.png` to see it.

From the changelog:

  * bug: missing click sounds: solved
  * added a TRIG IN TOGGLER to do instantaneous change between play directions
  *  form a Pyer Cllrd idea
  *  a tool that tries to imitate the use of Reel to Reel studio tape machine  using the SPEED input in the various Nysthi Sampler
  *  it works as an expander directly in Simpliciter and Confusing Simpler but using the AUX out can be connected to any other sample like Complex Simpler, Sussudio, Musical Box, Musical Box 2
  *  parameters and commands
  *  TAP CONTROL to START and STOP the machine
  *  TRIG IN to START and STOP the machine
  *  GATE IN to START when HIGH annd STOP when LOW the machine
  *  WOW control + INPUT (they work as SUM), visually is represented by an OFF-CENTER rotation
  *  INERTIAL control + INPUT (they work as SUM). Tries to simulate the inertia of Reels
  *
  *	machine controls:
  * 		BACKWARD + TRIG in
  * 	 	FORWARD + TRIG in
  * 		Half speed BACKWARD + TRIG in
  * 	 	Half speed FORWARD + TRIG in
  * 		Double speed BACKWARD + TRIG in (via contextual menu, the speed will be 4x)
  * 	 	Double speed FORWARD + TRIG in (via contextual menu, the speed will be 4x)
  * 	  SELECT by CV (midi note from 0 to 5 of any octave, for example C3 will be "Double speed BACKWARD", C#3 --> BACKWARD, D3 --> Half speed BACKWARD, D#3 --> Half speed FORWARD, E3 --> FORWARD, F3 --> Double speed FORWARD)
  * 	  NEXT will select next speed
  * 	  PREV will select prev speed
  * 	  RND will select a random speed between the 6
  *
  *	 IN & OUTs:
  *	 MOD IN : any voltage entering in will be added to the "AUX OUT"
  *	 CLICK OUT: sound output for the clicking buttons like real tape machine
  *	 HOLD OUT, it's the GATE of the current status of the TAPE machine, to sync with other MACHINEs
  *	 AUX OUT is the total of all internal calculation to be applied to the speed pf the samplers
  BEWARE that if you connect it directly to a SPEED in connection in a sampler, on that sampler SPEED must be set to ZERO and SPEED CV IN VCA (if present) must be set to MAX

## Programmer

- slug `Programmer`, 60 HP, tags: Utility, Controller, Sequencer
- Serge Style Programmer/Sequencer
- panel: sprogrammer.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `1` at -31, 37
  - `5` at -30, 43
  - `6` at -24, 43
  - `4` at -34, 45
  - `7` at -20, 46
  - `3` at -37, 50
  - `8` at -18, 52
  - `2` at -36, 55
  - `9` at -19, 57
  - `1` at -33, 60
  - `5` at -30, 65
  - `6` at -25, 66
  - `4` at -36, 68
  - `7` at -21, 69
  - `3` at -38, 72
  - `8` at -20, 74
  - `2` at -37, 78
  - `9` at -21, 79
  - `1` at -32, 83
  - `A B C D` at -29, 117
  - `A B C D` at 941, 117
  - `A` at 1019, 156
  - `B` at 1019, 206
  - `C` at 1019, 256
  - `D` at 1018, 306

Strings in its code:

  - `Programmer`
  - `Nudger`
  - `Stage pulse active trigger`
  - `Channel A Voltage`
  - `Channel B Voltage`
  - `Channel C Voltage`
  - `Channel D Voltage`
  - `Stage mode forward`
  - `Stage mode backward`
  - `Stage select trigger`
  - `Pulse repetitions (ratcheting)`
  - `NO QUANTIZER`
  - `AEOLIAN`
  - `BLUES`
  - `CHROMATIC`
  - `DIATONIC MINOR`
  - `DORIAN`
  - `HARMONIC MINOR`
  - `INDIAN`
  - `LOCRIAN`
  - `LYDIAN`
  - `MAJOR`
  - `MELODIC MINOR`
  - `MINOR`
  - `MIXOLYDIAN`
  - `NATURAL MINOR`
  - `NEAPOLITAN MINOR`
  - `PENTATONIC`
  - `PHRYGIAN`
  - `TURKISH`

From the changelog:

  * optimizations
  * bug: wrong behaviour CV knobs
  * added commands to copy track to track
  * added command to nudge forward (shift the sequence to the left, one 208) and backward (shift the sequence to the right, one step) per track
  * debug the knob values not loaded correctly in V2
  * debug current stage on load (not loaded correctly in V2)
  *	feature request from Pyer: add a contextual menu to use a STROBE signal to sample
  	the incoming CV signal in ADDR. The STROBE signal is the PULSE in > and PULSE in <
  	sockets.
  *	feature request from Pyer: add a contextual menu to use a STROBE signal to sample
  	the incoming CV signal in ADDR. The STROBE signal is the PULSE in > and PULSE in <
  	sockets.
  *	a new OLD sequencer/programmer module
  *  perfect imitation of the CGS 16 step SERGE programmer with extras
  *  the programmer can be used as step sequencer, or programmer or keyboard
  *  contains a minimal quantizer and can output polyphony using channel A
  	(only 4 channels...)
  *	it's 16 stages, and every stage contains:
  	* 	active ON/OFF (if OFF stage will not emit the pulse(s)
  	*  repetitions (number of subdivisions pulses)
  	*  SELECT STAGE IN TRIG
  	*  STAGE SELECTED PULSE OUT (with LED)
  	*  CHANNEL A, B, C, D, cv out controls with ranges set via contextual menu
  	*  backward control mode (works with backward clock)
  		*	RUN = GREEN
  		*	STOP = RED
  		*	SKIP = GRAY
  	*  forward control mode (works with forward clock)
  		*	RUN = GREEN
  		*	STOP = RED
  		*	SKIP = GRAY
  	*  Select Stage Button, with BLUE led
  * 	Global controls
  	*  ADDR input (to address directly a STAGE using CV: the CV is a clipped MIDI note in CV)
  	*  FORWARD CLOCK input
  	*  BACKWARD CLOCK input
  	*  A, B, C, D CV outputs (A is polyphonic, if set by contextual menu)
  	*  TRIG output, common pulse out for every emitting pulses stages (polyphonic when needed)
  	*  PUSH output, common GATE out when selecting or touching a STAGE (polyphonic when needed)  (can be used to sync more programmers)
  * 	Contextual Menu

## Nudger

- slug `Nudger`, 7 HP, tags: Utility, Controller, Expander, Sequencer
- Serge Programmer expander
- panel: Nudger.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `NUDGER` at 125, 38
  - `CV ADD` at 147, 311

Strings in its code:

  - `Quantizer:`
  - `Preset scales`
  - `From 0V to 10V`
  - `From -5V to 5V`
  - `From 0V to 5V`
  - `From -2.5V to 2.5V`
  - `From 0V to 2V`
  - `From -1V to 1V`
  - `From 0V to 1V`
  - `ADDR mode`
  - `ADDR: pulse out when stage change`
  - `OUTPUT mode`
  - `Channel A, TRIG and PUSH are poly 4 channel`
  - `Editing`
  - `Copy TRACK 1`
  - `Copy TRACK 2`
  - `Copy TRACK 3`
  - `Copy TRACK 4`
  - `Nudge Tracks`
  - `Nudge Forward`
  - `Nudge Backward`
  - `to 2`
  - `to 3`
  - `to 4`
  - `to 1`
  - `Track 1`
  - `Track 2`
  - `Track 3`
  - `Track 4`
  - `Nudge fwd`
  - `Nudge bwd`
  - `Nudge global fwd`
  - `Nudge global bwd`

From the changelog:

  * expander for the SERGE Programmer adding controls for nudging and CV signals
  * GLOBAL NUDGE via tap button or pulse signals (all tracks). FORWARD or BACKWARD.
  * TRACK NUDGE via tap button or pulse signals track by track. FORWARD or BACKWARD.
  * CV control of nudge, if CV is in nudging will be modulated in semitones (2 semitone, 2 nudges... example)
  *
  * Global CV Adder (the central one) (add a CV to all TRACKS)
  * Track by track CV ADD

## Modulo Magic

- slug `ModuloMagic`, 7 HP, tags: Polyphonic, Utility, Quantizer
- CGS Modulo Magic
- panel: ModuloMagic.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `MODULO MAGIC` at -148, 22
  - `OUTPUT` at -93, 36
  - `INPUT` at -142, 67
  - `MODULO` at -95, 68
  - `VC` at -136, 108
  - `VC` at -83, 108
  - `VC` at -82, 108
  - `+` at -141, 147
  - `-` at -126, 147
  - `+` at -89, 147
  - `+` at -89, 147
  - `-` at -73, 147
  - `-` at -73, 147
  - `INITIATION` at -150, 190
  - `OFFSET` at -92, 190
  - `VC SUB` at -144, 238
  - `VC ADD` at -92, 238
  - `5` at -81, 316
  - `6` at -73, 317
  - `4` at -88, 320
  - `7` at -67, 322
  - `3` at -91, 327
  - `8` at -65, 330
  - `2` at -90, 337
  - `9` at -68, 338
  - `1` at -84, 343
  - `STEP SIZE` at -150, 354
  - `STEPS` at -90, 354
  - `nysthi` at -114, 362

Strings in its code:

  - `ModuloMagic`
  - `Initiation voltage`
  - `Offset voltage`
  - `Initiation voltage mode`
  - `Offset voltage mode`
  - `VC ADD attenuator`
  - `VC SUB attenuator`
  - `Step size voltage`
  - `Number of repeated steps`

From the changelog:

  * highly inspired from the BOCGS MARSH panel CGS module
  * it's a CV folder based on modulo arithmetics
  * a great explanation is visible [here in the TuesdayNightMachines
   blog](https://github.com/TuesdayNightMachines/CGS-Serge-Modular-Synth/blob/master/CGS%20Modulo%20Magic/CGS%20Modulo%20Magic%20Guide.md)
  * highly inspired from the BOCGS MARSH panel CGS module
  * it's a CV folder based on modulo arithmetics
  * a great explanation is visible [here in the TuesdayNightMachines
   blog](https://github.com/TuesdayNightMachines/CGS-Serge-Modular-Synth/blob/master/CGS%20Modulo%20Magic/CGS%20Modulo%20Magic%20Guide.md)

## Infinite Melody

- slug `InfiniteMelody`, 10 HP, tags: Random, Utility
- CGS Infinite Melody
- panel: InfiniteMelody.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `INFINITE MELODY` at -231, 25
  - `OUTPUTS` at -202, 40
  - `STAGE 1` at -251, 70
  - `STAGE 2` at -201, 70
  - `STAGE 3` at -151, 70
  - `MIX` at -167, 110
  - `DIATONIC` at -229, 110
  - `SPAN` at -245, 150
  - `NOISE` at -147, 150
  - `VC SENSE` at -203, 151
  - `FINE` at -39, 159
  - `ROOT` at -246, 190
  - `MODE` at -146, 190
  - `SENSE` at -197, 198
  - `MAJ-MIN` at -251, 230
  - `ADVANCE` at -203, 230
  - `CLOCK` at -148, 230
  - `BIT 4 ON` at -227, 270
  - `BIT 5 ON` at -177, 270
  - `BIT 3` at -245, 312
  - `BIT 4` at -195, 312
  - `BIT 5` at -145, 312
  - `BIT 0` at -245, 352
  - `BIT 1` at -195, 352
  - `BIT 2` at -145, 352
  - `cgs - nysthi` at -75, 397

Strings in its code:

  - `InfiniteMelody`
  - `Stage 1 Trimmer`
  - `Stage 2 Trimmer`
  - `Stage 3 Trimmer`
  - `Bit 3 on off gate`
  - `Bit 4 on off gate`
  - `Bit 5 on off gate`
  - `Sense voltage`
  - `Bit Mix`
  - `Root Voltage`
  - `Voltage span between steps`
  - `Root Voltage Fine (-1 to +1V)`

From the changelog:

  * 	highly inspired from the BOCGS MARSH panel CGS module
  *  it's a CV melody generator based on a set of 1 BITSHIFT register + 4 analogue shift register
  *  a signal must be sent to the NOISE input
  	the signal is compared against the SENSE knob value (or to the VC SENSE input,
  	if connected)
  	when the incoming value is greater than SENSE the LED is ON, otherwise is OFF
  	the LED on represent binary 1 and OFF is binary 0.
  	If a clock signal is applied to CLOCK the bitshift register is shifed to the left
  	and the value of SENSE comparator is written in the first bit.
  	EXAMPLE with 6 clock consecutive:
  	main register status 000 000 (only 6 bits are important)
  	pulse 1 -> SENSE is 1 --> main register is 000 001
  	pulse 2 -> SENSE is 1 --> main register is 000 011
  	pulse 3 -> SENSE is 0 --> main register is 000 110
  	pulse 4 -> SENSE is 1 --> main register is 001 101
  	pulse 5 -> SENSE is 0 --> main register is 011 010
  	pulse 6 -> SENSE is 1 --> main register is 110 101
  	there is a secondary clock coming in ADVANCE
  	for every pulse in ADVANCE the main register is advanced into the 4 digital shift register
  	the ADVANCE can be in 1/f mode or f mode (called RANDOM MODE)
  	in f mode the result is this:
  	simulating the main register with 110 101
  	pulse 1:
  	reg1 110 101
  	reg2 000 000
  	reg3 000 000
  	reg4 000 000
  	pulse 2:
  	reg1 110 101
  	reg2 110 101
  	reg3 000 000
  	reg4 000 000
  	pulse 3:
  	reg1 110 101
  	reg2 110 101
  	reg3 110 101
  	reg4 000 000
  	pulse 4:
  	reg1 110 101
  	reg2 110 101

## Slope Detector

- slug `SlopeDetector`, 3 HP, tags: Dynamic, Utility
- CGS Slope Detector
- panel: SlopeDetector.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `SLOPE` at -71, 18
  - `DETECTOR` at -79, 27
  - `RISING` at -72, 76
  - `STEADY` at -73, 156
  - `FALLING` at -74, 236
  - `INPUT` at -70, 316
  - `SENSE` at -71, 358
  - `cgs - nysthi` at -108, 405

Strings in its code:

  - `Use AC wave draw style`
  - `SlopeDetector`
  - `Sensibility`

From the changelog:

  *  classic CGS module
  *  detects rise steady and fall states on signals and outputs a GATE
  *  classic CGS module
  *  detects rise steady and fall states on signals and outputs a GATE

## Dual Processor

- slug `DualProcessor`, 7 HP, tags: Polyphonic, Vca, CV Mixer, Audio Mixer, Utility
- CGS Dual Processor
- panel: DualProcessor.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `DUAL PROCESSOR` at -144, 25
  - `-5` at -140, 76
  - `+5` at -118, 76
  - `IN-1` at -132, 111
  - `IN-2` at -132, 151
  - `IN-3` at -132, 191
  - `-5` at -140, 236
  - `+5` at -118, 236
  - `IN-1` at -132, 271
  - `IN-2` at -132, 311
  - `IN-3` at -132, 351

Strings in its code:

  - `DualProcessor`
  - `Attenuverter`

From the changelog:

  *  classic CGS module
  *  dual CV/audio mixer, scaler, offsetter
  *  classic CGS module
  *  dual CV/audio mixer, scaler, offsetter

## CV Spread

- slug `CVSpread`, 3 HP, tags: Polyphonic, Vca, Spreader, Utility
- CGS CV Spread
- panel: CVSpread.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `SLOPE` at -71, 18
  - `CV SPREAD` at -158, 27
  - `DETECTOR` at -79, 27
  - `A+(A-B)*3` at -154, 72
  - `RISING` at -72, 76
  - `A+(A-B)*2` at -154, 112
  - `A+(A-B)*1` at -154, 152
  - `STEADY` at -73, 156
  - `INPUT A` at -150, 192
  - `A-(A-B)*3` at -153, 232
  - `FALLING` at -74, 236
  - `A-(A-B)*2` at -153, 272
  - `A-(A-B)*1` at -153, 312
  - `INPUT` at -70, 316
  - `OFFSET B` at -152, 352
  - `SENSE` at -71, 358
  - `cgs - nysthi` at -108, 405

Strings in its code:

  - `OUT`
  - `fonts/DejaVuSansMono.ttf`
  - `current IMPULSE FILE:`
  - `ADD/OPEN an IMPULSE file (wav, aiff)`
  - `Processing BLOCK SIZE:`
  - `Tight Loop`
  - `SET BLOCK SIZE to 256`
  - `SET BLOCK SIZE to 512`
  - `SET BLOCK SIZE to 1024`
  - `SET BLOCK SIZE to 2048`
  - `SET BLOCK SIZE to 4096`
  - `Special Commands`
  - `Impulse Normalized`
  - `Remove Zero Tails From Impulse`
  - `Remove Direct Current (DC block)`
  - `AVAILABLE Impulses:`
  - `SUB DIRECTORIES:`
  - `Invert Screen`
  - `WAV files:wav;WAV float files:wavf;AIFF files:aif,aiff`
  - `2z was  here`
  - `NYSTHI adv for sale!`
  - `impulse Sampling rate`
  - `2.3 seconds`
  - `impulse time`
  - `NYSTHI adv SPACE for sale/rent`
  - `Impulse:`
  - `. Sampling Rate:`
  - `. Duration:`
  - `NYSTHI adv SPACE for sale/rent`
  - `level`
  - `CVSpread`

From the changelog:

  *   simple module in Serge style
  *   creates correlated CV out from a INPUT and OFFSET
  *   simple module in Serge style
  *   creates correlated CV out from a INPUT and OFFSET

## MusicalBox2

- slug `MusicalBox2`, 40 HP, tags: Sampler, Quad, VCO
- Multi sampler 16 stereo channels (max 2Mb samples)
- panel: MusicalBox2.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `NYSTHI & DISSOCIATES - GIANTHOGWEED` at -175, 407
  - `CUE IN` at 164, 421
  - `SPEED` at 95, 427
  - `CUE IN` at 229, 433

Strings in its code:

  - `COMMAND`
  - `DELETE ALL`
  - `NAMES more visible`
  - `BUSY MODE`
  - `SELECTED SAMPLE:`
  - `CURRENT SAMPLES`
  - `Output 0V when stopped`
  - `Exclusive Play Mode`
  - `MusicalBox2`
  - `load file:`

From the changelog:

  * bug: when re-initialized, forget about connected files
  * change anticlick strategy
  *
  * removed offscreens (maybe offending in VST ?)
  * some tweaking about the tails in samplers
  * add exclusive play mode (activable using contextual menu). When ON, only one sampler at time can play

## RAEL

- slug `RAEL`, 29 HP, tags: Utility, Delay
- Multi TRIG/GATE with delay and settable lenght, clockable, with trigs and probabilities
- panel: RAEL.svg

Labels on the panel, roughly in reading order. The coordinates are indicative only — see the rendered panel for anything positional:

  - `GLOBAL TRIG` at -130, 18
  - `OCTAVE` at -280, 142
  - `RAEL` at -280, 186
  - `GATE` at -242, 252

Strings in its code:

  - `Voltage to slice (Voltage / slices), with 0v base`
  - `ROUNDING MODE`
  - `floor mode`
  - `ceil mode`
  - `round mode`
  - `RAEL`
  - `Global trig pulse`
  - `Global reset pulse`
  - `Clock pulse`
  - `Start trig pulse`
  - `Activate trig pulse`
  - `Global gate`
  - `Global start of gate pulse`
  - `Global end of gate pulse`
  - `Gate`
  - `Start of gate pulse`
  - `End of gate pulse`
  - `One shot mode`
  - `One shot mode used flag`
  - `Cycle global`
  - `SOG global`
  - `EOG global`
  - `Active line`
  - `Gate is on`
  - `SOG pulsed`
  - `EOG pulsed`
  - `Line is cycled`
  - `configLight`

From the changelog:

  * optimizations
  bug: not saving values!

## PolyVoltageMeter

- slug `PolyVoltageMeter`, 3 HP, tags: Utility, Polyphonic
- Voltage Visualizer-Voltmeter
- panel: PolyVoltageMeter.svg

No live text on the panel — outlined, or unlabelled. Read `panels/PolyVoltageMeter.png` to see it.

From the changelog:

  * an utility to present poly voltages, in small spaces
  * using contextual menu is possible to  switch from 0 to 10V to -5v to 5v representations
  * an utility to present poly voltages, in small spaces
  * using contextual menu is possible to  switch from 0 to 10V to -5v to 5v representations

