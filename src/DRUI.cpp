/** DRUI — clearer jacks, cables and knobs across every module in the rack.

WHY A MODULE EXISTS AT ALL. A Rack plugin cannot run code until one of its modules is
placed: plugins are initialised before the scene is created, and there is no callback
afterwards. So DRUI is a small module whose job is to install an overlay into the scene and
carry the options. Its only ports are the injectors' hidden outputs.

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
	bool bipolar = true;
	bool pinchZoom = true;
	bool sliderScroll = true;
};


struct DRUI : Module {
	DRUIOptions opt;

	DRUI() {
		// The outputs belong to the injectors. They carry no jacks on the panel: an injector
		// is cabled from one of them to the port it drives, and both that cable and its plugs
		// are hidden, because what should be visible is the callout loop at the terminal.
		config(0, 0, INJECT_MAX, 0);
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

	json_t* dataToJson() override {
		// Cable colours are saved in the patch, and tracing a cable dims the others by lowering
		// their alpha, so the true colours have to be back before anything is written. The
		// trace itself is KEPT: this also runs for the periodic autosave, and dropping it there
		// would have put the traced cable out every fifteen seconds.
		cableFocusPrepareSave();

		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "jacks", json_boolean(opt.jacks));
		json_object_set_new(rootJ, "knobs", json_boolean(opt.knobs));
		json_object_set_new(rootJ, "cableColor", json_boolean(opt.cableColor));
		json_object_set_new(rootJ, "cableFlow", json_boolean(opt.cableFlow));
		json_object_set_new(rootJ, "bipolar", json_boolean(opt.bipolar));
		json_object_set_new(rootJ, "pinchZoom", json_boolean(opt.pinchZoom));
		json_object_set_new(rootJ, "sliderScroll", json_boolean(opt.sliderScroll));
		// Scopes live in the rack, not in this module, but this module's JSON is where the
		// patch has room for them. Saving them here means a patch reopens looking at the same
		// signals, with the same scales, rather than losing every probe on close.
		json_object_set_new(rootJ, "scopes", scopeToJson());
		json_object_set_new(rootJ, "injectors", injectorToJson());
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		auto read = [&](const char* key, bool& target) {
			json_t* j = json_object_get(rootJ, key);
			if (j)
				target = json_boolean_value(j);
		};
		read("jacks", opt.jacks);
		read("knobs", opt.knobs);
		read("cableColor", opt.cableColor);
		read("cableFlow", opt.cableFlow);
		read("bipolar", opt.bipolar);
		read("pinchZoom", opt.pinchZoom);
		read("sliderScroll", opt.sliderScroll);
		scopeFromJson(json_object_get(rootJ, "scopes"));
		injectorFromJson(json_object_get(rootJ, "injectors"));
	}
};


/** Whether a port has been seen to go negative.

Nothing in Rack declares a port bipolar, so it is inferred by watching. A port is marked only
once it has actually swung below zero, which means an idle or unpatched port stays unmarked
until it runs — an honest limit of observation rather than a bug.
*/
static std::map<std::string, bool> bipolarSeen;

static std::string portKey(Module* module, bool isOutput, int portId) {
	return string::f("%lld:%d:%d", (long long) (module ? module->id : -1), isOutput ? 1 : 0, portId);
}


// ---- Drawing ----

static void drawJack(NVGcontext* vg, math::Vec c, float r, NVGcolor color, bool isOutput,
	bool isBipolar) {

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

	// BIPOLAR, by a second shape rather than another colour: a small notch at the top, so it
	// reads alongside the family colour instead of competing with it.
	if (isBipolar) {
		nvgBeginPath(vg);
		nvgCircle(vg, c.x, c.y - r * 0.98f, r * 0.16f);
		nvgFillColor(vg, nvgRGB(0xff, 0xff, 0xff));
		nvgFill(vg);
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
	std::set<int64_t> colouredCables;

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
				if (colouredCables.count(cw->cable->id))
					continue;
				colouredCables.insert(cw->cable->id);
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

		// Watch for ports that go negative. Nothing declares a port bipolar, so it is learned
		// by observation and only ever latches on.
		if (o.bipolar) {
			for (ModuleWidget* mw : APP->scene->rack->getModules()) {
				Module* m = mw->module;
				if (!m)
					continue;
				for (size_t i = 0; i < m->outputs.size(); i++) {
					if (m->outputs[i].getVoltage() < -0.05f)
						bipolarSeen[portKey(m, true, (int) i)] = true;
				}
			}
		}

		cableFocusStep();

		// Clips whose port has gone cannot remove themselves while the tree is being walked.
		clipPurgeDead();
		// And saved ones re-attach here, once the modules they name have finished loading.
		scopeRestoreStep();
		injectorRestoreStep();
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

				bool bip = false;
				if (o.bipolar && p->module)
					bip = bipolarSeen.count(portKey(p->module, isOutput, p->portId)) > 0;

				drawJack(args.vg, c, r, familyColor(guessFamily(name)), isOutput, bip);
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
			cableFocusDraw(args.vg);
		}
		widget::TransparentWidget::drawLayer(args, layer);
	}
};


// ---- The module's own small panel ----

/** Drawn rather than loaded: this plugin ships no artwork, which keeps it free of any
licensing entanglement and means there is nothing to keep in step with the code. */
struct DRUIPanel : widget::Widget {
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFillColor(args.vg, nvgRGB(0x1a, 0x1e, 0x24));
		nvgFill(args.vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (font && font->handle >= 0) {
			nvgFontFaceId(args.vg, font->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

			nvgFontSize(args.vg, 13);
			nvgFillColor(args.vg, nvgRGB(0xe6, 0xe8, 0xec));
			nvgText(args.vg, box.size.x / 2, 22, "DRUI", NULL);

			// A legend for the code the plugin applies, on the one panel that can explain it.
			nvgFontSize(args.vg, 7.5);
			nvgFillColor(args.vg, nvgRGB(0x98, 0x9e, 0xa8));
			const char* lines[] = {"colour", "= signal", "family", "", "ring out", "= output",
				"ring in", "= input"};
			float y = 60;
			for (const char* line : lines) {
				nvgText(args.vg, box.size.x / 2, y, line, NULL);
				y += 11;
			}
		}
		Widget::draw(args);
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
		box.size = Vec(3 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		DRUIPanel* panel = new DRUIPanel;
		panel->box.size = box.size;
		setPanel(panel);

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
			widget::Widget* o = createInterceptOverlay(&m->opt.sliderScroll);
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
		menu->addChild(createMenuLabel("Show"));
		menu->addChild(createBoolPtrMenuItem("Jacks by signal family", "", &m->opt.jacks));
		menu->addChild(createBoolPtrMenuItem("Bipolar marking", "", &m->opt.bipolar));
		menu->addChild(createBoolPtrMenuItem("Knobs", "", &m->opt.knobs));
		menu->addChild(createBoolPtrMenuItem("Cable colour by destination", "", &m->opt.cableColor));
		menu->addChild(createBoolPtrMenuItem("Cable signal flow", "", &m->opt.cableFlow));

		menu->addChild(new MenuSeparator);
		menu->addChild(createBoolPtrMenuItem("Pinch to zoom", "", &m->opt.pinchZoom));
		menu->addChild(createBoolPtrMenuItem("Scroll wheel adjusts sliders", "", &m->opt.sliderScroll));

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Knobs draw over LED rings on some"));
		menu->addChild(createMenuLabel("modules; switch them off if that"));
		menu->addChild(createMenuLabel("matters to you."));
	}
};


Model* modelDRUI = createModel<DRUI, DRUIWidget>("DRUI");
