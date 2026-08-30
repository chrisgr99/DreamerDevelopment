/** The two modules, and the overlay that draws over everyone else's.

WHY A MODULE EXISTS AT ALL. A Rack plugin cannot run code until one of its modules is placed:
plugins are initialised before the scene is created, and there is no callback afterwards. So a
module's first job is to install the overlays that do the drawing, and its second is to carry
the settings — one param per feature, shown as a button on its face, because a module whose
every feature hides in a right-click menu looks like a module that does nothing.

WHY TWO OF THEM. They are different kinds of thing. CLARITY changes how every module in the
rack is drawn and handled — it is ambient, and once switched on you stop thinking about it.
TEST GEAR is the things you place and attach to a terminal. Someone may well want one and not the
other, and with a single module there was no way to say so.

They share everything below. The settings live in one structure the overlays read, each module
writing its own fields; a feature whose module is not in the rack is off, because its group's
flags are cleared when the last of them goes. The overlays themselves are installed by
whichever module appears first and taken away when the last one leaves, so either module works
on its own.

TEST GEAR OWNS THE AUDIO-RATE WORK: the scope taps and the injectors both ride in its process(),
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
#include "Monitor.hpp"
#include "Hint.hpp"
#include "Sink.hpp"

#include <map>
#include <set>
#include <vector>
#include <cmath>

using namespace rack;


// ---- The signal-family colour code. Colour carries FAMILY, shape carries DIRECTION. ----
// Two readings, two cues, and never one cue carrying both: a jack has to answer "what kind of
// signal" and "which way does it go" at once, and folding them together would make each answer
// depend on reading the other.

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

struct Options {
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


/** ONE set of flags, shared by both modules and read by every overlay.

The overlays hold pointers to individual flags, which is why these are plain booleans rather
than being read from params directly: a pointer into a module's own storage would dangle the
moment that module was deleted. Living here, they outlive any module — and a group is cleared
when the last module that owns it goes, so a feature whose module is not in the rack is off
rather than stuck at whatever it was last set to.
*/
static Options gOpt;

/** How many of each module are in the rack, counted by their widgets. */
static int gClarityCount = 0;
static int gTestGearCount = 0;

static void clearClarityOptions() {
	gOpt.jacks = gOpt.knobs = gOpt.cableColor = gOpt.cableFlow = false;
	gOpt.pinchZoom = gOpt.sliderScroll = gOpt.clickCables = gOpt.trace = false;
}

static void clearWidgetOptions() {
	gOpt.scopes = gOpt.widgets = false;
}


/** Everything the rack is drawn and handled by: colour, animation, and the gestures. */
struct Clarity : Module {
	/** Every feature is a PARAM, not merely an internal flag.

	Params are saved with the patch by Rack itself, carry a right-click menu, and can be
	mapped to a controller — "map a knob to switch cable tracing" costs nothing to allow now
	and would break saved patches to add later.
	*/
	enum ParamId {
		P_JACKS, P_CABLE_COLOR, P_KNOBS, P_CABLE_FLOW,
		P_PINCH, P_TRACE, P_CLICK_CABLES, P_SLIDER_SCROLL,
		NUM_PARAMS
	};

	Clarity() {
		config(NUM_PARAMS, 0, 0, 0);
		configSwitch(P_JACKS, 0.f, 1.f, 1.f, "Colour code jacks", {"Off", "On"});
		configSwitch(P_CABLE_COLOR, 0.f, 1.f, 1.f, "Colour code cables", {"Off", "On"});
		configSwitch(P_KNOBS, 0.f, 1.f, 1.f, "Consistent knob style", {"Off", "On"});
		configSwitch(P_CABLE_FLOW, 0.f, 1.f, 1.f, "Animate cable directions", {"Off", "On"});
		configSwitch(P_PINCH, 0.f, 1.f, 1.f, "Pinch to zoom", {"Off", "On"});
		configSwitch(P_TRACE, 0.f, 1.f, 1.f, "Cable trace assist", {"Off", "On"});
		// ON, and not on the panel. It began as an alternative to Rack's drag, which is why it
		// shipped off and wore a switch — but a release after any movement now lands the cable
		// exactly as a drag always did, so holding the button and not holding it are the same
		// gesture and there is nothing to choose between. What is left is a switch in the menu,
		// for the one case that still differs: a click on a jack that never moves picks the
		// cable up, where Rack would do nothing.
		configSwitch(P_CLICK_CABLES, 0.f, 1.f, 1.f, "Click to pull cables", {"Off", "On"});
		configSwitch(P_SLIDER_SCROLL, 0.f, 1.f, 1.f, "Scroll wheel adjusts sliders",
			{"Off", "On"});
	}

	/** Copies the params into the flags the overlays read. Called from the widget's step, on
	the UI thread, which is where every one of those flags is used. */
	void syncOptions() {
		gOpt.jacks = params[P_JACKS].getValue() > 0.5f;
		gOpt.knobs = params[P_KNOBS].getValue() > 0.5f;
		gOpt.cableColor = params[P_CABLE_COLOR].getValue() > 0.5f;
		gOpt.cableFlow = params[P_CABLE_FLOW].getValue() > 0.5f;
		gOpt.pinchZoom = params[P_PINCH].getValue() > 0.5f;
		gOpt.trace = params[P_TRACE].getValue() > 0.5f;
		gOpt.clickCables = params[P_CLICK_CABLES].getValue() > 0.5f;
		gOpt.sliderScroll = params[P_SLIDER_SCROLL].getValue() > 0.5f;
	}

	json_t* dataToJson() override {
		// Cable colours are saved in the patch, and tracing a cable dims the others by lowering
		// their alpha, so the true colours have to be back before anything is written. The
		// trace itself is KEPT: this also runs for the periodic autosave, and dropping it there
		// would have put the traced cable out every fifteen seconds.
		cableFocusPrepareSave();

		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "demoPointer", json_boolean(gOpt.demoPointer));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		if (json_t* j = json_object_get(rootJ, "demoPointer"))
			gOpt.demoPointer = json_boolean_value(j);
	}
};


/** The things you clip onto a terminal, and the audio-rate work behind them. */
struct TestGear : Module {
	/** The injectors' hidden outputs, and then the one real jack on the face. */
	enum OutputId { O_MONITOR = INJECT_MAX, NUM_OUTPUTS };

	TestGear() {
		// Most of the outputs belong to the injectors. They carry no jacks on the panel: an
		// injector is cabled from one of them to the port it drives, and both that cable and
		// its plugs are hidden, because what should be visible is the callout loop at the
		// terminal.
		config(0, SINK_MAX, NUM_OUTPUTS, 0);
		for (int i = 0; i < INJECT_MAX; i++)
			configOutput(i, string::f("Injector %d", i + 1));
		configOutput(O_MONITOR, "Monitor");
		// The sinks: hidden inputs that a watched output can be cabled to, so that it computes.
		for (int i = 0; i < SINK_MAX; i++)
			configInput(i, string::f("Sink %d", i + 1));
	}

	/** NO SWITCHES. The widgets used to be gated by two buttons on this face, which said what
	the module offered but also offered a way to make everything you had attached disappear.
	Nothing needs gating: a widget exists because you asked for one, and the way not to have
	one is not to add it. The panel says what is on offer instead. */
	void syncOptions() {
		gOpt.scopes = true;
		gOpt.widgets = true;
	}

	/** The engine calls this once per sample, which is what makes an audio-rate capture
	possible from a plugin at all. Everything expensive is behind one atomic load in
	tapCaptureAll, so a patch with no scope open pays almost nothing.

	ONE MODULE DOES THIS, even if a rack holds several. Capturing twice per sample would write
	each sample into every tap's ring twice, so a scope would show a signal at double speed.
	*/
	void process(const ProcessArgs& args) override {
		if (owner() != this)
			return;
		tapSetSampleRate(args.sampleRate);
		tapCaptureAll();
		injectorProcessAll(this, args.sampleTime);

		outputs[O_MONITOR].setChannels(1);
		outputs[O_MONITOR].setVoltage(monitorMix(args.sampleTime));
	}

	/** The one instance that speaks for all of them: the lowest module id, which is stable
	across a session and does not depend on the order widgets happen to step in. */
	TestGear* owner() {
		TestGear* best = NULL;
		for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
			TestGear* w = dynamic_cast<TestGear*>(mw->module);
			if (w && (!best || w->id < best->id))
				best = w;
		}
		return best;
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		// Scopes and injectors live in the RACK, not in this module, but this module's JSON is
		// where the patch has room for them. Saving them here means a patch reopens looking at
		// the same signals, with the same scales, rather than losing every probe on close.
		// Only the owner writes them, or a rack with two of these would restore two of each.
		if (owner() != this)
			return rootJ;
		json_object_set_new(rootJ, "scopes", scopeToJson());
		json_object_set_new(rootJ, "injectors", injectorToJson());
		json_object_set_new(rootJ, "analysers", analyserToJson());
		json_object_set_new(rootJ, "monitors", monitorToJson());
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		scopeFromJson(json_object_get(rootJ, "scopes"));
		injectorFromJson(json_object_get(rootJ, "injectors"));
		analyserFromJson(json_object_get(rootJ, "analysers"));
		monitorFromJson(json_object_get(rootJ, "monitors"));
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

	/** The shared flags. Not a module's own copy: this overlay outlives any one module, and
	either module can be the one that put it here. */
	Options options() {
		return gOpt;
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
		const Options o = options();

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
		analyserSetVisible(o.scopes);
		monitorSetVisible(o.widgets);
		injectorSetEnabled(o.widgets);

		if (o.trace)
			cableFocusStep();
		else
			cableFocusClear();

		// Clips whose port has gone cannot remove themselves while the tree is being walked.
		clipPurgeDead();
		// Saved scopes and injectors re-attach here, once the modules they name have loaded.
		scopeRestoreStep();
		analyserRestoreStep();
		monitorRestoreStep();
		injectorRestoreStep();
		// And any cable out of the Test Gear module that no injector owns is not a cable at all.
		injectorPurgeStrayCables();
		// Outputs being watched by a viewer are woken with a hidden cable, since a module does
		// not compute an output that nothing is patched to.
		sinkStep();

		widget::TransparentWidget::step();
	}

	void drawControlsOf(ModuleWidget* mw, const DrawArgs& args, const Options& o) {
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
		const Options o = options();

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
		if (layer == 3) {
			const Options o = options();
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
static const float ROW_TOP = 62.f;
static const float ROW_X = 3.f;
/** The button itself: a small round cap, with its label beside it rather than inside it. */
static const float CAP_R = 5.5f;
static const float CAP_CX = 11.f;

static const NVGcolor PANEL_BG = nvgRGB(0x16, 0x1a, 0x20);
static const NVGcolor PANEL_INK = nvgRGB(0xe6, 0xe8, 0xec);
static const NVGcolor LAMP_ON = nvgRGB(0x3d, 0xe0, 0x7a);

/** PROPORTIONAL FACES, not the monospaced one Rack uses on its own panels.

A fixed-width face is right where digits must not shift as they change — the instrument faces
use it for exactly that. It is the wrong face for words: every letter is set in a slot wide
enough for the widest one, so the spacing fights the shapes and a caption is harder to read
than it needs to be. A module called Clarity should not be the hardest thing on the rack to
read.
*/
static std::shared_ptr<window::Font> panelFont() {
	return APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
}

static std::shared_ptr<window::Font> panelTitleFont() {
	return APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf"));
}


struct DRUIPanel : widget::Widget {
	/** A smaller line ABOVE the name, where the name is really two words and only the second
	is the name. Wrapped rather than set on one line because the lettering is what was pushing
	these panels wider than they need to be. */
	std::string titleAbove;
	std::string title;
	/** Lines about how the module is used. Set BELOW the list: the list is what the panel is
	for, and the instruction is a footnote to it rather than a preamble. */
	std::vector<std::string> hint;
	/** A heading over the list, where the list needs naming. */
	std::string legendTitle;
	/** Named down the face: what this module puts on a terminal, under headings, because a
	list of thirteen is a list and a list of two groups is a description. */
	struct LegendItem { std::string text; bool heading; };
	std::vector<LegendItem> legend;
	/** Written above the module's one jack, where it has one. */
	std::string jackLabel;

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFillColor(args.vg, PANEL_BG);
		nvgFill(args.vg);

		std::shared_ptr<window::Font> font = panelFont();
		std::shared_ptr<window::Font> titleFace = panelTitleFont();
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

		// Title band, so the name reads at rack distance. INSIDE the border, not under it: drawn
		// to the panel's own edge it covered the green line along the top, which read as a
		// border someone had forgotten to finish.
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 4.f, 4.f, box.size.x - 8.f, 25.f);
		nvgFillColor(args.vg, nvgRGB(0x24, 0x2a, 0x33));
		nvgFill(args.vg);

		nvgFillColor(args.vg, PANEL_INK);
		if (titleFace && titleFace->handle >= 0)
			nvgFontFaceId(args.vg, titleFace->handle);
		if (titleAbove.empty()) {
			nvgFontSize(args.vg, 15);
			nvgText(args.vg, box.size.x / 2, 14, title.c_str(), NULL);
		}
		else {
			nvgFontSize(args.vg, 8.5f);
			nvgText(args.vg, box.size.x / 2, 8, titleAbove.c_str(), NULL);
			nvgFontSize(args.vg, 14);
			nvgText(args.vg, box.size.x / 2, 20, title.c_str(), NULL);
		}
		nvgFontFaceId(args.vg, font->handle);

		// A rule under the title, and another under the list below: the panel has three things
		// on it — what this is, how to use it, and what there is — and the lines say so without
		// spending a word.
		auto rule = [&](float y, NVGcolor color) {
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, 10.f, y);
			nvgLineTo(args.vg, box.size.x - 10.f, y);
			nvgStrokeColor(args.vg, color);
			nvgStrokeWidth(args.vg, 1.f);
			nvgStroke(args.vg);
		};
		float y = 40.f;
		// Green under the title, tying the head of the panel to its border. The rule further
		// down stays white: that one separates one kind of text from another rather than
		// closing off the title.
		if (!legend.empty() || !hint.empty() || !legendTitle.empty())
			rule(33.f, LAMP_ON);

		if (!legendTitle.empty()) {
			if (titleFace && titleFace->handle >= 0)
				nvgFontFaceId(args.vg, titleFace->handle);
			nvgFontSize(args.vg, 13.f);
			nvgFillColor(args.vg, PANEL_INK);
			nvgText(args.vg, box.size.x / 2, y + 4.f, legendTitle.c_str(), NULL);
			nvgFontFaceId(args.vg, font->handle);
			y += 17.f;
		}

		// What is on offer, listed. A panel carrying two switches says nothing about what the
		// module actually gives you, and the list is the answer to "what can I clip on?"
		// without opening a menu to find out.
		// One list, no groupings. Naming the two kinds meant two headings competing with the
		// thirteen names they introduced, on a panel whose whole job is to name them.
		nvgFontSize(args.vg, 9.f);
		nvgFillColor(args.vg, PANEL_INK);
		for (const LegendItem& item : legend) {
			nvgText(args.vg, box.size.x / 2, y, item.text.c_str(), NULL);
			y += 12.f;
		}
		if (!hint.empty()) {
			rule(y - 3.f, nvgRGBA(0xe6, 0xe8, 0xec, 0x7f));
			y += 10.f;
			nvgFontSize(args.vg, 8.5f);
			for (const std::string& line : hint) {
				nvgText(args.vg, box.size.x / 2, y, line.c_str(), NULL);
				y += 11.f;
			}
		}

		if (!jackLabel.empty())
			nvgText(args.vg, box.size.x / 2, box.size.y - 76.f, jackLabel.c_str(), NULL);

		// Clear of the border along the foot, which the lower line used to sit on top of.
		nvgFontSize(args.vg, 8.f);
		nvgFillColor(args.vg, nvgRGB(0x8a, 0x90, 0x9a));
		nvgText(args.vg, box.size.x / 2, box.size.y - 24, "Dreamer", NULL);
		nvgText(args.vg, box.size.x / 2, box.size.y - 14, "Development", NULL);

		// LAST, so nothing is drawn over it — the band along the top used to hide the whole of
		// its top edge.
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 3.f, 3.f, box.size.x - 6.f, box.size.y - 6.f, 6.f);
		nvgStrokeColor(args.vg, LAMP_ON);
		nvgStrokeWidth(args.vg, 1.2f);
		nvgStroke(args.vg);

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

		std::shared_ptr<window::Font> font = panelFont();
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 8.5f);
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


/** The overlays, shared by both modules.

Installed by whichever module reaches its step() first and taken away when the last module
goes. They are shared because the features are: a widget chooser on a jack is no use without
the click handling that opens it, and cable tracing draws in the same pass as the injectors'
hidden cables. Kept as WEAK pointers, since the rack and the scene own these widgets and
delete them with the rest of the tree at shutdown — a raw pointer here was deleted a second
time on quit, reaching through a half-destroyed rack to do it.
*/
static WeakPtr<DRUIOverlay> gRackOverlay;
static WeakPtr<widget::Widget> gPinchOverlay;
static WeakPtr<widget::Widget> gInterceptOverlay;


static void installOverlays() {
	if (!APP->scene || !APP->scene->rack)
		return;
	if (!gRackOverlay) {
		DRUIOverlay* o = new DRUIOverlay;
		// Added last, so it draws after every module.
		APP->scene->rack->addChild(o);
		gRackOverlay = o;
	}
	if (!gPinchOverlay) {
		widget::Widget* o = createPinchZoomOverlay(&gOpt.pinchZoom);
		APP->scene->addChild(o);
		gPinchOverlay = o;
	}
	if (!gInterceptOverlay) {
		widget::Widget* o = createInterceptOverlay(&gOpt.sliderScroll, &gOpt.clickCables,
			&gOpt.scopes, &gOpt.widgets, &gOpt.trace, &gOpt.demoPointer);
		APP->scene->addChild(o);
		gInterceptOverlay = o;
	}
}


/** Takes an overlay away, if it is still there. Asks the widget's OWN parent rather than
assuming which one it is, so this is safe whether a module is being deleted from a live patch
or the whole tree is coming down around it. */
static void dropOverlay(widget::Widget* o) {
	if (!o)
		return;
	if (o->parent)
		o->parent->removeChild(o);
	delete o;
}


static void removeOverlaysIfIdle() {
	if (gClarityCount > 0 || gTestGearCount > 0)
		return;
	hintDismiss();
	dropOverlay(gRackOverlay);
	dropOverlay(gPinchOverlay);
	dropOverlay(gInterceptOverlay);
	gRackOverlay = NULL;
	gPinchOverlay = NULL;
	gInterceptOverlay = NULL;
}


/** What both module widgets do the same way: draw a panel, count themselves, and put the
overlays in place. NOTHING is installed without a module — the module browser builds a preview
widget of every module it shows, and those step like any other, so this used to add a fresh set
of overlays to the rack each time the browser was opened.
*/
struct DRUIWidgetBase : ModuleWidget {
	bool counted = false;

	void buildPanel(const char* titleAbove, const char* title,
		const std::vector<std::string>& hint,
		const std::vector<DRUIPanel::LegendItem>& legend, const char* jackLabel = "",
		const char* legendTitle = "") {

		box.size = Vec(PANEL_W, RACK_GRID_HEIGHT);
		DRUIPanel* panel = new DRUIPanel;
		panel->box.size = box.size;
		panel->titleAbove = titleAbove;
		panel->title = title;
		panel->hint = hint;
		panel->legendTitle = legendTitle;
		panel->legend = legend;
		panel->jackLabel = jackLabel;
		setPanel(panel);

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	}

	void addRow(int index, int paramId, const char* a, const char* b) {
		FeatureButton* button = createParam<FeatureButton>(
			Vec(ROW_X, ROW_TOP + ROW_H * index), module, paramId);
		button->label = a;
		button->label2 = b;
		addParam(button);
	}

	void countIn(int& counter) {
		if (counted || !module)
			return;
		counted = true;
		counter++;
	}
};


struct ClarityWidget : DRUIWidgetBase {
	ClarityWidget(Clarity* module) {
		setModule(module);
		buildPanel("", "Clarity", {}, {}, "", "Features");

		// Sliders are not here on purpose: they stay a param, saved and mappable, but live in
		// the right-click menu. Everything else is on the face, so a picture of the panel is a
		// list of what the module does.
		struct Row { int param; const char* a; const char* b; };
		static const Row rows[] = {
			{Clarity::P_JACKS,         "Colour code",   "jacks"},
			{Clarity::P_CABLE_COLOR,   "Colour code",   "cables"},
			{Clarity::P_KNOBS,         "Consistent",    "knob style"},
			{Clarity::P_CABLE_FLOW,    "Animate cable", "directions"},
			{Clarity::P_PINCH,         "Pinch to zoom", ""},
			{Clarity::P_TRACE,         "Cable trace",   "assist"},
			// Back on the face, and not because it needs choosing — it is on by default and
			// both gestures work at once. It is here because this panel is the list of what
			// the module does, and because a gesture this fundamental should have a visible
			// way out if it ever gets in the way of something we have not thought of.
			{Clarity::P_CLICK_CABLES,  "Add cables",    "without dragging"},
		};
		for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++)
			addRow((int) i, rows[i].param, rows[i].a, rows[i].b);
	}

	~ClarityWidget() {
		if (counted)
			gClarityCount--;
		if (gClarityCount <= 0)
			clearClarityOptions();
		removeOverlaysIfIdle();
	}

	void step() override {
		Clarity* m = dynamic_cast<Clarity*>(module);
		if (!m) {
			ModuleWidget::step();
			return;
		}
		countIn(gClarityCount);
		// The params are the truth; the flags the overlays read are a copy of them, refreshed
		// here every frame. Without this the buttons moved and nothing else did.
		m->syncOptions();
		installOverlays();
		ModuleWidget::step();
	}

	void appendContextMenu(Menu* menu) override {
		if (!module)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createBoolPtrMenuItem("Draw pointer (for screen recordings)", "",
			&gOpt.demoPointer));
		menu->addChild(createMenuItem("Show tips again", "", []() { hintResetAll(); }));

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Knobs draw over LED rings on some"));
		menu->addChild(createMenuLabel("modules; switch them off if that"));
		menu->addChild(createMenuLabel("matters to you."));
	}
};


struct TestGearWidget : DRUIWidgetBase {
	TestGearWidget(TestGear* module) {
		setModule(module);
		// The last line is the menu entry's own wording, so the panel and the menu agree.
		buildPanel("", "Test Gear",
			{"Right-click any", "terminal and select", "\"Widgets\". It follows",
			"the pointer. Click", "to place it."}, {
			{"Scope", false}, {"Analyser", false}, {"Audio monitor", false},
			{"Gate", false}, {"Pulse", false}, {"Clock", false}, {"DC level", false},
			{"LFO", false}, {"VCO", false}, {"Note", false}, {"Volt/oct", false},
			{"Noise", false}, {"Attenuverter", false},
		}, "Monitor out", "Widgets");

		// The one jack this module has: the monitors' mixing bus, out to your interface.
		addOutput(createOutput<PJ301MPort>(
			Vec(box.size.x / 2 - 12, RACK_GRID_HEIGHT - 68), module, TestGear::O_MONITOR));

		// The sink jacks, hidden like the injectors' — nothing reads them; they exist so a cable
		// can be laid into them, which is what makes an output compute. See Sink.cpp.
		for (int i = 0; i < SINK_MAX; i++) {
			PortWidget* p = createInput<PJ301MPort>(
				Vec(box.size.x / 2 - 12, RACK_GRID_HEIGHT - 40), module, i);
			p->visible = false;
			addInput(p);
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
	}

	~TestGearWidget() {
		if (counted)
			gTestGearCount--;
		if (gTestGearCount <= 0)
			clearWidgetOptions();
		removeOverlaysIfIdle();
	}

	void step() override {
		TestGear* m = dynamic_cast<TestGear*>(module);
		if (!m) {
			ModuleWidget::step();
			return;
		}
		countIn(gTestGearCount);
		m->syncOptions();
		installOverlays();
		ModuleWidget::step();
	}
};


Model* modelClarity = createModel<Clarity, ClarityWidget>("Clarity");
Model* modelTestGear = createModel<TestGear, TestGearWidget>("TestGear");
