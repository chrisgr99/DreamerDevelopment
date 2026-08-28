/** Signal injectors — see Injector.hpp for how a signal actually reaches the port. */
#include "Injector.hpp"
#include "Clip.hpp"
#include "WidgetAt.hpp"

#include <app/CableWidget.hpp>

#include <atomic>
#include <vector>
#include <cmath>


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
	/** Raised by a click, lowered by the audio thread when it starts the pulse. A counter
	rather than a flag so two quick presses both fire. */
	std::atomic<int> pulseRequests{0};

	// Audio thread only.
	float phase = 0.f;
	float pulseRemaining = 0.f;
};

static InjectorSlot slots[INJECT_MAX];
static std::atomic<int> activeCount{0};

/** A trigger's width. Rack's own convention, and long enough for every module to see it. */
static const float PULSE_SECONDS = 0.001f;


void injectorProcessAll(Module* drui, float sampleTime) {
	if (activeCount.load(std::memory_order_acquire) <= 0)
		return;

	for (int i = 0; i < INJECT_MAX; i++) {
		InjectorSlot& slot = slots[i];
		if (!slot.active.load(std::memory_order_acquire))
			continue;
		if (i >= (int) drui->outputs.size())
			continue;

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
				v = level;
				break;

			case INJECT_LFO: {
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
			} break;
		}

		drui->outputs[i].setVoltage(v);
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


// ---- The face ----

static const float INJ_SIZE = 46.f;


struct InjectorWidget : ClipWidget {
	int slot = -1;
	InjectorType type = INJECT_GATE;
	InjectorWave wave = WAVE_SINE;
	float level = 5.f;
	float rate = 2.f;
	/** The cable carrying this injector's signal. Owned by Rack, not by us. */
	WeakPtr<app::CableWidget> cable;

	InjectorWidget() {
		box.size = math::Vec(INJ_SIZE, INJ_SIZE);
		faceHeight = INJ_SIZE;
		offset = math::Vec(28, -56);
	}

	~InjectorWidget() {
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

	void step() override {
		followPort();
		// Rack hides the plugs on creation, but a cable rebuilds them when its ports change,
		// so this keeps them hidden rather than assuming they stayed that way.
		if (cable)
			hideCable(cable);
		ClipWidget::step();
	}

	std::string label() {
		switch (type) {
			case INJECT_GATE: return "GATE";
			case INJECT_PULSE: return "TRIG";
			case INJECT_DC: return string::f("%.2fV", level);
			default: break;
		}
		return string::f("%.2fHz", rate);
	}

	void draw(const DrawArgs& args) override {
		drawCallout(args.vg);

		const math::Vec c = box.size.div(2.f);
		const float r = INJ_SIZE / 2.f - 1.f;

		// Body: the same dark face as a scope, so a rack of our things looks like one set.
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, c.x, c.y, r);
		nvgFillColor(args.vg, nvgRGB(0x14, 0x17, 0x1c));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, CLIP_CALLOUT_COLOR);
		nvgStrokeWidth(args.vg, 1.6f);
		nvgStroke(args.vg);

		if (type == INJECT_GATE || type == INJECT_PULSE) {
			// A button, lit while it is doing something.
			const bool lit = (type == INJECT_GATE && slot >= 0)
				? slots[slot].gate.load(std::memory_order_relaxed)
				: false;
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, c.x, c.y - 3.f, r * 0.52f);
			nvgFillColor(args.vg, lit ? nvgRGB(0x2f, 0xd0, 0x6a) : nvgRGB(0x3d, 0x42, 0x4a));
			nvgFill(args.vg);
			nvgStrokeColor(args.vg, nvgRGB(0x6b, 0x70, 0x79));
			nvgStrokeWidth(args.vg, 1.f);
			nvgStroke(args.vg);
		}
		else {
			// A dial, drawn with the plugin's own knob so it matches the knobs around it.
			const float frac = (type == INJECT_DC)
				? math::rescale(level, -10.f, 10.f, 0.f, 1.f)
				: math::rescale(std::log10(math::clamp(rate, 0.01f, 100.f)), -2.f, 2.f, 0.f, 1.f);
			const float angle = -0.75f * M_PI + frac * 1.5f * M_PI;
			druiDrawKnob(args.vg, math::Vec(c.x, c.y - 3.f), r * 0.62f, angle, 7);
		}

		// The value, or what the button does.
		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (font && font->handle >= 0) {
			nvgFontFaceId(args.vg, font->handle);
			nvgFontSize(args.vg, 8.f);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(args.vg, nvgRGB(0xd8, 0xdc, 0xe2));
			nvgText(args.vg, c.x, box.size.y - 8.f, label().c_str(), NULL);
		}

		ClipWidget::draw(args);
	}

	/** Scroll adjusts the dial. Deliberately the same gesture as everywhere else in this
	plugin, and the reason a value can be set without a drag. */
	void onHoverScroll(const HoverScrollEvent& e) override {
		if (e.scrollDelta.y == 0.f || slot < 0)
			return;
		const float dir = (e.scrollDelta.y > 0.f) ? 1.f : -1.f;
		const bool fine = (APP->window->getMods() & RACK_MOD_MASK) == GLFW_MOD_SHIFT;
		if (type == INJECT_DC) {
			level = math::clamp(level + dir * (fine ? 0.01f : 0.1f), -10.f, 10.f);
			slots[slot].level.store(level, std::memory_order_relaxed);
		}
		else if (type == INJECT_LFO) {
			// Multiplicative, because a rate dial that steps by a fixed number of hertz is
			// unusable at one end or the other of a range this wide.
			rate = math::clamp(rate * std::pow(fine ? 1.01f : 1.06f, dir), 0.01f, 100.f);
			slots[slot].rate.store(rate, std::memory_order_relaxed);
		}
		e.consume(this);
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

	/** A gate must fall even if the pointer has left the face by the time the button comes
	up, or a drag off the button would leave the gate stuck high. */
	void onDragEnd(const DragEndEvent& e) override {
		if (slot >= 0 && type == INJECT_GATE)
			slots[slot].gate.store(false, std::memory_order_relaxed);
		ClipWidget::onDragEnd(e);
	}

	void setType(InjectorType t) {
		type = t;
		if (slot >= 0) {
			slots[slot].type.store(t, std::memory_order_relaxed);
			slots[slot].gate.store(false, std::memory_order_relaxed);
		}
	}

	void appendContextMenu(ui::Menu* menu) {
		menu->addChild(createMenuLabel("Injector"));
		static const char* typeNames[INJECT_TYPES] = {"Gate button", "Trigger button",
			"DC level", "Waveform"};
		for (int t = 0; t < INJECT_TYPES; t++) {
			menu->addChild(createCheckMenuItem(typeNames[t], "",
				[this, t]() { return type == t; },
				[this, t]() { setType((InjectorType) t); }));
		}

		if (type == INJECT_LFO) {
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

		menu->addChild(new ui::MenuSeparator);
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
		if (json_t* j = json_object_get(rootJ, "offsetX"))
			offset.x = json_number_value(j);
		if (json_t* j = json_object_get(rootJ, "offsetY"))
			offset.y = json_number_value(j);
	}
};


/** The cable a restored injector should adopt: one already running from a DRUI output into
this port. It exists because an injector's cable is an ordinary patch cable, so Rack restores
it with the patch — and laying another would double the injected signal every time the patch
was opened. */
static app::CableWidget* existingInjectorCable(app::PortWidget* port, Module* drui) {
	for (app::CableWidget* cw : APP->scene->rack->getCompleteCables()) {
		if (!cw->cable || cw->inputPort != port)
			continue;
		if (cw->cable->outputModule == drui)
			return cw;
	}
	return NULL;
}


static InjectorWidget* injectorMake(app::PortWidget* port, InjectorType type) {
	if (!injectorAcceptsPort(port))
		return NULL;
	Module* drui = findDruiModule();
	if (!drui) {
		WARN("Injector: no DRUI module in the patch to inject from");
		return NULL;
	}

	InjectorWidget* inj = new InjectorWidget;
	app::CableWidget* existing = existingInjectorCable(port, drui);
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
	INFO("Injector: attached to input port %d in slot %d", port->portId, inj->slot);
	return inj;
}


void injectorCreate(app::PortWidget* port, InjectorType type) {
	injectorMake(port, type);
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
			if (InjectorWidget* inj = injectorMake(port, INJECT_GATE))
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
