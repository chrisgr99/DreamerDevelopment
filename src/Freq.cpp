/** Frequency meters clipped onto terminals.

WHAT IT SHOWS. One reading, in one of three units, with a small chip above it saying which and
changing it when clicked — the voltmeter's arrangement, for the same reason: two numbers stacked
would make this widget twice the height of every other readout in the plugin.

    Hz      the pitch of whatever is on the terminal
    NOTE    that same pitch as a note, with its distance from equal temperament in cents
    V/OCT   the note a steady control voltage is ASKING for, which is a different question

The third is not a measurement of pitch at all, and that is the point of having it. A volt per
octave cable carries no frequency to measure — it carries an instruction — so on a tuning patch
the useful reading is the note that instruction names. Nought volts is C4, as everywhere else in
Rack.

HOW THE PITCH IS FOUND. The time between rising crossings of the signal's own slow mean, with a
threshold either side of it so that a wobble around the crossing is not counted as several. That
is the same estimate the scope's autoset uses, moved onto the audio thread and kept running: a
period cannot be measured by looking once a frame, and a meter that sampled at frame rate would
report a number that depended on when it happened to look.

WHAT IT REFUSES TO SAY. A signal with no clear fundamental — a chord, noise, a folded wave — has
no one frequency, and a number invented for it would be believed. So the reading is dashes when
the signal is too small to be anything, when nothing has crossed for a while, and when successive
periods disagree by more than a little. Dashes are a reading.

THE TUNING STANDARD affects the note and the cents, never the hertz. A440 unless the right-click
menu says otherwise, which matters to anybody playing with an ensemble tuned somewhere else.
*/
#include "plugin.hpp"
#include "Freq.hpp"
#include "Clip.hpp"
#include "SignalTap.hpp"

#include <atomic>
#include <cmath>
#include <vector>


/** One meter's state. The audio thread writes the readings and nothing else; the UI thread
writes everything else. */
struct FreqSlot {
	std::atomic<bool> active{false};
	std::atomic<int> tap{-1};
	/** The measured pitch, or nought for "nothing worth reporting". */
	std::atomic<float> hz{0.f};
	/** The signal's slow mean, which is what a control voltage's reading is taken from. */
	std::atomic<float> dc{0.f};
	std::atomic<int> channels{1};

	// Audio thread only.
	float mean = 0.f;          /**< a one-pole average, the line crossings are counted against */
	float env = 0.f;           /**< how far the signal swings either side of that line */
	float sinceCross = 0.f;    /**< seconds since the last rising crossing */
	float period = 0.f;        /**< the accepted period, smoothed */
	bool armed = false;        /**< below the lower threshold, so the next rise counts */
	float quiet = 0.f;         /**< seconds the signal has been too small to read */
};

static FreqSlot slots[FREQ_MAX];
static std::atomic<int> activeCount{0};

/** THE SMALLEST SIGNAL WORTH MEASURING, in volts either side of the mean. Below this the
crossings being counted are noise, and a frequency taken from noise is a random number. */
static const float FREQ_FLOOR = 0.02f;
/** How far either side of the mean the signal must go for a crossing to count, as a fraction of
how far it is swinging. Keeps one crossing from being counted as three on a signal that wobbles
as it passes the line. */
static const float FREQ_HYST = 0.25f;
/** How long the signal may stay too small before the reading is given up. */
static const float FREQ_QUIET = 0.25f;
/** A new period this far from the accepted one is a different signal rather than a bad reading,
so the meter starts again from it instead of averaging the two into a lie. */
static const float FREQ_JUMP = 0.25f;
/** How quickly the accepted period follows a new one. Slow enough to hold still on the last
digit, quick enough to follow a knob being turned. */
static const float FREQ_FOLLOW = 0.3f;


void freqProcess(float sampleTime) {
	if (activeCount.load(std::memory_order_acquire) <= 0)
		return;

	for (int i = 0; i < FREQ_MAX; i++) {
		FreqSlot& slot = slots[i];
		if (!slot.active.load(std::memory_order_acquire))
			continue;
		const int tap = slot.tap.load(std::memory_order_relaxed);
		if (tap < 0)
			continue;

		const float v = tapVoltage(tap);
		slot.channels.store(tapChannels(tap), std::memory_order_relaxed);

		// THE LINE THE CROSSINGS ARE COUNTED AGAINST, and the reading a steady voltage gets.
		// Half a second of memory: slow enough to sit still through a waveform, quick enough
		// that a control voltage moved by hand arrives within a moment.
		const float k = math::clamp(sampleTime / 0.5f, 0.f, 1.f);
		slot.mean += (v - slot.mean) * k;
		slot.dc.store(slot.mean, std::memory_order_relaxed);

		// How far it swings, followed quickly upwards and let go slowly, so the threshold sits
		// at a sensible height on a signal whose level is changing.
		const float swing = std::fabs(v - slot.mean);
		slot.env = (swing > slot.env) ? swing
			: slot.env + (swing - slot.env) * math::clamp(sampleTime / 0.25f, 0.f, 1.f);

		slot.sinceCross += sampleTime;

		if (slot.env < FREQ_FLOOR) {
			slot.quiet += sampleTime;
			if (slot.quiet >= FREQ_QUIET) {
				slot.period = 0.f;
				slot.hz.store(0.f, std::memory_order_relaxed);
			}
			continue;
		}
		slot.quiet = 0.f;

		// A rising crossing: below the lower threshold at some point, then above the upper one.
		const float thr = slot.env * FREQ_HYST;
		if (v < slot.mean - thr) {
			slot.armed = true;
		}
		else if (slot.armed && v > slot.mean + thr) {
			slot.armed = false;
			const float measured = slot.sinceCross;
			slot.sinceCross = 0.f;
			if (measured > 0.f) {
				if (slot.period <= 0.f
					|| std::fabs(measured - slot.period) > slot.period * FREQ_JUMP)
					slot.period = measured;      // a different signal: start again from it
				else
					slot.period += (measured - slot.period) * FREQ_FOLLOW;
				slot.hz.store(1.f / slot.period, std::memory_order_relaxed);
			}
		}

		// NOTHING HAS CROSSED FOR THREE OF ITS OWN PERIODS, so whatever was there has stopped.
		// Measured against the signal's own period rather than a fixed time, or a meter on a
		// slow LFO would give up every time between one cycle and the next.
		if (slot.period > 0.f && slot.sinceCross > slot.period * 3.f + 0.5f) {
			slot.period = 0.f;
			slot.hz.store(0.f, std::memory_order_relaxed);
		}
	}
}


static int slotAcquire() {
	for (int i = 0; i < FREQ_MAX; i++) {
		if (slots[i].active.load(std::memory_order_acquire))
			continue;
		slots[i].tap.store(-1, std::memory_order_relaxed);
		slots[i].hz.store(0.f, std::memory_order_relaxed);
		slots[i].dc.store(0.f, std::memory_order_relaxed);
		slots[i].mean = 0.f;
		slots[i].env = 0.f;
		slots[i].sinceCross = 0.f;
		slots[i].period = 0.f;
		slots[i].armed = false;
		slots[i].quiet = 0.f;
		slots[i].active.store(true, std::memory_order_release);
		activeCount.fetch_add(1, std::memory_order_release);
		return i;
	}
	return -1;
}

static void slotRelease(int i) {
	if (i < 0 || i >= FREQ_MAX || !slots[i].active.load(std::memory_order_relaxed))
		return;
	slots[i].active.store(false, std::memory_order_release);
	activeCount.fetch_sub(1, std::memory_order_release);
}


static const NVGcolor FRQ_GREEN = nvgRGB(0x3d, 0xe0, 0x7a);
/** The voltmeter's size exactly, so the two sit together as one kind of thing. */
static const float FRQ_W = 78.f, FRQ_H = 32.f;

/** The tuning standards worth offering, in hertz at A4. Baroque, classical, the two the
orchestras actually use, and the one the internet argues about. */
struct Standard { const char* name; float hz; };
static const Standard STANDARDS[] = {
	{"415 Hz (baroque)", 415.305f},
	{"430 Hz", 430.f},
	{"432 Hz", 432.f},
	{"435 Hz", 435.f},
	{"440 Hz (standard)", 440.f},
	{"442 Hz", 442.f},
	{"444 Hz", 444.f},
};
static const int STANDARD_COUNT = (int) (sizeof(STANDARDS) / sizeof(STANDARDS[0]));

static const char* NOTE_NAMES[12] = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
};


/** A pitch in semitones from middle C, written as a note and its distance from that note.

ALWAYS SIX COLUMNS, like every other readout here: three for the name and octave, three for the
sign and the cents. A number that changes width as it moves is one the eye cannot rest on. */
static std::string noteText(float midi) {
	if (!std::isfinite(midi) || midi < -12.f || midi > 147.f)
		return "-- ---";
	const int nearest = (int) std::lround(midi);
	const int cents = math::clamp((int) std::lround((midi - nearest) * 100.f), -50, 50);
	const int name = ((nearest % 12) + 12) % 12;
	const int octave = nearest / 12 - 1;
	const std::string label = std::string(NOTE_NAMES[name]) + string::f("%d", octave);
	return string::f("%-3s%+03d", label.c_str(), cents);
}


struct FreqWidget : ClipWidget {
	bool needsSignal() override {
		return true;
	}

	int slot = -1;
	int tapSlot = -1;

	enum Mode { MODE_HZ, MODE_NOTE, MODE_VOCT, MODE_COUNT };
	int mode = MODE_HZ;
	float tuning = 440.f;

	/** Where a press landed and how far it has travelled, so a drag that moves the widget is not
	also read as a click on the chip. */
	math::Vec pressPos;
	float travelled = 0.f;
	/** The chip's rectangle, measured while drawing because it is sized from the font, and
	whether the pointer is on it. Drawn rather than made a tooltip widget: a tooltip is one more
	thing in the scene with a lifetime to get wrong. */
	math::Rect chipHit;
	bool hovering = false;

	FreqWidget() {
		faceWidth = FRQ_W;
		faceHeight = FRQ_H;
		box.size = math::Vec(FRQ_W, FRQ_H);
	}

	~FreqWidget() {
		if (slot >= 0)
			slotRelease(slot);
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
	}

	const char* unitWord() const {
		return (mode == MODE_NOTE) ? "NOTE" : (mode == MODE_VOCT) ? "V/OCT" : "Hz";
	}

	/** What the display says, in six columns whichever unit is showing. */
	std::string readingText() {
		if (slot < 0)
			return "------";
		if (mode == MODE_VOCT) {
			// AN INSTRUCTION, NOT A MEASUREMENT. Nought volts is middle C, and every volt is an
			// octave, so the note is arithmetic rather than an estimate — no signal is needed
			// and none is looked for.
			const float v = slots[slot].dc.load(std::memory_order_relaxed);
			return noteText(60.f + 12.f * v);
		}
		const float hz = slots[slot].hz.load(std::memory_order_relaxed);
		if (hz <= 0.f)
			return (mode == MODE_NOTE) ? "-- ---" : "------";
		if (mode == MODE_NOTE)
			return noteText(69.f + 12.f * std::log2(hz / tuning));
		if (hz >= 10000.f)
			return string::f("%5.2fk", hz / 1000.f);
		if (hz >= 1000.f)
			return string::f("%5.3fk", hz / 1000.f);
		if (hz >= 100.f)
			return string::f("%6.2f", hz);
		return string::f("%6.3f", hz);
	}

	/** Either end of a cable. What arrives at an input is as worth reading as what leaves an
	output, and neither takes anything away from the patch. */
	bool acceptsPort(app::PortWidget* target) override {
		return target && target->module;
	}

	bool reattach(app::PortWidget* target) override {
		// No history: the period is measured as it happens, not read back afterwards.
		const int newTap = tapCreate(target->module->id, target->portId,
			target->type == engine::Port::OUTPUT, false);
		if (newTap < 0) {
			WARN("Freq: no tap slots available");
			return false;
		}
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
		tapSlot = newTap;
		if (slot >= 0)
			slots[slot].tap.store(newTap, std::memory_order_relaxed);
		port = target;
		return true;
	}

	void detach() override {
		if (slot >= 0) {
			slotRelease(slot);
			slot = -1;
		}
		if (tapSlot >= 0) {
			tapDestroy(tapSlot);
			tapSlot = -1;
		}
		ClipWidget::detach();
	}

	void step() override {
		followPort();
		ClipWidget::step();
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 3)
			drawFace(args);
		widget::OpaqueWidget::drawLayer(args, layer);
	}

	void draw(const DrawArgs& args) override {}

	void drawFace(const DrawArgs& args) {
		drawCallout(args.vg);

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, FRQ_W, FRQ_H, 3);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x12, 0x16));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, FRQ_GREEN);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);

		// THE CHIP, which is the control. It says which unit is showing and changes it when
		// clicked, and it is drawn as a chip rather than as a word so that it reads as
		// something to press. The channel count rides beside it when there is more than one,
		// since the reading is taken from the first channel alone.
		const int n = (slot >= 0) ? slots[slot].channels.load(std::memory_order_relaxed) : 1;
		const std::string word = std::string(unitWord())
			+ ((n > 1) ? string::f(" 1/%d", n) : "");

		nvgFontSize(args.vg, 9.f);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		float bounds[4] = {0.f, 0.f, 0.f, 0.f};
		nvgTextBounds(args.vg, 0.f, 0.f, word.c_str(), NULL, bounds);
		const float chipW = (bounds[2] - bounds[0]) + 8.f;
		const float chipH = 11.f;
		chipHit = math::Rect(math::Vec((FRQ_W - chipW) / 2.f, 1.5f),
			math::Vec(chipW, chipH));

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, chipHit.pos.x, chipHit.pos.y, chipW, chipH, 2.5f);
		nvgFillColor(args.vg, hovering ? nvgRGBA(0x3d, 0xe0, 0x7a, 0x30)
			: nvgRGBA(0x3d, 0xe0, 0x7a, 0x14));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGBA(0x3d, 0xe0, 0x7a, 0x90));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		nvgFillColor(args.vg, FRQ_GREEN);
		// Bold by overdrawing: there is no bold monospace to hand, and one pass of this face at
		// this size is too thin to read at a glance.
		for (int i = 0; i < 3; i++)
			nvgText(args.vg, FRQ_W / 2.f + i * 0.35f, chipHit.pos.y + chipH / 2.f + 0.5f,
				word.c_str(), NULL);

		// THE READING, set as large as the frame allows: the full width, and every pixel from
		// under the chip to the bottom frame. Sized by measuring the ink rather than by
		// trusting a font size, and never stretched.
		const std::string digits = readingText();
		const float top = chipHit.pos.y + chipH + 1.f, bottom = FRQ_H - 1.5f;
		const float availW = FRQ_W - 4.f, availH = bottom - top;

		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
		float size = 20.f;
		nvgFontSize(args.vg, size);
		float ink[4] = {0.f, 0.f, 0.f, 0.f};
		const float wide = nvgTextBounds(args.vg, 0.f, 0.f, digits.c_str(), NULL, ink);
		const float tall = ink[3] - ink[1];
		if (wide > 0.f && tall > 0.f)
			size *= std::fmin(availW / wide, availH / tall);
		nvgFontSize(args.vg, size);

		nvgTextBounds(args.vg, 0.f, 0.f, digits.c_str(), NULL, ink);
		const float digitsY = top + availH / 2.f - (ink[1] + ink[3]) / 2.f;
		for (int i = 0; i < 3; i++)
			nvgText(args.vg, FRQ_W / 2.f + i * 0.35f, digitsY, digits.c_str(), NULL);

		drawHint(args.vg);
	}

	/** A word about the chip while the pointer is on it, above the widget so it covers neither
	the chip it explains nor the reading. */
	void drawHint(NVGcontext* vg) {
		if (!hovering)
			return;
		const char* text = "Click for units";
		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(vg, font->handle);
		nvgFontSize(vg, 9.f);
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		float bounds[4] = {0.f, 0.f, 0.f, 0.f};
		nvgTextBounds(vg, 0.f, 0.f, text, NULL, bounds);
		const float w = bounds[2] - bounds[0] + 10.f;
		const float h = 14.f;
		const float x = (FRQ_W - w) / 2.f;
		const float y = -h - 4.f;

		nvgBeginPath(vg);
		nvgRoundedRect(vg, x, y, w, h, 3.f);
		nvgFillColor(vg, nvgRGBA(0x10, 0x12, 0x16, 0xf0));
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGBA(0xac, 0xb0, 0xb6, 0xc0));
		nvgStrokeWidth(vg, 1.f);
		nvgStroke(vg);
		nvgFillColor(vg, nvgRGB(0xe6, 0xe8, 0xea));
		nvgText(vg, FRQ_W / 2.f, y + h / 2.f, text, NULL);
	}

	void nextMode() {
		mode = (mode + 1) % MODE_COUNT;
	}

	void onHover(const HoverEvent& e) override {
		hovering = (chipHit.size.x > 0.f && chipHit.contains(e.pos));
		ClipWidget::onHover(e);
	}

	void onLeave(const LeaveEvent& e) override {
		hovering = false;
		ClipWidget::onLeave(e);
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && following) {
			following = false;
			e.consume(this);
			return;
		}
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			ui::Menu* menu = createMenu();
			menu->addChild(createMenuLabel("Frequency meter"));
			menu->addChild(createCheckMenuItem("Hertz", "",
				[this]() { return mode == MODE_HZ; }, [this]() { mode = MODE_HZ; }));
			menu->addChild(createCheckMenuItem("Note", "",
				[this]() { return mode == MODE_NOTE; }, [this]() { mode = MODE_NOTE; }));
			menu->addChild(createCheckMenuItem("Volt per octave", "",
				[this]() { return mode == MODE_VOCT; }, [this]() { mode = MODE_VOCT; }));
			menu->addChild(new ui::MenuSeparator);
			// THE TUNING STANDARD, which moves the note and the cents and leaves the hertz
			// alone. A patch played against an ensemble tuned elsewhere is in tune with itself
			// and out of tune with them, and this is the setting that says so.
			menu->addChild(createSubmenuItem("Tuning", string::f("%.0f Hz", tuning),
				[this](ui::Menu* sub) {
					for (int i = 0; i < STANDARD_COUNT; i++) {
						const float hz = STANDARDS[i].hz;
						sub->addChild(createCheckMenuItem(STANDARDS[i].name, "",
							[this, hz]() { return std::fabs(tuning - hz) < 0.01f; },
							[this, hz]() { tuning = hz; }));
					}
				}));
			menu->addChild(new ui::MenuSeparator);
			menu->addChild(createMenuItem("Remove", "", [this]() { detach(); }));
			e.consume(this);
			return;
		}
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			pressPos = e.pos;
			travelled = 0.f;
			e.consume(this);
			return;
		}
		widget::OpaqueWidget::onButton(e);
	}

	void onDragStart(const DragStartEvent& e) override {
		e.consume(this);
	}

	void onDragMove(const DragMoveEvent& e) override {
		const math::Vec d = e.mouseDelta.div(getAbsoluteZoom());
		travelled += d.norm();
		offset = offset.plus(d);
	}

	/** CHANGED ON RELEASE, and only by a press that stayed put on the chip. Every drag begins
	with a press, so acting on the press meant nudging the meter into place also changed what it
	was reading; and the reading itself is not a button, so a click anywhere else does nothing
	but pick the widget up. */
	void onDragEnd(const DragEndEvent& e) override {
		if (travelled < 2.f && chipHit.size.x > 0.f && chipHit.contains(pressPos))
			nextMode();
		travelled = 0.f;
	}

	json_t* toJson() {
		json_t* rootJ = json_object();
		if (port && port->module) {
			json_object_set_new(rootJ, "moduleId", json_integer(port->module->id));
			json_object_set_new(rootJ, "portId", json_integer(port->portId));
			json_object_set_new(rootJ, "isOutput",
				json_boolean(port->type == engine::Port::OUTPUT));
		}
		json_object_set_new(rootJ, "offsetX", json_real(offset.x));
		json_object_set_new(rootJ, "offsetY", json_real(offset.y));
		json_object_set_new(rootJ, "mode", json_integer(mode));
		json_object_set_new(rootJ, "tuning", json_real(tuning));
		return rootJ;
	}

	void fromJson(json_t* rootJ) {
		if (json_t* j = json_object_get(rootJ, "offsetX"))
			offset.x = json_number_value(j);
		if (json_t* j = json_object_get(rootJ, "offsetY"))
			offset.y = json_number_value(j);
		if (json_t* j = json_object_get(rootJ, "mode"))
			mode = math::clamp((int) json_integer_value(j), 0, (int) MODE_COUNT - 1);
		if (json_t* j = json_object_get(rootJ, "tuning")) {
			const float hz = json_number_value(j);
			if (hz > 100.f && hz < 1000.f)
				tuning = hz;
		}
	}
};


void freqCreate(app::PortWidget* port, bool place) {
	if (!port || !port->module)
		return;

	FreqWidget* f = new FreqWidget;
	f->port = port;
	f->slot = slotAcquire();
	if (f->slot < 0) {
		WARN("Freq: all %d frequency meter slots are in use", FREQ_MAX);
		delete f;
		return;
	}
	f->tapSlot = tapCreate(port->module->id, port->portId,
		port->type == engine::Port::OUTPUT, false);
	if (f->tapSlot < 0) {
		WARN("Freq: no tap slots available");
		slotRelease(f->slot);
		delete f;
		return;
	}
	slots[f->slot].tap.store(f->tapSlot, std::memory_order_relaxed);
	f->following = place;

	APP->scene->rack->addChild(f);
	clipAddHandle(f);
	clipAddClose(f);
	INFO("Freq: attached to port %d", port->portId);
}


void freqSetVisible(bool visible) {
	for (widget::Widget* child : APP->scene->rack->children) {
		if (FreqWidget* f = dynamic_cast<FreqWidget*>(child))
			clipSetVisible(f, visible);
	}
}


// ---- Saving with the patch ----

struct PendingFreq {
	int64_t moduleId = -1;
	int portId = 0;
	bool isOutput = true;
	json_t* stateJ = NULL;
	int budget = 300;
};

static std::vector<PendingFreq> pending;


json_t* freqToJson() {
	json_t* arrayJ = json_array();
	for (widget::Widget* child : APP->scene->rack->children) {
		FreqWidget* f = dynamic_cast<FreqWidget*>(child);
		if (f && f->port)
			json_array_append_new(arrayJ, f->toJson());
	}
	for (const PendingFreq& p : pending) {
		if (p.stateJ)
			json_array_append(arrayJ, p.stateJ);
	}
	return arrayJ;
}


void freqFromJson(json_t* arrayJ) {
	for (PendingFreq& p : pending) {
		if (p.stateJ)
			json_decref(p.stateJ);
	}
	pending.clear();
	if (!arrayJ || !json_is_array(arrayJ))
		return;

	size_t i;
	json_t* fJ;
	json_array_foreach(arrayJ, i, fJ) {
		json_t* moduleIdJ = json_object_get(fJ, "moduleId");
		if (!moduleIdJ)
			continue;
		PendingFreq p;
		p.moduleId = json_integer_value(moduleIdJ);
		if (json_t* j = json_object_get(fJ, "portId"))
			p.portId = json_integer_value(j);
		if (json_t* j = json_object_get(fJ, "isOutput"))
			p.isOutput = json_boolean_value(j);
		p.stateJ = json_incref(fJ);
		pending.push_back(p);
	}
}


void freqRestoreStep() {
	if (pending.empty())
		return;

	for (size_t i = 0; i < pending.size();) {
		PendingFreq& p = pending[i];
		app::PortWidget* found = NULL;
		for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
			if (!mw->module || mw->module->id != p.moduleId)
				continue;
			for (app::PortWidget* pw : mw->getPorts()) {
				if (pw->portId == p.portId
					&& (pw->type == engine::Port::OUTPUT) == p.isOutput) {
					found = pw;
					break;
				}
			}
			break;
		}

		if (found) {
			freqCreate(found, false);
			for (auto it = APP->scene->rack->children.rbegin();
				it != APP->scene->rack->children.rend(); it++) {
				FreqWidget* f = dynamic_cast<FreqWidget*>(*it);
				if (f && f->port == found) {
					f->fromJson(p.stateJ);
					break;
				}
			}
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		if (--p.budget <= 0) {
			WARN("Freq: module %lld never appeared, dropping it", (long long) p.moduleId);
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		i++;
	}
}
