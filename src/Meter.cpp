/** Voltmeters clipped onto terminals.

WHAT IT SHOWS, AND WHY IT IS ONE NUMBER. A meter wants to answer two questions: what is on this
terminal now, and how far does it go. Two numbers stacked would answer both at once and make the
widget twice the height of every other readout in this plugin — so it shows one, and a click
turns it over. The small word above it says which, so nothing has to be remembered.

THE PEAK IS FOUND ON THE AUDIO THREAD. Sampled once a frame, a meter looking at an audio signal
would catch one sample in eight hundred and report a number that depends on when it happened to
look. The peak is taken every sample and held for a quarter of a second, which is long enough to
read and short enough to follow.

WIDTH NEVER CHANGES. A sign, two digits, a point and two more digits: +05.00, -10.00. A reading
past ninety-nine volts is held there rather than allowed a third digit, because a number that
changes width as it moves is a number the eye cannot rest on — and past ninety-nine volts the
question has stopped being how many.
*/
#include "plugin.hpp"
#include "Meter.hpp"
#include "Clip.hpp"
#include "SignalTap.hpp"

#include <atomic>
#include <cmath>
#include <vector>


/** One meter's state, shared between the two threads. The audio thread writes the readings and
nothing else; the UI thread writes everything else. */
struct MeterSlot {
	std::atomic<bool> active{false};
	std::atomic<int> tap{-1};
	/** What the terminal is carrying, and the largest it has carried lately. */
	std::atomic<float> now{0.f};
	std::atomic<float> peak{0.f};
	/** How many channels the terminal is carrying, so a reading taken from the first one does
	not pass for the whole of a chord. */
	std::atomic<int> channels{1};

	// Audio thread only.
	float held = 0.f;
	float heldFor = 0.f;
};

static MeterSlot slots[METER_MAX];
static std::atomic<int> activeCount{0};

/** How long a peak is held before it is let go. Long enough to read, short enough that the
number still follows the music rather than reporting something that happened a while ago. */
static const float PEAK_SECONDS = 0.25f;


void meterProcess(float sampleTime) {
	if (activeCount.load(std::memory_order_acquire) <= 0)
		return;

	for (int i = 0; i < METER_MAX; i++) {
		MeterSlot& slot = slots[i];
		if (!slot.active.load(std::memory_order_acquire))
			continue;
		const int tap = slot.tap.load(std::memory_order_relaxed);
		if (tap < 0)
			continue;

		const float v = tapVoltage(tap);
		slot.now.store(v, std::memory_order_relaxed);
		slot.channels.store(tapChannels(tap), std::memory_order_relaxed);

		// LARGEST BY SIZE, SHOWN WITH ITS SIGN. A signal that swings to minus eight has a peak
		// of minus eight, not of whatever small positive number it also passed through.
		if (std::fabs(v) >= std::fabs(slot.held)) {
			slot.held = v;
			slot.heldFor = 0.f;
		}
		else {
			slot.heldFor += sampleTime;
			if (slot.heldFor >= PEAK_SECONDS) {
				// Let go and start again from what is there now, rather than decaying towards
				// it: a meter that slides down is reporting a number that was never true.
				slot.held = v;
				slot.heldFor = 0.f;
			}
		}
		slot.peak.store(slot.held, std::memory_order_relaxed);
	}
}


static int slotAcquire() {
	for (int i = 0; i < METER_MAX; i++) {
		if (slots[i].active.load(std::memory_order_acquire))
			continue;
		slots[i].tap.store(-1, std::memory_order_relaxed);
		slots[i].now.store(0.f, std::memory_order_relaxed);
		slots[i].peak.store(0.f, std::memory_order_relaxed);
		slots[i].held = 0.f;
		slots[i].heldFor = 0.f;
		slots[i].active.store(true, std::memory_order_release);
		activeCount.fetch_add(1, std::memory_order_release);
		return i;
	}
	return -1;
}

static void slotRelease(int i) {
	if (i < 0 || i >= METER_MAX || !slots[i].active.load(std::memory_order_relaxed))
		return;
	slots[i].active.store(false, std::memory_order_release);
	activeCount.fetch_sub(1, std::memory_order_release);
}


static const NVGcolor MET_GREEN = nvgRGB(0x3d, 0xe0, 0x7a);
/** The injectors' readout size exactly, so a meter sits among them without looking like a
different kind of object. */
static const float MET_W = 56.f, MET_H = 32.f;


struct MeterWidget : ClipWidget {
	bool needsSignal() override {
		return true;
	}

	int slot = -1;
	int tapSlot = -1;
	/** Which of the two readings is on show. */
	bool showPeak = false;
	/** Where a press landed and how far it has travelled, so a drag that moves the widget is
	not also read as the click that turns it over. */
	math::Vec pressPos;
	float travelled = 0.f;

	MeterWidget() {
		faceWidth = MET_W;
		faceHeight = MET_H;
		box.size = math::Vec(MET_W, MET_H);
	}

	~MeterWidget() {
		if (slot >= 0)
			slotRelease(slot);
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
	}

	float reading() {
		if (slot < 0)
			return 0.f;
		return showPeak ? slots[slot].peak.load(std::memory_order_relaxed)
			: slots[slot].now.load(std::memory_order_relaxed);
	}

	/** Either end of a cable. What ARRIVES at an input is as worth reading as what leaves an
	output, and neither takes anything away from the patch. */
	bool acceptsPort(app::PortWidget* target) override {
		return target && target->module;
	}

	bool reattach(app::PortWidget* target) override {
		// No history: a meter wants this sample, not the last eleven seconds.
		const int newTap = tapCreate(target->module->id, target->portId,
			target->type == engine::Port::OUTPUT, false);
		if (newTap < 0) {
			WARN("Meter: no tap slots available");
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
		// LET GO OF THE TAP HERE TOO, and remember that we have. A detached meter is not
		// reading anything, and holding a tap on a module that may be about to be destroyed is
		// how a patch load found itself unbinding the same handle twice.
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
		nvgRoundedRect(args.vg, 0, 0, MET_W, MET_H, 3);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x12, 0x16));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, MET_GREEN);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		// The word says both what this is and which of its two readings is showing, which is
		// why it is one word rather than a name and a state.
		nvgFontSize(args.vg, 11.f);
		nvgFillColor(args.vg, MET_GREEN);
		// THE CHANNEL COUNT WHEN THERE IS MORE THAN ONE. The reading is the first channel, and
		// a number that quietly describes one note of a chord while looking like the whole
		// thing is worse than no number.
		const int n = (slot >= 0) ? slots[slot].channels.load(std::memory_order_relaxed) : 1;
		const std::string word = std::string(showPeak ? "PEAK" : "VOLTS")
			+ ((n > 1) ? string::f(" 1/%d", n) : "");
		nvgText(args.vg, MET_W / 2.f, 8.f, word.c_str(), NULL);

		// Held at ninety-nine rather than allowed a third digit: a number that changes width as
		// it moves is one the eye cannot rest on.
		const float v = math::clamp(reading(), -99.99f, 99.99f);
		nvgFontSize(args.vg, 12.f);
		nvgText(args.vg, MET_W / 2.f, 21.f, string::f("%+06.2f", v).c_str(), NULL);
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && following) {
			following = false;
			e.consume(this);
			return;
		}
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			ui::Menu* menu = createMenu();
			menu->addChild(createMenuLabel("Voltmeter"));
			menu->addChild(createMenuItem(showPeak ? "Show the voltage now" : "Show the peak",
				"", [this]() { showPeak = !showPeak; }));
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

	/** TURNED OVER ON RELEASE, not on the press. Every drag begins with a press, so acting on
	the press meant nudging a meter into place also flipped it. */
	void onDragEnd(const DragEndEvent& e) override {
		if (travelled < 2.f)
			showPeak = !showPeak;
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
		json_object_set_new(rootJ, "peak", json_boolean(showPeak));
		return rootJ;
	}

	void fromJson(json_t* rootJ) {
		if (json_t* j = json_object_get(rootJ, "offsetX"))
			offset.x = json_number_value(j);
		if (json_t* j = json_object_get(rootJ, "offsetY"))
			offset.y = json_number_value(j);
		if (json_t* j = json_object_get(rootJ, "peak"))
			showPeak = json_boolean_value(j);
	}
};


void meterCreate(app::PortWidget* port, bool place) {
	if (!port || !port->module)
		return;

	MeterWidget* m = new MeterWidget;
	m->port = port;
	m->slot = slotAcquire();
	if (m->slot < 0) {
		WARN("Meter: all %d meter slots are in use", METER_MAX);
		delete m;
		return;
	}
	m->tapSlot = tapCreate(port->module->id, port->portId,
		port->type == engine::Port::OUTPUT, false);
	if (m->tapSlot < 0) {
		WARN("Meter: no tap slots available");
		slotRelease(m->slot);
		delete m;
		return;
	}
	slots[m->slot].tap.store(m->tapSlot, std::memory_order_relaxed);
	m->following = place;

	APP->scene->rack->addChild(m);
	clipAddHandle(m);
	clipAddClose(m);
	INFO("Meter: attached to port %d", port->portId);
}


void meterSetVisible(bool visible) {
	for (widget::Widget* child : APP->scene->rack->children) {
		if (MeterWidget* m = dynamic_cast<MeterWidget*>(child))
			clipSetVisible(m, visible);
	}
}


// ---- Saving with the patch ----

struct PendingMeter {
	int64_t moduleId = -1;
	int portId = 0;
	bool isOutput = true;
	json_t* stateJ = NULL;
	int budget = 300;
};

static std::vector<PendingMeter> pending;


json_t* meterToJson() {
	json_t* arrayJ = json_array();
	for (widget::Widget* child : APP->scene->rack->children) {
		MeterWidget* m = dynamic_cast<MeterWidget*>(child);
		if (m && m->port)
			json_array_append_new(arrayJ, m->toJson());
	}
	for (const PendingMeter& p : pending) {
		if (p.stateJ)
			json_array_append(arrayJ, p.stateJ);
	}
	return arrayJ;
}


void meterFromJson(json_t* arrayJ) {
	for (PendingMeter& p : pending) {
		if (p.stateJ)
			json_decref(p.stateJ);
	}
	pending.clear();
	if (!arrayJ || !json_is_array(arrayJ))
		return;

	size_t i;
	json_t* mJ;
	json_array_foreach(arrayJ, i, mJ) {
		json_t* moduleIdJ = json_object_get(mJ, "moduleId");
		if (!moduleIdJ)
			continue;
		PendingMeter p;
		p.moduleId = json_integer_value(moduleIdJ);
		if (json_t* j = json_object_get(mJ, "portId"))
			p.portId = json_integer_value(j);
		if (json_t* j = json_object_get(mJ, "isOutput"))
			p.isOutput = json_boolean_value(j);
		p.stateJ = json_incref(mJ);
		pending.push_back(p);
	}
}


void meterRestoreStep() {
	if (pending.empty())
		return;

	for (size_t i = 0; i < pending.size();) {
		PendingMeter& p = pending[i];
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
			meterCreate(found, false);
			for (auto it = APP->scene->rack->children.rbegin();
				it != APP->scene->rack->children.rend(); it++) {
				MeterWidget* m = dynamic_cast<MeterWidget*>(*it);
				if (m && m->port == found) {
					m->fromJson(p.stateJ);
					break;
				}
			}
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		if (--p.budget <= 0) {
			WARN("Meter: module %lld never appeared, dropping it", (long long) p.moduleId);
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		i++;
	}
}
