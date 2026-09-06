/** Audio-rate signal taps — see SignalTap.hpp for why this exists and how it stays safe. */
#include "SignalTap.hpp"

#include <atomic>
#include <cstring>
#include <vector>
#include <algorithm>


/** paramIds start here, far above any real param, so a tap can never collide with or steal
a mapping another module has made on the tapped module's params. */
static const int TAP_PARAM_ID_BASE = 1000000;

/** ONE ID PER HANDLE, NEVER REUSED — not one per slot.

A handle the engine has taken back is abandoned rather than freed, because there is no way to
ask the engine whether it still holds one. An abandoned handle may still be BOUND, and while it
is, it occupies its module and parameter id. Numbering by slot meant the replacement handle for
that slot asked for the very key its own abandoned predecessor was sitting on; the engine, told
not to overwrite, left the new one unbound; and the tap read nothing at all. An attenuverter
that had stopped attenuating was the first sign of it.

Counting instead of reusing costs one integer and cannot collide. */
static int gNextParamId = 0;


struct SignalTap {
	/** Written by the UI thread, read every sample by the audio thread. The ONLY gate: the
	audio thread touches nothing else in this slot unless this is true, and it is set last on
	creation and first on destruction. */
	std::atomic<bool> active{false};

	/** The engine owns this once it has been added, and NULLs its `module` field when the
	tapped module is removed — which is the whole reason a handle is used instead of a pointer.
	Set before `active` becomes true and not touched again while it is true.

	A POINTER, AND NEVER GIVEN BACK. See the note above tapCreate: a handle can only be handed
	to the engine, never taken from it, so a slot whose handle the engine has already discarded
	takes a fresh one and abandons the old. */
	engine::ParamHandle* handle = NULL;
	int portId = 0;
	bool isOutput = true;

	/** Allocated only when the tap is asked for history, and never freed while the plugin
	lives: freeing it would have to be co-ordinated with an audio thread that may be reading
	it, and 2 MB held per slot that has ever carried a scope is the cheaper problem. */
	std::vector<float> buffer;
	/** Total samples captured; the ring position is this masked. Released by the audio thread
	after each sample is written and acquired by the UI thread before reading, which is what
	makes a torn read impossible. */
	std::atomic<uint64_t> written{0};
};


/** A fixed array, never resized, so the audio thread walks plain memory. */
static SignalTap taps[TAP_MAX];

/** How many slots are active. Lets the audio thread return after ONE atomic load in the
overwhelmingly common case of no scope being open, rather than walking all 32 slots every
sample — at 48 kHz that would be 1.5 million pointless loads a second. */
static std::atomic<int> activeCount{0};

static std::atomic<float> lastSampleRate{48000.f};


/** Whether the engine still holds this tap's handle, bound where we last bound it.

RACK'S THREE RULES, WHICH BETWEEN THEM LEAVE ONE SAFE MOVE. addParamHandle asserts on a handle
the engine already holds; updateParamHandle and removeParamHandle assert on one it does not; and
there is no way to ask whether it holds a given handle. Any of those assertions takes the whole
application down.

What makes this hard is that the engine DOES discard them: loading a patch empties its list, and
nothing tells a plugin that it happened. A flag of our own is then a lie, and both ways of
believing it have crashed Rack today — trusting it crashed on the update, and clearing it
crashed on the add that followed.

So the flag is replaced by a question the engine can actually answer. A handle bound to a module
and parameter can be looked up by that pair: if the answer is ours, the engine still has it and
it may be rebound. Anything else — no answer, somebody else's handle, or a handle of ours that
is bound to nothing and therefore cannot be looked up at all — is treated as lost. A lost handle
is abandoned rather than freed, since the engine may still be holding it, and the slot starts
again with a new one. That costs a few dozen bytes on each patch load and cannot assert. */
static bool engineHolds(SignalTap& tap) {
	return tap.handle && tap.handle->moduleId >= 0
		&& APP->engine->getParamHandle(tap.handle->moduleId, tap.handle->paramId) == tap.handle;
}


int tapCreate(int64_t moduleId, int portId, bool isOutput, bool needsHistory) {
	if (moduleId < 0 || portId < 0)
		return -1;

	for (int i = 0; i < TAP_MAX; i++) {
		SignalTap& tap = taps[i];
		if (tap.active.load(std::memory_order_acquire))
			continue;

		tap.portId = portId;
		tap.isOutput = isOutput;
		tap.written.store(0, std::memory_order_relaxed);
		if (needsHistory) {
			if ((int) tap.buffer.size() != TAP_BUFFER_SIZE)
				tap.buffer.assign(TAP_BUFFER_SIZE, 0.f);
			else
				std::fill(tap.buffer.begin(), tap.buffer.end(), 0.f);
		}

		// The one the engine is known to have, or a new one it certainly has not.
		if (!engineHolds(tap))
			tap.handle = NULL;
		if (!tap.handle) {
			tap.handle = new engine::ParamHandle;
			APP->engine->addParamHandle(tap.handle);
		}
		// overwrite = false: if anything else already holds a handle on this pair we yield
		// rather than break it. With an id this high, and never reused, that cannot happen.
		APP->engine->updateParamHandle(tap.handle, moduleId,
			TAP_PARAM_ID_BASE + (gNextParamId++), false);
		if (!tap.handle->module)
			WARN("Tap: the engine would not bind slot %d to module %lld", i,
				(long long) moduleId);
		if (!tap.handle->module)
			return -1;

		// Last, and with release ordering, so the audio thread cannot see an active slot
		// whose fields are not yet written.
		tap.active.store(true, std::memory_order_release);
		activeCount.fetch_add(1, std::memory_order_release);
		return i;
	}
	return -1;
}


void tapDestroy(int slot) {
	if (slot < 0 || slot >= TAP_MAX)
		return;
	SignalTap& tap = taps[slot];
	// The audio thread stops looking at the handle, and that is all that happens.
	//
	// THE BINDING IS LEFT ALONE DELIBERATELY. Unbinding it is a call that asserts if the engine
	// has already discarded the handle, which is every patch load, and it buys nothing: nothing
	// reads a tap that is not active, and leaving it bound is what lets the next tap on this
	// slot recognise its own handle and reuse it instead of abandoning it. The engine unbinds
	// it by itself when the module it points at goes away.
	if (tap.active.exchange(false, std::memory_order_acq_rel))
		activeCount.fetch_sub(1, std::memory_order_release);
}


bool tapAlive(int slot) {
	if (slot < 0 || slot >= TAP_MAX)
		return false;
	SignalTap& tap = taps[slot];
	return tap.active.load(std::memory_order_acquire) && tap.handle && tap.handle->module;
}


void tapCaptureAll() {
	// The entire cost of this feature when no scope is open.
	if (activeCount.load(std::memory_order_acquire) <= 0)
		return;

	for (int i = 0; i < TAP_MAX; i++) {
		SignalTap& tap = taps[i];
		if (!tap.active.load(std::memory_order_acquire))
			continue;
		// NULL the moment the engine removes that module, under the lock that keeps this
		// thread out — so this test is the invalidation, not merely a guess at it.
		Module* module = tap.handle ? tap.handle->module : NULL;
		if (!module)
			continue;

		// Bounds-checked every sample rather than trusted from registration: a module can be
		// replaced by one with fewer ports, and reading past an array in the audio thread is
		// not a bug that announces itself politely.
		float v = 0.f;
		if (tap.isOutput) {
			if (tap.portId < (int) module->outputs.size())
				v = module->outputs[tap.portId].getVoltage();
		}
		else {
			if (tap.portId < (int) module->inputs.size())
				v = module->inputs[tap.portId].getVoltage();
		}

		if (tap.buffer.empty())
			continue;   // A tap with no history: nothing to store.
		const uint64_t w = tap.written.load(std::memory_order_relaxed);
		tap.buffer[w & (TAP_BUFFER_SIZE - 1)] = v;
		tap.written.store(w + 1, std::memory_order_release);
	}
}


/** The port this tap is on, or nothing. Bounds-checked every time rather than trusted from
registration: a module can be replaced by one with fewer ports. */
static engine::Port* tapPort(int slot) {
	if (slot < 0 || slot >= TAP_MAX)
		return NULL;
	SignalTap& tap = taps[slot];
	if (!tap.active.load(std::memory_order_acquire))
		return NULL;
	Module* module = tap.handle ? tap.handle->module : NULL;
	if (!module)
		return NULL;
	if (tap.isOutput)
		return (tap.portId < (int) module->outputs.size())
			? &module->outputs[tap.portId] : NULL;
	return (tap.portId < (int) module->inputs.size())
		? &module->inputs[tap.portId] : NULL;
}


float tapVoltage(int slot) {
	engine::Port* p = tapPort(slot);
	return p ? p->getVoltage() : 0.f;
}


float tapVoltage(int slot, int channel) {
	engine::Port* p = tapPort(slot);
	if (!p || channel < 0 || channel >= p->getChannels())
		return 0.f;
	return p->getVoltage(channel);
}


int tapChannels(int slot) {
	engine::Port* p = tapPort(slot);
	return p ? p->getChannels() : 0;
}


int tapReadAt(int slot, float* out, int count, int offset) {
	if (slot < 0 || slot >= TAP_MAX || !out || count <= 0)
		return 0;
	SignalTap& tap = taps[slot];
	if (tap.buffer.empty())
		return 0;
	if (count > TAP_BUFFER_SIZE)
		count = TAP_BUFFER_SIZE;
	if (offset < 0)
		offset = 0;

	const uint64_t w = tap.written.load(std::memory_order_acquire);
	if (w == 0)
		return 0;
	// The newest sample this read may see.
	uint64_t end = ((uint64_t) offset >= w) ? 0 : w - offset;
	if (end == 0)
		return 0;
	if ((uint64_t) count > end)
		count = (int) end;

	// Oldest first, newest last. The audio thread may overwrite the oldest of these while we
	// copy, but only after eleven seconds have gone by, which no frame takes.
	const uint64_t start = end - count;
	for (int i = 0; i < count; i++)
		out[i] = tap.buffer[(start + i) & (TAP_BUFFER_SIZE - 1)];
	return count;
}


int tapRead(int slot, float* out, int count) {
	return tapReadAt(slot, out, count, 0);
}


int tapAvailable(int slot) {
	if (slot < 0 || slot >= TAP_MAX)
		return 0;
	SignalTap& tap = taps[slot];
	if (tap.buffer.empty())
		return 0;
	const uint64_t w = tap.written.load(std::memory_order_acquire);
	return (int) std::min<uint64_t>(w, TAP_BUFFER_SIZE);
}


uint64_t tapFrameCount(int slot) {
	if (slot < 0 || slot >= TAP_MAX)
		return 0;
	return taps[slot].written.load(std::memory_order_acquire);
}


float tapSampleRate() {
	return lastSampleRate.load(std::memory_order_relaxed);
}


void tapSetSampleRate(float sr) {
	if (sr > 0.f)
		lastSampleRate.store(sr, std::memory_order_relaxed);
}
