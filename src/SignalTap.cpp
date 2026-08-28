/** Audio-rate signal taps — see SignalTap.hpp for why this exists and how it stays safe. */
#include "SignalTap.hpp"

#include <atomic>
#include <cstring>


/** paramIds start here, far above any real param, so a tap can never collide with or steal
a mapping another module has made on the tapped module's params. */
static const int TAP_PARAM_ID_BASE = 1000000;


struct SignalTap {
	/** Written by the UI thread, read every sample by the audio thread. The ONLY gate: the
	audio thread touches nothing else in this slot unless this is true, and it is set last on
	creation and first on destruction. */
	std::atomic<bool> active{false};

	/** The engine owns this while it is registered, and NULLs its `module` field when the
	tapped module is removed — which is the whole reason a handle is used instead of a
	pointer. Set before `active` becomes true and not touched again while it is true. */
	engine::ParamHandle handle;
	bool registered = false;
	int portId = 0;
	bool isOutput = true;

	float buffer[TAP_BUFFER_SIZE] = {};
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


int tapCreate(int64_t moduleId, int portId, bool isOutput) {
	if (moduleId < 0 || portId < 0)
		return -1;

	for (int i = 0; i < TAP_MAX; i++) {
		SignalTap& tap = taps[i];
		if (tap.active.load(std::memory_order_acquire))
			continue;

		tap.portId = portId;
		tap.isOutput = isOutput;
		tap.written.store(0, std::memory_order_relaxed);
		std::memset(tap.buffer, 0, sizeof(tap.buffer));

		if (!tap.registered) {
			APP->engine->addParamHandle(&tap.handle);
			tap.registered = true;
		}
		// overwrite = false: if anything else already holds a handle on this pair we yield
		// rather than break it. With a paramId this high that should never happen.
		APP->engine->updateParamHandle(&tap.handle, moduleId,
			TAP_PARAM_ID_BASE + i, false);
		if (!tap.handle.module)
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
	// First, so the audio thread stops looking at the handle before it is unbound.
	if (tap.active.exchange(false, std::memory_order_acq_rel))
		activeCount.fetch_sub(1, std::memory_order_release);
	if (tap.registered)
		APP->engine->updateParamHandle(&tap.handle, -1, 0, false);
}


bool tapAlive(int slot) {
	if (slot < 0 || slot >= TAP_MAX)
		return false;
	SignalTap& tap = taps[slot];
	return tap.active.load(std::memory_order_acquire) && tap.handle.module != NULL;
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
		Module* module = tap.handle.module;
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

		const uint64_t w = tap.written.load(std::memory_order_relaxed);
		tap.buffer[w & (TAP_BUFFER_SIZE - 1)] = v;
		tap.written.store(w + 1, std::memory_order_release);
	}
}


int tapRead(int slot, float* out, int count) {
	if (slot < 0 || slot >= TAP_MAX || !out || count <= 0)
		return 0;
	SignalTap& tap = taps[slot];
	if (count > TAP_BUFFER_SIZE)
		count = TAP_BUFFER_SIZE;

	const uint64_t w = tap.written.load(std::memory_order_acquire);
	if (w == 0)
		return 0;
	if ((uint64_t) count > w)
		count = (int) w;

	// Oldest first, newest last. The audio thread may overwrite the oldest of these while we
	// copy; on a 170 ms buffer read every frame that cannot reach the samples being drawn.
	const uint64_t start = w - count;
	for (int i = 0; i < count; i++)
		out[i] = tap.buffer[(start + i) & (TAP_BUFFER_SIZE - 1)];
	return count;
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
