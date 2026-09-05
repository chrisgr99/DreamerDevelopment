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

2^19 is about eleven seconds at 48 kHz, or five and a half at 96 — enough to pause a scope and
scroll back through what led up to whatever caught your eye. It costs 2 MB per tap, which is
why the buffer is allocated only for taps that ask for history: an attenuverter reads the
current sample and never looks back, so it takes none.
*/
static const int TAP_BUFFER_SIZE = 1 << 19;

/** Maximum simultaneous taps. Fixed so the audio thread walks a plain array and the UI
thread never resizes anything underneath it. */
static const int TAP_MAX = 32;


/** Registers a tap on a port. UI THREAD ONLY. Returns a slot index, or -1 if all slots are
in use. `isOutput` selects the module's output or input array. */
int tapCreate(int64_t moduleId, int portId, bool isOutput, bool needsHistory = true);

/** Releases a tap. UI THREAD ONLY. Safe while audio is running: the slot is deactivated
first, and the audio thread skips inactive slots. */
void tapDestroy(int slot);

/** True while the tapped module still exists. Goes false by itself when it is deleted. */
bool tapAlive(int slot);

/** AUDIO THREAD. Captures one sample for every active tap. Called from TestGear::process, which
the engine runs once per sample. */
void tapCaptureAll();

/** The port's voltage on one channel, and how many channels it is carrying.

POLYPHONY IS NOT THE SCOPE'S PROBLEM BUT IT IS EVERYBODY ELSE'S. A picture of a waveform is drawn
from the first channel and nobody minds. But anything that ANSWERS a signal — an attenuverter
scaling it, a mute cancelling it — has to answer every channel, or it does its work on the first
note of a chord and leaves the rest untouched. */
float tapVoltage(int slot, int channel);
int tapChannels(int slot);

/** UI THREAD. Copies the most recent samples into `out`, oldest first, newest last. Returns
how many were written, which is `count` unless the tap has not filled that far yet. */
int tapRead(int slot, float* out, int count);

/** UI THREAD. The same, but ending `offset` samples before the newest — which is how a scope
pans back through the history without reading all eleven seconds of it every frame. */
int tapReadAt(int slot, float* out, int count, int offset);

/** How many samples this tap holds that are still worth reading. */
int tapAvailable(int slot);

/** Total samples this tap has ever captured. Lets the UI tell a silent signal from a dead
tap, and is the evidence that capture is running at audio rate rather than frame rate. */
uint64_t tapFrameCount(int slot);

/** The engine sample rate seen by the last capture, for converting samples to seconds. */
float tapSampleRate();

/** AUDIO THREAD. Records the engine's current rate, so the UI can turn samples into time. */
void tapSetSampleRate(float sr);

/** AUDIO THREAD. The tapped port's voltage right now, or zero if the tap is dead.

Separate from the ring buffer because the attenuverter needs THIS sample rather than a window
of history: it reads what a cable is delivering and injects the difference that turns it into
the scaled version. Same ParamHandle and the same bounds check as the buffered capture.
*/
float tapVoltage(int slot);
