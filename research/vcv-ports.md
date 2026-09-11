# VCV's own ports, classified

840 ports across 73 modules, from the census taken in Rack. Every port here is named by its maker: 839 of 840 have a name, which is why this family is worth doing first.

| decided by | ports |
| --- | --- |
| name | 519 |
| module | 210 |
| takes anything | 110 |
| unknown | 1 |

| family | ports |
| --- | --- |
| CV | 278 |
| AUDIO | 239 |
| GATE | 193 |
| takes anything | 110 |
| PITCH | 19 |
| unknown | 1 |

## Ports nothing decides

These are the ones to settle by testing. A jack marked "takes anything" is not a gap: a merge, a mult, an attenuverter and a scope carry whatever you patch into them, and white is the right colour for those. The ones marked unknown are the real list.

| module | dir | # | port | description |
| --- | --- | --- | --- | --- |
| Fundamental/LFO | input | 1 | (no name) |  |

## Every module

### Audio 2 — Core/AudioInterface2

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | To "device output 1" | AUDIO | name |
| input | 1 | To "device output 2" | AUDIO | name |
| output | 0 | From "device input 1" | AUDIO | name |
| output | 1 | From "device input 2" | AUDIO | name |

### Audio 8 — Core/AudioInterface

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | To "device output 1" | AUDIO | name |
| input | 1 | To "device output 2" | AUDIO | name |
| input | 2 | To "device output 3" | AUDIO | name |
| input | 3 | To "device output 4" | AUDIO | name |
| input | 4 | To "device output 5" | AUDIO | name |
| input | 5 | To "device output 6" | AUDIO | name |
| input | 6 | To "device output 7" | AUDIO | name |
| input | 7 | To "device output 8" | AUDIO | name |
| output | 0 | From "device input 1" | AUDIO | name |
| output | 1 | From "device input 2" | AUDIO | name |
| output | 2 | From "device input 3" | AUDIO | name |
| output | 3 | From "device input 4" | AUDIO | name |
| output | 4 | From "device input 5" | AUDIO | name |
| output | 5 | From "device input 6" | AUDIO | name |
| output | 6 | From "device input 7" | AUDIO | name |
| output | 7 | From "device input 8" | AUDIO | name |

### Audio 16 — Core/AudioInterface16

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | To "device output 1" | AUDIO | name |
| input | 1 | To "device output 2" | AUDIO | name |
| input | 2 | To "device output 3" | AUDIO | name |
| input | 3 | To "device output 4" | AUDIO | name |
| input | 4 | To "device output 5" | AUDIO | name |
| input | 5 | To "device output 6" | AUDIO | name |
| input | 6 | To "device output 7" | AUDIO | name |
| input | 7 | To "device output 8" | AUDIO | name |
| input | 8 | To "device output 9" | AUDIO | name |
| input | 9 | To "device output 10" | AUDIO | name |
| input | 10 | To "device output 11" | AUDIO | name |
| input | 11 | To "device output 12" | AUDIO | name |
| input | 12 | To "device output 13" | AUDIO | name |
| input | 13 | To "device output 14" | AUDIO | name |
| input | 14 | To "device output 15" | AUDIO | name |
| input | 15 | To "device output 16" | AUDIO | name |
| output | 0 | From "device input 1" | AUDIO | name |
| output | 1 | From "device input 2" | AUDIO | name |
| output | 2 | From "device input 3" | AUDIO | name |
| output | 3 | From "device input 4" | AUDIO | name |
| output | 4 | From "device input 5" | AUDIO | name |
| output | 5 | From "device input 6" | AUDIO | name |
| output | 6 | From "device input 7" | AUDIO | name |
| output | 7 | From "device input 8" | AUDIO | name |
| output | 8 | From "device input 9" | AUDIO | name |
| output | 9 | From "device input 10" | AUDIO | name |
| output | 10 | From "device input 11" | AUDIO | name |
| output | 11 | From "device input 12" | AUDIO | name |
| output | 12 | From "device input 13" | AUDIO | name |
| output | 13 | From "device input 14" | AUDIO | name |
| output | 14 | From "device input 15" | AUDIO | name |
| output | 15 | From "device input 16" | AUDIO | name |

### MIDI to CV — Core/MIDIToCVInterface

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| output | 0 | 1V/octave pitch | PITCH | name |
| output | 1 | Gate | GATE | name |
| output | 2 | Velocity | CV | name |
| output | 3 | Aftertouch | CV | name |
| output | 4 | Pitch wheel | PITCH | name |
| output | 5 | Mod wheel | CV | name |
| output | 6 | Retrigger | GATE | name |
| output | 7 | Clock | GATE | name |
| output | 8 | Clock divider | GATE | name |
| output | 9 | Start trigger | GATE | name |
| output | 10 | Stop trigger | GATE | name |
| output | 11 | Continue trigger | GATE | name |

### MIDI CC to CV — Core/MIDICCToCVInterface

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| output | 0 | Cell 1 | CV | module |
| output | 1 | Cell 2 | CV | module |
| output | 2 | Cell 3 | CV | module |
| output | 3 | Cell 4 | CV | module |
| output | 4 | Cell 5 | CV | module |
| output | 5 | Cell 6 | CV | module |
| output | 6 | Cell 7 | CV | module |
| output | 7 | Cell 8 | CV | module |
| output | 8 | Cell 9 | CV | module |
| output | 9 | Cell 10 | CV | module |
| output | 10 | Cell 11 | CV | module |
| output | 11 | Cell 12 | CV | module |
| output | 12 | Cell 13 | CV | module |
| output | 13 | Cell 14 | CV | module |
| output | 14 | Cell 15 | CV | module |
| output | 15 | Cell 16 | CV | module |

### MIDI to Gate — Core/MIDITriggerToCVInterface

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| output | 0 | Gate 1 | GATE | name |
| output | 1 | Gate 2 | GATE | name |
| output | 2 | Gate 3 | GATE | name |
| output | 3 | Gate 4 | GATE | name |
| output | 4 | Gate 5 | GATE | name |
| output | 5 | Gate 6 | GATE | name |
| output | 6 | Gate 7 | GATE | name |
| output | 7 | Gate 8 | GATE | name |
| output | 8 | Gate 9 | GATE | name |
| output | 9 | Gate 10 | GATE | name |
| output | 10 | Gate 11 | GATE | name |
| output | 11 | Gate 12 | GATE | name |
| output | 12 | Gate 13 | GATE | name |
| output | 13 | Gate 14 | GATE | name |
| output | 14 | Gate 15 | GATE | name |
| output | 15 | Gate 16 | GATE | name |

### CV to MIDI — Core/CV-MIDI

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | 1V/octave pitch | PITCH | name |
| input | 1 | Gate | GATE | name |
| input | 2 | Velocity | CV | name |
| input | 3 | Aftertouch | CV | name |
| input | 4 | Pitch wheel | PITCH | name |
| input | 5 | Mod wheel | CV | name |
| input | 6 | Clock | GATE | name |
| input | 7 | Volume | CV | name |
| input | 8 | Pan | CV | name |
| input | 9 | Start trigger | GATE | name |
| input | 10 | Stop trigger | GATE | name |
| input | 11 | Continue trigger | GATE | name |

### CV to MIDI CC — Core/CV-CC

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Cell 1 | CV | module |
| input | 1 | Cell 2 | CV | module |
| input | 2 | Cell 3 | CV | module |
| input | 3 | Cell 4 | CV | module |
| input | 4 | Cell 5 | CV | module |
| input | 5 | Cell 6 | CV | module |
| input | 6 | Cell 7 | CV | module |
| input | 7 | Cell 8 | CV | module |
| input | 8 | Cell 9 | CV | module |
| input | 9 | Cell 10 | CV | module |
| input | 10 | Cell 11 | CV | module |
| input | 11 | Cell 12 | CV | module |
| input | 12 | Cell 13 | CV | module |
| input | 13 | Cell 14 | CV | module |
| input | 14 | Cell 15 | CV | module |
| input | 15 | Cell 16 | CV | module |

### Gate to MIDI — Core/CV-Gate

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Cell 1 | GATE | module |
| input | 1 | Cell 2 | GATE | module |
| input | 2 | Cell 3 | GATE | module |
| input | 3 | Cell 4 | GATE | module |
| input | 4 | Cell 5 | GATE | module |
| input | 5 | Cell 6 | GATE | module |
| input | 6 | Cell 7 | GATE | module |
| input | 7 | Cell 8 | GATE | module |
| input | 8 | Cell 9 | GATE | module |
| input | 9 | Cell 10 | GATE | module |
| input | 10 | Cell 11 | GATE | module |
| input | 11 | Cell 12 | GATE | module |
| input | 12 | Cell 13 | GATE | module |
| input | 13 | Cell 14 | GATE | module |
| input | 14 | Cell 15 | GATE | module |
| input | 15 | Cell 16 | GATE | module |

### Chorus — VCV-Pro/Chorus

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Delay | CV | name |
| input | 1 | Tone | CV | name |
| input | 2 | Depth | CV | name |
| input | 3 | Rate | CV | name |
| input | 4 | Modulation 3 | CV | name |
| input | 5 | Modulation left | AUDIO | name |
| input | 6 | Modulation right | AUDIO | name |
| input | 7 | Left/mono | AUDIO | name |
| input | 8 | Right | AUDIO | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |

### Flanger — VCV-Pro/Flanger

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Delay | CV | name |
| input | 1 | Feedback | CV | name |
| input | 2 | Depth | CV | name |
| input | 3 | Rate | CV | name |
| input | 4 | Pitch (1V/octave) | PITCH | name |
| input | 5 | Modulation left | AUDIO | name |
| input | 6 | Modulation right | AUDIO | name |
| input | 7 | Left/mono | AUDIO | name |
| input | 8 | Right | AUDIO | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |

### Phaser — VCV-Pro/Phaser

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Frequency | CV | name |
| input | 1 | Resonance | CV | name |
| input | 2 | Depth | CV | name |
| input | 3 | Rate | CV | name |
| input | 4 | Pitch (1V/octave) | PITCH | name |
| input | 5 | Modulation left | AUDIO | name |
| input | 6 | Modulation right | AUDIO | name |
| input | 7 | Left/mono | AUDIO | name |
| input | 8 | Right | AUDIO | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |

### Reverb — VCV-Pro/Reverb

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Time | CV | name |
| input | 1 | Morph | CV | name |
| input | 2 | High-pass | CV | name |
| input | 3 | Low-pass | CV | name |
| input | 4 | Width | CV | name |
| input | 5 | Mix | AUDIO | name |
| input | 6 | Left/mono | AUDIO | name |
| input | 7 | Right | AUDIO | name |
| input | 8 | Freeze gate | GATE | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |

### Convolver — VCV-Pro/Convolver

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Gain | CV | name |
| input | 1 | Feedback | CV | name |
| input | 2 | High-pass | CV | name |
| input | 3 | Low-pass | CV | name |
| input | 4 | Width | CV | name |
| input | 5 | Mix | AUDIO | name |
| input | 6 | Left/mono | AUDIO | name |
| input | 7 | Right | AUDIO | name |
| input | 8 | Record gate | GATE | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |

### Compressor — VCV-Pro/Compressor

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Sidechain left/mono | AUDIO | name |
| input | 1 | Sidechain right | AUDIO | name |
| input | 2 | Threshold | CV | name |
| input | 3 | Audio left/mono | AUDIO | name |
| input | 4 | Audio right | AUDIO | name |
| output | 0 | Gain reduction | CV | name |
| output | 1 | Audio left | AUDIO | name |
| output | 2 | Audio right | AUDIO | name |

### Drum Machine — VCV-Drums/DrumMachine

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Kick trigger | GATE | name |
| input | 1 | Kick tune | CV | name |
| input | 2 | Kick sweep | CV | name |
| input | 3 | Kick attack | CV | name |
| input | 4 | Kick decay | CV | name |
| input | 5 | Snare trigger | GATE | name |
| input | 6 | Snare noise | AUDIO | name |
| input | 7 | Snare tune | CV | name |
| input | 8 | Snare decay | CV | name |
| input | 9 | Snare snap | CV | name |
| input | 10 | Tom 1 trigger | GATE | name |
| input | 11 | Tom 1 tune | CV | name |
| input | 12 | Tom 1 sweep | CV | name |
| input | 13 | Tom 1 attack | CV | name |
| input | 14 | Tom 1 decay | CV | name |
| input | 15 | Tom 2 trigger | GATE | name |
| input | 16 | Tom 2 tune | CV | name |
| input | 17 | Tom 2 sweep | CV | name |
| input | 18 | Tom 2 attack | CV | name |
| input | 19 | Tom 2 decay | CV | name |
| input | 20 | Rim trigger | GATE | name |
| input | 21 | Rim tune | CV | name |
| input | 22 | Rim velocity | CV | name |
| input | 23 | Rim attack | CV | name |
| input | 24 | Rim decay | CV | name |
| input | 25 | Clap trigger | GATE | name |
| input | 26 | Clap tone | CV | name |
| input | 27 | Clap velocity | CV | name |
| input | 28 | Clap space | CV | name |
| input | 29 | Clap decay | CV | name |
| input | 30 | Closed hat trigger | GATE | name |
| input | 31 | Closed hat tune | CV | name |
| input | 32 | Closed hat metal | CV | name |
| input | 33 | Closed hat attack | CV | name |
| input | 34 | Closed hat decay | CV | name |
| input | 35 | Open hat trigger | GATE | name |
| input | 36 | Open hat tune | CV | name |
| input | 37 | Open hat metal | CV | name |
| input | 38 | Open hat attack | CV | name |
| input | 39 | Open hat decay | CV | name |
| input | 40 | Crash trigger | GATE | name |
| input | 41 | Crash tune | CV | name |
| input | 42 | Crash metal | CV | name |
| input | 43 | Crash attack | CV | name |
| input | 44 | Crash decay | CV | name |
| input | 45 | Ride trigger | GATE | name |
| input | 46 | Ride tune | CV | name |
| input | 47 | Ride metal | CV | name |
| input | 48 | Ride attack | CV | name |
| input | 49 | Ride decay | CV | name |
| input | 50 | All accent | CV | name |
| input | 51 | All tune | CV | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |

### Kick — VCV-Drums/Kick

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tune | CV | name |
| input | 2 | Sweep | CV | name |
| input | 3 | Accent | CV | name |
| input | 4 | Attack | CV | name |
| input | 5 | Decay | CV | name |
| output | 0 | Kick | AUDIO | module |

### Snare — VCV-Drums/Snare

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tune | CV | name |
| input | 2 | Noise | AUDIO | name |
| input | 3 | Accent | CV | name |
| input | 4 | Snap | CV | name |
| input | 5 | Decay | CV | name |
| output | 0 | Snare | AUDIO | module |

### Tom — VCV-Drums/Tom

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tune | CV | name |
| input | 2 | Sweep | CV | name |
| input | 3 | Accent | CV | name |
| input | 4 | Attack | CV | name |
| input | 5 | Decay | CV | name |
| output | 0 | Tom | AUDIO | module |

### Rim — VCV-Drums/Rim

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tune | CV | name |
| input | 2 | Velocity | CV | name |
| input | 3 | Accent | CV | name |
| input | 4 | Attack | CV | name |
| input | 5 | Decay | CV | name |
| output | 0 | Rim | AUDIO | module |

### Clap — VCV-Drums/Clap

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tone | CV | name |
| input | 2 | Velocity | CV | name |
| input | 3 | Accent | CV | name |
| input | 4 | Space | CV | name |
| input | 5 | Decay | CV | name |
| output | 0 | Clap | AUDIO | module |

### Closed Hat — VCV-Drums/ClosedHat

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tune | CV | name |
| input | 2 | Metal | CV | name |
| input | 3 | Accent | CV | name |
| input | 4 | Mute | GATE | name |
| input | 5 | Envelope (attack/decay) | CV | name |
| output | 0 | Closed hat | AUDIO | module |

### Open Hat — VCV-Drums/OpenHat

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tune | CV | name |
| input | 2 | Metal | CV | name |
| input | 3 | Accent | CV | name |
| input | 4 | Mute | GATE | name |
| input | 5 | Envelope (attack/decay) | CV | name |
| output | 0 | Open hat | AUDIO | module |

### Crash — VCV-Drums/Crash

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tune | CV | name |
| input | 2 | Metal | CV | name |
| input | 3 | Accent | CV | name |
| input | 4 | Attack | CV | name |
| input | 5 | Decay | CV | name |
| output | 0 | Crash | AUDIO | module |

### Ride — VCV-Drums/Ride

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| input | 1 | Tune | CV | name |
| input | 2 | Metal | CV | name |
| input | 3 | Accent | CV | name |
| input | 4 | Attack | CV | name |
| input | 5 | Decay | CV | name |
| output | 0 | Ride | AUDIO | module |

### Chords — VCV-Chords/Chords

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Root | PITCH | name |
| input | 1 | Reset | GATE | name |
| input | 2 | Advance trigger | GATE | name |
| input | 3 | Address | CV | name |
| output | 0 | Note 1 | PITCH | name |
| output | 1 | Note 2 | PITCH | name |
| output | 2 | Note 3 | PITCH | name |
| output | 3 | Note 4 | PITCH | name |

### VCO — Fundamental/VCO

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | 1V/octave pitch | PITCH | name |
| input | 1 | Frequency modulation | CV | name |
| input | 2 | Sync | GATE | name |
| input | 3 | Pulse width modulation | CV | name |
| output | 0 | Sine | AUDIO | name |
| output | 1 | Triangle | AUDIO | name |
| output | 2 | Sawtooth | AUDIO | name |
| output | 3 | Square | AUDIO | name |

### Wavetable VCO — Fundamental/VCO2

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Frequency modulation | CV | name |
| input | 1 | Sync | GATE | name |
| input | 2 | Wavetable position | AUDIO | name |
| input | 3 | 1V/octave pitch | PITCH | name |
| output | 0 | Wavetable | AUDIO | name |

### VCF — Fundamental/VCF

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Frequency | CV | name |
| input | 1 | Resonance | CV | name |
| input | 2 | Drive | CV | name |
| input | 3 | Audio | AUDIO | name |
| output | 0 | Lowpass filter | AUDIO | name |
| output | 1 | Highpass filter | AUDIO | name |

### VCA — Fundamental/VCA-1

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | CV | CV | name |
| input | 1 | Channel | AUDIO | module |
| output | 0 | Channel | AUDIO | module |

### VCA-2 — Fundamental/VCA

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Channel 1 exponential CV | CV | name |
| input | 1 | Channel 1 linear CV | CV | name |
| input | 2 | Channel 1 | AUDIO | module |
| input | 3 | Channel 2 exponential CV | CV | name |
| input | 4 | Channel 2 linear CV | CV | name |
| input | 5 | Channel 2 | AUDIO | module |
| output | 0 | Channel 1 | AUDIO | module |
| output | 1 | Channel 2 | AUDIO | module |

### LFO — Fundamental/LFO

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Frequency modulation | CV | name |
| input | 1 | (no name) | — | unknown |
| input | 2 | Reset | GATE | name |
| input | 3 | Pulse width modulation | CV | name |
| input | 4 | Clock | GATE | name |
| output | 0 | Sine | AUDIO | name |
| output | 1 | Triangle | AUDIO | name |
| output | 2 | Sawtooth | AUDIO | name |
| output | 3 | Square | AUDIO | name |

### Wavetable LFO — Fundamental/LFO2

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Frequency modulation | CV | name |
| input | 1 | Reset | GATE | name |
| input | 2 | Wavetable position | AUDIO | name |
| input | 3 | Clock | GATE | name |
| output | 0 | Wavetable | AUDIO | name |

### Delay — Fundamental/Delay

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Time | PITCH | name |
| input | 1 | Feedback | CV | name |
| input | 2 | Tone | CV | name |
| input | 3 | Mix | AUDIO | name |
| input | 4 | Audio | AUDIO | name |
| input | 5 | Clock | GATE | name |
| output | 0 | Mix | AUDIO | name |
| output | 1 | Wet | AUDIO | name |

### ADSR EG — Fundamental/ADSR

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Attack | CV | name |
| input | 1 | Decay | CV | name |
| input | 2 | Sustain | CV | name |
| input | 3 | Release | CV | name |
| input | 4 | Gate | GATE | name |
| input | 5 | Retrigger | GATE | name |
| output | 0 | Envelope | CV | name |

### Mix — Fundamental/Mixer

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Channel 1 | AUDIO | module |
| input | 1 | Channel 2 | AUDIO | module |
| input | 2 | Channel 3 | AUDIO | module |
| input | 3 | Channel 4 | AUDIO | module |
| input | 4 | Channel 5 | AUDIO | module |
| input | 5 | Channel 6 | AUDIO | module |
| output | 0 | Mix | AUDIO | name |

### VCA Mix — Fundamental/VCMixer

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Mix CV | AUDIO | name |
| input | 1 | Channel 1 | AUDIO | module |
| input | 2 | Channel 2 | AUDIO | module |
| input | 3 | Channel 3 | AUDIO | module |
| input | 4 | Channel 4 | AUDIO | module |
| input | 5 | Channel 1 CV | CV | name |
| input | 6 | Channel 2 CV | CV | name |
| input | 7 | Channel 3 CV | CV | name |
| input | 8 | Channel 4 CV | CV | name |
| output | 0 | Mix | AUDIO | name |
| output | 1 | Channel 1 | AUDIO | module |
| output | 2 | Channel 2 | AUDIO | module |
| output | 3 | Channel 3 | AUDIO | module |
| output | 4 | Channel 4 | AUDIO | module |

### 8vert — Fundamental/8vert

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Row 1 | CV | module |
| input | 1 | Row 2 | CV | module |
| input | 2 | Row 3 | CV | module |
| input | 3 | Row 4 | CV | module |
| input | 4 | Row 5 | CV | module |
| input | 5 | Row 6 | CV | module |
| input | 6 | Row 7 | CV | module |
| input | 7 | Row 8 | CV | module |
| output | 0 | Row 1 | CV | module |
| output | 1 | Row 2 | CV | module |
| output | 2 | Row 3 | CV | module |
| output | 3 | Row 4 | CV | module |
| output | 4 | Row 5 | CV | module |
| output | 5 | Row 6 | CV | module |
| output | 6 | Row 7 | CV | module |
| output | 7 | Row 8 | CV | module |

### Unity — Fundamental/Unity

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Channel 1 #1 | AUDIO | module |
| input | 1 | Channel 1 #2 | AUDIO | module |
| input | 2 | Channel 1 #3 | AUDIO | module |
| input | 3 | Channel 1 #4 | AUDIO | module |
| input | 4 | Channel 1 #5 | AUDIO | module |
| input | 5 | Channel 1 #6 | AUDIO | module |
| input | 6 | Channel 2 #1 | AUDIO | module |
| input | 7 | Channel 2 #2 | AUDIO | module |
| input | 8 | Channel 2 #3 | AUDIO | module |
| input | 9 | Channel 2 #4 | AUDIO | module |
| input | 10 | Channel 2 #5 | AUDIO | module |
| input | 11 | Channel 2 #6 | AUDIO | module |
| output | 0 | Channel 1 mix | AUDIO | name |
| output | 1 | Channel 1 inverse mix | AUDIO | name |
| output | 2 | Channel 2 mix | AUDIO | name |
| output | 3 | Channel 2 inverse mix | AUDIO | name |

### Mutes — Fundamental/Mutes

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Row 1 | — | takes anything |
| input | 1 | Row 2 | — | takes anything |
| input | 2 | Row 3 | — | takes anything |
| input | 3 | Row 4 | — | takes anything |
| input | 4 | Row 5 | — | takes anything |
| input | 5 | Row 6 | — | takes anything |
| input | 6 | Row 7 | — | takes anything |
| input | 7 | Row 8 | — | takes anything |
| input | 8 | Row 9 | — | takes anything |
| input | 9 | Row 10 | — | takes anything |
| output | 0 | Row 1 | — | takes anything |
| output | 1 | Row 2 | — | takes anything |
| output | 2 | Row 3 | — | takes anything |
| output | 3 | Row 4 | — | takes anything |
| output | 4 | Row 5 | — | takes anything |
| output | 5 | Row 6 | — | takes anything |
| output | 6 | Row 7 | — | takes anything |
| output | 7 | Row 8 | — | takes anything |
| output | 8 | Row 9 | — | takes anything |
| output | 9 | Row 10 | — | takes anything |

### Pulses — Fundamental/Pulses

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| output | 0 | Row 1 trigger | GATE | name |
| output | 1 | Row 2 trigger | GATE | name |
| output | 2 | Row 3 trigger | GATE | name |
| output | 3 | Row 4 trigger | GATE | name |
| output | 4 | Row 5 trigger | GATE | name |
| output | 5 | Row 6 trigger | GATE | name |
| output | 6 | Row 7 trigger | GATE | name |
| output | 7 | Row 8 trigger | GATE | name |
| output | 8 | Row 9 trigger | GATE | name |
| output | 9 | Row 10 trigger | GATE | name |
| output | 10 | Row 1 gate | GATE | name |
| output | 11 | Row 2 gate | GATE | name |
| output | 12 | Row 3 gate | GATE | name |
| output | 13 | Row 4 gate | GATE | name |
| output | 14 | Row 5 gate | GATE | name |
| output | 15 | Row 6 gate | GATE | name |
| output | 16 | Row 7 gate | GATE | name |
| output | 17 | Row 8 gate | GATE | name |
| output | 18 | Row 9 gate | GATE | name |
| output | 19 | Row 10 gate | GATE | name |

### Scope — Fundamental/Scope

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Ch 1 | — | takes anything |
| input | 1 | Ch 2 | — | takes anything |
| input | 2 | External trigger | GATE | name |
| output | 0 | Ch 1 | — | takes anything |
| output | 1 | Ch 2 | — | takes anything |

### SEQ 3 — Fundamental/SEQ3

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Tempo | CV | name |
| input | 1 | Clock | GATE | name |
| input | 2 | Reset | GATE | name |
| input | 3 | Steps | CV | name |
| input | 4 | Run | GATE | name |
| output | 0 | Trigger | GATE | name |
| output | 1 | CV 1 | CV | name |
| output | 2 | CV 2 | CV | name |
| output | 3 | CV 3 | CV | name |
| output | 4 | Step 1 | GATE | name |
| output | 5 | Step 2 | GATE | name |
| output | 6 | Step 3 | GATE | name |
| output | 7 | Step 4 | GATE | name |
| output | 8 | Step 5 | GATE | name |
| output | 9 | Step 6 | GATE | name |
| output | 10 | Step 7 | GATE | name |
| output | 11 | Step 8 | GATE | name |
| output | 12 | Steps | CV | name |
| output | 13 | Clock | GATE | name |
| output | 14 | Run | GATE | name |
| output | 15 | Reset | GATE | name |

### Sequential Switch 1 to 4 — Fundamental/SequentialSwitch1

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Clock | GATE | name |
| input | 1 | Reset | GATE | name |
| input | 2 | Main | — | takes anything |
| output | 0 | Channel 1 | — | takes anything |
| output | 1 | Channel 2 | — | takes anything |
| output | 2 | Channel 3 | — | takes anything |
| output | 3 | Channel 4 | — | takes anything |

### Sequential Switch 4 to 1 — Fundamental/SequentialSwitch2

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Clock | GATE | name |
| input | 1 | Reset | GATE | name |
| input | 2 | Channel 1 | — | takes anything |
| input | 3 | Channel 2 | — | takes anything |
| input | 4 | Channel 3 | — | takes anything |
| input | 5 | Channel 4 | — | takes anything |
| output | 0 | Main | — | takes anything |

### Octave — Fundamental/Octave

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | 1V/octave pitch | PITCH | name |
| input | 1 | Octave shift CV | CV | name |
| output | 0 | Pitch | PITCH | name |

### Quantizer — Fundamental/Quantizer

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | 1V/octave pitch | PITCH | name |
| output | 0 | Pitch | PITCH | name |

### Split — Fundamental/Split

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Polyphonic | — | takes anything |
| output | 0 | Channel 1 | — | takes anything |
| output | 1 | Channel 2 | — | takes anything |
| output | 2 | Channel 3 | — | takes anything |
| output | 3 | Channel 4 | — | takes anything |
| output | 4 | Channel 5 | — | takes anything |
| output | 5 | Channel 6 | — | takes anything |
| output | 6 | Channel 7 | — | takes anything |
| output | 7 | Channel 8 | — | takes anything |
| output | 8 | Channel 9 | — | takes anything |
| output | 9 | Channel 10 | — | takes anything |
| output | 10 | Channel 11 | — | takes anything |
| output | 11 | Channel 12 | — | takes anything |
| output | 12 | Channel 13 | — | takes anything |
| output | 13 | Channel 14 | — | takes anything |
| output | 14 | Channel 15 | — | takes anything |
| output | 15 | Channel 16 | — | takes anything |

### Merge — Fundamental/Merge

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Channel 1 | — | takes anything |
| input | 1 | Channel 2 | — | takes anything |
| input | 2 | Channel 3 | — | takes anything |
| input | 3 | Channel 4 | — | takes anything |
| input | 4 | Channel 5 | — | takes anything |
| input | 5 | Channel 6 | — | takes anything |
| input | 6 | Channel 7 | — | takes anything |
| input | 7 | Channel 8 | — | takes anything |
| input | 8 | Channel 9 | — | takes anything |
| input | 9 | Channel 10 | — | takes anything |
| input | 10 | Channel 11 | — | takes anything |
| input | 11 | Channel 12 | — | takes anything |
| input | 12 | Channel 13 | — | takes anything |
| input | 13 | Channel 14 | — | takes anything |
| input | 14 | Channel 15 | — | takes anything |
| input | 15 | Channel 16 | — | takes anything |
| output | 0 | Polyphonic | — | takes anything |

### Sum — Fundamental/Sum

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Polyphonic | — | takes anything |
| output | 0 | Monophonic | — | takes anything |

### Viz — Fundamental/Viz

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Polyphonic | — | takes anything |

### Mid/Side — Fundamental/MidSide

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Encoder width | CV | name |
| input | 1 | Encoder left | AUDIO | name |
| input | 2 | Encoder right | AUDIO | name |
| input | 3 | Decoder width | CV | name |
| input | 4 | Decoder mid | AUDIO | name |
| input | 5 | Decoder side | AUDIO | name |
| output | 0 | Encoder mid | AUDIO | name |
| output | 1 | Encoder side | AUDIO | name |
| output | 2 | Decoder left | AUDIO | name |
| output | 3 | Decoder right | AUDIO | name |

### Noise — Fundamental/Noise

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| output | 0 | White noise | AUDIO | name |
| output | 1 | Pink noise | AUDIO | name |
| output | 2 | Red noise | AUDIO | name |
| output | 3 | Violet noise | AUDIO | name |
| output | 4 | Blue noise | AUDIO | name |
| output | 5 | Gray noise | AUDIO | name |
| output | 6 | Black noise | AUDIO | name |

### Random — Fundamental/Random

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Internal trigger rate | GATE | name |
| input | 1 | Shape | CV | name |
| input | 2 | Trigger | GATE | name |
| input | 3 | External | CV | name |
| input | 4 | Trigger probability | GATE | name |
| input | 5 | Random spread | CV | name |
| output | 0 | Stepped | CV | name |
| output | 1 | Linear | CV | name |
| output | 2 | Smooth | CV | name |
| output | 3 | Exponential | CV | name |
| output | 4 | Trigger | GATE | name |

### CV Mix — Fundamental/CVMix

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | CV 1 | CV | name |
| input | 1 | CV 2 | CV | name |
| input | 2 | CV 3 | CV | name |
| output | 0 | Mix | AUDIO | name |

### Fade — Fundamental/Fade

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Crossfade | CV | name |
| input | 1 | Ch 1 | — | takes anything |
| input | 2 | Ch 2 | — | takes anything |
| output | 0 | Ch 1 | — | takes anything |
| output | 1 | Ch 2 | — | takes anything |

### Logic — Fundamental/Logic

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | A | GATE | module |
| input | 1 | B | GATE | module |
| output | 0 | NOT A | GATE | name |
| output | 1 | NOT B | GATE | name |
| output | 2 | OR | GATE | name |
| output | 3 | NOR | GATE | name |
| output | 4 | AND | GATE | name |
| output | 5 | NAND | GATE | name |
| output | 6 | XOR | GATE | name |
| output | 7 | XNOR | GATE | name |

### Compare — Fundamental/Compare

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | A | — | takes anything |
| input | 1 | B | — | takes anything |
| output | 0 | Maximum | CV | name |
| output | 1 | Minimum | CV | name |
| output | 2 | Clip | CV | name |
| output | 3 | Limit | CV | name |
| output | 4 | Clip gate | GATE | name |
| output | 5 | Limit gate | GATE | name |
| output | 6 | A>B | GATE | name |
| output | 7 | A<B | GATE | name |

### Gates — Fundamental/Gates

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Gate length | GATE | name |
| input | 1 | Gate | GATE | name |
| input | 2 | Reset flip/flop | GATE | name |
| output | 0 | Rising edge trigger | GATE | name |
| output | 1 | Falling edge trigger | GATE | name |
| output | 2 | Flip | GATE | name |
| output | 3 | Flop | GATE | name |
| output | 4 | Gate | GATE | name |
| output | 5 | Gate delay | GATE | name |

### Process — Fundamental/Process

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Slew | CV | name |
| input | 1 | Voltage | CV | name |
| input | 2 | Gate | GATE | name |
| output | 0 | Sample & hold | GATE | name |
| output | 1 | Sample & hold 2 | GATE | name |
| output | 2 | Track & hold | GATE | name |
| output | 3 | Hold & track | GATE | name |
| output | 4 | Slew | CV | name |
| output | 5 | Glide | CV | name |

### Mult — Fundamental/Mult

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Mult | — | takes anything |
| output | 0 | Mult 1 | — | takes anything |
| output | 1 | Mult 2 | — | takes anything |
| output | 2 | Mult 3 | — | takes anything |
| output | 3 | Mult 4 | — | takes anything |
| output | 4 | Mult 5 | — | takes anything |
| output | 5 | Mult 6 | — | takes anything |
| output | 6 | Mult 7 | — | takes anything |
| output | 7 | Mult 8 | — | takes anything |

### Rescale — Fundamental/Rescale

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Signal | AUDIO | name |
| output | 0 | Signal | AUDIO | name |

### Random Values — Fundamental/RandomValues

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Trigger | GATE | name |
| output | 0 | Random 1 | CV | module |
| output | 1 | Random 2 | CV | module |
| output | 2 | Random 3 | CV | module |
| output | 3 | Random 4 | CV | module |
| output | 4 | Random 5 | CV | module |
| output | 5 | Random 6 | CV | module |
| output | 6 | Random 7 | CV | module |

### Push — Fundamental/Push

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Hold | GATE | name |
| input | 1 | Push | GATE | name |
| output | 0 | Trigger | GATE | name |
| output | 1 | Gate | GATE | name |

### Sample & Hold Analog Shift Register — Fundamental/SHASR

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Sample 1 | CV | module |
| input | 1 | Sample 2 | CV | module |
| input | 2 | Sample 3 | CV | module |
| input | 3 | Sample 4 | CV | module |
| input | 4 | Sample 5 | CV | module |
| input | 5 | Sample 6 | CV | module |
| input | 6 | Sample 7 | CV | module |
| input | 7 | Sample 8 | CV | module |
| input | 8 | Trigger 1 | GATE | name |
| input | 9 | Trigger 2 | GATE | name |
| input | 10 | Trigger 3 | GATE | name |
| input | 11 | Trigger 4 | GATE | name |
| input | 12 | Trigger 5 | GATE | name |
| input | 13 | Trigger 6 | GATE | name |
| input | 14 | Trigger 7 | GATE | name |
| input | 15 | Trigger 8 | GATE | name |
| output | 0 | Sample 1 | CV | module |
| output | 1 | Sample 2 | CV | module |
| output | 2 | Sample 3 | CV | module |
| output | 3 | Sample 4 | CV | module |
| output | 4 | Sample 5 | CV | module |
| output | 5 | Sample 6 | CV | module |
| output | 6 | Sample 7 | CV | module |
| output | 7 | Sample 8 | CV | module |

### Sound Stage — VCV-SoundStage/SoundStage

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Source 1 | AUDIO | module |
| input | 1 | Source 2 | AUDIO | module |
| input | 2 | Source 3 | AUDIO | module |
| input | 3 | Source 4 | AUDIO | module |
| input | 4 | X1 position | CV | name |
| input | 5 | X2 position | CV | name |
| input | 6 | X3 position | CV | name |
| input | 7 | X4 position | CV | name |
| input | 8 | Y1 position | CV | name |
| input | 9 | Y2 position | CV | name |
| input | 10 | Y3 position | CV | name |
| input | 11 | Y4 position | CV | name |
| input | 12 | Low-cut frequency | CV | name |
| input | 13 | High-cut frequency | CV | name |
| input | 14 | Reflectivity | CV | name |
| input | 15 | Diffusion | CV | name |
| output | 0 | Destination 1 | AUDIO | module |
| output | 1 | Destination 2 | AUDIO | module |
| output | 2 | Destination 3 | AUDIO | module |
| output | 3 | Destination 4 | AUDIO | module |

### Host — VCV-Host/Host

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Cell 1 | AUDIO | module |
| input | 1 | Cell 2 | AUDIO | module |
| input | 2 | Cell 3 | AUDIO | module |
| input | 3 | Cell 4 | AUDIO | module |
| input | 4 | Cell 5 | AUDIO | module |
| input | 5 | Cell 6 | AUDIO | module |
| input | 6 | Cell 7 | AUDIO | module |
| input | 7 | Cell 8 | AUDIO | module |
| input | 8 | Cell 9 | AUDIO | module |
| input | 9 | Cell 10 | AUDIO | module |
| input | 10 | Cell 11 | AUDIO | module |
| input | 11 | Cell 12 | AUDIO | module |
| input | 12 | Cell 13 | AUDIO | module |
| input | 13 | Cell 14 | AUDIO | module |
| input | 14 | Cell 15 | AUDIO | module |
| input | 15 | Cell 16 | AUDIO | module |
| input | 16 | CV | CV | name |
| input | 17 | Gate | GATE | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |

### Host-FX — VCV-Host/Host-FX

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Cell 1 | AUDIO | module |
| input | 1 | Cell 2 | AUDIO | module |
| input | 2 | Cell 3 | AUDIO | module |
| input | 3 | Cell 4 | AUDIO | module |
| input | 4 | Cell 5 | AUDIO | module |
| input | 5 | Cell 6 | AUDIO | module |
| input | 6 | Cell 7 | AUDIO | module |
| input | 7 | Cell 8 | AUDIO | module |
| input | 8 | Cell 9 | AUDIO | module |
| input | 9 | Cell 10 | AUDIO | module |
| input | 10 | Cell 11 | AUDIO | module |
| input | 11 | Cell 12 | AUDIO | module |
| input | 12 | Cell 13 | AUDIO | module |
| input | 13 | Cell 14 | AUDIO | module |
| input | 14 | Cell 15 | AUDIO | module |
| input | 15 | Cell 16 | AUDIO | module |
| input | 16 | Left | AUDIO | name |
| input | 17 | Right | AUDIO | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |

### Host-XL — VCV-Host/Host-XL

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Cell 1 | — | takes anything |
| input | 1 | Cell 2 | — | takes anything |
| input | 2 | Cell 3 | — | takes anything |
| input | 3 | Cell 4 | — | takes anything |
| input | 4 | Cell 5 | — | takes anything |
| input | 5 | Cell 6 | — | takes anything |
| input | 6 | Cell 7 | — | takes anything |
| input | 7 | Cell 8 | — | takes anything |
| input | 8 | Cell 9 | — | takes anything |
| input | 9 | Cell 10 | — | takes anything |
| input | 10 | Cell 11 | — | takes anything |
| input | 11 | Cell 12 | — | takes anything |
| input | 12 | Cell 13 | — | takes anything |
| input | 13 | Cell 14 | — | takes anything |
| input | 14 | Cell 15 | — | takes anything |
| input | 15 | Cell 16 | — | takes anything |
| input | 16 | Cell 17 | — | takes anything |
| input | 17 | Cell 18 | — | takes anything |
| input | 18 | Cell 19 | — | takes anything |
| input | 19 | Cell 20 | — | takes anything |
| input | 20 | Cell 21 | — | takes anything |
| input | 21 | Cell 22 | — | takes anything |
| input | 22 | Cell 23 | — | takes anything |
| input | 23 | Cell 24 | — | takes anything |
| input | 24 | Left | AUDIO | name |
| input | 25 | Right | AUDIO | name |
| input | 26 | Audio 3 | AUDIO | name |
| input | 27 | Audio 4 | AUDIO | name |
| input | 28 | Audio 5 | AUDIO | name |
| input | 29 | Audio 6 | AUDIO | name |
| input | 30 | Audio 7 | AUDIO | name |
| input | 31 | Audio 8 | AUDIO | name |
| output | 0 | Left | AUDIO | name |
| output | 1 | Right | AUDIO | name |
| output | 2 | Audio 3 | AUDIO | name |
| output | 3 | Audio 4 | AUDIO | name |
| output | 4 | Audio 5 | AUDIO | name |
| output | 5 | Audio 6 | AUDIO | name |
| output | 6 | Audio 7 | AUDIO | name |
| output | 7 | Audio 8 | AUDIO | name |

### Host-CV — VCV-Host/Host-CV

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| output | 0 | CV | CV | name |
| output | 1 | Gate | GATE | name |
| output | 2 | Velocity | CV | name |
| output | 3 | Aftertouch | CV | name |
| output | 4 | Pitch | PITCH | name |
| output | 5 | Mod wheel | CV | name |
| output | 6 | Clock | GATE | name |
| output | 7 | Clock divider | GATE | name |
| output | 8 | Start | GATE | name |
| output | 9 | Stop | GATE | name |
| output | 10 | Continue | GATE | name |
| output | 11 | Retrigger | GATE | name |

### Host-CC — VCV-Host/Host-CC

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| output | 0 | Cell 0 | CV | module |
| output | 1 | Cell 1 | CV | module |
| output | 2 | Cell 2 | CV | module |
| output | 3 | Cell 3 | CV | module |
| output | 4 | Cell 4 | CV | module |
| output | 5 | Cell 5 | CV | module |
| output | 6 | Cell 6 | CV | module |
| output | 7 | Cell 7 | CV | module |
| output | 8 | Cell 8 | CV | module |
| output | 9 | Cell 9 | CV | module |
| output | 10 | Cell 10 | CV | module |
| output | 11 | Cell 11 | CV | module |
| output | 12 | Cell 12 | CV | module |
| output | 13 | Cell 13 | CV | module |
| output | 14 | Cell 14 | CV | module |
| output | 15 | Cell 15 | CV | module |
| output | 16 | Cell 16 | CV | module |
| output | 17 | Cell 17 | CV | module |
| output | 18 | Cell 18 | CV | module |
| output | 19 | Cell 19 | CV | module |

### Host-Gate — VCV-Host/Host-Gate

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| output | 0 | Cell 0 | GATE | module |
| output | 1 | Cell 1 | GATE | module |
| output | 2 | Cell 2 | GATE | module |
| output | 3 | Cell 3 | GATE | module |
| output | 4 | Cell 4 | GATE | module |
| output | 5 | Cell 5 | GATE | module |
| output | 6 | Cell 6 | GATE | module |
| output | 7 | Cell 7 | GATE | module |
| output | 8 | Cell 8 | GATE | module |
| output | 9 | Cell 9 | GATE | module |
| output | 10 | Cell 10 | GATE | module |
| output | 11 | Cell 11 | GATE | module |
| output | 12 | Cell 12 | GATE | module |
| output | 13 | Cell 13 | GATE | module |
| output | 14 | Cell 14 | GATE | module |
| output | 15 | Cell 15 | GATE | module |
| output | 16 | Cell 16 | GATE | module |
| output | 17 | Cell 17 | GATE | module |
| output | 18 | Cell 18 | GATE | module |
| output | 19 | Cell 19 | GATE | module |

### Recorder — VCV-Recorder/Recorder

| dir | # | port | family | decided by |
| --- | --- | --- | --- | --- |
| input | 0 | Gate | GATE | name |
| input | 1 | Trigger | GATE | name |
| input | 2 | Left/mono | AUDIO | name |
| input | 3 | Right | AUDIO | name |
