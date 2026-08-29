#pragma once
/** Signal injectors: small faces that clip onto an input and drive it.

HOW A SIGNAL GETS IN. Not by writing into the target module's input from our process(). That
looks like it should work and does not: the engine steps every module and only THEN copies
each cable's output into its input, so a cable connected to that port overwrites whatever we
wrote, every sample. It would appear to work on an empty input and fail on a patched one,
which is the worse of the two failures.

Instead an injector is a real cable, from one of DRUI's own outputs into the target input.
The engine supports several cables on one input and SUMS them — the UI simply never offers
to make one. So an injector adds to whatever is already patched there rather than replacing
it, and no cable has to be pulled to use one.

Rack owns that cable, which is what keeps it safe: cables are removed with the modules they
touch, so nothing is left pointing at a deleted module. The cable and the plugs at its ends
are hidden, because the attachment the user should see is the callout loop, not a patch lead
running across the rack.

The audio thread reads each injector's settings and writes its output. Settings are atomics
written by the UI thread, and the phase of a running waveform belongs to the audio thread
alone.
*/
#include "plugin.hpp"

/** Injectors that can exist at once, and therefore hidden outputs on DRUI. */
static const int INJECT_MAX = 8;

enum InjectorType {
	INJECT_GATE,     /**< High while the button is held. */
	INJECT_PULSE,    /**< A single pulse each time the button is pressed. */
	INJECT_DC,       /**< A steady voltage. */
	INJECT_LFO,      /**< A repeating waveform, 0.01 to 100 Hz. */
	INJECT_AUDIO,    /**< The same, at audio rates: 1 Hz to 8 kHz. */
	INJECT_NOTE,     /**< A pitch as one volt per octave, set and shown by note name. */
	INJECT_AV,       /**< Scales and inverts what a cable is already delivering to the port. */
	INJECT_NOISE,    /**< White, pink, brown, blue or violet noise. */
	INJECT_CLOCK,    /**< A stream of pulses at a rate in beats per minute. */
	INJECT_TYPES,
};

/** Noise colours, in the usual sense: the slope of the spectrum. White is flat, pink falls
3 dB per octave, brown 6, and blue and violet rise by the same amounts. */
enum NoiseColour {
	NOISE_WHITE,
	NOISE_PINK,
	NOISE_BROWN,
	NOISE_BLUE,
	NOISE_VIOLET,
	NOISE_COUNT,
};

enum InjectorWave {
	WAVE_SINE,
	WAVE_TRIANGLE,
	WAVE_SQUARE,
	WAVE_RAMP,
	WAVE_COUNT,
};


/** AUDIO THREAD. Writes every active injector's value to its output. Called from
DRUI::process, and costs one atomic load when no injector exists. */
void injectorProcessAll(Module* drui, float sampleTime);

/** Clips an injector onto a port. UI thread. */
void injectorCreate(app::PortWidget* port, InjectorType type, bool noteMode = false);

/** True if a port can take an injector: inputs only, since a signal is injected INTO
something. An output is driven by its own module and nothing else may write to it. */
bool injectorAcceptsPort(app::PortWidget* port);

/** Every injector's attachment and settings, for saving with the patch. */
json_t* injectorToJson();
/** Queues saved injectors to be re-attached as their modules load. */
void injectorFromJson(json_t* arrayJ);
/** Re-attaches queued injectors. Does nothing once none are waiting. */
void injectorRestoreStep();
/** Hides every injector and silences it, or brings them all back. */
void injectorSetEnabled(bool on);
/** Removes cables out of DRUI that no injector owns — see the note on the definition. */
void injectorPurgeStrayCables();
