/** Clip-on signal scope, ported from the forked Rack build.

Anchored to the PORT it probes rather than to the screen: it is a child of the RackWidget and
its position is recomputed each frame relative to that port, so it scrolls and zooms with its
module and follows if the module is moved.
*/
#include "plugin.hpp"
#include "SignalTap.hpp"
#include "WidgetAt.hpp"
#include <ui/ScrollWidget.hpp>
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
/** A division is a FIXED size on screen, not a fraction of the face.

So the scales set magnification and the window size sets how much you see, which are two
different questions and now have two different controls. Sizing a division as a fraction of
the face made them one: enlarging a scope magnified a signal that was already framed, and a
trace running off the top clipped exactly the same however big you made the window — the one
remedy anybody actually reaches for.

Twelve pixels is close to what the default face showed before, so existing scopes look much as
they did; they simply reveal more when dragged out. */
static const float PX_PER_DIV = 12.f;

// The callout: ring at the jack, line to the scope, and a rounded grab tab where they meet.
// Wcoast CALLOUT_COLOR, the scope's own border grey, so it reads as part of the frame rather
// than as decoration. Opaque: the muted colour already reads as secondary.

// Wcoast SCOPE_HANDLE. Also the shortest the loop-to-scope line may get.

// The values box below the face. A readout, not a settings panel.
/** The values box is sized to its text rather than the other way round — see drawValues. */
static const float VALUES_TEXT_SIZE = 15.f;
static const float VALUES_PAD = 3.f;
static const NVGcolor VALUES_GREEN = nvgRGB(0x3d, 0xe0, 0x7a);
/** Every on-face control. Distinct from the green readouts and the white trace, so what is a
control and what is a measurement never have to be worked out. */
static const NVGcolor CONTROL_AMBER = nvgRGB(0xe0, 0xa0, 0x3b);
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
/** The window autoset actually wants: the same 4096 it reads, every time it runs.

WHY THE WHOLE WINDOW. A scope attached to a terminal and a scope with A pressed on it were
choosing different time bases, and the reason was simply that the first one ran early — the
tap had been alive for a few frames and held 3328 samples where the later run had 4096. Fewer
samples means fewer counted cycles and a different mean, so both the time base and the trigger
level came out elsewhere. Waiting for the full window costs about eighty-five milliseconds at
48 kHz, and makes attaching a scope and pressing A the same act.
*/
static const int AUTOSET_WINDOW = 4096;
/** Frames to wait for that full window before settling for what has arrived. Only reached if
the engine is stopped or crawling; otherwise the window fills in about three frames. */
static const int AUTOSET_PATIENCE = 60;
/** Units of scroll per step. Raising this slows scrolling proportionally. */
/** How much of the history a scope reads each frame: enough for a full window plus room to
find a trigger edge ahead of it. */
static const int SEARCH_CHUNK = 16384;
static const float SCROLL_PER_STEP = 30.f;
/** Panning moves a whole division per step, so it takes less scroll than a nudge of the
vertical position does. */
static const float PAN_SCROLL_PER_STEP = 10.f;
/** A scale step is a whole 1-2-5 jump, so it wants fewer units than a nudge of position. */
static const float SCALE_SCROLL_PER_STEP = 40.f;
/** A gesture is considered over after this long with no scroll, which is when the axis it
locked onto is released. */
static const double SCROLL_IDLE = 0.7;
/** How much scroll it takes to CLAIM an axis, and how far it must beat the other one. A
trackpad glide tails off into small ragged deltas, and a claim made from those flipped the
axis at the end of every gesture — a horizontal scroll finishing with a flick of vertical. */
static const float AXIS_CLAIM_TOTAL = 6.f;
static const float BTN = 14.f;
static const float BTN_PAD = 4.f;
/** How far a control sits from the edge of the face it is anchored to. */
static const float EDGE = 1.f;
/** The trigger strip down the left of the face: drawn this wide, but accepting scroll and
clicks a good deal further in, since five pixels is not a target anyone should have to aim
at. */
static const float TRIG_STRIP_W = 10.f;
static const float TRIG_STRIP_REACH = 10.f;
/** How far in from an edge still counts as grabbing it to resize. */
static const float RESIZE_EDGE = 6.f;
static const float LEFT_RESIZE_EDGE = 3.f;
static const float MIN_W = 70.f, MIN_H = 40.f;
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


/** The same cursor, shared with the analyser: the two resize alike, so they should say so
alike. The cursor cache above is what makes this cheap enough to call every hover. */
void druiSetCursorShape(int shape) {
	setCursorShape(shape);
}


/** Defined below, beside the patch-restoring code that also uses it. */
static PortWidget* findPort(int64_t moduleId, int portId, bool isOutput);


struct ScopeWidget : ClipWidget {
	bool needsSignal() override {
		return true;
	}

	int tapSlot = -1;

	/** Scroll delta banked but not yet spent, so a scale steps once per three units of scroll
	rather than once per event. */
	float scrollAccumX = 0.f, scrollAccumY = 0.f;
	/** The values box, measured while drawing so its frame fits its text. */
	float valuesW = 120.f, valuesH = 22.f;
	/** How far the drawn window is shifted back through the captured samples, in divisions.
	Panning, as the horizontal control on a real scope does. */
	float timeShift = 0.f;
	/** Which axis this gesture has claimed: 0 none, 1 sideways, 2 up and down. A trackpad
	glide is never perfectly straight, so without this a horizontal pan carries a little
	vertical drift with it and moves the trace while you are trying to scan along it. */
	int scrollAxis = 0;
	/** The opening movement of the current gesture, summed until it is decisive. */
	math::Vec axisClaim;
	double lastScrollTime = 0.0;

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
	/** Frames spent waiting for the window to fill. */
	int autosetWaited = 0;
	bool frozen = false;
	bool acCoupled = false;

	/** Self-trigger on a rising edge at this level, in volts. */
	float triggerLevel = 0.f;
	/** How far the signal must travel the other way before a crossing counts again. Sized
	from the measured amplitude, because anti-aliased edges ring across the level several
	times and would otherwise each look like a separate trigger. */
	float triggerHyst = 0.05f;
	bool triggerRising = true;
	/** Whether the trace is triggered at all. Off free-runs: the window is simply the newest
	samples, which is what you want while hunting for something rather than measuring it.
	Switched from the strip down the left edge of the face, which replaced the T button. */
	bool triggerOn = true;
	/** An EXTERNAL trigger: the scope displays one terminal and triggers on another.

	Its own tap, with its own history, read from the newest end alongside the trace's. Both
	taps capture the same sample in the same call, so their newest entries are simultaneous —
	which is what makes the two buffers align without any bookkeeping, as long as both are read
	from the end rather than by index.

	The threshold is fixed at one volt. That is a gate or a trigger in Rack, which is what an
	external trigger is nearly always fed, and it removes a level control whose height on the
	face would have meant nothing: the level belongs to the trigger signal, and the strip is
	drawn against the trace. */
	WeakPtr<PortWidget> trigPort;
	int trigTapSlot = -1;
	/** The grab tab on the amber link. A separate widget, and a child of the rack, for the same
	reason the trace's is: it is drawn out at the terminal, outside this widget's box, and Rack
	offers a click only to a widget whose box contains the point. */
	widget::Widget* trigHandle = NULL;
	/** An external trigger read from a saved patch, waiting for its module to appear. Modules
	arrive over several frames as a patch loads, and the one carrying the trigger may well come
	after the one carrying the trace. */
	int64_t pendingTrigModuleId = -1;
	int pendingTrigPortId = 0;
	bool pendingTrigIsOutput = true;
	int pendingTrigBudget = 0;
	/** True while a link is being dragged out of the strip. */
	bool linking = false;
	math::Vec linkPos;
	bool pressedInStrip = false;

	static constexpr float EXT_TRIG_LEVEL = 1.f;
	static constexpr float EXT_TRIG_ARM = 0.5f;

	bool externalTrigger() {
		return trigPort && trigTapSlot >= 0 && tapAlive(trigTapSlot);
	}

	void dropExternalTrigger() {
		if (trigTapSlot >= 0)
			tapDestroy(trigTapSlot);
		trigTapSlot = -1;
		trigPort = NULL;
	}

	void setExternalTrigger(PortWidget* p) {
		dropExternalTrigger();
		if (!p || !p->module)
			return;
		trigTapSlot = tapCreate(p->module->id, p->portId, p->type == engine::Port::OUTPUT);
		if (trigTapSlot < 0) {
			WARN("Scope: no tap slots left for an external trigger");
			return;
		}
		trigPort = p;
		triggerOn = true;
		timeShift = 0.f;
		INFO("Scope: triggering externally from port %d", p->portId);
	}

	/** Kept so old patches load, and because Normal mode may come back on the strip as a second
	click state. Nothing sets it now. */
	bool triggerAuto = true;

	/** The values box: shown by a click on the face, cycling scale -> freq -> peak. */
	/** Set by a press on the face, cleared on release, with how far the pointer moved in
	between — which is what tells a click from a drag. */
	bool pressedOnFace = false;
	float travelled = 0.f;
	/** Where the press landed, in face coordinates, so the release can tell what was under it. */
	math::Vec pressPos;

	bool valuesShown = false;
	int valueMode = 0;
	/** Hover drives the buttons' visibility, as in Wcoast. */
	bool hovered = false;
	/** Owned by the scene while it exists; cleared whenever the pointer leaves the control it
	belongs to, and in the destructor, or it would outlive the scope. */
	ui::Tooltip* tooltip = NULL;
	std::string tooltipText;
	bool gridShown = true;
	bool minimized = false;
	// `following` is ClipWidget's now, shared with the injectors: a new widget rides the
	// pointer until it is put down, and the scope's F button uses the same state to pick one up
	// again.
	/** The spot beside its terminal that the home button returns it to. */
	math::Vec homeOffset = math::Vec(30, -70);
	/** Which edge or corner is being dragged, as (x, y) in {-1, 0, 1}. */
	math::Vec resizeDir;
	bool resizing = false;

	std::vector<float> scratch;
	/** What the tap held at the moment of pausing. Panning a paused scope has to re-window
	this, not the live buffer — the tap goes on filling while the scope is held, so re-reading
	it would quietly show newer samples than the ones frozen on screen. */
	/** The external trigger's window, read alongside the trace's. */
	std::vector<float> extScratch;
	std::vector<float> frozenBuf;
	int frozenCount = 0;
	/** The last captured sweep, kept so a frozen scope still has something to show. */
	std::vector<float> lastWin;
	int lastCount = 0;
	/** Measured from the last drawn window, for the values box. */
	float measMin = 0.f, measMax = 0.f, measMean = 0.f, measFreq = 0.f;

	/** The face alone. The widget's own box grows to include the values box when it is shown,
	because Rack only dispatches a click to a child whose box CONTAINS the point — a box drawn
	outside the widget would be visible and completely unclickable. */
	// faceWidth and faceHeight are ClipWidget's: the BOX is not the record of them, since
	// minimising replaces it with a token a few pixels across and the readout can make it wider
	// than the trace. Set here to this scope's own default size.

	ScopeWidget() {
		faceWidth = 98.f;
		faceHeight = 49.f;
		// Two thirds of what it was. A scope is a thing you clip on beside a jack, and a smaller
		// one hides less of the rack; it can always be dragged bigger by an edge.
		// Small on purpose. A scope is usually a peek at a signal, and a small one hides less of
		// the rack; drag an edge when you want to see more.
		box.size = math::Vec(faceWidth, faceHeight);
		// A working window, not the whole history. The buffer now holds eleven seconds, and
		// copying and searching two megabytes every frame for every scope would cost far more
		// than the feature is worth.
		scratch.resize(SEARCH_CHUNK);
	}

	/** How many divisions fit across and down. This is what changes when the window is
	resized: the division itself is a fixed size, so a bigger scope shows more of the signal
	rather than the same signal larger. */
	/** The voltage at the vertical centre of the face.

	AC coupling follows the signal's own mean, so a small waveform riding on a large offset is
	readable; otherwise it is the vertical position. Shared, because the grid, the trace and
	the trigger marker must all agree about it or they describe different pictures.
	*/
	float centreVolts() {
		if (acCoupled && lastCount > 1) {
			float sum = 0.f;
			for (int i = 0; i < lastCount; i++)
				sum += lastWin[i];
			return sum / lastCount;
		}
		return vPos;
	}

	float divsX() {
		return std::fmax(1.f, faceW() / PX_PER_DIV);
	}
	float divsY() {
		return std::fmax(1.f, faceHeight / PX_PER_DIV);
	}

	~ScopeWidget() {
		destroyTooltip();
		if (trigHandle) {
			if (trigHandle->parent)
				trigHandle->parent->removeChild(trigHandle);
			delete trigHandle;
			trigHandle = NULL;
		}
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
	}

	void step() override {
		// Anchored to the port, not to the screen, so the scope follows if the module moves.
		followPort();

		// If the trigger source has gone — its module deleted — fall back to triggering on the
		// scope's own signal rather than sitting there never triggering.
		if (trigTapSlot >= 0 && !externalTrigger())
			dropExternalTrigger();

		// A saved external trigger, re-attached once its module has loaded. Given up on after
		// about ten seconds, which is what happens when the patch is opened without the plugin
		// that carried the trigger source.
		if (pendingTrigModuleId >= 0 && pendingTrigBudget > 0) {
			if (PortWidget* p = findPort(pendingTrigModuleId, pendingTrigPortId,
				pendingTrigIsOutput)) {

				setExternalTrigger(p);
				pendingTrigModuleId = -1;
			}
			else if (--pendingTrigBudget <= 0) {
				WARN("Scope: the module carrying its external trigger never appeared");
				pendingTrigModuleId = -1;
			}
		}
		if (minimized) {
			// Exactly wide enough for the same two buttons, in the same places they occupy on
			// the full face — see the note on minBox().
			box.size = math::Vec(TRIG_STRIP_W + BTN_PAD * 3 + BTN * 2, EDGE * 2 + BTN);
		}
		else {
			// The readout may be wider than the face — it holds two calibrated scales, and a
			// narrow scope cannot shrink them. The box grows to whichever is wider, because
			// Rack only offers a click to a widget whose box CONTAINS the point: a readout
			// drawn outside the box would be visible and completely unclickable.
			box.size.x = std::fmax(faceWidth, valuesShown ? valuesW : 0.f);
			box.size.y = faceHeight + (valuesShown ? valuesH + 3.f : 0.f);
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
		// The loop is in the air: the scope is attached to nothing, so there is nothing to
		// draw. Showing the last sweep would suggest it was still measuring something.
		if (retargeting)
			return 0;

		// Panning is a HORIZONTAL POSITION control, not a different place to trigger.
		//
		// The trigger still finds its edge in the newest capture, so the trace stays locked;
		// the pan then slides the window along from that edge, which is what shows you a
		// different part of the waveform while the phase holds still. Reading a different
		// stretch of history instead — which is what this did before — leaves a periodic signal
		// looking identical wherever you scroll to, because it IS identical.
		const int perDiv = std::max(1, (int) (wanted / divsX()));
		const int pan = (int) (timeShift * perDiv);
		// Enough to hold the window, room to find an edge ahead of it, and the pan itself.
		const int chunk = std::min((int) scratch.size(), wanted * 2 + pan + 64);

		int have;
		if (frozen && frozenCount > 0) {
			// Paused: window the copy taken at the moment of pausing, never the live buffer,
			// which goes on filling while the scope is held. Here the pan can reach the whole
			// eleven seconds, since nothing is moving.
			have = std::min(chunk, frozenCount);
			if (have <= 0)
				return 0;
			std::copy(frozenBuf.begin() + (frozenCount - have), frozenBuf.begin() + frozenCount,
				scratch.begin());
		}
		else {
			have = tapReadAt(tapSlot, scratch.data(), chunk, 0);
		}
		if (have <= 0)
			return 0;

		// A little pre-trigger, so the edge itself is visible rather than sitting on the frame.
		const int pre = (int) (wanted / divsX());
		int start = have - wanted;
		if (start < 0)
			start = 0;

		// An edge only counts if a whole window follows it; otherwise the few samples left
		// would be stretched across the face and the time base would be a lie.
		const int latest = have - (wanted - pre);

		int edge = -1;
		bool armed = false;

		if (externalTrigger()) {
			// The trigger signal, read from the newest end so its last sample lines up with the
			// trace's last sample — that is what aligns the two buffers, since both taps
			// capture the same sample in the same call but may have started at different times.
			//
			// Fixed threshold and fixed arming band: a Rack gate rests at zero and rises to
			// ten, so one volt catches it, and requiring a return below half a volt first stops
			// a single edge triggering twice.
			extScratch.resize(scratch.size());
			const int extHave = tapReadAt(trigTapSlot, extScratch.data(), have, 0);
			const int off = have - extHave;
			// Always the RISING edge. A gate or trigger starts something on its rise, and the
			// trace of the trigger signal is not on screen — so a choice between rising and
			// falling would be one the user has no way to make an informed decision about.
			for (int i = std::max(1, off + 1); i <= latest; i++) {
				const float a = extScratch[i - 1 - off], b = extScratch[i - off];
				if (b < EXT_TRIG_ARM)
					armed = true;
				else if (armed && a < EXT_TRIG_LEVEL && b >= EXT_TRIG_LEVEL) {
					edge = i;
					armed = false;
				}
			}
		}
		else {
			// Triggering on the displayed signal itself, at the level the strip sets, armed by
			// a hysteresis band taken from the signal's own amplitude.
			const float hyst = std::fmax(triggerHyst, 1e-4f);
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
		}

		if (!triggerOn) {
			// Free-running: the newest window, offset by however far it has been panned.
			start = math::clamp(have - wanted - pan, 0, std::max(0, have - wanted));
		}
		else if (edge >= 0) {
			start = math::clamp(edge - pre - pan, 0, std::max(0, have - wanted));
		}

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

		// Not the full window yet. Keep waiting WITHOUT spending the give-up budget: the tap is
		// plainly working, it simply has not filled. Waiting matters even once there is enough
		// to judge a peak by, because the time base is measured from how many cycles fit in
		// the window — see AUTOSET_WINDOW.
		if (have >= 1 && have < AUTOSET_WINDOW) {
			autosetWaited++;
			const bool patienceGone = autosetWaited > AUTOSET_PATIENCE
				&& have >= AUTOSET_MIN_SAMPLES;
			if (!patienceGone)
				return;
		}

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
		const float wantPerDiv = peak / ((divsY() / 2.f) * 0.9f);
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
			const float wantT = wantSpan / divsX();
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
		// EXACTLY what pressing A does, including returning to the live end: a scope moved to
		// another terminal is looking at a different signal, and a pan into the history of the
		// last one has nothing to do with it.
		autosetPending = true;
		autosetBudget = AUTOSET_BUDGET;
		autosetWaited = 0;
		timeShift = 0.f;
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
		json_object_set_new(rootJ, "width", json_real(faceWidth));
		json_object_set_new(rootJ, "faceHeight", json_real(faceHeight));
		json_object_set_new(rootJ, "vDiv", json_integer(vDivIndex));
		json_object_set_new(rootJ, "vPos", json_real(vPos));
		json_object_set_new(rootJ, "tDiv", json_integer(tDivIndex));
		json_object_set_new(rootJ, "trigLevel", json_real(triggerLevel));
		json_object_set_new(rootJ, "trigHyst", json_real(triggerHyst));
		json_object_set_new(rootJ, "trigRising", json_boolean(triggerRising));
		json_object_set_new(rootJ, "trigOn", json_boolean(triggerOn));
		if (trigPort && trigPort->module) {
			json_object_set_new(rootJ, "extTrigModuleId", json_integer(trigPort->module->id));
			json_object_set_new(rootJ, "extTrigPortId", json_integer(trigPort->portId));
			json_object_set_new(rootJ, "extTrigIsOutput",
				json_boolean(trigPort->type == engine::Port::OUTPUT));
		}
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
		num("width", faceWidth);
		num("faceHeight", faceHeight);
		integer("vDiv", vDivIndex);
		num("vPos", vPos);
		integer("tDiv", tDivIndex);
		num("trigLevel", triggerLevel);
		num("trigHyst", triggerHyst);
		boolean("trigRising", triggerRising);
		boolean("trigOn", triggerOn);
		if (json_t* j = json_object_get(rootJ, "extTrigModuleId")) {
			pendingTrigModuleId = json_integer_value(j);
			pendingTrigBudget = 300;
			if (json_t* k = json_object_get(rootJ, "extTrigPortId"))
				pendingTrigPortId = json_integer_value(k);
			if (json_t* k = json_object_get(rootJ, "extTrigIsOutput"))
				pendingTrigIsOutput = json_boolean_value(k);
		}
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
		// Measured while drawing, so the frame hugs whatever is currently in it.
		return math::Rect(math::Vec(0, faceHeight + 3),
			math::Vec(valuesW, valuesH));
	}

	/** Everything is pushed to the very edge of the face, a pixel in. The middle of the window
	is where the trace lives, and a control sitting in it is covering the reading.

	MINIMISE and CLOSE sit at the top left in exactly the same places whether the scope is open
	or minimised, because the minimised token is drawn from the same origin and is sized to
	hold precisely those two buttons. So minimising and restoring can be done repeatedly
	WITHOUT MOVING THE POINTER: the button stays under it in both states. That is also why
	minimise is the outer one rather than close, which is the opposite of the convention — the
	button you will press twice belongs where it does not move — and since both boxes are
	defined once and used by both states, close can sit outside it without breaking that. */
	math::Rect closeBox()    { return math::Rect(math::Vec(TRIG_STRIP_W + BTN_PAD, EDGE), math::Vec(BTN, BTN)); }
	math::Rect minBox()      { return math::Rect(math::Vec(TRIG_STRIP_W + BTN_PAD * 2 + BTN, EDGE), math::Vec(BTN, BTN)); }
	math::Rect followBox()   { return math::Rect(math::Vec(faceWidth - BTN - EDGE, EDGE), math::Vec(BTN, BTN)); }
	math::Rect homeBox()     { return math::Rect(math::Vec(faceWidth - BTN * 2 - BTN_PAD - EDGE, EDGE), math::Vec(BTN, BTN)); }
	/** The bottom row, left to right: pause/run, A for autoset, T for trigger mode, G for
	grid — the II A T G row on the Wcoast face. */
	math::Rect autoBox()     { return math::Rect(math::Vec(TRIG_STRIP_W + BTN_PAD * 2 + TRANSPORT_SIZE, faceHeight - BTN - EDGE), math::Vec(BTN, BTN)); }
	math::Rect acBox()       { return math::Rect(math::Vec(TRIG_STRIP_W + BTN_PAD * 3 + TRANSPORT_SIZE + BTN, faceHeight - BTN - EDGE), math::Vec(BTN, BTN)); }
	math::Rect gridBox()     { return math::Rect(math::Vec(TRIG_STRIP_W + BTN_PAD * 4 + TRANSPORT_SIZE + BTN * 2, faceHeight - BTN - EDGE), math::Vec(BTN, BTN)); }
	/** The strip itself, and the wider area that answers to it. */
	math::Rect trigStrip()   { return math::Rect(math::Vec(0.f, 0.f), math::Vec(TRIG_STRIP_W, faceHeight)); }
	bool inTrigStrip(math::Vec pos) { return pos.x >= 0.f && pos.x <= TRIG_STRIP_REACH && pos.y >= 0.f && pos.y <= faceHeight; }

	/** Where the trigger marker is drawn, in face coordinates, or false if it is out of view.
	Shared by the drawing and the hit test, so what you click is what you see. */
	bool trigMarkerY(float& ty) {
		if (!triggerOn)
			return false;
		const float h = faceHeight;
		float centre = vPos;
		if (acCoupled && lastCount > 1) {
			float sum = 0.f;
			for (int i = 0; i < lastCount; i++)
				sum += lastWin[i];
			centre = sum / lastCount;
		}
		const float scale = PX_PER_DIV / V_DIVS[vDivIndex];
		ty = h / 2 - (triggerLevel - centre) * scale;
		return ty > 0.f && ty < h;
	}

	bool inTrigTriangle(math::Vec pos) {
		float ty = 0.f;
		if (!trigMarkerY(ty))
			return false;
		const float lo = triggerRising ? ty : ty - TRIG_STRIP_W;
		const float hi = triggerRising ? ty + TRIG_STRIP_W : ty;
		return pos.x >= 0.f && pos.x <= TRIG_STRIP_W && pos.y >= lo && pos.y <= hi;
	}

	bool atHome() {
		return offset.minus(homeOffset).norm() < 1.f;
	}

	/** Which edge or corner the pointer is on, as (x, y) in {-1, 0, 1}. Zero means neither. */
	math::Vec resizeZoneAt(math::Vec pos) {
		math::Vec dir;
		if (minimized)
			return dir;
		// The LEFT edge resizes from its outermost few pixels only. The trigger strip owns the
		// rest of that column, and the press test below reaches the resize check first — so a
		// full-width resize zone there would swallow every click meant for the strip. Three
		// pixels is enough to grab and leaves the strip most of its width.
		if (pos.x <= LEFT_RESIZE_EDGE)
			dir.x = -1;
		else if (pos.x >= faceWidth - RESIZE_EDGE)
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
		// Clear of the trigger strip, which owns the left edge from top to bottom.
		return math::Rect(math::Vec(TRIG_STRIP_W + BTN_PAD, faceHeight - TRANSPORT_SIZE - EDGE),
			math::Vec(TRANSPORT_SIZE, TRANSPORT_SIZE));
	}

	void drawValues(NVGcontext* vg) {
		if (!valuesShown)
			return;

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;
		const std::string text = valuesText();

		// Measure first, then draw a frame that fits: the box used to be a fixed 30 px tall
		// with 11 px text floating in the middle of it, which wasted the space and read as an
		// empty panel with a caption in it.
		nvgFontFaceId(vg, font->handle);
		nvgFontSize(vg, VALUES_TEXT_SIZE);
		float bounds[4] = {};
		const float textW = nvgTextBounds(vg, 0, 0, text.c_str(), NULL, bounds);
		valuesW = textW + 2.f * VALUES_PAD + 8.f;
		valuesH = (bounds[3] - bounds[1]) + 2.f * VALUES_PAD;

		const math::Rect r = valuesBox();

		nvgBeginPath(vg);
		nvgRoundedRect(vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 3);
		nvgFillColor(vg, nvgRGB(0x10, 0x12, 0x16));
		nvgFill(vg);
		nvgStrokeColor(vg, VALUES_GREEN);
		nvgStrokeWidth(vg, 1.2f);
		nvgStroke(vg);

		// Green, like every other readout in the plugin, so they read as one instrument.
		nvgFontFaceId(vg, font->handle);
		nvgFontSize(vg, VALUES_TEXT_SIZE);
		nvgFillColor(vg, VALUES_GREEN);
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2, text.c_str(), NULL);
	}

	/** A control is its LETTER, not a disc with a letter in it.

	The discs were solid and sat over the trace, which is the one thing on the face worth
	seeing. A letter alone covers a fraction as much and is just as easy to hit — the click
	target is unchanged, since that comes from the box, not from what is drawn in it.

	Amber throughout, which separates the controls at a glance from the green readouts and the
	white trace, and states nothing about their state. State is carried by brightness: a
	control that is doing something is full strength, one that is not is dimmed.
	*/
	/** The two that CLOSE or HIDE the scope keep a filled disc behind them.

	They are the destructive pair, and a coloured target that cannot be mistaken for a reading
	is worth the pixels it covers — the rest of the controls only change how the trace is
	shown, and can afford to be letters.
	*/
	void drawDiscButton(NVGcontext* vg, math::Rect r, NVGcolor fill, const char* glyph) {
		nvgBeginPath(vg);
		nvgCircle(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2, r.size.x / 2);
		nvgFillColor(vg, fill);
		nvgFill(vg);

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (font && font->handle >= 0) {
			nvgFontFaceId(vg, font->handle);
			nvgFontSize(vg, 10);
			nvgFillColor(vg, nvgRGB(0x10, 0x12, 0x16));
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgText(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2 + 1, glyph, NULL);
		}
	}

	void drawButton(NVGcontext* vg, math::Rect r, NVGcolor fill, const char* glyph, bool dim) {
		if (!glyph || !glyph[0])
			return;
		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;

		nvgFontFaceId(vg, font->handle);
		nvgFontSize(vg, 12);
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, dim ? nvgRGBA(0xe0, 0xa0, 0x3b, 0x66) : CONTROL_AMBER);
		nvgText(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2, glyph, NULL);
	}

	/** Close, minimise, follow and home; grid sits with the transport at the bottom. All
	revealed on hover, as in Wcoast. */
	void drawButtons(NVGcontext* vg) {
		// Also while FOLLOWING. A following scope is click-through, so the pointer never counts
		// as hovering it and every control vanished — including the one you had just pressed,
		// which is why F never appeared lit once it was doing its job.
		if ((!hovered && !following) || minimized)
			return;
		drawDiscButton(vg, closeBox(), nvgRGB(0xe0, 0x3b, 0x3b), "x");
		drawDiscButton(vg, minBox(), nvgRGB(0xe8, 0xb3, 0x2a), "-");
		// Bright when it can be PRESSED, dim when it cannot. A following scope is click-through,
		// so while it is carrying there is no way to hit F at all — brightness here says what
		// is available, not what is switched on.
		drawButton(vg, followBox(), CONTROL_AMBER, "F", following);
		// Dimmed while it is already home: there is nothing to send.
		drawButton(vg, homeBox(), CONTROL_AMBER, "<", atHome());
		// Never dimmed. Autoset is an action you can always ask for, and dimming it the moment it
		// finished — which is within a frame or two — made a working button look disabled.
		drawButton(vg, autoBox(), CONTROL_AMBER, "A", false);
		// Lit is AC; dimmed is DC, which is the resting state of a scope.
		drawButton(vg, acBox(), CONTROL_AMBER, "AC", !acCoupled);
		drawButton(vg, gridBox(), CONTROL_AMBER, "G", !gridShown);
	}

	/** Lower-left transport: a right-pointing triangle runs, two vertical bars pause. Shown
	on hover, as in Wcoast. */
	void drawTransport(NVGcontext* vg) {
		if (!hovered && !following)
			return;
		const math::Rect r = transportBox();

		nvgFillColor(vg, CONTROL_AMBER);
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

	/** Drawn in the CABLE layer, not the ordinary one.

	The rack draws its children, then plugs at layer 2 and cables at layer 3 — so a scope drawn
	normally had cables and their plugs painted over it, and the module panel's own text showed
	through wherever the face was not opaque. Drawing here, as the rack's last child, puts the
	scope in front of everything.
	*/
	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 3)
			drawFace(args);
		widget::OpaqueWidget::drawLayer(args, layer);
	}

	void draw(const DrawArgs& args) override {}

	/** Where the trigger link's loop and grab tab sit, in face coordinates.

	The same shape as the trace's callout — a ring round the terminal and a tab on the line —
	because it is the same kind of attachment and should be handled the same way. It differs
	only in colour and in where it leaves the face: the middle of the trigger strip, which is
	the control it belongs to.
	*/
	bool trigCalloutGeometry(math::Vec& ring, float& rr, math::Vec& tab) {
		if (linking) {
			ring = linkPos.minus(box.pos);
			rr = 9.f;
		}
		else if (externalTrigger()) {
			ring = trigPort->getRelativeOffset(
				trigPort->box.zeroPos().getCenter(), APP->scene->rack).minus(box.pos);
			rr = std::fmin(trigPort->box.size.x, trigPort->box.size.y) / 2.f + 2.f;
		}
		else {
			return false;
		}

		// The nearest side of the face, exactly as the trace's callout does — leaving from the
		// strip meant the amber line crossed the picture whenever the trigger source was to the
		// right of the scope.
		const math::Vec from = nearestSideCentre(ring);
		math::Vec dir = from.minus(ring);
		const float dist = dir.norm();
		dir = (dist < 1e-3f) ? math::Vec(0.f, -1.f) : dir.div(dist);
		tab = ring.plus(dir.mult(rr + CLIP_HANDLE / 2.f));
		return true;
	}

	void drawTriggerLink(NVGcontext* vg) {
		math::Vec ring, tab;
		float rr = 0.f;
		if (!trigCalloutGeometry(ring, rr, tab))
			return;

		const math::Vec from = nearestSideCentre(ring);
		math::Vec dir = from.minus(ring);
		const float dist = dir.norm();
		dir = (dist < 1e-3f) ? math::Vec(0.f, -1.f) : dir.div(dist);

		nvgBeginPath(vg);
		nvgMoveTo(vg, ring.x + dir.x * rr, ring.y + dir.y * rr);
		nvgLineTo(vg, from.x, from.y);
		nvgStrokeColor(vg, CONTROL_AMBER);
		nvgStrokeWidth(vg, 1.6f);
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgCircle(vg, ring.x, ring.y, rr);
		nvgStrokeWidth(vg, 1.8f);
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, tab.x - CLIP_HANDLE / 2.f, tab.y - CLIP_HANDLE / 2.f,
			CLIP_HANDLE, CLIP_HANDLE, 3.f);
		nvgFillColor(vg, CONTROL_AMBER);
		nvgFill(vg);
	}

	void drawFace(const DrawArgs& args) {
		drawCallout(args.vg);
		drawTriggerLink(args.vg);

		if (minimized) {
			// A token: it still drags and still shows its callout, but ignores clicks except
			// the plus that restores it.
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
			nvgFillColor(args.vg, nvgRGB(0x10, 0x12, 0x16));
			nvgFill(args.vg);
			nvgStrokeColor(args.vg, frozen ? FRAME_PAUSED : FRAME_RUN);
			nvgStrokeWidth(args.vg, 1.5f);
			nvgStroke(args.vg);
			// The same two buttons, in the same places: restore where minimise was, close beside
			// it, so neither has moved under the pointer.
			drawDiscButton(args.vg, minBox(), nvgRGB(0xe8, 0xb3, 0x2a), "+");
			drawDiscButton(args.vg, closeBox(), nvgRGB(0xe0, 0x3b, 0x3b), "x");
			OpaqueWidget::draw(args);
			return;
		}

		const float w = faceWidth;
		const float h = faceHeight;

		// Face
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, w, h, 3);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x12, 0x16));
		nvgFill(args.vg);
		// A paused scope wears a RED frame. A held trace looks exactly like a live one, so the
		// frame is the only thing that can tell you which you are reading.
		nvgStrokeColor(args.vg, frozen ? FRAME_PAUSED : FRAME_RUN);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		// Graticule anchored to what it MEASURES, not to the window.
		//
		// A division is one volts-per-division tall and one time-base wide, so counting
		// divisions and multiplying by the scale gives you the value — but only if the lines
		// fall at exact multiples of it. Anchored to the middle of the face they did not: zero
		// volts sits wherever the vertical position puts it, so every line stood at some
		// arbitrary voltage and the counting told you nothing.
		//
		// So the horizontal lines are hung from ZERO VOLTS and the vertical ones from the
		// TRIGGER POINT. Three lines above zero is now exactly three divisions of signal, and
		// four lines right of the trigger is exactly four of the time base.
		//
		// Every fifth line is brighter, which lets you count in fives rather than tracking
		// single lines. Zero itself is left to the dashed line that already marks it.
		if (!gridShown)
			goto afterGrid;
		{
			const float gScale = PX_PER_DIV / V_DIVS[vDivIndex];
			const float zeroY = h / 2.f + centreVolts() * gScale;
			const float trigX = PX_PER_DIV;

			for (int pass = 0; pass < 2; pass++) {
				const bool major = (pass == 1);
				nvgBeginPath(args.vg);
				nvgStrokeWidth(args.vg, major ? 0.8f : 0.6f);
				nvgStrokeColor(args.vg, major ? nvgRGBA(0xff, 0xff, 0xff, 0x3a)
					: nvgRGBA(0xff, 0xff, 0xff, 0x18));

				for (int i = -400; i <= 400; i++) {
					if (i == 0 || (major != (i % 5 == 0)))
						continue;
					const float x = trigX + i * PX_PER_DIV;
					if (x > 0.f && x < w) {
						nvgMoveTo(args.vg, x, 0.f);
						nvgLineTo(args.vg, x, h);
					}
					const float y = zeroY + i * PX_PER_DIV;
					if (y > 0.f && y < h) {
						nvgMoveTo(args.vg, 0.f, y);
						nvgLineTo(args.vg, w, y);
					}
				}
				nvgStroke(args.vg);
			}
		}

	afterGrid:
		if (autosetPending)
			autoset();

		// Capture unless frozen. Freezing HOLDS the last sweep rather than stopping the
		// drawing — a paused scope whose trace vanishes is showing you nothing, which is the
		// opposite of what pausing is for.
		// Take a copy the moment it is paused, and let it go the moment it runs again.
		if (frozen && frozenCount == 0 && tapSlot >= 0) {
			frozenBuf.resize(TAP_BUFFER_SIZE);
			frozenCount = tapRead(tapSlot, frozenBuf.data(), TAP_BUFFER_SIZE);
		}
		else if (!frozen && frozenCount != 0) {
			frozenCount = 0;
		}

		// Re-window every frame whether running or paused: paused still has to answer a pan.
		if (tapSlot >= 0) {
			const float sampleRate = APP->engine->getSampleRate();
			const float span = T_DIVS[tDivIndex] * divsX();
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
			const float centreV = centreVolts();

			const float perDiv = V_DIVS[vDivIndex];
			const float scale = PX_PER_DIV / perDiv;

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

			// ZERO VOLTS, dashed and neutral, all the way across. Not the same as the centre of
			// the face: the vertical position moves, so zero moves with it, and knowing where
			// it is is what makes an offset signal readable at a glance.
			const float zy = h / 2 - (0.f - centreV) * scale;
			if (zy > 0 && zy < h) {
				nvgBeginPath(args.vg);
				for (float x = 0; x < w; x += 6) {
					nvgMoveTo(args.vg, x, zy);
					nvgLineTo(args.vg, std::fmin(w, x + 3), zy);
				}
				nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x70));
				nvgStrokeWidth(args.vg, 0.8f);
				nvgStroke(args.vg);
			}

			// THE TRIGGER STRIP. A band down the left of the face, always present, with a
			// triangle whose tip marks the level. The band being permanent is what makes it a
			// control rather than a decoration: there is somewhere to scroll and click even
			// when triggering is off and nothing is drawn in it.
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0.f, 0.f, TRIG_STRIP_W, h);
			// Amber, like every other control on this face — green belongs to the readouts and
			// to the running frame. Darker than the green band it replaces, because a pale wash
			// over a dark face was barely there.
			nvgFillColor(args.vg, nvgRGBA(0xe0, 0xa0, 0x3b, 0x55));
			nvgFill(args.vg);

			if (triggerOn && externalTrigger()) {
				// An X, centred. Nothing here is adjustable — the threshold is fixed and the
				// edge is always the rising one — so a marker with a position or a slope would
				// be claiming a meaning it does not have. A badge says "the trigger comes from
				// elsewhere", which is the whole of it.
				const float cy = h / 2.f;
				const float cx = TRIG_STRIP_W / 2.f;
				const float r = 3.5f;
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, cx - r, cy - r);
				nvgLineTo(args.vg, cx + r, cy + r);
				nvgMoveTo(args.vg, cx + r, cy - r);
				nvgLineTo(args.vg, cx - r, cy + r);
				nvgStrokeColor(args.vg, CONTROL_AMBER);
				nvgStrokeWidth(args.vg, 2.f);
				nvgLineCap(args.vg, NVG_ROUND);
				nvgStroke(args.vg);
			}
			else if (triggerOn) {
				// Drawn against the trace as SHOWN, not against the raw voltage. With AC
				// coupling the trace is drawn about its own mean, so a marker placed at the
				// absolute level pointed at a height the trace never crossed.
				const float ty = h / 2 - (triggerLevel - centreV) * scale;
				if (ty > 0 && ty < h) {
					// A right triangle that DRAWS the edge it triggers on. The tip is at the
					// trigger point — the inner edge of the band, at the level — the right angle
					// sits on the frame beside it, and the third corner goes below for a rising
					// edge and above for a falling one. So the sloping side, read left to right,
					// is a small picture of the slope being caught.
					nvgBeginPath(args.vg);
					nvgMoveTo(args.vg, TRIG_STRIP_W, ty);
					nvgLineTo(args.vg, 0.f, ty);
					nvgLineTo(args.vg, 0.f, triggerRising ? ty + TRIG_STRIP_W : ty - TRIG_STRIP_W);
					nvgClosePath(args.vg);
					nvgFillColor(args.vg, CONTROL_AMBER);
					nvgFill(args.vg);
				}

				// WHERE in time the trigger sits: one division in, which is the pre-trigger.
				// The level says at what height, this says at what moment, and the trace should
				// cross the one at the other.
				const float tx = PX_PER_DIV;
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, tx, h);
				nvgLineTo(args.vg, tx, h - 5.f);
				nvgStrokeColor(args.vg, CONTROL_AMBER);
				nvgStrokeWidth(args.vg, 2.f);
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
		destroyTooltip();
		setCursorShape(GLFW_ARROW_CURSOR);
	}

	/** What the control under the pointer does, or nothing if the pointer is not on one.

	Close and minimise are deliberately left out: a red cross and a yellow dash in the corner of
	a window need no explaining, and a tooltip over them would cover the face every time the
	pointer crossed on its way somewhere else. So is the readout, which says what it is.
	*/
	const char* tipFor(math::Vec pos) {
		if (minimized)
			return NULL;
		if (followBox().contains(pos))
			return "Follow mouse";
		if (homeBox().contains(pos))
			return "Restore";
		if (autoBox().contains(pos))
			return "Autoset";
		if (externalTrigger() && inTrigStrip(pos))
			return "External trigger — right-click to remove";
		if (inTrigTriangle(pos))
			return triggerRising ? "Rising edge — click for falling" : "Falling edge — click for rising";
		if (inTrigStrip(pos))
			return triggerOn ? "Trigger level — click to free-run" : "Free-running — click to trigger";
		if (acBox().contains(pos))
			return acCoupled ? "AC coupled — click for DC" : "DC coupled — click for AC";
		if (gridBox().contains(pos))
			return "Grid";
		if (transportBox().contains(pos))
			return frozen ? "Run" : "Pause";
		return NULL;
	}

	/** Deliberately NOT gated on Rack's own tooltip setting.

	That setting exists because a tooltip on every knob and port gets in the way while
	patching. Six unlabelled letters on a scope face are a different matter: they have no other
	explanation, and a scope is our feature rather than part of Rack.
	*/
	void updateTooltip(math::Vec pos) {
		const char* tip = tipFor(pos);
		if (!tip) {
			destroyTooltip();
			return;
		}
		if (tooltip && tooltipText == tip)
			return;
		destroyTooltip();
		tooltipText = tip;
		tooltip = new ui::Tooltip;
		tooltip->text = tip;
		// A child of the SCENE, not of this widget: a tooltip positions itself against the
		// pointer in screen space, and would be dragged around by the scope otherwise.
		APP->scene->addChild(tooltip);
	}

	void destroyTooltip() {
		if (!tooltip)
			return;
		APP->scene->removeChild(tooltip);
		delete tooltip;
		tooltip = NULL;
		tooltipText.clear();
	}

	void onHover(const HoverEvent& e) override {
		// Click-through while following, so the scope never blocks the knob you are heading
		// for: not consuming the hover lets the widget beneath receive it.
		if (following) {
			destroyTooltip();
			return;
		}
		updateTooltip(e.pos);
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
		// A link dragged out of the strip lands on whatever jack is under the pointer.
		if (linking) {
			linking = false;
			pressedInStrip = false;
			if (PortWidget* target = widgetAt<PortWidget>(APP->scene->rack, linkPos))
				setExternalTrigger(target);
			return;
		}
		// A press in the strip that went nowhere was a click: on the triangle it flips the
		// edge, anywhere else it switches triggering.
		if (pressedInStrip) {
			pressedInStrip = false;
			if (travelled < 2.f) {
				if (triggerOn && !externalTrigger() && inTrigTriangle(pressPos)) {
					triggerRising ^= true;
				}
				else {
					triggerOn ^= true;
					if (triggerOn)
						timeShift = 0.f;
				}
			}
			return;
		}

		// A press on the face that went nowhere was a click: toggle the readout, opening it in
		// scale mode, because you clicked the wave to inspect it.
		if (pressedOnFace && travelled < 2.f) {
			valuesShown ^= true;
			if (valuesShown)
				valueMode = 0;
		}
		pressedOnFace = false;

		resizing = false;
		resizeDir = math::Vec();
	}

	void onDragMove(const DragMoveEvent& e) override {
		const math::Vec d = e.mouseDelta.div(getAbsoluteZoom());
		travelled += d.norm();

		// Pulling a trigger link out of the strip: the scope itself must not move with it.
		if (pressedInStrip) {
			if (!linking && travelled >= 3.f) {
				linking = true;
				linkPos = box.pos.plus(pressPos);
			}
			if (linking)
				linkPos = linkPos.plus(d);
			return;
		}

		if (!resizing) {
			// Plain drag moves the scope. Held-drag is accepted here: it is short, and the
			// alternative would put a mode in front of a positioning nudge.
			offset = offset.plus(d);
			return;
		}

		// Dragging the left or top edge has to move the scope as well as resize it, or the
		// far edge would walk across the rack while you pull the near one.
		if (resizeDir.x > 0.f) {
			faceWidth = std::fmax(MIN_W, faceWidth + d.x);
		}
		else if (resizeDir.x < 0.f) {
			const float newW = std::fmax(MIN_W, faceWidth - d.x);
			offset.x += faceWidth - newW;
			faceWidth = newW;
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
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT
			&& inTrigStrip(e.pos) && externalTrigger()) {
			dropExternalTrigger();
			e.consume(this);
			return;
		}
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
				// A token takes only its two buttons, which are where they always are.
				if (minBox().contains(e.pos)) {
					minimized = false;
					e.consume(this);
					return;
				}
				if (closeBox().contains(e.pos)) {
					detach();   // removed by clipPurgeDead on the next step
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
				autosetWaited = 0;
				// Autoset frames what is arriving now, so it also returns to the live end.
				timeShift = 0.f;
				e.consume(this);
				return;
			}
			if (acBox().contains(e.pos)) {
				acCoupled ^= true;
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
				// Running again means running from now, not from wherever the pan had reached.
				if (frozen)
					timeShift = 0.f;
				frozen ^= true;
				e.consume(this);
				return;
			}
			if (inTrigStrip(e.pos)) {
				// A press here may become a DRAG that pulls an external trigger link out to
				// another jack, so nothing is decided yet — travel decides, on release.
				pressedInStrip = true;
				travelled = 0.f;
				pressPos = e.pos;
				e.consume(this);
				return;
			}
			// Both of the strip's clicks — which edge, and triggering on or off — are decided on
			// RELEASE, in onDragEnd, since either press may instead turn into a drag that pulls
			// out an external trigger link.

			// A click in the values box, or on its top-edge triangle, steps the mode.
			if (valuesShown && valuesBox().contains(e.pos)) {
				valueMode = (valueMode + 1) % 3;
				e.consume(this);
				return;
			}
			// A click on the face toggles the box — but only a CLICK. Deciding here meant every
			// drag of the scope opened or closed the readout on the way past, since a drag
			// begins with a press on the face like any other. The press is only noted; the
			// release decides, by whether the pointer travelled.
			pressedOnFace = true;
			travelled = 0.f;
			pressPos = e.pos;
			e.consume(this);
			return;
		}
		OpaqueWidget::onButton(e);
	}

	/** Scroll POSITIONS the trace; it does not scale it.

	Up and down move the trace through the vertical, sideways pans back and forth through the
	captured samples — the horizontal control of a real scope. The scales belong to the
	controls on the face, so a scroll can never leave you looking at a signal that has silently
	changed size.

	Five times slower than it was, and banked rather than stepped per event, because a trackpad
	glide delivers a stream of small deltas and one step per event ran through the whole range
	in a single gesture.
	*/
	/** Shift turns a wheel into horizontal scrolling, for anyone without a trackpad.

	Rack's own scroll widget does this, and only off macOS: on a Mac the window server swaps
	the axes before Rack ever sees the event, so doing it again here would swap them back. This
	is copied from ScrollWidget deliberately — a scope that panned with a different gesture
	from the rack behind it would be a small cruelty.
	*/
	math::Vec scrollDeltaFor(const HoverScrollEvent& e) {
		math::Vec delta = e.scrollDelta;
#if !defined ARCH_MAC
		if ((APP->window->getMods() & RACK_MOD_MASK) & GLFW_MOD_SHIFT)
			delta = delta.flip();
#endif
		return delta;
	}

	void onHoverScroll(const HoverScrollEvent& e) override {
		const math::Vec delta = scrollDeltaFor(e);

		// Over the trigger strip, scroll sets the LEVEL. A twentieth of a division per step, so
		// crossing one division costs about what a scale step does.
		if (inTrigStrip(e.pos) && externalTrigger()) {
			e.consume(this);   // Fixed threshold: nothing to set.
			return;
		}
		if (inTrigStrip(e.pos)) {
			scrollAccumY += delta.y;
			while (std::fabs(scrollAccumY) >= SCROLL_PER_STEP) {
				const float dir = (scrollAccumY > 0.f) ? 1.f : -1.f;
				scrollAccumY -= dir * SCROLL_PER_STEP;
				triggerLevel += dir * V_DIVS[vDivIndex] / 20.f;
			}
			e.consume(this);
			return;
		}

		// Over the readout below the face, scroll changes the SCALES — which is what the
		// numbers there are. Which scale depends on which of them the pointer is over: the
		// volts per division on the left, the time base on the right. Positioning the trace is
		// what scrolling the face itself is for.
		if (valuesShown && valuesBox().contains(e.pos)) {
			scrollAccumY += delta.y;
			const math::Rect r = valuesBox();
			const bool volts = (e.pos.x < r.pos.x + r.size.x / 2.f);
			while (std::fabs(scrollAccumY) >= SCALE_SCROLL_PER_STEP) {
				const float dir = (scrollAccumY > 0.f) ? 1.f : -1.f;
				scrollAccumY -= dir * SCALE_SCROLL_PER_STEP;
				if (volts)
					vDivIndex = math::clamp(vDivIndex - (int) dir, 0, V_DIV_COUNT - 1);
				else
					tDivIndex = math::clamp(tDivIndex - (int) dir, 0, T_DIV_COUNT - 1);
			}
			e.consume(this);
			return;
		}

		// One axis at a time. The axis is claimed by whichever way the gesture is mostly going
		// when it starts, and held until the scrolling stops for a moment.
		const double now = APP->window->getFrameTime();
		if (now - lastScrollTime > SCROLL_IDLE) {
			scrollAxis = 0;
			axisClaim = math::Vec();
			scrollAccumX = 0.f;
			scrollAccumY = 0.f;
		}
		lastScrollTime = now;
		// The axis comes from the WHOLE opening movement, summed until it is decisive — not
		// from the first event. A horizontal glide almost always starts with a pixel or two of
		// vertical, and judging on that alone locked the scope to the wrong axis for the rest
		// of the gesture. Nothing moves until the sum is worth judging, which is right: a
		// gesture that has not declared itself should not act.
		if (scrollAxis == 0) {
			axisClaim = axisClaim.plus(delta);
			if (std::fabs(axisClaim.x) + std::fabs(axisClaim.y) >= AXIS_CLAIM_TOTAL)
				scrollAxis = (std::fabs(axisClaim.x) > std::fabs(axisClaim.y)) ? 1 : 2;
		}

		// The banked totals belong to the SCALES, which do step, and are left alone here — the
		// face applies its scroll directly, so nothing is stored between events.

		// SMOOTH, not stepped. The same distance per unit of scroll as before, applied
		// continuously — a trace that jumps a quarter of a division at a time reads as a fault
		// in the scope rather than as movement.
		if (scrollAxis == 2) {
			vPos -= (delta.y / SCROLL_PER_STEP) * V_DIVS[vDivIndex] * 0.25f;
		}
		// Sideways only while PAUSED. A running trace is anchored to its trigger, and panning it
		// breaks the very relationship the trigger marks describe — the crossing stops being
		// where they say it is. Paused, there is nothing to anchor to and panning is the point.
		if (scrollAxis == 1 && frozen) {
			const float perDiv = std::fmax(1.f, T_DIVS[tDivIndex] * APP->engine->getSampleRate());
			const float reach = (float) frozenCount;
			const float maxShift = std::fmax(0.f, reach / perDiv - divsX());
			// Negated: scrolling left pulls the trace left, as dragging the paper under a pen
			// would. Following the raw delta moved it the other way, which reads as the window
			// travelling rather than the signal.
			timeShift = math::clamp(timeShift - delta.x / PAN_SCROLL_PER_STEP,
				0.f, maxShift);
		}

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


/** Shows or hides every scope. Hidden, NOT removed: switching the feature off and on again
must give back the scopes that were there, with their scales and their places. */
/** The grab tab on a scope's external trigger link: drag it to another jack to trigger from
there instead, or drop it clear of any jack to go back to triggering on the scope's own
signal. The same gesture as the trace's tab, on the same kind of attachment. */
struct TrigHandleWidget : widget::OpaqueWidget {
	ScopeWidget* scope = NULL;

	TrigHandleWidget() {
		box.size = math::Vec(CLIP_HANDLE, CLIP_HANDLE);
	}

	void step() override {
		math::Vec ring, tab;
		float rr = 0.f;
		visible = scope && scope->parent && scope->visible
			&& scope->trigCalloutGeometry(ring, rr, tab);
		if (visible)
			box.pos = scope->box.pos.plus(tab).minus(box.size.div(2.f));
		widget::OpaqueWidget::step();
	}

	/** Nothing of its own: the tab you see is the one the scope draws. */
	void draw(const DrawArgs& args) override {}

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT || !scope)
			return;
		scope->linking = true;
		scope->linkPos = box.pos.plus(box.size.div(2.f));
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (!scope || !scope->linking)
			return;
		scope->linkPos = scope->linkPos.plus(e.mouseDelta.div(getAbsoluteZoom()));
	}

	void onDragEnd(const DragEndEvent& e) override {
		if (!scope || !scope->linking)
			return;
		scope->linking = false;
		if (PortWidget* target = widgetAt<PortWidget>(APP->scene->rack, scope->linkPos))
			scope->setExternalTrigger(target);
		else
			scope->dropExternalTrigger();
	}
};


void scopeSetVisible(bool visible) {
	for (widget::Widget* child : APP->scene->rack->children) {
		if (ScopeWidget* scope = dynamic_cast<ScopeWidget*>(child))
			clipSetVisible(scope, visible);
	}
}


void scopeCreate(PortWidget* port, bool place) {
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
	// Made under the pointer and carried until it is put down, rather than landing at a fixed
	// offset from the jack — which is often on top of something. NOT when restoring a patch:
	// those already have a place, and carrying them would hand the user a fistful of widgets
	// riding the pointer the moment a patch opened.
	scope->following = place;
	APP->scene->rack->addChild(scope);

	clipAddHandle(scope);

	// The trigger link's tab. Made with the scope and kept for its lifetime — it hides itself
	// when there is no link, which is simpler than making and destroying it as links come
	// and go.
	TrigHandleWidget* th = new TrigHandleWidget;
	th->scope = scope;
	scope->trigHandle = th;
	APP->scene->rack->addChild(th);

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
			scopeCreate(port, false);
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






