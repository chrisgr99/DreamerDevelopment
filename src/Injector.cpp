/** Signal injectors — see Injector.hpp for how a signal actually reaches the port. */
#include "Injector.hpp"
#include "Clip.hpp"
#include "SignalTap.hpp"
#include "WidgetAt.hpp"

#include <app/CableWidget.hpp>

#include <atomic>
#include <vector>
#include <cmath>
#include <string>


/** One injector's settings, shared between the two threads.

Everything the audio thread READS is atomic and written only by the UI thread. Everything the
audio thread OWNS — phase, and how much of a pulse is left to emit — is touched by nothing
else, so it needs no protection at all.
*/
struct InjectorSlot {
	std::atomic<bool> active{false};
	std::atomic<int> type{INJECT_GATE};
	std::atomic<int> wave{WAVE_SINE};
	/** Volts for DC, and the amplitude of a waveform. */
	std::atomic<float> level{5.f};
	std::atomic<float> rate{2.f};
	std::atomic<bool> gate{false};
	/** Switched off sends a steady zero rather than stopping: a port left holding the last
	value it was given would keep whatever the injector happened to be sending. */
	std::atomic<bool> enabled{true};
	/** Unipolar: the waveform swings from zero up to the level instead of either side of it.
	Same shape, shifted and halved — which is what a modulation source usually wants. */
	std::atomic<bool> unipolar{false};
	/** The attenuverter's tap on whatever is feeding the port, and its gain. Reading the SOURCE
	rather than the port itself is essential: the port already carries our own contribution, so
	reading it would feed us back into ourselves. */
	std::atomic<int> sourceTap{-1};
	std::atomic<int> colour{NOISE_WHITE};
	/** Raised by a click, lowered by the audio thread when it starts the pulse. A counter
	rather than a flag so two quick presses both fire. */
	std::atomic<int> pulseRequests{0};

	// Audio thread only.
	float phase = 0.f;
	float pulseRemaining = 0.f;
	/** Rises to 1 when the injector is sending and falls to 0 when it is not, over a few
	milliseconds. Switching a signal in or out instantaneously puts a step into the audio, and
	a step is a click — audible, and unpleasant through headphones. */
	float gain = 0.f;
	/** Filter state for the coloured noises. */
	float pinkA = 0.f, pinkB = 0.f, pinkC = 0.f;
	float brown = 0.f;
	float lastWhite = 0.f;
};

/** How long the switch-on and switch-off ramps take. Long enough to remove the click, short
enough that a gate still feels immediate. */
static const float RAMP_SECONDS = 0.005f;

static InjectorSlot slots[INJECT_MAX];
static std::atomic<int> activeCount{0};
/** The master switch on DreamRack's face. A switched-off injector is hidden AND silent: one
that went on driving a port while invisible would be a trap rather than a feature. */
static std::atomic<bool> injectorsOn{true};

/** A trigger's width. Rack's own convention, and long enough for every module to see it. */
static const float PULSE_SECONDS = 0.001f;


void injectorProcessAll(Module* drui, float sampleTime) {
	if (activeCount.load(std::memory_order_acquire) <= 0)
		return;
	const bool masterOn = injectorsOn.load(std::memory_order_relaxed);

	for (int i = 0; i < INJECT_MAX; i++) {
		InjectorSlot& slot = slots[i];
		if (!slot.active.load(std::memory_order_acquire))
			continue;
		if (i >= (int) drui->outputs.size())
			continue;

		// The ramp runs whether the injector is sending or not: that is the whole point, since
		// the fall to silence has to be as gradual as the rise from it.
		const float target = (masterOn && slot.enabled.load(std::memory_order_relaxed))
			? 1.f : 0.f;
		const float rampStep = sampleTime / RAMP_SECONDS;
		if (slot.gain < target)
			slot.gain = std::fmin(target, slot.gain + rampStep);
		else if (slot.gain > target)
			slot.gain = std::fmax(target, slot.gain - rampStep);

		if (slot.gain <= 0.f && target <= 0.f) {
			drui->outputs[i].setChannels(1);
			drui->outputs[i].setVoltage(0.f);
			continue;
		}

		const int type = slot.type.load(std::memory_order_relaxed);
		const float level = slot.level.load(std::memory_order_relaxed);
		float v = 0.f;

		switch (type) {
			case INJECT_GATE:
				v = slot.gate.load(std::memory_order_relaxed) ? 10.f : 0.f;
				break;

			case INJECT_PULSE: {
				// Each request starts a pulse. Taking the count down by one, rather than
				// clearing it, means a second press during a pulse still gets its own.
				int requests = slot.pulseRequests.load(std::memory_order_relaxed);
				if (requests > 0 && slot.pulseRemaining <= 0.f) {
					slot.pulseRequests.fetch_sub(1, std::memory_order_relaxed);
					slot.pulseRemaining = PULSE_SECONDS;
				}
				if (slot.pulseRemaining > 0.f) {
					slot.pulseRemaining -= sampleTime;
					v = 10.f;
				}
			} break;

			case INJECT_DC:
			// A pitch IS a steady voltage; only the way it is set and shown differs.
			case INJECT_NOTE:
				v = level;
				break;

			case INJECT_CLOCK: {
				// Beats per minute to hertz. The pulse is the same width as the button's, so
				// everything downstream sees the shape it expects however slow the clock is.
				const float hz = std::fmax(0.01f, slot.rate.load(std::memory_order_relaxed) / 60.f);
				slot.phase += hz * sampleTime;
				if (slot.phase >= 1.f) {
					slot.phase -= std::floor(slot.phase);
					slot.pulseRemaining = PULSE_SECONDS;
				}
				if (slot.pulseRemaining > 0.f) {
					slot.pulseRemaining -= sampleTime;
					v = 10.f;
				}
			} break;

			case INJECT_NOISE: {
				// One white sample drives them all; each colour is a filter of it.
				const float w = random::uniform() * 2.f - 1.f;
				switch (slot.colour.load(std::memory_order_relaxed)) {
					case NOISE_PINK: {
						// Paul Kellett's economy pink filter: three one-pole sections summed,
						// which tracks a 3 dB per octave fall closely enough to hear as pink.
						slot.pinkA = 0.99765f * slot.pinkA + w * 0.0990460f;
						slot.pinkB = 0.96300f * slot.pinkB + w * 0.2965164f;
						slot.pinkC = 0.57000f * slot.pinkC + w * 1.0526913f;
						v = (slot.pinkA + slot.pinkB + slot.pinkC + w * 0.1848f) * 0.4f;
					} break;
					case NOISE_BROWN: {
						// White integrated, with a leak so it cannot wander off to a DC offset.
						slot.brown = math::clamp(slot.brown * 0.995f + w * 0.05f, -1.f, 1.f);
						v = slot.brown * 3.f;
					} break;
					case NOISE_BLUE: {
						slot.pinkA = 0.99765f * slot.pinkA + w * 0.0990460f;
						slot.pinkB = 0.96300f * slot.pinkB + w * 0.2965164f;
						slot.pinkC = 0.57000f * slot.pinkC + w * 1.0526913f;
						const float pink = (slot.pinkA + slot.pinkB + slot.pinkC + w * 0.1848f) * 0.4f;
						// Differentiated pink rises 3 dB per octave.
						v = (pink - slot.lastWhite) * 1.4f;
						slot.lastWhite = pink;
					} break;
					case NOISE_VIOLET: {
						// Differentiated white rises 6 dB per octave.
						v = (w - slot.lastWhite) * 0.7f;
						slot.lastWhite = w;
					} break;
					default:
						v = w;
						break;
				}
				v *= level;
			} break;

			case INJECT_AV: {
				// The engine SUMS everything arriving at an input. So to make the port see
				// gain * x when a cable is delivering x, inject (gain - 1) * x and let the
				// summing do the arithmetic — no re-routing, and the user's cable stays exactly
				// where they put it.
				const int src = slot.sourceTap.load(std::memory_order_relaxed);
				if (src >= 0)
					v = (level - 1.f) * tapVoltage(src);
			} break;

			case INJECT_LFO:
			// An audio oscillator is an LFO that is allowed to go fast. Same phase, same
			// waveforms; the range and the way it is dialled are UI matters.
			case INJECT_AUDIO: {
				const float rate = slot.rate.load(std::memory_order_relaxed);
				slot.phase += rate * sampleTime;
				slot.phase -= std::floor(slot.phase);
				const float p = slot.phase;
				switch (slot.wave.load(std::memory_order_relaxed)) {
					case WAVE_TRIANGLE:
						v = (p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p) * level;
						break;
					case WAVE_SQUARE:
						v = (p < 0.5f ? 1.f : -1.f) * level;
						break;
					case WAVE_RAMP:
						v = (2.f * p - 1.f) * level;
						break;
					default:
						v = std::sin(2.f * M_PI * p) * level;
						break;
				}
				if (slot.unipolar.load(std::memory_order_relaxed))
					v = (v + level) / 2.f;
			} break;
		}

		// Declare the channel count, every sample, as any module does for its own outputs. A
		// port whose channels are zero copies NOTHING down its cable however often its voltage
		// is written, and the receiving module sees an unconnected input.
		//
		// It happened to work at first because Engine::addCable sets an output's channels to 1
		// when the first cable arrives — but removing a cable sets it back to 0, so a slot that
		// had ever been disconnected and reconnected went permanently silent. Relying on the
		// engine's courtesy rather than saying what we produce was the mistake.
		drui->outputs[i].setChannels(1);
		drui->outputs[i].setVoltage(v * slot.gain);

	}
}


/** Claims one particular slot, for an injector being restored onto the cable it saved. */
static bool slotAcquireAt(int i) {
	if (i < 0 || i >= INJECT_MAX)
		return false;
	if (slots[i].active.load(std::memory_order_acquire))
		return false;
	slots[i].phase = 0.f;
	slots[i].pulseRemaining = 0.f;
	slots[i].pulseRequests.store(0, std::memory_order_relaxed);
	slots[i].gate.store(false, std::memory_order_relaxed);
	slots[i].enabled.store(true, std::memory_order_relaxed);
	slots[i].active.store(true, std::memory_order_release);
	activeCount.fetch_add(1, std::memory_order_release);
	return true;
}

static int slotAcquire() {
	for (int i = 0; i < INJECT_MAX; i++) {
		if (slots[i].active.load(std::memory_order_acquire))
			continue;
		slots[i].phase = 0.f;
		slots[i].pulseRemaining = 0.f;
		slots[i].pulseRequests.store(0, std::memory_order_relaxed);
		slots[i].gate.store(false, std::memory_order_relaxed);
		slots[i].enabled.store(true, std::memory_order_relaxed);
		slots[i].active.store(true, std::memory_order_release);
		activeCount.fetch_add(1, std::memory_order_release);
		return i;
	}
	return -1;
}

static void slotRelease(int i) {
	if (i < 0 || i >= INJECT_MAX)
		return;
	if (slots[i].active.exchange(false, std::memory_order_acq_rel))
		activeCount.fetch_sub(1, std::memory_order_release);
}


bool injectorAcceptsPort(app::PortWidget* port) {
	return port && port->module && port->type == engine::Port::INPUT;
}


/** DRUI's own module, found by the output ports we gave it. There is only ever one that
matters: injectors are cabled from it. */
static Module* findDruiModule() {
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		if (!mw->module || !mw->model || !mw->model->plugin)
			continue;
		if (mw->model->plugin->slug == "DreamerDevelopment" && mw->model->slug == "DRUI")
			return mw->module;
	}
	return NULL;
}

static app::PortWidget* findDruiOutputWidget(int slot) {
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		if (!mw->model || !mw->model->plugin)
			continue;
		if (mw->model->plugin->slug != "DreamerDevelopment" || mw->model->slug != "DRUI")
			continue;
		for (app::PortWidget* p : mw->getPorts()) {
			if (p->type == engine::Port::OUTPUT && p->portId == slot)
				return p;
		}
	}
	return NULL;
}


// ---- The faces ----

/** The button injectors keep a round face: they are a thing you press, and looking different
from the readouts is the point. */
static const float INJ_SIZE = 46.f;
/** The readout injectors are a panel meter — digits first, sized so they can be read at the
same distance as a module's own display rather than squinted at. */
static const float READ_W = 84.f;
/** Sized to the digits rather than the digits to it: the window is the height of a digit plus
a couple of pixels top and bottom. */
static const float READ_H = 26.f;
static const float READ_PAD = 2.f;
/** Units of scroll per step, so one trackpad glide does not run the whole range. */
static const float SCROLL_PER_STEP = 3.f;

static const NVGcolor LIT_GREEN = nvgRGB(0x3d, 0xe0, 0x7a);
static const NVGcolor OFF_RED = nvgRGB(0xe0, 0x3b, 0x3b);
/** Bypassed: neither on nor off, and not the injector's own doing. */
static const NVGcolor BYPASS_AMBER = nvgRGB(0xe0, 0xa0, 0x3b);


struct InjectorWidget : ClipWidget {
	int slot = -1;
	InjectorType type = INJECT_GATE;
	InjectorWave wave = WAVE_SINE;
	float level = 5.f;
	float rate = 2.f;
	/** Whether it is sending. The border says which: green sending, red not. */
	bool enabled = true;
	/** VFO only: dial and show the pitch as a note name rather than a frequency. The signal is
	identical either way — this is how it is set and read, not what it produces. */
	bool noteMode = false;
	/** Oscillators only: zero-to-level rather than either side of zero. */
	bool unipolar = false;
	NoiseColour colour = NOISE_WHITE;
	/** Set once a press has travelled far enough to be a reposition rather than a click, so
	the two gestures never both fire. */
	bool dragged = false;
	/** Where the decimal point was last drawn, in this widget's coordinates, so a scroll can
	tell which side of it the pointer is on. Measured during drawing rather than guessed,
	because the digits are scaled to fit and the point moves with them. */
	float dotX = READ_W / 2.f;
	/** One digit's width as last drawn, which is the same for every digit in a monospaced
	face. Turns a pointer position into a decimal column. */
	float lastDigitW = 8.f;
	/** Scroll banked but not yet spent. */
	float scrollAccum = 0.f;
	/** The width the frame wants, measured while drawing. Taken from the WIDEST value this
	injector could show rather than the one it is showing, so the frame does not breathe in and
	out as the digits change. */
	float wantWidth = READ_W;
	/** The cable carrying this injector's signal. Owned by Rack, not by us. */
	WeakPtr<app::CableWidget> cable;

	InjectorWidget() {
		setShape();
		offset = math::Vec(28, -56);
	}

	bool isReadout() {
		return type != INJECT_GATE && type != INJECT_PULSE;
	}

	/** The tap on whatever is feeding our port. Only an attenuverter has one. */
	int sourceTapSlot = -1;
	/** What that tap is currently pointed at, so it is only rebuilt when the patch changes. */
	int64_t sourceModuleId = -1;
	int sourcePortId = -1;

	bool isOscillator() {
		return type == INJECT_LFO || type == INJECT_AUDIO;
	}

	bool isClock() {
		return type == INJECT_CLOCK;
	}

	void setColour(NoiseColour c) {
		colour = c;
		if (slot >= 0)
			slots[slot].colour.store(c, std::memory_order_relaxed);
	}

	static const char* colourName(NoiseColour c) {
		switch (c) {
			case NOISE_PINK: return "PINK";
			case NOISE_BROWN: return "BROWN";
			case NOISE_BLUE: return "BLUE";
			case NOISE_VIOLET: return "VIOLET";
			default: return "WHITE";
		}
	}

	/** The pitch this injector is sending, as a MIDI note number. Kept as the voltage, since
	that is what is actually sent — 0 V is C4, an octave is a volt. */
	int noteNumber() {
		return (int) std::lround(level * 12.f) + 60;
	}

	void setNoteNumber(int n) {
		n = math::clamp(n, 0, 127);
		level = (n - 60) / 12.f;
		if (slot >= 0)
			slots[slot].level.store(level, std::memory_order_relaxed);
	}

	static std::string nameOfNote(int n) {
		static const char* names[12] = {"C", "C#", "D", "D#", "E", "F",
			"F#", "G", "G#", "A", "A#", "B"};
		n = math::clamp(n, 0, 127);
		return string::f("%s%d", names[n % 12], n / 12 - 1);
	}

	std::string noteName() {
		return nameOfNote(noteNumber());
	}

	/** The VFO's pitch as a note number, and back. Concert A is 440 Hz at note 69, which is
	the same convention the note injector uses one volt per octave against. */
	int rateNoteNumber() {
		return (int) std::lround(69.f + 12.f * std::log2(std::fmax(rate, 1.f) / 440.f));
	}

	void setRateNoteNumber(int n) {
		n = math::clamp(n, 0, 127);
		rate = math::clamp(440.f * std::pow(2.f, (n - 69) / 12.f), rateMin(), rateMax());
		if (slot >= 0)
			slots[slot].rate.store(rate, std::memory_order_relaxed);
	}

	/** A press-me thing is round and a readout is a rectangle, so which is which is obvious
	before reading anything on it. */
	void setShape() {
		box.size = isReadout() ? math::Vec(READ_W, READ_H) : math::Vec(INJ_SIZE, INJ_SIZE);
		faceHeight = box.size.y;
	}

	~InjectorWidget() {
		releaseSourceTap();
		slotRelease(slot);
		removeCable();
	}

	bool acceptsPort(app::PortWidget* target) override {
		return injectorAcceptsPort(target);
	}

	/** Removing the widget is not enough: RackWidget::removeCable only detaches it from the
	cable container, and CableWidget::onRemove takes away only the plugs. The ENGINE cable is
	dropped by the widget's DESTRUCTOR, so a widget that is removed and not deleted leaves a
	live cable behind, still naming DRUI and the target module.

	That is not a leak that stays quiet. The engine asserts that no cable references a module
	when it is removed, so every abandoned cable turned into an abort the next time either
	module went away — which for DRUI meant quitting Rack. Rack's own code always deletes
	immediately after removing, for exactly this reason.
	*/
	void removeCable() {
		if (cable) {
			app::CableWidget* cw = cable;
			APP->scene->rack->removeCable(cw);
			delete cw;
		}
		cable = NULL;
	}

	/** Lays the hidden cable from DRUI's output for this slot to the target input. */
	bool connectTo(app::PortWidget* target) {
		app::PortWidget* source = findDruiOutputWidget(slot);
		Module* drui = findDruiModule();
		if (!source || !drui || !target->module)
			return false;

		engine::Cable* c = new engine::Cable;
		c->outputModule = drui;
		c->outputId = slot;
		c->inputModule = target->module;
		c->inputId = target->portId;
		APP->engine->addCable(c);

		app::CableWidget* cw = new app::CableWidget;
		cw->setCable(c);
		cw->outputPort = source;
		cw->inputPort = target;
		APP->scene->rack->addCable(cw);
		hideCable(cw);
		cable = cw;
		return true;
	}

	/** The attachment the user should see is the callout, not a patch lead running across the
	rack — so the cable and the plugs at both its ends are hidden. The engine cable underneath
	is untouched and carries the signal exactly as any other does. */
	static void hideCable(app::CableWidget* cw) {
		cw->visible = false;
		if (cw->inputPlug)
			cw->inputPlug->visible = false;
		if (cw->outputPlug)
			cw->outputPlug->visible = false;
	}

	bool reattach(app::PortWidget* target) override {
		removeCable();
		port = target;
		if (!connectTo(target)) {
			WARN("Injector: could not connect to the new port");
			detach();
			return false;
		}
		return true;
	}

	void detach() override {
		removeCable();
		port = NULL;
	}

	/** Keeps the attenuverter's tap pointed at whatever is feeding its port.

	Done every frame rather than at attachment, because the cable feeding a port comes and goes
	— and an attenuverter clipped on before the cable is patched should simply start working
	when it arrives, not need to be attached again.
	*/
	void updateSourceTap() {
		if (type != INJECT_AV || !port) {
			releaseSourceTap();
			return;
		}
		Module* drui = findDruiModule();
		app::CableWidget* feeding = NULL;
		for (app::CableWidget* cw : APP->scene->rack->getCompleteCables()) {
			if (!cw->cable || cw->inputPort != port)
				continue;
			// Our own injections are not the signal being attenuated.
			if (cw->cable->outputModule == drui)
				continue;
			feeding = cw;
			break;
		}

		if (!feeding) {
			releaseSourceTap();
			return;
		}
		const int64_t mid = feeding->cable->outputModule->id;
		const int pid = feeding->cable->outputId;
		if (sourceTapSlot >= 0 && mid == sourceModuleId && pid == sourcePortId)
			return;

		releaseSourceTap();
		// No history: an attenuverter reads this sample and never looks back, so it need not
		// carry two megabytes of buffer.
		sourceTapSlot = tapCreate(mid, pid, true, false);
		sourceModuleId = mid;
		sourcePortId = pid;
		if (slot >= 0)
			slots[slot].sourceTap.store(sourceTapSlot, std::memory_order_relaxed);
	}

	void releaseSourceTap() {
		if (slot >= 0)
			slots[slot].sourceTap.store(-1, std::memory_order_relaxed);
		if (sourceTapSlot >= 0)
			tapDestroy(sourceTapSlot);
		sourceTapSlot = -1;
		sourceModuleId = -1;
		sourcePortId = -1;
	}

	void step() override {
		updateSourceTap();
		if (isReadout() && wantWidth > 0.f && std::fabs(box.size.x - wantWidth) > 0.5f) {
			// Grow and shrink to the LEFT, keeping the right edge where it is. A value going
			// from 9.99 to 10.00 needs another digit, and moving the right edge would carry the
			// captions and the decimal point out from under the pointer that is scrolling them.
			offset.x -= wantWidth - box.size.x;
			box.size.x = wantWidth;
			faceHeight = box.size.y;
		}
		followPort();
		// Rack hides the plugs on creation, but a cable rebuilds them when its ports change,
		// so this keeps them hidden rather than assuming they stayed that way.
		if (cable)
			hideCable(cable);
		ClipWidget::step();
	}

	/** The digits, with as many decimal places as the value is worth and no more: a rate of
	0.25 Hz and one of 40 Hz should not be shown to the same precision. */
	std::string digits() {
		if (type == INJECT_CLOCK)
			return string::f("%.0f", rate);
		if (type == INJECT_NOISE)
			return string::f("%.2f", level);
		if (type == INJECT_NOTE)
			return noteName();
		if (type == INJECT_AUDIO && noteMode)
			return nameOfNote(rateNoteNumber());
		if (type == INJECT_DC || type == INJECT_AV)
			return string::f("%.2f", level);
		if (rate < 10.f)
			return string::f("%.2f", rate);
		if (rate < 100.f)
			return string::f("%.1f", rate);
		return string::f("%.0f", rate);
	}

	/** The top of the caption column: what this injector is, in a few pixels. */
	std::string typeLabel() {
		switch (type) {
			case INJECT_DC: return "DC";
			case INJECT_AV: return "AV";
			case INJECT_NOISE: return colourName(colour);
			case INJECT_CLOCK: return "CLK";
			case INJECT_NOTE: return string::f("%.2f", level);
			case INJECT_LFO: return "LFO";
			case INJECT_AUDIO: return noteMode ? "PITCH" : "VFO";
			default: return "";
		}
	}

	std::string unit() {
		// In note mode the frequency takes the unit's place: the note name is already in the
		// digits, and seeing 440 beside A4 is the point of a test tone.
		if (type == INJECT_AUDIO && noteMode)
			return string::f("%.0f", rate);
		if (type == INJECT_AV)
			return "x";
		if (type == INJECT_NOISE)
			return "V";
		if (type == INJECT_CLOCK)
			return "BPM";
		return isOscillator() ? "Hz" : "V";
	}

	float rateMax() {
		if (type == INJECT_AUDIO)
			return 8000.f;
		if (type == INJECT_CLOCK)
			return 999.f;
		return 100.f;
	}

	float rateMin() {
		if (type == INJECT_AUDIO)
			return 1.f;
		if (type == INJECT_CLOCK)
			return 1.f;
		return 0.01f;
	}

	std::string buttonLabel() {
		return (type == INJECT_GATE) ? "GATE" : "PULSE";
	}

	/** A small picture of the wave, drawn rather than named. A shape is recognised faster than
	a three-letter abbreviation, and it cannot be confused with the digits beside it. */
	static void drawWaveGlyph(NVGcontext* vg, InjectorWave wave, math::Rect r, NVGcolor col,
		bool unipolar) {

		const float x0 = r.pos.x, x1 = r.pos.x + r.size.x;
		// Unipolar sits entirely above its zero line, bipolar straddles it. The picture says
		// which mode is in effect, which is more use than a word would be in this much space.
		const float top = r.pos.y;
		const float bot = unipolar ? r.pos.y + r.size.y * 0.72f : r.pos.y + r.size.y;
		const float mid = (top + bot) / 2.f;

		const float zeroY = unipolar ? r.pos.y + r.size.y : mid;
		nvgBeginPath(vg);
		nvgMoveTo(vg, x0 - 1.f, zeroY);
		nvgLineTo(vg, x1 + 1.f, zeroY);
		nvgStrokeColor(vg, nvgRGBA((int) (col.r * 255), (int) (col.g * 255),
			(int) (col.b * 255), 0x66));
		nvgStrokeWidth(vg, 0.8f);
		nvgStroke(vg);

		nvgBeginPath(vg);
		switch (wave) {
			case WAVE_SQUARE:
				nvgMoveTo(vg, x0, bot);
				nvgLineTo(vg, x0, top);
				nvgLineTo(vg, (x0 + x1) / 2.f, top);
				nvgLineTo(vg, (x0 + x1) / 2.f, bot);
				nvgLineTo(vg, x1, bot);
				break;
			case WAVE_TRIANGLE:
				nvgMoveTo(vg, x0, bot);
				nvgLineTo(vg, (x0 + x1) / 2.f, top);
				nvgLineTo(vg, x1, bot);
				break;
			case WAVE_RAMP:
				nvgMoveTo(vg, x0, bot);
				nvgLineTo(vg, x1, top);
				nvgLineTo(vg, x1, bot);
				break;
			default: {
				const int steps = 12;
				for (int i = 0; i <= steps; i++) {
					const float t = (float) i / steps;
					const float x = x0 + (x1 - x0) * t;
					const float y = mid - std::sin(2.f * M_PI * t) * (bot - top) / 2.f;
					if (i == 0)
						nvgMoveTo(vg, x, y);
					else
						nvgLineTo(vg, x, y);
				}
			} break;
		}
		nvgStrokeColor(vg, col);
		nvgStrokeWidth(vg, 1.3f);
		nvgLineCap(vg, NVG_ROUND);
		nvgLineJoin(vg, NVG_ROUND);
		nvgStroke(vg);
	}

	/** Nothing an injector does reaches a port while DRUI is bypassed: bypass stops process(),
	which is where the voltage is written — while every widget goes on drawing exactly as
	before. That is a genuinely confusing state, since the faces, the jacks and the cable
	dashes all still look alive, so it is worth saying plainly on the face itself. */
	bool bypassed() {
		Module* drui = findDruiModule();
		return drui && drui->isBypassed();
	}

	void drawReadout(const DrawArgs& args) {
		const bool byp = bypassed();
		const NVGcolor border = byp ? BYPASS_AMBER : (enabled ? LIT_GREEN : OFF_RED);

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 1.f, 1.f, box.size.x - 2.f, box.size.y - 2.f, 5.f);
		nvgFillColor(args.vg, nvgRGB(0x0d, 0x11, 0x0e));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, border);
		nvgStrokeWidth(args.vg, 2.f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;

		if (byp) {
			nvgFontFaceId(args.vg, font->handle);
			nvgFontSize(args.vg, 11.f);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(args.vg, BYPASS_AMBER);
			nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, "DRUI BYPASSED", NULL);
			return;
		}
		const std::string text = digits();
		const std::string u = unit();

		// A narrow column on the right carries what the widget is producing, above the unit it
		// is measured in. Everything left of that column is digits.
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 9.f);
		// Wide enough for the widest thing the column carries: for an oscillator that is the
		// wave glyph AND the word beside it, since a sine at 5 Hz and one at 5 kHz are
		// otherwise identical on the face.
		const float labelW = nvgTextBounds(args.vg, 0, 0, typeLabel().c_str(), NULL, NULL);
		const float topW = isOscillator() ? (10.f + 2.f + labelW) : labelW;
		const float rightW = std::fmax(std::fmax(
			nvgTextBounds(args.vg, 0, 0, u.c_str(), NULL, NULL), topW), 12.f) + 2.f;

		// Sized by MEASURING the digits, not by assuming the font size is their height. A font
		// size is the em, and a digit fills only about seven tenths of it — which is why asking
		// for a 30 px font in a 38 px window left the digits looking lost in it.
		float size = 20.f;
		nvgFontSize(args.vg, size);
		float bounds[4] = {};
		nvgTextBounds(args.vg, 0, 0, text.c_str(), NULL, bounds);
		const float measured = bounds[3] - bounds[1];
		const float availH = box.size.y - 2.f * READ_PAD;
		if (measured > 0.f) {
			size *= availH / measured;
			nvgFontSize(args.vg, size);
		}
		float w = nvgTextBounds(args.vg, 0, 0, text.c_str(), NULL, NULL);
		// The captions' column, and the gap the digits keep from it.
		const float colX = box.size.x - READ_PAD - rightW;
		const float DIGIT_GAP = 3.f;
		const float availW = colX - DIGIT_GAP - READ_PAD;
		if (w > availW && w > 0.f) {
			size *= availW / w;
			nvgFontSize(args.vg, size);
			w = nvgTextBounds(args.vg, 0, 0, text.c_str(), NULL, NULL);
		}

		// Right-aligned against the caption column, so the gap is the same three pixels whether
		// the value reads 0.25 or -10.00. Left-aligning left every spare pixel sitting between
		// the last digit and the caption, which is exactly where it was most visible.
		const float x0 = std::fmax(READ_PAD, colX - DIGIT_GAP - w);
		const float y = box.size.y / 2.f;
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, enabled ? LIT_GREEN : nvgRGBA(0x3d, 0xe0, 0x7a, 0x55));

		// Bold by overdrawing. Rack ships no bold monospace, and swapping to a proportional
		// bold would move the digits about as the value changes — which matters here, because
		// where the decimal point sits is what decides the scroll step.
		for (int i = 0; i < 3; i++)
			nvgText(args.vg, x0 + i * 0.35f, y, text.c_str(), NULL);

		// Remember where the point landed, for the scroll.
		const size_t dot = text.find('.');
		dotX = (dot == std::string::npos)
			? x0 + w - nvgTextBounds(args.vg, 0, 0, "0", NULL, NULL)
			: x0 + nvgTextBounds(args.vg, 0, 0, text.c_str(), text.c_str() + dot, NULL);
		lastDigitW = nvgTextBounds(args.vg, 0, 0, "0", NULL, NULL);

		// The right column: what it produces on top, the unit underneath.
		const float rx = box.size.x - READ_PAD;
		const NVGcolor small = nvgRGBA(0x3d, 0xe0, 0x7a, 0xcc);
		nvgFontSize(args.vg, 9.f);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, small);

		const float topY = box.size.y * 0.32f;
		nvgText(args.vg, rx, topY, typeLabel().c_str(), NULL);
		if (isOscillator()) {
			// The glyph sits to the left of the word, both on the top row.
			drawWaveGlyph(args.vg, wave,
				math::Rect(math::Vec(rx - labelW - 2.f - 10.f, topY - 4.f),
					math::Vec(10.f, 8.f)), small, unipolar);
		}

		nvgText(args.vg, rx, box.size.y * 0.72f, u.c_str(), NULL);
	}

	void drawButton(const DrawArgs& args) {
		const math::Vec c = box.size.div(2.f);
		const float r = INJ_SIZE / 2.f - 1.f;

		nvgBeginPath(args.vg);
		nvgCircle(args.vg, c.x, c.y, r);
		nvgFillColor(args.vg, nvgRGB(0x14, 0x17, 0x1c));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, bypassed() ? BYPASS_AMBER : CLIP_CALLOUT_COLOR);
		nvgStrokeWidth(args.vg, 1.6f);
		nvgStroke(args.vg);

		const bool lit = (type == INJECT_GATE && slot >= 0)
			? slots[slot].gate.load(std::memory_order_relaxed)
			: false;
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, c.x, c.y - 3.f, r * 0.52f);
		nvgFillColor(args.vg, lit ? LIT_GREEN : nvgRGB(0x3d, 0x42, 0x4a));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGB(0x6b, 0x70, 0x79));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (font && font->handle >= 0) {
			nvgFontFaceId(args.vg, font->handle);
			nvgFontSize(args.vg, 8.f);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(args.vg, nvgRGB(0xd8, 0xdc, 0xe2));
			nvgText(args.vg, c.x, box.size.y - 8.f, buttonLabel().c_str(), NULL);
		}
	}

	void draw(const DrawArgs& args) override {
		drawCallout(args.vg);
		if (isReadout())
			drawReadout(args);
		else
			drawButton(args);
		ClipWidget::draw(args);
	}

	/** Scroll adjusts the dial. Deliberately the same gesture as everywhere else in this
	plugin, and the reason a value can be set without a drag. */
	/** Scroll adjusts the value, by an amount taken from WHERE the pointer is: over the digits
	left of the decimal point it moves in tenths, over those right of it in hundredths. The
	pointer is already on the digit whose size it should change, so the widget need not carry a
	coarse-or-fine mode at all.

	Banked and spent three units at a time, because a trackpad glide delivers a stream of small
	deltas rather than one notch, and one step per event ran away with the value.
	*/
	bool usingNotes() {
		return type == INJECT_NOTE || (type == INJECT_AUDIO && noteMode);
	}

	void onHoverScroll(const HoverScrollEvent& e) override {
		if (e.scrollDelta.y == 0.f || slot < 0 || !isReadout())
			return;

		// A semitone is a large step compared with a hundredth of a volt, so notes need nine
		// times as much scroll per step as the numeric readouts — the same gesture then moves
		// about as far musically as it does numerically.
		const float per = usingNotes() ? SCROLL_PER_STEP * 9.f : SCROLL_PER_STEP;

		scrollAccum += e.scrollDelta.y;
		while (std::fabs(scrollAccum) >= per) {
			const int dir = (scrollAccum > 0.f) ? 1 : -1;
			scrollAccum -= dir * per;
			stepValue(dir, e.pos.x);
		}
		e.consume(this);
	}

	/** One step of the value, its size taken from WHERE the pointer is.

	Over the digits left of the decimal point the step is a tenth, over those right of it a
	hundredth — the pointer is already on the digit whose size it should change, so there is no
	coarse-or-fine mode to remember.

	Two things need more than that. An audio oscillator spans 1 Hz to 8 kHz, where tenths would
	take four thousand steps to cross, so each digit steps by its OWN place value: over the
	thousands it moves in thousands, over the units in units. And a note has no decimal point at
	all, so the octave digit at the end moves by octaves and the rest by semitones.
	*/
	void stepValue(int dir, float pointerX) {
		if (type == INJECT_NOTE) {
			// The last character is the octave; everything before it names the note.
			setNoteNumber(noteNumber() + dir * (pointerX >= dotX ? 12 : 1));
			return;
		}

		if (type == INJECT_CLOCK) {
			// Whole beats per minute, stepped by the place value of the digit under the pointer:
			// hundreds over the hundreds column, units over the units.
			rate = math::clamp(std::round(rate + dir * std::fmax(1.f, placeValueAt(pointerX))),
				rateMin(), rateMax());
			slots[slot].rate.store(rate, std::memory_order_relaxed);
			return;
		}

		if (type == INJECT_AUDIO) {
			if (noteMode) {
				setRateNoteNumber(rateNoteNumber() + dir * (pointerX >= dotX ? 12 : 1));
				return;
			}
			rate = math::clamp(rate + dir * placeValueAt(pointerX), rateMin(), rateMax());
			slots[slot].rate.store(rate, std::memory_order_relaxed);
			return;
		}

		const float step = (pointerX < dotX) ? 0.1f : 0.01f;
		if (type == INJECT_AV) {
			// Unity down through zero to full inversion, and up to twice — the range an
			// attenuverter's knob usually covers.
			level = math::clamp(level + dir * step, -2.f, 2.f);
			slots[slot].level.store(level, std::memory_order_relaxed);
			return;
		}
		if (type == INJECT_NOISE) {
			level = math::clamp(level + dir * step, 0.f, 10.f);
			slots[slot].level.store(level, std::memory_order_relaxed);
			return;
		}
		if (type == INJECT_DC) {
			level = math::clamp(level + dir * step, -10.f, 10.f);
			slots[slot].level.store(level, std::memory_order_relaxed);
		}
		else {
			rate = math::clamp(rate + dir * step, rateMin(), rateMax());
			slots[slot].rate.store(rate, std::memory_order_relaxed);
		}
	}

	/** The place value of the digit under the pointer: 1000 over the thousands column, 1 over
	the units, 0.1 over the first decimal. Worked out from the digit width, which is safe to do
	because the readout font is monospaced. */
	float placeValueAt(float pointerX) {
		if (lastDigitW <= 0.f)
			return 1.f;
		// Whole digits to the left of the point, counted outwards from it.
		const float fromDot = dotX - pointerX;
		if (fromDot >= 0.f) {
			const int place = (int) (fromDot / lastDigitW);
			return std::pow(10.f, (float) place);
		}
		const int place = (int) (-fromDot / lastDigitW);
		return std::pow(10.f, -(float) (place + 1));
	}

	/** A press starts BOTH possibilities: it may become a reposition or stay a click. Which it
	was is decided on release, by whether the pointer travelled. */
	void onDragStart(const DragStartEvent& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT)
			dragged = false;
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		// Screen pixels to rack distance, or the widget outruns the pointer when zoomed in.
		const math::Vec d = e.mouseDelta.div(getAbsoluteZoom());
		if (!dragged && d.norm() < 0.5f)
			return;
		if (!dragged) {
			dragged = true;
			// A gate held down while the widget is being moved would be left high by a drag
			// that ends somewhere else, so the press is given up as soon as it becomes a drag.
			if (slot >= 0)
				slots[slot].gate.store(false, std::memory_order_relaxed);
		}
		offset = offset.plus(d);
	}

	void onDragEnd(const DragEndEvent& e) override {
		if (slot >= 0 && type == INJECT_GATE)
			slots[slot].gate.store(false, std::memory_order_relaxed);
		// A click that did not travel switches a readout in or out of circuit. The buttons act
		// on the press instead, since a gate has to rise the moment it is pressed.
		if (!dragged && isReadout() && e.button == GLFW_MOUSE_BUTTON_LEFT)
			setEnabled(!enabled);
		dragged = false;
	}

	void setUnipolar(bool on) {
		unipolar = on;
		if (slot >= 0)
			slots[slot].unipolar.store(on, std::memory_order_relaxed);
	}

	void setEnabled(bool on) {
		enabled = on;
		if (slot >= 0)
			slots[slot].enabled.store(on, std::memory_order_relaxed);
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			ui::Menu* menu = createMenu();
			appendContextMenu(menu);
			e.consume(this);
			return;
		}
		if (slot >= 0 && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0) {
			if (type == INJECT_GATE) {
				// Held, not toggled: pressed is high and released is low, which is what a gate
				// button on a panel does.
				slots[slot].gate.store(e.action == GLFW_PRESS, std::memory_order_relaxed);
				e.consume(this);
				return;
			}
			if (type == INJECT_PULSE && e.action == GLFW_PRESS) {
				slots[slot].pulseRequests.fetch_add(1, std::memory_order_relaxed);
				e.consume(this);
				return;
			}
		}
		ClipWidget::onButton(e);
	}

	void setType(InjectorType t) {
		const InjectorType was = type;
		type = t;
		// Coming into a type for the first time, start somewhere useful rather than at a value
		// carried over from what it used to be: 2 Hz is a fine LFO and a silent oscillator.
		if (was != t) {
			// Concert A, whether it is dialled by frequency or by note. The old test only
			// corrected a rate BELOW the audio range, so a VFO made from the menu inherited the
			// LFO's 2 Hz default and started inaudible.
			if (t == INJECT_AUDIO)
				rate = 440.f;
			if (t == INJECT_LFO && rate > 100.f)
				rate = 2.f;
			if (t == INJECT_NOTE)
				setNoteNumber(noteNumber());
			// Unity gain passes the signal through unchanged, which is the only sane place for
			// an attenuverter to start.
			if (t == INJECT_CLOCK) {
				rate = 120.f;
				if (slot >= 0)
					slots[slot].rate.store(rate, std::memory_order_relaxed);
			}
			if (t == INJECT_NOISE) {
				level = 5.f;
				if (slot >= 0)
					slots[slot].level.store(level, std::memory_order_relaxed);
			}
			if (t == INJECT_AV) {
				level = 1.f;
				if (slot >= 0)
					slots[slot].level.store(level, std::memory_order_relaxed);
			}
		}
		setShape();
		rate = math::clamp(rate, rateMin(), rateMax());
		if (slot >= 0) {
			slots[slot].type.store(t, std::memory_order_relaxed);
			slots[slot].rate.store(rate, std::memory_order_relaxed);
			slots[slot].gate.store(false, std::memory_order_relaxed);
		}
	}

	/** The widget's own properties, and nothing else.

	Changing an injector's TYPE has gone from here deliberately. The type is chosen when the
	widget is made, from the Option-click menu on the jack, so this menu is free to carry what
	each kind of injector actually needs — a waveform, a way of reading a pitch — rather than
	spending its first six lines on a list that is settled by then.
	*/
	void appendContextMenu(ui::Menu* menu) {
		menu->addChild(createMenuLabel(typeLabel()));

		if (isOscillator()) {
			menu->addChild(new ui::MenuSeparator);
			menu->addChild(createMenuLabel("Waveform"));
			static const char* waveNames[WAVE_COUNT] = {"Sine", "Triangle", "Square", "Ramp"};
			for (int w = 0; w < WAVE_COUNT; w++) {
				menu->addChild(createCheckMenuItem(waveNames[w], "",
					[this, w]() { return wave == w; },
					[this, w]() {
						wave = (InjectorWave) w;
						if (slot >= 0)
							slots[slot].wave.store(w, std::memory_order_relaxed);
					}));
			}
		}

		if (isOscillator()) {
			menu->addChild(new ui::MenuSeparator);
			menu->addChild(createMenuLabel("Swing"));
			menu->addChild(createCheckMenuItem("Bipolar", "",
				[this]() { return !unipolar; },
				[this]() { setUnipolar(false); }));
			menu->addChild(createCheckMenuItem("Unipolar", "",
				[this]() { return unipolar; },
				[this]() { setUnipolar(true); }));
		}

		if (type == INJECT_NOISE) {
			menu->addChild(new ui::MenuSeparator);
			menu->addChild(createMenuLabel("Colour"));
			static const NoiseColour colours[NOISE_COUNT] = {NOISE_WHITE, NOISE_PINK,
				NOISE_BROWN, NOISE_BLUE, NOISE_VIOLET};
			static const char* names[NOISE_COUNT] = {"White", "Pink", "Brown (red)",
				"Blue", "Violet"};
			for (int i = 0; i < NOISE_COUNT; i++) {
				const NoiseColour c = colours[i];
				menu->addChild(createCheckMenuItem(names[i], "",
					[this, c]() { return colour == c; },
					[this, c]() { setColour(c); }));
			}
		}

		if (type == INJECT_AUDIO) {
			menu->addChild(new ui::MenuSeparator);
			menu->addChild(createMenuLabel("Dial by"));
			menu->addChild(createCheckMenuItem("Frequency", "",
				[this]() { return !noteMode; },
				[this]() { noteMode = false; }));
			menu->addChild(createCheckMenuItem("Note", "",
				[this]() { return noteMode; },
				[this]() {
					noteMode = true;
					// Snap to the nearest note, so what is shown is what is sent.
					setRateNoteNumber(rateNoteNumber());
				}));
		}

		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createCheckMenuItem("Sending", "",
			[this]() { return enabled; },
			[this]() { setEnabled(!enabled); }));
		menu->addChild(createMenuItem("Remove", "", [this]() { detach(); }));
	}

	void onHoverKey(const HoverKeyEvent& e) override {
		if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ESCAPE) {
			detach();
			e.consume(this);
			return;
		}
		ClipWidget::onHoverKey(e);
	}

	json_t* toJson() {
		json_t* rootJ = json_object();
		if (port && port->module) {
			json_object_set_new(rootJ, "moduleId", json_integer(port->module->id));
			json_object_set_new(rootJ, "portId", json_integer(port->portId));
		}
		json_object_set_new(rootJ, "type", json_integer(type));
		json_object_set_new(rootJ, "wave", json_integer(wave));
		json_object_set_new(rootJ, "level", json_real(level));
		json_object_set_new(rootJ, "rate", json_real(rate));
		json_object_set_new(rootJ, "enabled", json_boolean(enabled));
		json_object_set_new(rootJ, "noteMode", json_boolean(noteMode));
		json_object_set_new(rootJ, "unipolar", json_boolean(unipolar));
		json_object_set_new(rootJ, "colour", json_integer(colour));
		json_object_set_new(rootJ, "offsetX", json_real(offset.x));
		json_object_set_new(rootJ, "offsetY", json_real(offset.y));
		return rootJ;
	}

	void fromJson(json_t* rootJ) {
		if (json_t* j = json_object_get(rootJ, "type"))
			setType((InjectorType) json_integer_value(j));
		if (json_t* j = json_object_get(rootJ, "wave")) {
			wave = (InjectorWave) json_integer_value(j);
			if (slot >= 0)
				slots[slot].wave.store(wave, std::memory_order_relaxed);
		}
		if (json_t* j = json_object_get(rootJ, "level")) {
			level = json_number_value(j);
			if (slot >= 0)
				slots[slot].level.store(level, std::memory_order_relaxed);
		}
		if (json_t* j = json_object_get(rootJ, "rate")) {
			rate = json_number_value(j);
			if (slot >= 0)
				slots[slot].rate.store(rate, std::memory_order_relaxed);
		}
		if (json_t* j = json_object_get(rootJ, "enabled"))
			setEnabled(json_boolean_value(j));
		if (json_t* j = json_object_get(rootJ, "noteMode"))
			noteMode = json_boolean_value(j);
		if (json_t* j = json_object_get(rootJ, "unipolar"))
			setUnipolar(json_boolean_value(j));
		if (json_t* j = json_object_get(rootJ, "colour"))
			setColour((NoiseColour) json_integer_value(j));
		if (json_t* j = json_object_get(rootJ, "offsetX"))
			offset.x = json_number_value(j);
		if (json_t* j = json_object_get(rootJ, "offsetY"))
			offset.y = json_number_value(j);
	}
};


/** Whether some injector already owns this cable. */
static bool cableIsOwned(app::CableWidget* cw) {
	for (widget::Widget* child : APP->scene->rack->children) {
		InjectorWidget* inj = dynamic_cast<InjectorWidget*>(child);
		if (inj && inj->cable == cw)
			return true;
	}
	return false;
}

/** The cable a RESTORED injector should adopt: one already running from a DRUI output into
this port, that no other injector has claimed.

It exists because an injector's cable is an ordinary patch cable, so Rack restores it with the
patch — and laying another would double the injected signal every time the patch was opened.
Skipping the ones already claimed is what lets two injectors on the same port each find their
own.
*/
static app::CableWidget* existingInjectorCable(app::PortWidget* port, Module* drui) {
	for (app::CableWidget* cw : APP->scene->rack->getCompleteCables()) {
		if (!cw->cable || cw->inputPort != port)
			continue;
		if (cw->cable->outputModule == drui && !cableIsOwned(cw))
			return cw;
	}
	return NULL;
}


/** `adopt` is true only when restoring a saved patch. Creating one by hand must always lay a
NEW cable: adopting there meant a second injector on a port took over the first one's cable
and then failed, because its slot was already in use — so a port could only ever carry one.
The engine sums cables on an input, which is the whole reason injectors work this way, and
stacking a DC offset under a waveform is exactly what that is for. */
static InjectorWidget* injectorMake(app::PortWidget* port, InjectorType type,
	bool adopt = false) {
	if (!injectorAcceptsPort(port))
		return NULL;
	Module* drui = findDruiModule();
	if (!drui) {
		WARN("Injector: no DRUI module in the patch to inject from");
		return NULL;
	}

	InjectorWidget* inj = new InjectorWidget;
	app::CableWidget* existing = adopt ? existingInjectorCable(port, drui) : NULL;
	if (existing && existing->cable) {
		// Adopt the restored cable, and with it the slot it was already using.
		const int slot = existing->cable->outputId;
		if (!slotAcquireAt(slot)) {
			WARN("Injector: slot %d is already in use, cannot adopt its cable", slot);
			delete inj;
			return NULL;
		}
		inj->slot = slot;
		inj->cable = existing;
		InjectorWidget::hideCable(existing);
	}
	else {
		const int slot = slotAcquire();
		if (slot < 0) {
			WARN("Injector: all %d slots are in use", INJECT_MAX);
			delete inj;
			return NULL;
		}
		inj->slot = slot;
		if (!inj->connectTo(port)) {
			WARN("Injector: could not lay its cable");
			slotRelease(slot);
			delete inj;
			return NULL;
		}
	}

	inj->port = port;
	inj->setType(type);
	APP->scene->rack->addChild(inj);
	clipAddHandle(inj);
	clipAddClose(inj);
	INFO("Injector: attached to input port %d in slot %d", port->portId, inj->slot);
	return inj;
}


void injectorCreate(app::PortWidget* port, InjectorType type, bool noteMode) {
	if (InjectorWidget* inj = injectorMake(port, type)) {
		if (noteMode && type == INJECT_AUDIO) {
			inj->noteMode = true;
			inj->setRateNoteNumber(inj->rateNoteNumber());
		}
	}
}


// ---- Saving with the patch ----

struct PendingInjector {
	int64_t moduleId = -1;
	int portId = 0;
	json_t* stateJ = NULL;
	int budget = 300;
};

static std::vector<PendingInjector> pendingInjectors;


json_t* injectorToJson() {
	json_t* arrayJ = json_array();
	for (widget::Widget* child : APP->scene->rack->children) {
		InjectorWidget* inj = dynamic_cast<InjectorWidget*>(child);
		if (inj && inj->port)
			json_array_append_new(arrayJ, inj->toJson());
	}
	for (const PendingInjector& p : pendingInjectors) {
		if (p.stateJ)
			json_array_append(arrayJ, p.stateJ);
	}
	return arrayJ;
}


void injectorFromJson(json_t* arrayJ) {
	for (PendingInjector& p : pendingInjectors) {
		if (p.stateJ)
			json_decref(p.stateJ);
	}
	pendingInjectors.clear();
	if (!arrayJ || !json_is_array(arrayJ))
		return;

	size_t i;
	json_t* injJ;
	json_array_foreach(arrayJ, i, injJ) {
		json_t* moduleIdJ = json_object_get(injJ, "moduleId");
		if (!moduleIdJ)
			continue;
		PendingInjector p;
		p.moduleId = json_integer_value(moduleIdJ);
		if (json_t* j = json_object_get(injJ, "portId"))
			p.portId = json_integer_value(j);
		p.stateJ = json_incref(injJ);
		pendingInjectors.push_back(p);
	}
}


static app::PortWidget* findInputPort(int64_t moduleId, int portId) {
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		if (!mw->module || mw->module->id != moduleId)
			continue;
		for (app::PortWidget* p : mw->getPorts()) {
			if (p->type == engine::Port::INPUT && p->portId == portId)
				return p;
		}
		return NULL;
	}
	return NULL;
}


/** Removes cables running from DRUI that no injector owns.

They accumulate because an injector's cable is an ORDINARY patch cable, which is what makes
Rack responsible for its lifetime — and also means Rack saves it. Reopening a patch therefore
restores every such cable, while only the injectors that were saved come back to claim them.
Anything else — a cable whose injector was removed after the save, or a duplicate laid by an
earlier bug — is left with no owner: still connected, still injecting, and now visible,
because the widget that was hiding it is gone. That is what put an apparent jack with cables
hanging off it on DRUI's panel.

So the invariant is enforced from the other end: a cable out of DRUI that no injector claims
does not exist. Run only once the restore queue is empty, or this would delete the cables the
injectors are still waiting to adopt.
*/
void injectorSetEnabled(bool on) {
	injectorsOn.store(on, std::memory_order_relaxed);
	for (widget::Widget* child : APP->scene->rack->children) {
		if (InjectorWidget* inj = dynamic_cast<InjectorWidget*>(child))
			clipSetVisible(inj, on);
	}
}


void injectorPurgeStrayCables() {
	if (!pendingInjectors.empty())
		return;
	Module* drui = findDruiModule();
	if (!drui)
		return;

	std::vector<app::CableWidget*> strays;
	for (app::CableWidget* cw : APP->scene->rack->getCompleteCables()) {
		if (!cw->cable || cw->cable->outputModule != drui)
			continue;
		bool owned = false;
		for (widget::Widget* child : APP->scene->rack->children) {
			InjectorWidget* inj = dynamic_cast<InjectorWidget*>(child);
			if (inj && inj->cable == cw) {
				owned = true;
				break;
			}
		}
		if (!owned)
			strays.push_back(cw);
	}

	for (app::CableWidget* cw : strays) {
		INFO("Injector: removing an unowned cable from DRUI output %d", cw->cable->outputId);
		APP->scene->rack->removeCable(cw);
		delete cw;
	}
}


void injectorRestoreStep() {
	if (pendingInjectors.empty())
		return;

	for (size_t i = 0; i < pendingInjectors.size();) {
		PendingInjector& p = pendingInjectors[i];
		app::PortWidget* port = findInputPort(p.moduleId, p.portId);
		if (port && findDruiModule()) {
			if (InjectorWidget* inj = injectorMake(port, INJECT_GATE, true))
				inj->fromJson(p.stateJ);
			json_decref(p.stateJ);
			pendingInjectors.erase(pendingInjectors.begin() + i);
			continue;
		}
		if (--p.budget <= 0) {
			WARN("Injector: module %lld never appeared, dropping its injector",
				(long long) p.moduleId);
			json_decref(p.stateJ);
			pendingInjectors.erase(pendingInjectors.begin() + i);
			continue;
		}
		i++;
	}
}
