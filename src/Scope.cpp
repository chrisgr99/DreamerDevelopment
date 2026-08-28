/** Clip-on signal scope, ported from the forked Rack build.

Anchored to the PORT it probes rather than to the screen: it is a child of the RackWidget and
its position is recomputed each frame relative to that port, so it scrolls and zooms with its
module and follows if the module is moved.
*/
#include "plugin.hpp"
#include "SignalTap.hpp"
#include "WidgetAt.hpp"
#include "Clip.hpp"

#include <GLFW/glfw3.h>

#include <vector>
#include <cmath>
#include <algorithm>



// Calibrated 1-2-5 sequences, per division, replacing continuous auto-scaling. What makes a
// scale legible is that you can read "per division" off it. See design/scope.md section 4.
static const float V_DIVS[] = {
	0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.f, 2.f, 5.f, 10.f, 20.f,
};
static const int V_DIV_COUNT = 11;

static const float T_DIVS[] = {
	0.00002f, 0.00005f, 0.0001f, 0.0002f, 0.0005f, 0.001f, 0.002f, 0.005f,
	0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f,
};
static const int T_DIV_COUNT = 14;

// Roughly 10 by 4, the aspect the spec calls for.
static const int DIV_X = 10;
static const int DIV_Y = 4;

// The callout: ring at the jack, line to the scope, and a rounded grab tab where they meet.
// Wcoast CALLOUT_COLOR, the scope's own border grey, so it reads as part of the frame rather
// than as decoration. Opaque: the muted colour already reads as secondary.

// Wcoast SCOPE_HANDLE. Also the shortest the loop-to-scope line may get.

// The values box below the face. A readout, not a settings panel.
static const float VALUES_HEIGHT = 30.f;
// Lower-left transport button: a right-pointing triangle runs, two vertical bars pause.
static const float TRANSPORT_SIZE = 15.f;
/** About ten seconds at Rack's 30 Hz default. */
static const int AUTOSET_BUDGET = 300;
/** Samples autoset insists on before it will commit to a scale.

The first version accepted anything over eight, which is a glimpse rather than a window: a
scope attached mid-cycle framed a quarter of a waveform at low amplitude, held that scale as
designed, and then overflowed once the signal reached full amplitude. Measured in one case at
256 samples with a peak of 1.7 V on a signal that actually swings several times that.

2048 samples is about 46 ms at 48 kHz — long enough to contain a whole cycle of anything down
to roughly 20 Hz, so the peak it measures is the real one. */
static const int AUTOSET_MIN_SAMPLES = 2048;
static const float BTN = 14.f;
static const float BTN_PAD = 4.f;
/** How far in from an edge still counts as grabbing it to resize. */
static const float RESIZE_EDGE = 6.f;
static const float MIN_W = 120.f, MIN_H = 60.f;
/** A paused scope wears a red frame, so a held trace can never be mistaken for a live one. */
static const NVGcolor FRAME_RUN = nvgRGB(0x2f, 0xd0, 0x6a);
static const NVGcolor FRAME_PAUSED = nvgRGB(0xe0, 0x3b, 0x3b);


/** Cursor shapes. Rack has no cursor API — it never calls glfwCreateStandardCursor — but the
GLFWwindow is reachable and the bundled GLFW is 3.4, which carries all four resize shapes. So
this needs no change to any SDK header. */
static GLFWcursor* cursorFor(int shape) {
	static GLFWcursor* cache[8] = {};
	static const int shapes[8] = {
		GLFW_ARROW_CURSOR, GLFW_RESIZE_EW_CURSOR, GLFW_RESIZE_NS_CURSOR,
		GLFW_RESIZE_NWSE_CURSOR, GLFW_RESIZE_NESW_CURSOR, 0, 0, 0,
	};
	for (int i = 0; i < 8; i++) {
		if (shapes[i] != shape)
			continue;
		if (!cache[i])
			cache[i] = glfwCreateStandardCursor(shape);
		return cache[i];
	}
	return NULL;
}

static void setCursorShape(int shape) {
	static int current = -1;
	if (shape == current)
		return;
	current = shape;
	if (APP && APP->window && APP->window->win)
		glfwSetCursor(APP->window->win, cursorFor(shape));
}


struct ScopeWidget : ClipWidget {
	int tapSlot = -1;

	int vDivIndex = 6;
	/** Volts at the vertical centre of the face. Autoset puts the signal's midpoint here, so a
	unipolar CV riding on an offset is centred rather than sitting off the top. */
	float vPos = 0.f;
	int tDivIndex = 8;
	bool autosetPending = true;
	/** Frames left to wait for a signal worth framing. Without a budget a scope opened before
	the sound is running would retry for ever, and it would be impossible to tell that from a
	broken autoset. */
	int autosetBudget = AUTOSET_BUDGET;
	bool frozen = false;
	bool acCoupled = false;

	/** Self-trigger on a rising edge at this level, in volts. */
	float triggerLevel = 0.f;
	/** How far the signal must travel the other way before a crossing counts again. Sized
	from the measured amplitude, because anti-aliased edges ring across the level several
	times and would otherwise each look like a separate trigger. */
	float triggerHyst = 0.05f;
	bool triggerRising = true;
	/** Auto free-runs when no edge arrives; Normal waits for one. */
	bool triggerAuto = true;

	/** The values box: shown by a click on the face, cycling scale -> freq -> peak. */
	bool valuesShown = false;
	int valueMode = 0;
	/** Hover drives the buttons' visibility, as in Wcoast. */
	bool hovered = false;
	bool gridShown = true;
	bool minimized = false;
	/** Follow mode: the scope rides the pointer, click-through, until a click or Escape
	deposits it where the pointer was. */
	bool following = false;
	/** The spot beside its terminal that the home button returns it to. */
	math::Vec homeOffset = math::Vec(30, -70);
	/** Which edge or corner is being dragged, as (x, y) in {-1, 0, 1}. */
	math::Vec resizeDir;
	bool resizing = false;

	std::vector<float> scratch;
	/** The last captured sweep, kept so a frozen scope still has something to show. */
	std::vector<float> lastWin;
	int lastCount = 0;
	/** Measured from the last drawn window, for the values box. */
	float measMin = 0.f, measMax = 0.f, measMean = 0.f, measFreq = 0.f;

	/** The face alone. The widget's own box grows to include the values box when it is shown,
	because Rack only dispatches a click to a child whose box CONTAINS the point — a box drawn
	outside the widget would be visible and completely unclickable. */
	float faceHeight = 110.f;

	ScopeWidget() {
		box.size = math::Vec(220, faceHeight);
		scratch.resize(TAP_BUFFER_SIZE);
	}

	~ScopeWidget() {
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
	}

	void step() override {
		// Anchored to the port, not to the screen, so the scope follows if the module moves.
		followPort();
		if (minimized) {
			box.size = math::Vec(BTN + BTN_PAD * 2, BTN + BTN_PAD * 2);
		}
		else {
			box.size.y = faceHeight + (valuesShown ? VALUES_HEIGHT + 3.f : 0.f);
		}

		// Follow mode: ride the pointer, just down and right of the tip. A pure position
		// write, no layout reads, which is what keeps it clear of the pointer-warp hazard the
		// accessibility zoom creates.
		if (following) {
			box.pos = APP->scene->rack->getMousePos().plus(math::Vec(14, 14));
			if (port) {
				math::Vec centre = port->getRelativeOffset(port->box.zeroPos().getCenter(), APP->scene->rack);
				offset = box.pos.minus(centre);
			}
		}
		OpaqueWidget::step();
	}

	/** Reads the newest samples and picks the window to draw, honouring the trigger.

	Two rules decide the window, and getting either wrong produces a trace that looks like a
	broken signal rather than a broken scope.

	AN EDGE ONLY COUNTS IF A WHOLE WINDOW FOLLOWS IT. The first version took the most recent
	edge it could find, which is usually within a few samples of the newest capture — leaving
	nothing after it to draw. The handful of samples that remained were then stretched across
	the full width, so a square wave arrived as a wandering line and the time base said 200 us
	per division while showing something else entirely.

	THE SEARCH COVERS THE WHOLE BUFFER. It used to look back only twice the drawn window,
	which for anything slower than a few hundred hertz contains no edge at all, so the trigger
	silently gave up and the trace slid. The buffer holds 170 ms, so this locks down to about
	6 Hz.

	The crossing is armed with hysteresis. Rack's oscillators are anti-aliased and their edges
	overshoot and ring, which crosses the trigger level several times per edge; without arming
	the scope picks a different one of those crossings each frame and the trace jitters.
	*/
	int gather(float* out, int wanted) {
		if (tapSlot < 0)
			return 0;
		const int have = tapRead(tapSlot, scratch.data(), (int) scratch.size());
		if (have <= 0)
			return 0;

		// A little pre-trigger, so the edge itself is visible rather than sitting on the frame.
		const int pre = wanted / DIV_X;
		int start = have - wanted;
		if (start < 0)
			start = 0;

		// The newest index an edge may sit at and still leave a full window behind it.
		const int latest = have - (wanted - pre);
		const float hyst = std::fmax(triggerHyst, 1e-4f);

		int edge = -1;
		bool armed = false;
		for (int i = 1; i <= latest; i++) {
			const float a = scratch[i - 1], b = scratch[i];
			if (triggerRising) {
				if (b < triggerLevel - hyst)
					armed = true;
				else if (armed && a < triggerLevel && b >= triggerLevel) {
					edge = i;
					armed = false;
				}
			}
			else {
				if (b > triggerLevel + hyst)
					armed = true;
				else if (armed && a > triggerLevel && b <= triggerLevel) {
					edge = i;
					armed = false;
				}
			}
		}

		if (edge >= 0)
			start = math::clamp(edge - pre, 0, std::max(0, have - wanted));
		else if (!triggerAuto)
			return 0;   // Normal mode shows nothing until an edge arrives

		const int count = std::min(wanted, have - start);
		for (int i = 0; i < count; i++)
			out[i] = scratch[start + i];
		return count;
	}

	/** One-shot: frames the signal with headroom, then HOLDS. A continuous auto-scale hides
	the very thing you want to watch, which is the trace breathing as the signal changes. */
	void autoset() {
		if (tapSlot < 0)
			return;
		const int have = tapRead(tapSlot, scratch.data(), 4096);

		// Not enough yet to judge a peak by. Keep waiting WITHOUT spending the budget: the tap
		// is plainly working, it simply has not filled.
		if (have >= 1 && have < AUTOSET_MIN_SAMPLES)
			return;

		if (have < 1) {
			// No samples AT ALL is a different matter from a signal that is silent or still
			// filling: the tap itself may not be running, which is a fault worth reporting
			// rather than waiting on for ever.
			if (--autosetBudget <= 0) {
				autosetPending = false;
				WARN("Scope autoset: gave up, no samples captured. Is the audio engine running?");
			}
			return;
		}

		float lo = scratch[0], hi = scratch[0], sum = 0.f;
		for (int i = 0; i < have; i++) {
			lo = std::fmin(lo, scratch[i]);
			hi = std::fmax(hi, scratch[i]);
			sum += scratch[i];
		}
		const float mid = sum / have;
		const float peak = std::fmax(std::fabs(hi - mid), std::fabs(lo - mid));

		// A flat signal has no peak to frame — so STAY ARMED and frame it the moment it
		// arrives. The budget is deliberately NOT spent here.
		//
		// This is the guard Wcoast states explicitly: "Only spend the give-up budget while the
		// context is RUNNING (else a scope created before sound-on would exhaust it on silence
		// and never auto-scale once sound starts)." Spending it on silence is exactly what made
		// a scope attached to a quiet port draw its signal at the default scale for ever after,
		// with no sign that anything had been decided.
		if (peak < 1e-4f)
			return;

		// Pick the FINEST scale the peak still fits inside, with a tenth of the half-height
		// left as headroom. Asking for the peak to occupy a fixed fraction and then rounding
		// the 1-2-5 scale up threw that fraction away: a 7.9 V peak asked for 6 V per division
		// and got 10, which drew the whole signal inside two fifths of the face.
		const float wantPerDiv = peak / ((DIV_Y / 2.f) * 0.9f);
		vDivIndex = 0;
		for (int i = 0; i < V_DIV_COUNT; i++) {
			vDivIndex = i;
			if (V_DIVS[i] >= wantPerDiv)
				break;
		}

		// Trigger at the midpoint, where a periodic signal crosses predictably, and centre the
		// face on it too.
		triggerLevel = mid;
		vPos = mid;

		// The trigger arms once the signal has swung a quarter of its amplitude the other way.
		triggerHyst = peak * 0.25f;

		// Time base from the mean interval between rising crossings of the midpoint, aiming
		// for a few cycles across the screen. Armed with the same hysteresis as the trigger:
		// counting bare crossings counted the ringing on each anti-aliased edge as extra
		// cycles, which reported a square wave as three times its real frequency and chose a
		// time base showing a fraction of one cycle.
		int crossings = 0;
		int first = -1, last = -1;
		bool armed = false;
		for (int i = 1; i < have; i++) {
			if (scratch[i] < mid - triggerHyst) {
				armed = true;
			}
			else if (armed && scratch[i - 1] < mid && scratch[i] >= mid) {
				if (first < 0)
					first = i;
				last = i;
				crossings++;
				armed = false;
			}
		}
		if (crossings >= 2 && last > first) {
			const float periodSamples = (float) (last - first) / (crossings - 1);
			const float sampleRate = APP->engine->getSampleRate();
			const float wantSpan = periodSamples / sampleRate * 3.f;   // about three cycles
			const float wantT = wantSpan / DIV_X;
			tDivIndex = T_DIV_COUNT - 1;
			for (int i = 0; i < T_DIV_COUNT; i++) {
				if (T_DIVS[i] >= wantT) {
					tDivIndex = i;
					break;
				}
			}
		}
		autosetPending = false;

		// Deliberately logged. This is the only evidence available that the audio-thread tap
		// is capturing at the engine's rate rather than the frame rate: a plausible peak and
		// a frame count in the tens of thousands after a second means real capture, whereas a
		// count near the frame rate would mean the tap is not running.
		INFO("Scope autoset: %d samples, min %.3f max %.3f mid %.3f -> %g V/div, %g s/div, %llu frames captured",
			have, lo, hi, mid, V_DIVS[vDivIndex], T_DIVS[tDivIndex],
			(unsigned long long) tapFrameCount(tapSlot));
	}

	/** Where the callout's ring sits, in this widget's coordinates. */

	/** Re-probing: a new tap on the new port, and a fresh look at the scale, since a
	different signal deserves one exactly as opening a scope does. */
	bool reattach(app::PortWidget* target) override {
		const int slot = tapCreate(target->module->id, target->portId,
			target->type == engine::Port::OUTPUT);
		if (slot < 0) {
			WARN("Scope: no tap slots available to re-probe");
			return false;
		}
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
		tapSlot = slot;
		port = target;
		autosetPending = true;
		autosetBudget = AUTOSET_BUDGET;
		INFO("Scope: re-probing %s port %d",
			target->type == engine::Port::OUTPUT ? "output" : "input", target->portId);
		return true;
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
		json_object_set_new(rootJ, "homeX", json_real(homeOffset.x));
		json_object_set_new(rootJ, "homeY", json_real(homeOffset.y));
		json_object_set_new(rootJ, "width", json_real(box.size.x));
		json_object_set_new(rootJ, "faceHeight", json_real(faceHeight));
		json_object_set_new(rootJ, "vDiv", json_integer(vDivIndex));
		json_object_set_new(rootJ, "vPos", json_real(vPos));
		json_object_set_new(rootJ, "tDiv", json_integer(tDivIndex));
		json_object_set_new(rootJ, "trigLevel", json_real(triggerLevel));
		json_object_set_new(rootJ, "trigHyst", json_real(triggerHyst));
		json_object_set_new(rootJ, "trigRising", json_boolean(triggerRising));
		json_object_set_new(rootJ, "trigAuto", json_boolean(triggerAuto));
		json_object_set_new(rootJ, "ac", json_boolean(acCoupled));
		json_object_set_new(rootJ, "grid", json_boolean(gridShown));
		json_object_set_new(rootJ, "minimized", json_boolean(minimized));
		json_object_set_new(rootJ, "values", json_boolean(valuesShown));
		json_object_set_new(rootJ, "valueMode", json_integer(valueMode));
		return rootJ;
	}

	void fromJson(json_t* rootJ) {
		auto num = [&](const char* key, float& target) {
			if (json_t* j = json_object_get(rootJ, key))
				target = json_number_value(j);
		};
		auto integer = [&](const char* key, int& target) {
			if (json_t* j = json_object_get(rootJ, key))
				target = json_integer_value(j);
		};
		auto boolean = [&](const char* key, bool& target) {
			if (json_t* j = json_object_get(rootJ, key))
				target = json_boolean_value(j);
		};
		num("offsetX", offset.x);
		num("offsetY", offset.y);
		num("homeX", homeOffset.x);
		num("homeY", homeOffset.y);
		num("width", box.size.x);
		num("faceHeight", faceHeight);
		integer("vDiv", vDivIndex);
		num("vPos", vPos);
		integer("tDiv", tDivIndex);
		num("trigLevel", triggerLevel);
		num("trigHyst", triggerHyst);
		boolean("trigRising", triggerRising);
		boolean("trigAuto", triggerAuto);
		boolean("ac", acCoupled);
		boolean("grid", gridShown);
		boolean("minimized", minimized);
		boolean("values", valuesShown);
		integer("valueMode", valueMode);
		vDivIndex = math::clamp(vDivIndex, 0, V_DIV_COUNT - 1);
		tDivIndex = math::clamp(tDivIndex, 0, T_DIV_COUNT - 1);
		// The saved scales ARE the settings: re-running autoset on load would discard
		// whatever the patch was saved looking at.
		autosetPending = false;
	}

	/** Measures the drawn window for the values box. Frequency comes from the mean interval
	between rising crossings of the mean, which is the same estimate autoset uses to pick a
	time base — it simply was not surfaced before. */
	void measure(const float* win, int count) {
		if (count < 2)
			return;
		float lo = win[0], hi = win[0], sum = 0.f;
		for (int i = 0; i < count; i++) {
			lo = std::fmin(lo, win[i]);
			hi = std::fmax(hi, win[i]);
			sum += win[i];
		}
		measMin = lo;
		measMax = hi;
		measMean = sum / count;

		int crossings = 0, first = -1, last = -1;
		for (int i = 1; i < count; i++) {
			if (win[i - 1] < measMean && win[i] >= measMean) {
				if (first < 0)
					first = i;
				last = i;
				crossings++;
			}
		}
		if (crossings >= 2 && last > first) {
			const float periodSamples = (float) (last - first) / (crossings - 1);
			const float sampleRate = APP->engine->getSampleRate();
			measFreq = (periodSamples > 0.f) ? sampleRate / periodSamples : 0.f;
		}
		else {
			measFreq = 0.f;
		}
	}

	std::string valuesText() {
		if (valueMode == 1) {
			if (measFreq <= 0.f)
				return "-- Hz";
			if (measFreq >= 1000.f)
				return string::f("%.3f kHz", measFreq / 1000.f);
			return string::f("%.2f Hz", measFreq);
		}
		if (valueMode == 2)
			return string::f("min %.2f  mean %.2f  max %.2f", measMin, measMean, measMax);
		return string::f("%g V/div    %s/div", V_DIVS[vDivIndex],
			T_DIVS[tDivIndex] >= 0.001f
				? string::f("%g ms", T_DIVS[tDivIndex] * 1000.f).c_str()
				: string::f("%g us", T_DIVS[tDivIndex] * 1000000.f).c_str());
	}

	math::Rect valuesBox() {
		return math::Rect(math::Vec(0, faceHeight + 3), math::Vec(box.size.x, VALUES_HEIGHT));
	}

	math::Rect closeBox()    { return math::Rect(math::Vec(BTN_PAD, BTN_PAD), math::Vec(BTN, BTN)); }
	math::Rect minBox()      { return math::Rect(math::Vec(BTN_PAD * 2 + BTN, BTN_PAD), math::Vec(BTN, BTN)); }
	math::Rect followBox()   { return math::Rect(math::Vec(box.size.x - BTN - BTN_PAD, BTN_PAD), math::Vec(BTN, BTN)); }
	math::Rect homeBox()     { return math::Rect(math::Vec(box.size.x - BTN * 2 - BTN_PAD * 2, BTN_PAD), math::Vec(BTN, BTN)); }
	/** The bottom row, left to right: pause/run, A for autoset, T for trigger mode, G for
	grid — the II A T G row on the Wcoast face. */
	math::Rect autoBox()     { return math::Rect(math::Vec(BTN_PAD * 2 + TRANSPORT_SIZE, faceHeight - BTN - BTN_PAD), math::Vec(BTN, BTN)); }
	math::Rect trigBox()     { return math::Rect(math::Vec(BTN_PAD * 3 + TRANSPORT_SIZE + BTN, faceHeight - BTN - BTN_PAD), math::Vec(BTN, BTN)); }
	math::Rect gridBox()     { return math::Rect(math::Vec(BTN_PAD * 4 + TRANSPORT_SIZE + BTN * 2, faceHeight - BTN - BTN_PAD), math::Vec(BTN, BTN)); }

	bool atHome() {
		return offset.minus(homeOffset).norm() < 1.f;
	}

	/** Which edge or corner the pointer is on, as (x, y) in {-1, 0, 1}. Zero means neither. */
	math::Vec resizeZoneAt(math::Vec pos) {
		math::Vec dir;
		if (minimized)
			return dir;
		if (pos.x <= RESIZE_EDGE)
			dir.x = -1;
		else if (pos.x >= box.size.x - RESIZE_EDGE)
			dir.x = 1;
		if (pos.y <= RESIZE_EDGE)
			dir.y = -1;
		else if (pos.y >= faceHeight - RESIZE_EDGE)
			dir.y = 1;
		// Inside the face proper, not on a border.
		if (pos.y > faceHeight)
			return math::Vec();
		return dir;
	}

	static int cursorForZone(math::Vec dir) {
		if (dir.x != 0.f && dir.y != 0.f)
			return (dir.x * dir.y > 0.f) ? GLFW_RESIZE_NWSE_CURSOR : GLFW_RESIZE_NESW_CURSOR;
		if (dir.x != 0.f)
			return GLFW_RESIZE_EW_CURSOR;
		if (dir.y != 0.f)
			return GLFW_RESIZE_NS_CURSOR;
		return GLFW_ARROW_CURSOR;
	}

	math::Rect transportBox() {
		return math::Rect(math::Vec(4, faceHeight - TRANSPORT_SIZE - 4),
			math::Vec(TRANSPORT_SIZE, TRANSPORT_SIZE));
	}

	void drawValues(NVGcontext* vg) {
		if (!valuesShown)
			return;
		const math::Rect r = valuesBox();

		nvgBeginPath(vg);
		nvgRoundedRect(vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 3);
		nvgFillColor(vg, nvgRGBA(0x10, 0x12, 0x16, 0xf0));
		nvgFill(vg);
		nvgStrokeColor(vg, nvgRGB(0x2f, 0xd0, 0x6a));
		nvgStrokeWidth(vg, 1.2f);
		nvgStroke(vg);

		// The top-edge triangle: clicking it cycles the mode, as clicking the box does.
		nvgBeginPath(vg);
		nvgMoveTo(vg, r.pos.x + r.size.x / 2 - 5, r.pos.y + 1);
		nvgLineTo(vg, r.pos.x + r.size.x / 2 + 5, r.pos.y + 1);
		nvgLineTo(vg, r.pos.x + r.size.x / 2, r.pos.y + 6);
		nvgFillColor(vg, nvgRGBA(0x2f, 0xd0, 0x6a, 0xc0));
		nvgFill(vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (font && font->handle >= 0) {
			nvgFontFaceId(vg, font->handle);
			nvgFontSize(vg, 11);
			nvgFillColor(vg, nvgRGB(0xe6, 0xe6, 0xe6));
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			const std::string text = valuesText();
			nvgText(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2 + 3, text.c_str(), NULL);
		}
	}

	void drawButton(NVGcontext* vg, math::Rect r, NVGcolor fill, const char* glyph, bool dim) {
		nvgBeginPath(vg);
		nvgCircle(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2, r.size.x / 2);
		NVGcolor c = fill;
		c.a = dim ? 0.35f : 1.f;
		nvgFillColor(vg, c);
		nvgFill(vg);

		if (!glyph || !glyph[0])
			return;
		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (font && font->handle >= 0) {
			nvgFontFaceId(vg, font->handle);
			nvgFontSize(vg, 10);
			nvgFillColor(vg, nvgRGBA(0x10, 0x12, 0x16, dim ? 0x60 : 0xff));
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgText(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2 + 1, glyph, NULL);
		}
	}

	/** Close, minimise, follow and home; grid sits with the transport at the bottom. All
	revealed on hover, as in Wcoast. */
	void drawButtons(NVGcontext* vg) {
		if (!hovered || minimized)
			return;
		drawButton(vg, closeBox(), nvgRGB(0xe0, 0x3b, 0x3b), "x", false);
		drawButton(vg, minBox(), nvgRGB(0xe8, 0xb3, 0x2a), "-", false);
		drawButton(vg, followBox(), following ? nvgRGB(0x2f, 0xd0, 0x6a) : nvgRGB(0xb8, 0xbc, 0xc4), "F", false);
		// Dimmed while it is already home: there is nothing to send.
		drawButton(vg, homeBox(), nvgRGB(0xb8, 0xbc, 0xc4), "<", atHome());
		// Lit while autoset is still waiting for a signal to frame, so a scope opened before
		// the sound is running does not look like it simply ignored the button.
		drawButton(vg, autoBox(), autosetPending ? nvgRGB(0x2f, 0xd0, 0x6a) : nvgRGB(0xb8, 0xbc, 0xc4), "A", false);
		drawButton(vg, trigBox(), triggerAuto ? nvgRGB(0xb8, 0xbc, 0xc4) : nvgRGB(0x2f, 0xd0, 0x6a), "T", false);
		drawButton(vg, gridBox(), gridShown ? nvgRGB(0xb8, 0xbc, 0xc4) : nvgRGB(0x5a, 0x5f, 0x67), "G", false);
	}

	/** Lower-left transport: a right-pointing triangle runs, two vertical bars pause. Shown
	on hover, as in Wcoast. */
	void drawTransport(NVGcontext* vg) {
		if (!hovered)
			return;
		const math::Rect r = transportBox();

		nvgBeginPath(vg);
		nvgRoundedRect(vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 3);
		nvgFillColor(vg, nvgRGBA(0x2a, 0x2e, 0x34, 0xd0));
		nvgFill(vg);

		nvgFillColor(vg, nvgRGB(0xe6, 0xe6, 0xe6));
		const float cx = r.pos.x + r.size.x / 2;
		const float cy = r.pos.y + r.size.y / 2;
		if (frozen) {
			// Paused, so offer Run.
			nvgBeginPath(vg);
			nvgMoveTo(vg, cx - 3, cy - 4);
			nvgLineTo(vg, cx + 4, cy);
			nvgLineTo(vg, cx - 3, cy + 4);
			nvgFill(vg);
		}
		else {
			nvgBeginPath(vg);
			nvgRect(vg, cx - 3.5f, cy - 4, 2.5f, 8);
			nvgRect(vg, cx + 1.f, cy - 4, 2.5f, 8);
			nvgFill(vg);
		}
	}

	void draw(const DrawArgs& args) override {
		drawCallout(args.vg);

		if (minimized) {
			// A token: it still drags and still shows its callout, but ignores clicks except
			// the plus that restores it.
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
			nvgFillColor(args.vg, nvgRGBA(0x10, 0x12, 0x16, 0xf0));
			nvgFill(args.vg);
			nvgStrokeColor(args.vg, frozen ? FRAME_PAUSED : FRAME_RUN);
			nvgStrokeWidth(args.vg, 1.5f);
			nvgStroke(args.vg);
			drawButton(args.vg, math::Rect(math::Vec(BTN_PAD, BTN_PAD), math::Vec(BTN, BTN)),
				nvgRGB(0x2f, 0xd0, 0x6a), "+", false);
			OpaqueWidget::draw(args);
			return;
		}

		const float w = box.size.x;
		const float h = faceHeight;

		// Face
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, w, h, 3);
		nvgFillColor(args.vg, nvgRGBA(0x10, 0x12, 0x16, 0xf0));
		nvgFill(args.vg);
		// A paused scope wears a RED frame. A held trace looks exactly like a live one, so the
		// frame is the only thing that can tell you which you are reading.
		nvgStrokeColor(args.vg, frozen ? FRAME_PAUSED : FRAME_RUN);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		// Graticule: light divisions, brighter centre cross. The divisions are what make a
		// calibrated scale readable.
		if (!gridShown)
			goto afterGrid;
		nvgStrokeWidth(args.vg, 0.6f);
		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x22));
		nvgBeginPath(args.vg);
		for (int i = 1; i < DIV_X; i++) {
			nvgMoveTo(args.vg, w * i / DIV_X, 0);
			nvgLineTo(args.vg, w * i / DIV_X, h);
		}
		for (int i = 1; i < DIV_Y; i++) {
			nvgMoveTo(args.vg, 0, h * i / DIV_Y);
			nvgLineTo(args.vg, w, h * i / DIV_Y);
		}
		nvgStroke(args.vg);

		nvgBeginPath(args.vg);
		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x40));
		nvgMoveTo(args.vg, 0, h / 2);
		nvgLineTo(args.vg, w, h / 2);
		nvgMoveTo(args.vg, w / 2, 0);
		nvgLineTo(args.vg, w / 2, h);
		nvgStroke(args.vg);

	afterGrid:
		if (autosetPending)
			autoset();

		// Capture unless frozen. Freezing HOLDS the last sweep rather than stopping the
		// drawing — a paused scope whose trace vanishes is showing you nothing, which is the
		// opposite of what pausing is for.
		if (tapSlot >= 0 && !frozen) {
			const float sampleRate = APP->engine->getSampleRate();
			const float span = T_DIVS[tDivIndex] * DIV_X;
			int wanted = (int) (span * sampleRate);
			wanted = math::clamp(wanted, 8, TAP_BUFFER_SIZE / 2);

			lastWin.resize(wanted);
			lastCount = gather(lastWin.data(), wanted);
			if (lastCount > 1)
				measure(lastWin.data(), lastCount);
		}

		// Draw whatever the last captured sweep was, frozen or running.
		if (lastCount > 1 && (int) lastWin.size() >= lastCount) {
			const int count = lastCount;

			// What sits at the vertical centre. AC coupling follows the signal's own mean, so a
			// small waveform on a large DC offset is readable; otherwise it is the vertical
			// position, which autoset sets to the signal's midpoint.
			float centreV = vPos;
			if (acCoupled) {
				float sum = 0.f;
				for (int i = 0; i < count; i++)
					sum += lastWin[i];
				centreV = sum / count;
			}

			const float perDiv = V_DIVS[vDivIndex];
			const float scale = (h / DIV_Y) / perDiv;

			// CLIP TO THE FACE. Without this a signal larger than the current scale draws
			// straight out of the scope and across the rack. A real scope clips: the trace
			// leaves the top and comes back. Scissoring does that honestly, whereas clamping
			// the values would draw a flat line along the edge and imply a signal that is
			// sitting still when it is off-screen.
			nvgSave(args.vg);
			nvgScissor(args.vg, 0, 0, w, h);

			nvgBeginPath(args.vg);
			for (int i = 0; i < count; i++) {
				const float x = w * i / (count - 1);
				float y = h / 2 - (lastWin[i] - centreV) * scale;
				// Still bounded, but far outside the face: nanovg does not need absurd
				// coordinates, and the scissor decides what is actually seen.
				y = math::clamp(y, -4.f * h, 5.f * h);
				if (i == 0)
					nvgMoveTo(args.vg, x, y);
				else
					nvgLineTo(args.vg, x, y);
			}
			nvgStrokeColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
			nvgStrokeWidth(args.vg, 1.2f);
			nvgLineJoin(args.vg, NVG_ROUND);
			nvgStroke(args.vg);

			// Trigger level, dotted, in the trace's colour.
			const float ty = h / 2 - (triggerLevel - centreV) * scale;
			if (ty > 0 && ty < h) {
				nvgBeginPath(args.vg);
				for (float x = 0; x < w; x += 6) {
					nvgMoveTo(args.vg, x, ty);
					nvgLineTo(args.vg, std::fmin(w, x + 3), ty);
				}
				nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x70));
				nvgStrokeWidth(args.vg, 0.8f);
				nvgStroke(args.vg);
			}

			nvgRestore(args.vg);
		}

		drawTransport(args.vg);
		drawButtons(args.vg);
		drawValues(args.vg);

		OpaqueWidget::draw(args);
	}

	void onEnter(const EnterEvent& e) override {
		hovered = true;
	}

	void onLeave(const LeaveEvent& e) override {
		hovered = false;
		setCursorShape(GLFW_ARROW_CURSOR);
	}

	void onHover(const HoverEvent& e) override {
		// Click-through while following, so the scope never blocks the knob you are heading
		// for: not consuming the hover lets the widget beneath receive it.
		if (following)
			return;
		setCursorShape(cursorForZone(resizeZoneAt(e.pos)));
		OpaqueWidget::onHover(e);
	}

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		// The zone was decided on the press, in onButton, so a drag that starts on an edge
		// resizes and one that starts on the face moves.
		e.consume(this);
	}

	void onDragEnd(const DragEndEvent& e) override {
		resizing = false;
		resizeDir = math::Vec();
	}

	void onDragMove(const DragMoveEvent& e) override {
		const math::Vec d = e.mouseDelta.div(getAbsoluteZoom());

		if (!resizing) {
			// Plain drag moves the scope. Held-drag is accepted here: it is short, and the
			// alternative would put a mode in front of a positioning nudge.
			offset = offset.plus(d);
			return;
		}

		// Dragging the left or top edge has to move the scope as well as resize it, or the
		// far edge would walk across the rack while you pull the near one.
		if (resizeDir.x > 0.f) {
			box.size.x = std::fmax(MIN_W, box.size.x + d.x);
		}
		else if (resizeDir.x < 0.f) {
			const float newW = std::fmax(MIN_W, box.size.x - d.x);
			offset.x += box.size.x - newW;
			box.size.x = newW;
		}
		if (resizeDir.y > 0.f) {
			faceHeight = std::fmax(MIN_H, faceHeight + d.y);
		}
		else if (resizeDir.y < 0.f) {
			const float newH = std::fmax(MIN_H, faceHeight - d.y);
			offset.y += faceHeight - newH;
			faceHeight = newH;
		}
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && following) {
			following = false;
			e.consume(this);
			return;
		}
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			createContextMenu();
			e.consume(this);
			return;
		}
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0) {

			// A click while following deposits the scope where the pointer is, and is NOT
			// consumed, so the same click lands on the panel or knob beneath.
			if (following) {
				following = false;
				return;
			}

			if (minimized) {
				// A token ignores clicks except the plus that restores it.
				if (math::Rect(math::Vec(BTN_PAD, BTN_PAD), math::Vec(BTN, BTN)).contains(e.pos)) {
					minimized = false;
					e.consume(this);
					return;
				}
				e.consume(this);
				return;
			}

			if (closeBox().contains(e.pos)) {
				detach();   // removed by clipPurgeDead on the next step
				e.consume(this);
				return;
			}
			if (minBox().contains(e.pos)) {
				minimized = true;
				valuesShown = false;
				e.consume(this);
				return;
			}
			if (followBox().contains(e.pos)) {
				following = true;
				e.consume(this);
				return;
			}
			if (homeBox().contains(e.pos)) {
				offset = homeOffset;
				e.consume(this);
				return;
			}
			if (autoBox().contains(e.pos)) {
				autosetPending = true;
				autosetBudget = AUTOSET_BUDGET;
				e.consume(this);
				return;
			}
			if (trigBox().contains(e.pos)) {
				triggerAuto ^= true;
				e.consume(this);
				return;
			}
			if (gridBox().contains(e.pos)) {
				gridShown ^= true;
				e.consume(this);
				return;
			}

			// An edge or corner starts a resize rather than a move.
			const math::Vec zone = resizeZoneAt(e.pos);
			if (zone.x != 0.f || zone.y != 0.f) {
				resizing = true;
				resizeDir = zone;
				e.consume(this);
				return;
			}
			resizing = false;
			// The transport button runs and pauses. Freeze is NOT on the face click: an
			// earlier draft of the Wcoast spec said it was, which was misleading.
			if (transportBox().contains(e.pos)) {
				frozen ^= true;
				e.consume(this);
				return;
			}
			// A click in the values box, or on its top-edge triangle, steps the mode.
			if (valuesShown && valuesBox().contains(e.pos)) {
				valueMode = (valueMode + 1) % 3;
				e.consume(this);
				return;
			}
			// A click on the face toggles the box, always opening in scale mode, because you
			// clicked the wave to inspect it.
			valuesShown ^= true;
			if (valuesShown)
				valueMode = 0;
			e.consume(this);
			return;
		}
		OpaqueWidget::onButton(e);
	}

	void onHoverScroll(const HoverScrollEvent& e) override {
		// Vertical scroll steps the vertical scale.
		if (e.scrollDelta.y > 0.f)
			vDivIndex = std::max(0, vDivIndex - 1);
		else if (e.scrollDelta.y < 0.f)
			vDivIndex = std::min(V_DIV_COUNT - 1, vDivIndex + 1);
		e.consume(this);
	}

	void createContextMenu() {
		ui::Menu* menu = rack::createMenu();
		menu->addChild(createMenuLabel("Scope"));

		menu->addChild(createMenuItem("Autoset", "", [=]() {
			autosetPending = true;
		}));

		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createMenuLabel(string::f("Vertical: %g V/div", V_DIVS[vDivIndex])));
		menu->addChild(createMenuItem("Vertical finer", "", [=]() {
			vDivIndex = std::max(0, vDivIndex - 1);
		}));
		menu->addChild(createMenuItem("Vertical coarser", "", [=]() {
			vDivIndex = std::min(V_DIV_COUNT - 1, vDivIndex + 1);
		}));
		menu->addChild(createMenuItem("Centre on signal", "", [=]() {
			// Re-centre without disturbing the scales autoset chose.
			vPos = measMean;
		}));

		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createMenuLabel(string::f("Time base: %g s/div", T_DIVS[tDivIndex])));
		menu->addChild(createMenuItem("Time faster", "", [=]() {
			tDivIndex = std::max(0, tDivIndex - 1);
		}));
		menu->addChild(createMenuItem("Time slower", "", [=]() {
			tDivIndex = std::min(T_DIV_COUNT - 1, tDivIndex + 1);
		}));

		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createBoolMenuItem("Trigger: auto", "",
			[=]() {return triggerAuto;}, [=](bool s) {triggerAuto = s;}));
		menu->addChild(createBoolMenuItem("Trigger: rising edge", "",
			[=]() {return triggerRising;}, [=](bool s) {triggerRising = s;}));
		menu->addChild(createBoolMenuItem("AC coupling", "",
			[=]() {return acCoupled;}, [=](bool s) {acCoupled = s;}));
		menu->addChild(createBoolMenuItem("Frozen", "",
			[=]() {return frozen;}, [=](bool s) {frozen = s;}));

		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createMenuItem("Remove scope", "", [=]() {
			detach();   // removed by clipPurgeDead on the next step
		}));
	}
};


void scopeCreate(PortWidget* port) {
	if (!port || !port->module)
		return;

	ScopeWidget* scope = new ScopeWidget;
	scope->port = port;
	scope->tapSlot = tapCreate(port->module ? port->module->id : -1, port->portId,
		port->type == engine::Port::OUTPUT);
	if (scope->tapSlot < 0) {
		WARN("Scope: no tap slots available");
		delete scope;
		return;
	}
	APP->scene->rack->addChild(scope);

	clipAddHandle(scope);

	INFO("Scope: probing %s port %d", port->type == engine::Port::OUTPUT ? "output" : "input",
		port->portId);
}


bool scopeDepositFollowing() {
	bool any = false;
	for (widget::Widget* child : APP->scene->rack->children) {
		ScopeWidget* scope = dynamic_cast<ScopeWidget*>(child);
		if (scope && scope->following) {
			scope->following = false;
			any = true;
		}
	}
	return any;
}


/** Scopes waiting for their module to come back after a patch load. */
struct PendingScope {
	int64_t moduleId = -1;
	int portId = 0;
	bool isOutput = true;
	json_t* stateJ = NULL;
	/** Frames left to keep looking. Modules appear over several frames as a patch loads, and
	a module whose plugin is missing never appears at all — so this waits, then gives up and
	says so, rather than retrying for the life of the session. */
	int budget = 300;
};

static std::vector<PendingScope> pending;


json_t* scopeToJson() {
	json_t* arrayJ = json_array();
	for (widget::Widget* child : APP->scene->rack->children) {
		ScopeWidget* scope = dynamic_cast<ScopeWidget*>(child);
		if (scope && scope->port)
			json_array_append_new(arrayJ, scope->toJson());
	}
	// Anything still waiting to be restored is saved again as it was, so saving a patch
	// before its modules have finished loading cannot quietly drop scopes.
	for (const PendingScope& p : pending) {
		if (p.stateJ)
			json_array_append(arrayJ, p.stateJ);
	}
	return arrayJ;
}


void scopeFromJson(json_t* arrayJ) {
	for (PendingScope& p : pending) {
		if (p.stateJ)
			json_decref(p.stateJ);
	}
	pending.clear();
	if (!arrayJ || !json_is_array(arrayJ))
		return;

	size_t i;
	json_t* scopeJ;
	json_array_foreach(arrayJ, i, scopeJ) {
		json_t* moduleIdJ = json_object_get(scopeJ, "moduleId");
		if (!moduleIdJ)
			continue;
		PendingScope p;
		p.moduleId = json_integer_value(moduleIdJ);
		if (json_t* j = json_object_get(scopeJ, "portId"))
			p.portId = json_integer_value(j);
		if (json_t* j = json_object_get(scopeJ, "isOutput"))
			p.isOutput = json_boolean_value(j);
		p.stateJ = json_incref(scopeJ);
		pending.push_back(p);
	}
}


/** The port a saved scope was attached to, or NULL while its module is still loading.

Identified by module id rather than by anything about the widget, because that is what the
patch preserves across a save and load.
*/
static PortWidget* findPort(int64_t moduleId, int portId, bool isOutput) {
	for (ModuleWidget* mw : APP->scene->rack->getModules()) {
		if (!mw->module || mw->module->id != moduleId)
			continue;
		for (PortWidget* p : mw->getPorts()) {
			if (p->portId == portId
				&& (p->type == engine::Port::OUTPUT) == isOutput)
				return p;
		}
		return NULL;   // right module, no such port: it has changed, so do not guess
	}
	return NULL;
}


/** Re-attaches saved scopes as their modules appear. Called every frame while any are
waiting, and does nothing at all once the list is empty. */
void scopeRestoreStep() {
	if (pending.empty())
		return;

	for (size_t i = 0; i < pending.size();) {
		PendingScope& p = pending[i];
		PortWidget* port = findPort(p.moduleId, p.portId, p.isOutput);
		if (port) {
			scopeCreate(port);
			// scopeCreate appends, so the scope just made is the last one that has our port.
			for (auto it = APP->scene->rack->children.rbegin();
				it != APP->scene->rack->children.rend(); it++) {
				ScopeWidget* scope = dynamic_cast<ScopeWidget*>(*it);
				if (scope && scope->port == port) {
					scope->fromJson(p.stateJ);
					break;
				}
			}
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		if (--p.budget <= 0) {
			WARN("Scope: module %lld never appeared, dropping its scope",
				(long long) p.moduleId);
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		i++;
	}
}






