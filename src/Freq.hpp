#pragma once
/** Frequency meters: clip one onto a terminal and read what pitch is on it.

Like the voltmeter, it takes no rack space and inserts nothing into the signal: it taps the port
and reads. See Freq.cpp for how the pitch is found and why it is found on the audio thread.
*/
#include "plugin.hpp"

/** Frequency meters that can exist at once. Fewer than voltmeters because each one carries a
little state per sample, and because wanting eight pitches at once is already unusual. */
static const int FREQ_MAX = 8;

/** AUDIO THREAD. Advances every frequency meter's crossing detector. Called from
TestGear::process, and costs one atomic load when no meter exists.

ON THE AUDIO THREAD BECAUSE A PERIOD IS. A frequency is the time between two crossings, and a
reading taken once a frame can see neither. */
void freqProcess(float sampleTime);

/** Clips a frequency meter onto a port. UI thread. Either an input or an output. */
void freqCreate(app::PortWidget* port, bool place = true);

/** Every frequency meter's attachment and settings, for saving with the patch. */
json_t* freqToJson();
void freqFromJson(json_t* arrayJ);
void freqRestoreStep();
/** Shows or hides every frequency meter, without removing any. */
void freqSetVisible(bool visible);
