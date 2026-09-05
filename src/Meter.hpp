#pragma once
/** Voltmeters: clip one onto a terminal and read what is on it, in volts.

Like the monitor and the scope, it takes no rack space and inserts nothing into the signal: it
taps the port and reads. See Meter.cpp for why it shows one number rather than two.
*/
#include "plugin.hpp"

/** Meters that can exist at once. Generous, since each costs one tap with no history. */
static const int METER_MAX = 16;

/** AUDIO THREAD. Reads every meter's tap and keeps its peak. Called from TestGear::process, and
costs one atomic load when no meter exists.

ON THE AUDIO THREAD BECAUSE A PEAK IS. Sampled once a frame, a meter would miss almost all of an
audio signal and report a number that depends on when it happened to look. */
void meterProcess(float sampleTime);

/** Clips a meter onto a port. UI thread. Either an input or an output. */
void meterCreate(app::PortWidget* port, bool place = true);

/** Every meter's attachment and settings, for saving with the patch. */
json_t* meterToJson();
void meterFromJson(json_t* arrayJ);
void meterRestoreStep();
/** Shows or hides every meter, without removing any. */
void meterSetVisible(bool visible);
