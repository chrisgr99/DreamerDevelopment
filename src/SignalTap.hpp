#pragma once
/** Audio-rate signal taps for the scope.

THIS IS THE ONLY PART OF THE PLUGIN THAT RUNS IN THE AUDIO THREAD. Everything else is
visual: get it wrong and something looks odd. Get this wrong and the result is a click, a
dropout, or a crash in the middle of someone's performance. So the capture path allocates
nothing, locks nothing, and touches no container.

WHY IT EXISTS. Reading a port from the UI thread samples at the frame rate — 30 Hz by
default. Sampling a 440 Hz tone thirty times a second does not give a small picture of the
waveform, it gives noise. Seeing the SHAPE of a signal needs capture at the engine's rate.

HOW A PLUGIN READS ANOTHER MODULE'S PORT SAFELY. Not by holding a Module*. When a module is
deleted the pointer is freed, and a plugin gets no notification, so the audio thread would
be reading freed memory until the UI next noticed — up to a frame, which is tens of
thousands of samples. Instead each tap holds a ParamHandle, the SDK's weak handle to another
module: the engine sets its `module` field to NULL as part of removing that module, holding
the same lock that keeps the audio thread out. Checking the handle each sample is therefore
the invalidation the fork got from an engine hook.

The handle names a paramId far above any real param, so it can never collide with, or steal,
a mapping some other module has made on that module's params.
*/

#include "plugin.hpp"


/** Samples held per tap. A power of two so the ring wraps with a mask rather than a modulo.
8192 at 48 kHz is 170 ms, enough to search backwards for a trigger edge on a slow signal. */
static const int TAP_BUFFER_SIZE = 8192;

/** Maximum simultaneous taps. Fixed so the audio thread walks a plain array and the UI
thread never resizes anything underneath it. */
static const int TAP_MAX = 32;


/** Registers a tap on a port. UI THREAD ONLY. Returns a slot index, or -1 if all slots are
in use. `isOutput` selects the module's output or input array. */
int tapCreate(int64_t moduleId, int portId, bool isOutput);

/** Releases a tap. UI THREAD ONLY. Safe while audio is running: the slot is deactivated
first, and the audio thread skips inactive slots. */
void tapDestroy(int slot);

/** True while the tapped module still exists. Goes false by itself when it is deleted. */
bool tapAlive(int slot);

/** AUDIO THREAD. Captures one sample for every active tap. Called from DRUI::process, which
the engine runs once per sample. */
void tapCaptureAll();

/** UI THREAD. Copies the most recent samples into `out`, oldest first, newest last. Returns
how many were written, which is `count` unless the tap has not filled that far yet. */
int tapRead(int slot, float* out, int count);

/** Total samples this tap has ever captured. Lets the UI tell a silent signal from a dead
tap, and is the evidence that capture is running at audio rate rather than frame rate. */
uint64_t tapFrameCount(int slot);

/** The engine sample rate seen by the last capture, for converting samples to seconds. */
float tapSampleRate();

/** AUDIO THREAD. Records the engine's current rate, so the UI can turn samples into time. */
void tapSetSampleRate(float sr);
