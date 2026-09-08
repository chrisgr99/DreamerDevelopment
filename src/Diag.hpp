#pragma once
/** WHAT THE MODULE ACTUALLY COSTS, measured on the machine that is complaining.

Somebody reports that Test Gear reads ten per cent on Rack's CPU meter with nothing clipped onto
anything. Reading the code says that cannot be: with nothing attached the module's process() is a
decrement, one atomic load and a setVoltage. Reading the code is not a measurement, and the
machine in question is not the one the code was written on, so the argument cannot be settled
from here.

This settles it there. It reports three things, and it is the THIRD that decides the question:

  - what our process() costs, timed with the same clock Rack times it with
  - how much of that is Rack's own accounting, got by timing two clock reads back to back
  - what is actually running, counted rather than assumed

If the work is twenty nanoseconds and a pair of clock reads is sixty, then a meter that brackets
every module with two clock reads is mostly reporting itself, and the ten per cent is not ours.
If the work is genuinely large, the switches below say which part of it — turn each subsystem off
in turn and watch Rack's own meter under the module move, which is the same experiment done from
the other end.

NOTHING IS MEASURED UNTIL THE WINDOW IS OPEN. Off, this costs one atomic load per sample, which is
the load the module already does.
*/

#include "plugin.hpp"


/** The pieces of audio-rate work, each of which can be switched off on its own so that the cost
can be bisected rather than guessed at. */
enum DiagPart {
	/** THE WHOLE OF process(), AND THE ONLY ONE THAT SAYS ANYTHING WHEN NOTHING IS ATTACHED.

	FIRST, because it is the one to reach for. The four below switch off work that is already
	being skipped when no clip exists, so in the case actually being reported — high CPU with
	nothing clipped on — they change nothing and would read as a window that does not work. This
	one makes the module return at its first line, doing literally nothing per sample beyond
	writing its output.

	If Rack's own meter still reads high with this off, the cost is provably not ours. What is
	left is what Rack spends per module whatever the module does: the two clock reads it brackets
	every module with, and stepping the plug lights of our twenty-one ports. */
	DIAG_BODY,

	/** The parts, for bisecting a rack that DOES have clips on it. */
	DIAG_CAPTURE,     /**< the taps: what a scope and an analyser read */
	DIAG_INJECT,      /**< the injectors' ramps and generators */
	DIAG_MONITOR,     /**< the monitor mix */
	DIAG_METER,       /**< the voltmeters */
	DIAG_FREQ,        /**< the frequency counters */
	NUM_DIAG_PARTS,
};

/** Which parts are allowed to run, as a bitmask — ONE atomic load for all five, read once at the
top of process() rather than five times through five function calls. */
uint32_t diagGate();

/** Whether a part is switched on. For the window; the audio thread uses the mask. */
bool diagPartOn(int part);
void diagSetPart(int part, bool on);
const char* diagPartName(int part);

/** Timing, called around the body of process(). Both return at once unless the window is open. */
void diagBegin();
void diagEnd(float sampleTime);

/** Opens and closes the window. */
void diagShow();
void diagDismiss();
bool diagVisible();
/** True while the window covers this scene position, so our own overlays let it have the click. */
bool diagCovers(math::Vec scenePos);
