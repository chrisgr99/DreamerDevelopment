/** DreamRack — the module that switches everything else on.

Its slug is still DRUI, and must stay so: a patch records the slug, and changing it would
orphan every patch anyone has saved. DreamRack is only what the module is called.

WHY A MODULE EXISTS AT ALL. A Rack plugin cannot run code until one of its modules is placed:
plugins are initialised before the scene is created, and there is no callback afterwards. So
this module's first job is to install the overlays that do the drawing, and its second is to
carry the settings — one param per feature, shown as a button on its face, because a module
whose every feature hides in a right-click menu looks like a module that does nothing.

It also owns the audio-rate work: the scope taps and the injectors both ride in its process(),
which is why bypassing it silences them while everything drawn goes on looking alive. Its only
ports are the injectors' hidden outputs.

HOW THE DRAWING REACHES OTHER PEOPLE'S MODULES. The overlay is added as the last child of the
RackWidget, so it draws after every module and can paint over their jacks and knobs. It lives
in rack coordinates, so it scrolls and zooms with the rack for free.

The one cost of drawing last: anything a module draws in its light layer — LED rings around
knobs, for instance — is painted before us and would be covered. Knob drawing is therefore
optional and can be switched off per user.
*/
#include "plugin.hpp"
#include "SignalTap.hpp"
#include "Clip.hpp"
#include "Injector.hpp"

#include <map>
#include <set>
#include <vector>
#include <cmath>

using namespace rack;


// ---- The signal-family colour code. Colour carries FAMILY, shape carries DIRECTION. ----
// A jack told apart only by hue cannot be told apart in peripheral vision, under
// magnification, or by anyone whose colour vision differs. Never fold one into the other.

static NVGcolor familyColor(const std::string& family) {
	if (family == "audio")   return nvgRGB(0xf3, 0xc4, 0x0b);   // yellow
	if (family == "cv")      return nvgRGB(0xff, 0x73, 0x00);   // orange
	if (family == "trigger") return nvgRGB(0x5a, 0xa0, 0xe6);   // light blue
	if (family == "pitch")   return nvgRGB(0x39, 0xa8, 0x5a);   // green
	return nvgRGB(0xf3, 0xc4, 0x0b);
}

/** Rack has no concept of signal family, but it does expose port names. Guessing from the
name is what makes this useful on plugins nobody has described by hand. */
static std::string guessFamily(const std::string& name) {
	const std::string n = string::uppercase(name);
	if (n.find("V/OCT") != std::string::npos || n.find("PITCH") != std::string::npos
		|| n.find("NOTE") != std::string::npos)
		return "pitch";
	if (n.find("GATE") != std::string::npos || n.find("TRIG") != std::string::npos
		|| n.find("CLOCK") != std::string::npos || n.find("CLK") != std::string::npos
		|| n.find("RESET") != std::string::npos || n.find("SYNC") != std::string::npos)
		return "trigger";
	if (n.find("CV") != std::string::npos || n.find("MOD") != std::string::npos
		|| n.find("FM") != std::string::npos)
		return "cv";
	return "audio";
}

/** Flow-dash length per family, in cable widths: gate coarse, audio fine. */
static float flowDashLength(const std::string& family) {
	if (family == "audio")
		return 1.6f;
	if (family == "trigger")
		return 5.6f;
	return 3.4f;
}


// ---- Options, held by the module and persisted with the patch ----

struct DRUIOptions {
	bool jacks = true;
	bool knobs = true;
	bool cableColor = true;
	bool cableFlow = true;
	bool pinchZoom = true;
	bool sliderScroll = true;
	/** Click a jack to pick up its cable, click another to drop it — no button held between.
	Off by default: it changes the most basic gesture in Rack. */
	bool clickCables = false;
	bool trace = true;
	bool scopes = true;
	bool widgets = true;
	/** Draws a pointer into the rack, for videos recorded with VCV Recorder — which cannot see
	the real cursor. A recording aid rather than a feature, so it lives in the menu. */
	bool demoPointer = false;
};


struct DRUI : Module {
	/** Every feature is a PARAM, not merely an internal flag.

	Params are saved with the patch by Rack itself, carry a right-click menu, and can be
	mapped to a controller — "map a knob to switch cable tracing" costs nothing to allow now
	and would break saved patches to add later. The plain booleans below are a mirror of them,
	kept because the overlays hold pointers to individual flags.
	*/
	enum ParamId {
		P_JACKS, P_KNOBS, P_CABLE_COLOR, P_CABLE_FLOW,
		P_PINCH, P_SLIDER_SCROLL, P_CLICK_CABLES,
		// Appended rather than inserted: a param's INDEX is what a saved patch stores, so
		// putting a new one in the middle would silently shift everything after it.
		P_TRACE, P_SCOPES, P_WIDGETS, NUM_PARAMS
	};

	DRUIOptions opt;

	DRUI() {
		// The outputs belong to the injectors. They carry no jacks on the panel: an injector
		// is cabled from one of them to the port it drives, and both that cable and its plugs
		// are hidden, because what should be visible is the callout loop at the terminal.
		config(NUM_PARAMS, 0, INJECT_MAX, 0);
		configSwitch(P_JACKS, 0.f, 1.f, 1.f, "Jacks by signal family", {"Off", "On"});
		configSwitch(P_KNOBS, 0.f, 1.f, 1.f, "Knobs", {"Off", "On"});
		configSwitch(P_CABLE_COLOR, 0.f, 1.f, 1.f, "Cable colour by destination", {"Off", "On"});
		configSwitch(P_CABLE_FLOW, 0.f, 1.f, 1.f, "Animate cable directions", {"Off", "On"});
		configSwitch(P_PINCH, 0.f, 1.f, 1.f, "Pinch to zoom", {"Off", "On"});
		configSwitch(P_SLIDER_SCROLL, 0.f, 1.f, 1.f, "Scroll wheel adjusts sliders", {"Off", "On"});
		// Off by default: it changes the most basic gesture in Rack.
		configSwitch(P_CLICK_CABLES, 0.f, 1.f, 0.f, "Click to pick up and drop cables", {"Off", "On"});
		configSwitch(P_TRACE, 0.f, 1.f, 1.f, "Cable trace assist", {"Off", "On"});
		configSwitch(P_SCOPES, 0.f, 1.f, 1.f, "Oscilloscopes on terminals", {"Off", "On"});
		configSwitch(P_WIDGETS, 0.f, 1.f, 1.f, "Signal widgets on terminals", {"Off", "On"});
		for (int i = 0; i < INJECT_MAX; i++)
			configOutput(i, string::f("Injector %d", i + 1));
	}

	/** The engine calls this once per sample, which is what makes an audio-rate capture
	possible from a plugin at all. Everything expensive is behind one atomic load in
	tapCaptureAll, so a patch with no scope open pays almost nothing. */
	void process(const ProcessArgs& args) override {
		tapSetSampleRate(args.sampleRate);
		tapCaptureAll();
		injectorProcessAll(this, args.sampleTime);
	}

	/** Copies the params into the flags the overlays read. Called from the widget's step, on
	the UI thread, which is where every one of those flags is used. */
	void syncOptions() {
		opt.jacks = params[P_JACKS].getValue() > 0.5f;
		opt.knobs = params[P_KNOBS].getValue() > 0.5f;
		opt.cableColor = params[P_CABLE_COLOR].getValue() > 0.5f;
		opt.cableFlow = params[P_CABLE_FLOW].getValue() > 0.5f;
		opt.pinchZoom = params[P_PINCH].getValue() > 0.5f;
		opt.sliderScroll = params[P_SLIDER_SCROLL].getValue() > 0.5f;
		opt.clickCables = params[P_CLICK_CABLES].getValue() > 0.5f;
		opt.trace = params[P_TRACE].getValue() > 0.5f;
		opt.scopes = params[P_SCOPES].getValue() > 0.5f;
		opt.widgets = params[P_WIDGETS].getValue() > 0.5f;
	}

	json_t* dataToJson() override {
		// Cable colours are saved in the patch, and tracing a cable dims the others by lowering
		// their alpha, so the true colours have to be back before anything is written. The
		// trace itself is KEPT: this also runs for the periodic autosave, and dropping it there
		// would have put the traced cable out every fifteen seconds.
		cableFocusPrepareSave();

		json_t* rootJ = json_object();
		// Scopes live in the rack, not in this module, but this module's JSON is where the
		// patch has room for them. Saving them here means a patch reopens looking at the same
		// signals, with the same scales, rather than losing every probe on close.
		json_object_set_new(rootJ, "scopes", scopeToJson());
		json_object_set_new(rootJ, "injectors", injectorToJson());
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		// Migration: patches written before the features became params carry them here. Read
		// into the params so an old patch opens set up as it was left.
		auto read = [&](const char* key, int paramId) {
			json_t* j = json_object_get(rootJ, key);
			if (j)
				params[paramId].setValue(json_boolean_value(j) ? 1.f : 0.f);
		};
		read("jacks", P_JACKS);
		read("knobs", P_KNOBS);
		read("cableColor", P_CABLE_COLOR);
		read("cableFlow", P_CABLE_FLOW);
		read("pinchZoom", P_PINCH);
		read("sliderScroll", P_SLIDER_SCROLL);
		read("clickCables", P_CLICK_CABLES);
		scopeFromJson(json_object_get(rootJ, "scopes"));
		injectorFromJson(json_object_get(rootJ, "injectors"));
	}
};


// ---- Drawing ----

static void drawJack(NVGcontext* vg, math::Vec c, float r, NVGcolor color, bool isOutput) {

	const float rh = r * 0.53f;

	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, r);
	nvgFillColor(vg, color);
	nvgFill(vg);
	nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 200));
	nvgStrokeWidth(vg, r * 0.1f);
	nvgStroke(vg);

	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, rh);
	nvgFillColor(vg, nvgRGB(0x2f, 0x2f, 0x33));
	nvgFill(vg);

	// DIRECTION, by shape: an output's dashes hug the outer edge of the coloured band, an
	// input's hug the hole. A third of the band wide, equal dash and gap, the count taken
	// from the circumference so the rhythm reads evenly at any size.
	const float band = r - rh;
	if (band <= 0.f)
		return;
	const float w = band / 3.f;
	// Flush against whichever edge it marks, with no colour showing between: an output's
	// dashes meet the inner edge of the rim stroke (which straddles r, so its inner edge is
	// at 0.95r), an input's meet the hole. The dashes stay legible because the gaps BETWEEN
	// them are the jack's colour, not because of any margin on either side.
	const float ringR = isOutput
		? (r * 0.95f - w / 2.f)
		: (rh + w / 2.f);
	if (ringR <= 0.f)
		return;
	const float circ = 2.f * M_PI * ringR;
	const int n = std::max(6, (int) std::round(circ / (w * 1.6f)));
	const float step = 2.f * M_PI / n;

	nvgStrokeColor(vg, nvgRGB(0, 0, 0));
	nvgStrokeWidth(vg, w);
	nvgLineCap(vg, NVG_BUTT);
	for (int i = 0; i < n; i++) {
		nvgBeginPath(vg);
		nvgArc(vg, c.x, c.y, ringR, i * step, i * step + step / 2.f, NVG_CW);
		nvgStroke(vg);
	}

}


static NVGcolor lighten(NVGcolor c, float amount) {
	return nvgRGBAf(
		math::clamp(c.r + (1.f - c.r) * amount * 0.45f, 0.f, 1.f),
		math::clamp(c.g + (1.f - c.g) * amount * 0.45f, 0.f, 1.f),
		math::clamp(c.b + (1.f - c.b) * amount * 0.45f, 0.f, 1.f), c.a);
}

/** A disc shaded by three stops. nanovg gradients carry two colours, so a multi-stop radial
has to be built from concentric bands, largest drawn first. */
static void drawShadedDisc(NVGcontext* vg, math::Vec c, float r, float mid,
	NVGcolor inner, NVGcolor middle, NVGcolor outer) {

	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, r);
	nvgFillPaint(vg, nvgRadialGradient(vg, c.x, c.y, r * mid, r, middle, outer));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, r * mid);
	nvgFillPaint(vg, nvgRadialGradient(vg, c.x, c.y, 0.f, r * mid, inner, middle));
	nvgFill(vg);
}

/** The knob cap: four stops running MONOTONICALLY LIGHTER from centre to rim, closely
spaced. Both properties matter — a bright-dip-bright pattern, or widely spaced stops, stop
reading as brushed metal and start reading as a target. */
static void drawCapDome(NVGcontext* vg, math::Vec c, float cap) {
	const NVGcolor s0 = nvgRGB(0x3a, 0x3d, 0x43);
	const NVGcolor s1 = nvgRGB(0x4c, 0x50, 0x58);
	const NVGcolor s2 = nvgRGB(0x5a, 0x5f, 0x67);
	const NVGcolor s3 = nvgRGB(0x6b, 0x70, 0x79);

	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, cap);
	nvgFillPaint(vg, nvgRadialGradient(vg, c.x, c.y, cap * 0.62f, cap, s2, s3));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, cap * 0.62f);
	nvgFillPaint(vg, nvgRadialGradient(vg, c.x, c.y, cap * 0.4f, cap * 0.62f, s1, s2));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, cap * 0.4f);
	nvgFillPaint(vg, nvgRadialGradient(vg, c.x, c.y, 0.f, cap * 0.4f, s0, s1));
	nvgFill(vg);
}


void druiDrawKnob(NVGcontext* vg, math::Vec c, float r, float angle, int ticks) {
	const float cap = r * 0.72f;

	drawShadedDisc(vg, c, r, 0.55f,
		nvgRGB(0x16, 0x88, 0xcc), nvgRGB(0x00, 0x6d, 0xa8), nvgRGB(0x00, 0x3d, 0x62));
	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, r);
	nvgStrokeColor(vg, nvgRGB(0x6f, 0xa8, 0xd6));
	nvgStrokeWidth(vg, r * 0.077f);
	nvgStroke(vg);

	drawCapDome(vg, c, cap);

	// The ring separating cap from rim, so the knob reads as two concentric parts rather
	// than one blurry disc.
	nvgBeginPath(vg);
	nvgCircle(vg, c.x, c.y, cap);
	nvgStrokeColor(vg, nvgRGB(0xb8, 0xb8, 0xbc));
	nvgStrokeWidth(vg, r * 0.06f);
	nvgStroke(vg);

	// Ticks and pointer rotate together: the ticks are grip texture, not a calibration scale.
	nvgSave(vg);
	nvgTranslate(vg, c.x, c.y);
	nvgRotate(vg, angle);

	nvgStrokeColor(vg, nvgRGB(0xff, 0xff, 0xff));
	nvgStrokeWidth(vg, r * 0.1f);
	nvgLineCap(vg, NVG_BUTT);
	for (int k = 0; k < ticks; k++) {
		const float frac = (ticks == 1) ? 0.5f : (float) k / (ticks - 1);
		const float a = (frac - 0.5f) * 2.f * (150.f * M_PI / 180.f);
		nvgBeginPath(vg);
		nvgMoveTo(vg, std::sin(a) * r * 0.78f, -std::cos(a) * r * 0.78f);
		nvgLineTo(vg, std::sin(a) * r * 1.04f, -std::cos(a) * r * 1.04f);
		nvgStroke(vg);
	}

	nvgBeginPath(vg);
	nvgMoveTo(vg, 0.f, 0.f);
	nvgLineTo(vg, 0.f, -cap);
	nvgStrokeColor(vg, nvgRGB(0xb8, 0xb8, 0xbc));
	nvgStrokeWidth(vg, r * 0.12f);
	nvgLineCap(vg, NVG_ROUND);
	nvgStroke(vg);

	nvgRestore(vg);
}


// ---- Cable geometry ----

/** Rack's own cable slump, reproduced.

Rack computes this in a static function that is not exported, so a plugin has to carry its
own copy. It reads settings::cableTension, which IS public, so the curve stays in step with
whatever the user has set rather than drifting away from the cable it is drawn over.
*/
static math::Vec cableSlump(math::Vec pos1, math::Vec pos2) {
	const float dist = pos1.minus(pos2).norm();
	math::Vec avg = pos1.plus(pos2).div(2);
	avg.y += (1.0 - settings::cableTension) * (150.0 + 1.0 * dist);
	return avg;
}

/** Marching ants along a cable: black dashes crawling source to destination at half the
cable's width, their length keyed to the DESTINATION's family.

nanovg has no dash support of any kind, so what is one attribute in SVG is a hand-rolled walk
along the curve by arc length. The crawl states direction only; it is NOT synchronised with
the signal.
*/
static void drawFlowDashes(NVGcontext* vg, math::Vec p0, math::Vec ctrl, math::Vec p1,
	float thickness, float dashUnits, float time) {

	const int SAMPLES = 40;
	math::Vec pts[SAMPLES + 1];
	float cum[SAMPLES + 1];
	pts[0] = p0;
	cum[0] = 0.f;
	for (int i = 1; i <= SAMPLES; i++) {
		const float t = (float) i / SAMPLES;
		const float u = 1.f - t;
		pts[i] = p0.mult(u * u).plus(ctrl.mult(2.f * u * t)).plus(p1.mult(t * t));
		cum[i] = cum[i - 1] + pts[i].minus(pts[i - 1]).norm();
	}
	const float total = cum[SAMPLES];
	if (total <= 0.f)
		return;

	const float dash = dashUnits * thickness;
	const float gap = 2.6f * thickness;
	const float period = dash + gap;
	// 5.5 mm/s in Rack's own pixels at 75 DPI: a slow drift.
	const float phase = std::fmod(time * 5.5f * (75.f / 25.4f), period);

	auto pointAt = [&](float s) -> math::Vec {
		if (s <= 0.f)
			return pts[0];
		if (s >= total)
			return pts[SAMPLES];
		int i = 1;
		while (i < SAMPLES && cum[i] < s)
			i++;
		const float segLen = cum[i] - cum[i - 1];
		const float f = (segLen > 0.f) ? (s - cum[i - 1]) / segLen : 0.f;
		return pts[i - 1].plus(pts[i].minus(pts[i - 1]).mult(f));
	};

	nvgStrokeColor(vg, nvgRGB(0, 0, 0));
	nvgStrokeWidth(vg, thickness / 2.f);
	nvgLineCap(vg, NVG_BUTT);
	for (float start = phase - period; start < total; start += period) {
		const float a = std::fmax(0.f, start);
		const float b = std::fmin(total, start + dash);
		if (b <= a)
			continue;
		nvgBeginPath(vg);
		math::Vec pa = pointAt(a);
		nvgMoveTo(vg, pa.x, pa.y);
		for (int i = 1; i < SAMPLES; i++) {
			if (cum[i] > a && cum[i] < b)
				nvgLineTo(vg, pts[i].x, pts[i].y);
		}
		math::Vec pb = pointAt(b);
		nvgLineTo(vg, pb.x, pb.y);
		nvgStroke(vg);
	}
}


// ---- The overlay ----

/** Added as the LAST child of the RackWidget, so it draws after every module and can paint
over their jacks and knobs. Transparent and event-free: it never consumes a click, so
everything underneath behaves exactly as it did. */
struct DRUIOverlay : widget::TransparentWidget {
	DRUI* module = NULL;
	/** Cables already coloured, by engine cable id, so a colour the user changes afterwards
	is not overwritten on the next frame. */
	/** Where each cable was last seen to END, so a cable that is re-plugged is recoloured.

	Remembering merely that a cable HAD been coloured meant it kept the colour of the first
	port it was ever plugged into: move an audio cable to a gate input and it stayed yellow.
	The colour belongs to the destination, so what has to be remembered is the destination.
	*/
	std::map<int64_t, std::string> cableDestination;

	DRUIOverlay() {
		box.pos = math::Vec();
		box.size = math::Vec(1e5, 1e5);
	}

	DRUIOptions options() {
		return module ? module->opt : DRUIOptions();
	}

	/** A widget's centre in the coordinate system this overlay draws in.

	The ancestor MUST be the RackWidget and not this overlay. getRelativeOffset walks UP
	from the widget until it meets the ancestor, and this overlay is a SIBLING of the
	container the modules live in, never an ancestor of anything. Passing `this` therefore
	never matches, the walk runs all the way to the root, and the result is in scene
	coordinates — off by the rack's whole scroll offset, which put every jack and dash far
	outside the window. This overlay sits at the rack's origin, so rack coordinates are its
	own.
	*/
	math::Vec centreOf(widget::Widget* w) {
		return w->getRelativeOffset(w->box.zeroPos().getCenter(), APP->scene->rack);
	}

	void step() override {
		if (!module) {
			widget::TransparentWidget::step();
			return;
		}
		const DRUIOptions o = options();

		// Colour newly connected cables by their destination. There is no "cable connected"
		// hook available to a plugin, so this polls — but only acts once per cable, so a
		// colour changed afterwards by hand survives.
		if (o.cableColor) {
			for (CableWidget* cw : APP->scene->rack->getCompleteCables()) {
				if (!cw->cable || !cw->inputPort)
					continue;
				const std::string dest = string::f("%lld:%d",
					(long long) (cw->inputPort->module ? cw->inputPort->module->id : -1),
					cw->inputPort->portId);
				auto it = cableDestination.find(cw->cable->id);
				if (it != cableDestination.end() && it->second == dest)
					continue;   // Same destination as last time: leave whatever colour it has.
				cableDestination[cw->cable->id] = dest;
				std::string name;
				if (engine::PortInfo* info = cw->inputPort->getPortInfo())
					name = info->getName();
				cw->color = familyColor(guessFamily(name));
			}
		}

		// A cable in flight takes the colour of the port it came FROM, so it reads as that
		// signal from the moment it leaves the jack. Rack gives a new cable the next colour in
		// its own rotating palette, which says nothing about the signal. Recoloured every frame
		// rather than once, because dragging an existing cable off a port and dropping it back
		// re-uses the same widget.
		if (o.cableColor) {
			for (CableWidget* cw : APP->scene->rack->getIncompleteCables()) {
				PortWidget* origin = cw->outputPort ? cw->outputPort : cw->inputPort;
				if (!origin)
					continue;
				std::string name;
				if (engine::PortInfo* info = origin->getPortInfo())
					name = info->getName();
				cw->color = familyColor(guessFamily(name));
			}
		}

		// ---- Per-frame housekeeping. Everything below has to run every frame, and all of it
		// was lost in one edit once before: a text replacement that removed the bipolar watcher
		// swallowed the block that followed it, and the symptom was scopes that saved correctly
		// and then never came back. Keep it together, and keep this note with it.

		// The three master switches. Scopes and injectors are HIDDEN rather than removed, so
		// switching one off and on again gives back what was there.
		scopeSetVisible(o.scopes);
		injectorSetEnabled(o.widgets);

		if (o.trace)
			cableFocusStep();
		else
			cableFocusClear();

		// Clips whose port has gone cannot remove themselves while the tree is being walked.
		clipPurgeDead();
		// Saved scopes and injectors re-attach here, once the modules they name have loaded.
		scopeRestoreStep();
		injectorRestoreStep();
		// And any cable out of DRUI that no injector owns is not a cable at all.
		injectorPurgeStrayCables();

		widget::TransparentWidget::step();
	}

	void drawControlsOf(ModuleWidget* mw, const DrawArgs& args, const DRUIOptions& o) {
		std::vector<PortWidget*> ports = mw->getPorts();
		if (o.jacks) {
			for (PortWidget* p : ports) {
				if (!p->isVisible())
					continue;
				const float r = std::fmin(p->box.size.x, p->box.size.y) / 2.f;
				if (r <= 1.f)
					continue;
				const math::Vec c = centreOf(p);
				const bool isOutput = (p->type == engine::Port::OUTPUT);

				std::string name;
				if (engine::PortInfo* info = p->getPortInfo())
					name = info->getName();

				drawJack(args.vg, c, r, familyColor(guessFamily(name)), isOutput);
			}
		}

		if (o.knobs) {
			std::vector<ParamWidget*> params = mw->getParams();
			for (ParamWidget* pw : params) {
				Knob* knob = dynamic_cast<Knob*>(pw);
				if (!knob || !knob->isVisible())
					continue;
				const float r = std::fmin(knob->box.size.x, knob->box.size.y) / 2.f;
				if (r <= 1.f)
					continue;
				const math::Vec c = centreOf(knob);
				float frac = 0.5f;
				if (engine::ParamQuantity* pq = knob->getParamQuantity())
					frac = math::clamp(pq->getScaledValue(), 0.f, 1.f);
				// The knob's OWN angle range, so it still sweeps the arc its author intended.
				const float angle = knob->minAngle + frac * (knob->maxAngle - knob->minAngle);
				druiDrawKnob(args.vg, c, r, angle, 7);
			}
		}
	}

	void draw(const DrawArgs& args) override {
		if (!module)
			return;
		const DRUIOptions o = options();

		if (o.jacks || o.knobs) {
			for (ModuleWidget* mw : APP->scene->rack->getModules())
				drawControlsOf(mw, args, o);
		}

		widget::TransparentWidget::draw(args);
	}

	/** The flow dashes are drawn here rather than in draw(), because cables are not drawn in
	the ordinary pass at all: RackWidget draws its children, and only THEN calls
	drawLayer(3), which is where CableWidget paints. A dash drawn in draw() is therefore
	painted over by the very cable it belongs to, and vanishes. Drawing in the same layer
	puts the dashes back on top, since this overlay is the rack's last child.
	*/
	void drawLayer(const DrawArgs& args, int layer) override {
		if (module && layer == 3) {
			const DRUIOptions o = options();
			if (o.cableFlow) {
				const float time = (float) APP->window->getFrameTime();
				for (CableWidget* cw : APP->scene->rack->getCompleteCables()) {
					if (!cw->inputPort || !cw->outputPort)
						continue;
					// Any hidden cable takes its dashes with it — one kept out of the way while
					// another is traced, and an injector's, which is hidden on purpose. The dashes
					// are drawn by us rather than by Rack, so hiding the cable alone would have
					// left them crawling along nothing.
					if (!cw->visible)
						continue;
					math::Vec p0 = centreOf(cw->outputPort);
					math::Vec p1 = centreOf(cw->inputPort);
					math::Vec ctrl = cableSlump(p0, p1);
					// Match the inset the cable's own draw applies to each end.
					p0 = p0.plus(ctrl.minus(p0).normalize().mult(14.f));
					p1 = p1.plus(ctrl.minus(p1).normalize().mult(14.f));

					std::string name;
					if (engine::PortInfo* info = cw->inputPort->getPortInfo())
						name = info->getName();
					drawFlowDashes(args.vg, p0, ctrl, p1, 6.f,
						flowDashLength(guessFamily(name)), time);
				}
			}
			// With the cables, and after them, so the pill sits on top of the cable it belongs
			// to rather than under the ones crossing it.
			if (o.trace)
				cableFocusDraw(args.vg);
		}
		widget::TransparentWidget::drawLayer(args, layer);
	}
};


// ---- The module's own small panel ----

/** Drawn rather than loaded: this plugin ships no artwork, which keeps it free of any
licensing entanglement and means there is nothing to keep in step with the code. */
/** The panel: eight rows, each a switch you can see the state of.

Everything this plugin does used to live in a right-click menu, which meant the module looked
like it did nothing at all — no use in a screenshot and no use to someone meeting it for the
first time. The face is now the feature list.

Drawn rather than loaded from artwork, which keeps the plugin free of any licensing
entanglement and means there is nothing to keep in step with the code.
*/
static const float PANEL_W = 6 * RACK_GRID_WIDTH;
static const float ROW_H = 26.f;
static const float ROW_TOP = 54.f;
static const float ROW_X = 3.f;
/** The button itself: a small round cap, with its label beside it rather than inside it. */
static const float CAP_R = 5.5f;
static const float CAP_CX = 11.f;

static const NVGcolor PANEL_BG = nvgRGB(0x16, 0x1a, 0x20);
static const NVGcolor PANEL_INK = nvgRGB(0xe6, 0xe8, 0xec);
static const NVGcolor LAMP_ON = nvgRGB(0x3d, 0xe0, 0x7a);


struct DRUIPanel : widget::Widget {
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFillColor(args.vg, PANEL_BG);
		nvgFill(args.vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		// Title band, so the name reads at rack distance.
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, 28);
		nvgFillColor(args.vg, nvgRGB(0x24, 0x2a, 0x33));
		nvgFill(args.vg);

		nvgFontSize(args.vg, 14);
		nvgFillColor(args.vg, PANEL_INK);
		nvgText(args.vg, box.size.x / 2, 14, "DreamRack", NULL);

		nvgFontSize(args.vg, 7);
		nvgFillColor(args.vg, nvgRGB(0x8a, 0x90, 0x9a));
		nvgText(args.vg, box.size.x / 2, 40, "OPT-CLICK A JACK", NULL);

		nvgFillColor(args.vg, nvgRGB(0x6d, 0x74, 0x80));
		nvgText(args.vg, box.size.x / 2, box.size.y - 14, "Dreamer", NULL);
		nvgText(args.vg, box.size.x / 2, box.size.y - 5, "Development", NULL);

		Widget::draw(args);
	}
};


/** One feature: a round push button with its name beside it.

The BUTTON is small and the CLICK TARGET is the whole row. Those are deliberately different
things — a cap five pixels across is something to aim at, and there is no reason to make
anyone aim when the label belongs to the same control and the row is otherwise empty.
*/
struct FeatureButton : app::Switch {
	std::string label;
	/** A second line, used only where a caption is too long for the panel's width. Wrapping the
	few that need it keeps the module narrow, which matters more than uniform captions. */
	std::string label2;

	FeatureButton() {
		box.size = math::Vec(PANEL_W - ROW_X * 2, ROW_H - 2.f);
	}

	void draw(const DrawArgs& args) override {
		const bool on = getParamQuantity() && getParamQuantity()->getValue() > 0.5f;
		const float cy = box.size.y / 2.f;

		// A cap with a rim and a lit face, so it reads as something that has been pressed in
		// rather than as a lamp that happens to be on.
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, CAP_CX, cy, CAP_R + 1.5f);
		nvgFillColor(args.vg, nvgRGB(0x0f, 0x12, 0x17));
		nvgFill(args.vg);

		nvgBeginPath(args.vg);
		nvgCircle(args.vg, CAP_CX, cy, CAP_R);
		nvgFillPaint(args.vg, nvgRadialGradient(args.vg, CAP_CX, cy - 1.f, 0.5f, CAP_R,
			on ? nvgRGB(0x7d, 0xff, 0xaa) : nvgRGB(0x4a, 0x50, 0x59),
			on ? nvgRGB(0x24, 0xa8, 0x58) : nvgRGB(0x2a, 0x2f, 0x36)));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, on ? LAMP_ON : nvgRGB(0x1e, 0x22, 0x29));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 7.5f);
		nvgFillColor(args.vg, on ? PANEL_INK : nvgRGB(0x83, 0x89, 0x93));
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		const float tx = CAP_CX + CAP_R + 5.f;
		if (label2.empty()) {
			nvgText(args.vg, tx, cy, label.c_str(), NULL);
		}
		else {
			nvgText(args.vg, tx, cy - 4.5f, label.c_str(), NULL);
			nvgText(args.vg, tx, cy + 4.5f, label2.c_str(), NULL);
		}
	}
};


struct DRUIWidget : ModuleWidget {
	/** WEAK pointers, and that is the whole point. These widgets are added to the rack and the
	scene, so those own them and delete them with the rest of the tree at shutdown. A raw
	pointer here meant this destructor then deleted them a second time, and reached through
	APP->scene->rack to do it while the rack itself was half destroyed. That crashed on quit,
	inside the module browser's teardown. A WeakPtr goes NULL the moment the widget is gone,
	so both mistakes become impossible to make. */
	WeakPtr<DRUIOverlay> overlay;
	/** Screen-anchored, unlike the rack overlay: it stands in for the whole viewport. */
	WeakPtr<widget::Widget> pinchOverlay;
	/** Also screen-anchored, and kept as the Scene's last child so it sees events first. */
	WeakPtr<widget::Widget> sliderOverlay;

	DRUIWidget(DRUI* module) {
		setModule(module);
		box.size = Vec(PANEL_W, RACK_GRID_HEIGHT);

		DRUIPanel* panel = new DRUIPanel;
		panel->box.size = box.size;
		setPanel(panel);

		// Sliders are not here on purpose: they stay a param, saved and mappable, but live in
		// the right-click menu. Everything else is on the face, so a picture of the panel is a
		// list of what the module does.
		struct Row { int param; const char* a; const char* b; };
		static const Row rows[] = {
			{DRUI::P_JACKS,         "COLOUR CODE",    "JACKS"},
			{DRUI::P_CABLE_COLOR,   "COLOUR CODE",    "CABLES"},
			{DRUI::P_KNOBS,         "CONSISTENT",     "KNOB STYLE"},
			{DRUI::P_CABLE_FLOW,    "ANIMATE CABLE",  "DIRECTIONS"},
			// Ordered by what will catch someone's eye first, not by how the code is arranged.
			{DRUI::P_SCOPES,        "OSCILLOSCOPES",  "ON TERMINALS"},
			{DRUI::P_WIDGETS,       "SIGNAL WIDGETS", "ON TERMINALS"},
			{DRUI::P_PINCH,         "PINCH TO ZOOM",  ""},
			{DRUI::P_TRACE,         "CABLE TRACE",    "ASSIST"},
			{DRUI::P_CLICK_CABLES,  "CLICK TO PULL",  "CABLES"},
		};
		for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
			FeatureButton* b = createParam<FeatureButton>(
				Vec(ROW_X, ROW_TOP + ROW_H * i), module, rows[i].param);
			b->label = rows[i].a;
			b->label2 = rows[i].b;
			addParam(b);
		}

		// The injectors' output jacks. Present so their cables are ordinary, complete Rack
		// cables — which is what makes Rack responsible for removing them when a module goes
		// away — but invisible, and therefore unclickable, so the panel stays a legend rather
		// than a patch bay.
		for (int i = 0; i < INJECT_MAX; i++) {
			PortWidget* p = createOutput<PJ301MPort>(
				Vec(box.size.x / 2 - 12, RACK_GRID_HEIGHT - 40), module, i);
			p->visible = false;
			addOutput(p);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	}

	void step() override {
		// Install the overlay once the widget is in the scene. This is why the module exists:
		// a plugin is initialised before the scene is created and gets no callback afterwards,
		// so a module's step() is the first moment any of this can run.
		// NOTHING is installed without a module. The module browser builds a preview widget of
		// every module it shows, and those step like any other — so this used to add a fresh
		// set of overlays to the rack each time the browser was opened, and then destroy them
		// when it closed.
		DRUI* m = dynamic_cast<DRUI*>(module);
		if (!m) {
			ModuleWidget::step();
			return;
		}

		// The params are the truth; the flags the overlays read are a copy of them, refreshed
		// here every frame. Without this the buttons moved and nothing else did.
		m->syncOptions();

		if (!overlay && APP->scene && APP->scene->rack) {
			DRUIOverlay* o = new DRUIOverlay;
			o->module = m;
			// Added last, so it draws after every module.
			APP->scene->rack->addChild(o);
			overlay = o;
		}
		if (!pinchOverlay && APP->scene) {
			widget::Widget* o = createPinchZoomOverlay(&m->opt.pinchZoom);
			APP->scene->addChild(o);
			pinchOverlay = o;
		}
		if (!sliderOverlay && APP->scene) {
			widget::Widget* o = createInterceptOverlay(&m->opt.sliderScroll, &m->opt.clickCables,
				&m->opt.scopes, &m->opt.widgets, &m->opt.trace, &m->opt.demoPointer);
			APP->scene->addChild(o);
			sliderOverlay = o;
		}
		ModuleWidget::step();
	}

	/** Takes an overlay away, if it is still there. Asks the widget's OWN parent rather than
	assuming which one it is, so this is safe whether the module is being deleted from a live
	patch or the whole tree is coming down around it. */
	static void dropOverlay(widget::Widget* o) {
		if (!o)
			return;
		if (o->parent)
			o->parent->removeChild(o);
		delete o;
	}

	~DRUIWidget() {
		dropOverlay(overlay);
		dropOverlay(pinchOverlay);
		dropOverlay(sliderOverlay);
	}

	void appendContextMenu(Menu* menu) override {
		DRUI* m = dynamic_cast<DRUI*>(module);
		if (!m)
			return;

		menu->addChild(new MenuSeparator);
		menu->addChild(createBoolPtrMenuItem("Scroll wheel adjusts sliders", "",
			&m->opt.sliderScroll));
		menu->addChild(createBoolPtrMenuItem("Draw pointer (for VCV Recorder)", "",
			&m->opt.demoPointer));

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Knobs draw over LED rings on some"));
		menu->addChild(createMenuLabel("modules; switch them off if that"));
		menu->addChild(createMenuLabel("matters to you."));
	}
};


Model* modelDRUI = createModel<DRUI, DRUIWidget>("DRUI");
