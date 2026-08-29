/** Listen to any terminal in the rack.

A scope shows you a signal; this one plays it. Clip a monitor onto a terminal, patch the
Test Gear module's MONITOR jack to your audio interface once, and every monitor you attach after
that is audible through it — each with its own level, each mutable on its own. Half of what
goes wrong in a patch is a signal that is not what you think it is, and hearing it is often
faster than reading it.

IT SUMS, and that is the point: the jack is a mixing bus, not a switch. Several monitors at
once lets you hear one thing against another — a modulator under the sound it is shaping —
rather than flipping between them and remembering.

WHY THE VOLTAGE AND NOT A CABLE. The monitor reads the tapped port's current sample directly,
the same way the attenuverter does, so it can listen to an OUTPUT as easily as an input and
takes nothing away from what is already patched. Nothing is inserted into the signal path at
all: a monitor cannot change what it is listening to.

The mix is built in the audio thread from atomics the UI writes, and every level change ramps,
because a level that jumps is a click and a click through headphones is unpleasant.
*/
#include "plugin.hpp"
#include "Clip.hpp"
#include "SignalTap.hpp"
#include "Monitor.hpp"

#include <atomic>
#include <vector>
#include <cmath>


/** How long a level change or a mute takes to arrive. Long enough to remove the click, short
enough that pressing mute feels immediate. */
static const float MON_RAMP_SECONDS = 0.01f;

struct MonitorSlot {
	std::atomic<bool> active{false};
	std::atomic<int> tap{-1};
	std::atomic<float> level{0.5f};
	std::atomic<bool> muted{false};

	/** Audio thread only: the ramp that gets us to the level without a step. */
	float gain = 0.f;
};

static MonitorSlot slots[MONITOR_MAX];
static std::atomic<int> activeCount{0};

/** Audio thread only. One pole, removing any steady offset from the mix.

A CV is often a long way from zero — an envelope resting at five volts, a pitch at one — and
sending that to an interface is a DC offset on a loudspeaker, which is heat in the voice coil
and nothing to listen to. Cutting it costs nothing audible and makes CV safe to monitor.
*/
static float dcLast = 0.f, dcOut = 0.f;


float monitorMix(float sampleTime) {
	if (activeCount.load(std::memory_order_acquire) <= 0)
		return 0.f;

	float sum = 0.f;
	const float rampStep = sampleTime / MON_RAMP_SECONDS;

	for (int i = 0; i < MONITOR_MAX; i++) {
		MonitorSlot& slot = slots[i];
		if (!slot.active.load(std::memory_order_acquire))
			continue;
		const float target = slot.muted.load(std::memory_order_relaxed)
			? 0.f : slot.level.load(std::memory_order_relaxed);
		if (slot.gain < target)
			slot.gain = std::fmin(target, slot.gain + rampStep);
		else if (slot.gain > target)
			slot.gain = std::fmax(target, slot.gain - rampStep);
		if (slot.gain <= 0.f)
			continue;

		const int tap = slot.tap.load(std::memory_order_relaxed);
		if (tap < 0)
			continue;
		sum += tapVoltage(tap) * slot.gain;
	}

	// The DC blocker, at about 20 Hz.
	const float r = 1.f - 2.f * M_PI * 20.f * sampleTime;
	dcOut = sum - dcLast + math::clamp(r, 0.f, 0.9999f) * dcOut;
	dcLast = sum;
	return math::clamp(dcOut, -10.f, 10.f);
}


static int slotAcquire() {
	for (int i = 0; i < MONITOR_MAX; i++) {
		if (slots[i].active.load(std::memory_order_relaxed))
			continue;
		slots[i].gain = 0.f;
		slots[i].active.store(true, std::memory_order_release);
		activeCount.fetch_add(1, std::memory_order_release);
		return i;
	}
	return -1;
}

static void slotRelease(int i) {
	if (i < 0 || i >= MONITOR_MAX || !slots[i].active.load(std::memory_order_relaxed))
		return;
	slots[i].active.store(false, std::memory_order_release);
	activeCount.fetch_sub(1, std::memory_order_release);
}


static const NVGcolor MON_GREEN = nvgRGB(0x3d, 0xe0, 0x7a);
static const NVGcolor MON_RED = nvgRGB(0xe0, 0x5b, 0x4b);
/** The injectors' readout size exactly, so a monitor sits among them without looking like a
different kind of object. */
static const float MON_W = 56.f, MON_H = 32.f;

/** The quietest and loudest a monitor goes. A LEVEL, never silence: silence is what mute is
for, and a level that reaches zero cannot be scrolled back up — the logarithm of nothing is
not a number to step from. */
static const float MON_MIN_DB = -60.f, MON_MAX_DB = 6.f;


struct MonitorWidget : ClipWidget {
	int slot = -1;
	int tapSlot = -1;
	/** Where a press landed and how far it has travelled, so a drag that moves the widget is
	not also read as the click that mutes it. */
	math::Vec pressPos;
	float travelled = 0.f;

	MonitorWidget() {
		faceWidth = MON_W;
		faceHeight = MON_H;
		box.size = math::Vec(MON_W, MON_H);
	}

	~MonitorWidget() {
		if (slot >= 0)
			slotRelease(slot);
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
	}

	float level() {
		return (slot >= 0) ? slots[slot].level.load(std::memory_order_relaxed) : 0.f;
	}

	/** The level in decibels, floored so it is always a real number to step from. */
	float levelDb() {
		return math::clamp(20.f * std::log10(std::fmax(level(), 1e-4f)),
			MON_MIN_DB, MON_MAX_DB);
	}

	void setLevelDb(float db) {
		if (slot < 0)
			return;
		db = math::clamp(db, MON_MIN_DB, MON_MAX_DB);
		slots[slot].level.store(std::pow(10.f, db / 20.f), std::memory_order_relaxed);
	}

	bool muted() {
		return (slot >= 0) && slots[slot].muted.load(std::memory_order_relaxed);
	}

	/** Either end of a cable will do. Listening to what ARRIVES at an input is as useful as
	listening to what leaves an output, and neither takes anything away from the patch. */
	bool acceptsPort(app::PortWidget* target) override {
		return target && target->module;
	}

	bool reattach(app::PortWidget* target) override {
		// No history: a monitor wants this sample, not the last eleven seconds, and a ring
		// buffer per monitor would be two megabytes each for nothing.
		const int newTap = tapCreate(target->module->id, target->portId,
			target->type == engine::Port::OUTPUT, false);
		if (newTap < 0) {
			WARN("Monitor: no tap slots available");
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

		const bool off = muted();
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, MON_W, MON_H, 3);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x12, 0x16));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, off ? MON_RED : MON_GREEN);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		nvgFontSize(args.vg, 11.f);
		nvgFillColor(args.vg, off ? MON_RED : MON_GREEN);
		// The whole word: seven monospaced letters at this size come to about forty-six pixels,
		// which the fifty-six-pixel face already had room for.
		nvgText(args.vg, MON_W / 2.f, 8.f, "MONITOR", NULL);

		// Decibels, not a percentage: a monitor is a listening level, and the numbers people
		// have a feel for on a listening level are decibels. Fixed width, so the point does
		// not move as it changes.
		const std::string text = off ? "MUTED" : string::f("%5.1f", levelDb());
		nvgFontSize(args.vg, 12.f);
		nvgFillColor(args.vg, off ? nvgRGBA(0xe0, 0x5b, 0x4b, 0xcc) : MON_GREEN);
		nvgText(args.vg, MON_W / 2.f, 21.f, text.c_str(), NULL);
	}

	/** Scroll sets the level, in steps of a decibel and a half — fine enough to balance two
	sources against each other, coarse enough to cross the useful range in a short glide. */
	void onHoverScroll(const HoverScrollEvent& e) override {
		if (slot < 0)
			return;
		setLevelDb(levelDb() + ((e.scrollDelta.y > 0.f) ? 1.5f : -1.5f));
		e.consume(this);
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && following) {
			following = false;
			e.consume(this);
			return;
		}
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			ui::Menu* menu = createMenu();
			menu->addChild(createMenuLabel("Monitor"));
			menu->addChild(createMenuItem(muted() ? "Unmute" : "Mute", "", [this]() {
				if (slot >= 0)
					slots[slot].muted.store(!muted(), std::memory_order_relaxed);
			}));
			menu->addChild(createMenuItem("Level to 0 dB", "", [this]() {
				setLevelDb(0.f);
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

	/** MUTE IS DECIDED ON RELEASE, not on the press. Every drag begins with a press, so acting
	on the press meant nudging a monitor into place also muted it. */
	void onDragEnd(const DragEndEvent& e) override {
		if (travelled < 2.f && slot >= 0)
			slots[slot].muted.store(!muted(), std::memory_order_relaxed);
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
		json_object_set_new(rootJ, "level", json_real(level()));
		json_object_set_new(rootJ, "muted", json_boolean(muted()));
		return rootJ;
	}

	void fromJson(json_t* rootJ) {
		if (json_t* j = json_object_get(rootJ, "offsetX"))
			offset.x = json_number_value(j);
		if (json_t* j = json_object_get(rootJ, "offsetY"))
			offset.y = json_number_value(j);
		if (slot < 0)
			return;
		if (json_t* j = json_object_get(rootJ, "level"))
			slots[slot].level.store(json_number_value(j), std::memory_order_relaxed);
		if (json_t* j = json_object_get(rootJ, "muted"))
			slots[slot].muted.store(json_boolean_value(j), std::memory_order_relaxed);
	}
};


void monitorCreate(app::PortWidget* port, bool place) {
	if (!port || !port->module)
		return;

	MonitorWidget* m = new MonitorWidget;
	m->port = port;
	m->slot = slotAcquire();
	if (m->slot < 0) {
		WARN("Monitor: all %d monitor slots are in use", MONITOR_MAX);
		delete m;
		return;
	}
	m->tapSlot = tapCreate(port->module->id, port->portId,
		port->type == engine::Port::OUTPUT, false);
	if (m->tapSlot < 0) {
		WARN("Monitor: no tap slots available");
		slotRelease(m->slot);
		delete m;
		return;
	}
	slots[m->slot].tap.store(m->tapSlot, std::memory_order_relaxed);
	slots[m->slot].level.store(0.5f, std::memory_order_relaxed);   // About -6 dB.
	slots[m->slot].muted.store(false, std::memory_order_relaxed);
	m->following = place;

	APP->scene->rack->addChild(m);
	clipAddHandle(m);
	clipAddClose(m);
	INFO("Monitor: attached to port %d", port->portId);
}


void monitorSetVisible(bool visible) {
	for (widget::Widget* child : APP->scene->rack->children) {
		if (MonitorWidget* m = dynamic_cast<MonitorWidget*>(child))
			clipSetVisible(m, visible);
	}
}


// ---- Saving with the patch ----

struct PendingMonitor {
	int64_t moduleId = -1;
	int portId = 0;
	bool isOutput = true;
	json_t* stateJ = NULL;
	int budget = 300;
};

static std::vector<PendingMonitor> pending;


json_t* monitorToJson() {
	json_t* arrayJ = json_array();
	for (widget::Widget* child : APP->scene->rack->children) {
		MonitorWidget* m = dynamic_cast<MonitorWidget*>(child);
		if (m && m->port)
			json_array_append_new(arrayJ, m->toJson());
	}
	for (const PendingMonitor& p : pending) {
		if (p.stateJ)
			json_array_append(arrayJ, p.stateJ);
	}
	return arrayJ;
}


void monitorFromJson(json_t* arrayJ) {
	for (PendingMonitor& p : pending) {
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
		PendingMonitor p;
		p.moduleId = json_integer_value(moduleIdJ);
		if (json_t* j = json_object_get(mJ, "portId"))
			p.portId = json_integer_value(j);
		if (json_t* j = json_object_get(mJ, "isOutput"))
			p.isOutput = json_boolean_value(j);
		p.stateJ = json_incref(mJ);
		pending.push_back(p);
	}
}


void monitorRestoreStep() {
	if (pending.empty())
		return;

	for (size_t i = 0; i < pending.size();) {
		PendingMonitor& p = pending[i];
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
			monitorCreate(found, false);
			for (auto it = APP->scene->rack->children.rbegin();
				it != APP->scene->rack->children.rend(); it++) {
				MonitorWidget* m = dynamic_cast<MonitorWidget*>(*it);
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
			WARN("Monitor: module %lld never appeared, dropping it", (long long) p.moduleId);
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		i++;
	}
}
