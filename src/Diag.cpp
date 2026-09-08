#include "Diag.hpp"
#include "SignalTap.hpp"
#include "Injector.hpp"
#include "Monitor.hpp"
#include "Meter.hpp"
#include "Freq.hpp"
#include "Clip.hpp"

#include <atomic>
#include <cmath>

// The window's metrics, deliberately the same shape as the colour chooser's, so two dialogues
// from one plugin look like two dialogues from one plugin.
static const float DIAG_W = 216.f;
static const float DIAG_PAD = 12.f;
static const float DIAG_TITLE = 14.f;
static const float DIAG_TEXT = 11.f;
static const float DIAG_ROW = 15.f;
static const float DIAG_BTN_H = 20.f;

static const NVGcolor DIAG_BG = nvgRGB(0x21, 0x26, 0x2e);
/** PURE WHITE, every word of it. This is a window somebody reads once, on somebody else's
machine, to copy three numbers into a forum post — legibility beats the softer greys the rest
of the plugin uses to keep a panel calm. The two-tone look is carried by the buttons instead. */
static const NVGcolor DIAG_INK = nvgRGB(0xff, 0xff, 0xff);
static const NVGcolor DIAG_DIM = nvgRGB(0xff, 0xff, 0xff);
static const NVGcolor DIAG_EDGE = nvgRGB(0x3d, 0xd6, 0x8c);
static const NVGcolor DIAG_OFF = nvgRGB(0x2a, 0x31, 0x3b);


/** ALL FIVE ON, which is the plugin behaving normally. A switch here only ever takes work AWAY,
so a rack left with the window open and something switched off is a rack doing less than it
should rather than more — visibly so, since the window says which. */
static std::atomic<uint32_t> gGate{(1u << NUM_DIAG_PARTS) - 1u};
/** Whether anything is being timed. Separate from the window existing, so that closing the
window stops the measuring even if the widget outlives the decision. */
static std::atomic<bool> gMeasuring{false};

static const char* DIAG_NAMES[NUM_DIAG_PARTS] = {
	"Whole process()", "Capture", "Injectors", "Monitors", "Voltmeters", "Frequency",
};


uint32_t diagGate() {
	return gGate.load(std::memory_order_relaxed);
}

bool diagPartOn(int part) {
	if (part < 0 || part >= NUM_DIAG_PARTS)
		return true;
	return (diagGate() & (1u << part)) != 0u;
}

void diagSetPart(int part, bool on) {
	if (part < 0 || part >= NUM_DIAG_PARTS)
		return;
	const uint32_t bit = 1u << part;
	uint32_t now = gGate.load(std::memory_order_relaxed);
	while (!gGate.compare_exchange_weak(now, on ? (now | bit) : (now & ~bit),
			std::memory_order_relaxed)) {
	}
}

const char* diagPartName(int part) {
	if (part < 0 || part >= NUM_DIAG_PARTS)
		return "";
	return DIAG_NAMES[part];
}


// ---- the measurement -------------------------------------------------------------------------

/** Written by the audio thread and read by the drawing one. Plain doubles rather than atomics:
these are statistics on a screen, and a torn read shows a wrong number for one sixtieth of a
second. Making them atomic would put the cost of the instrument into the thing it is measuring. */
static double gAccum = 0.0;         /**< seconds spent inside process(), this block */
static uint64_t gSamples = 0;
static double gClockAccum = 0.0;    /**< seconds spent doing nothing but reading the clock twice */
static uint64_t gClockSamples = 0;
/** THE ROLLING AVERAGES, which is what lets the window have no Reset button.

A total divided by a count never recovers from a change: switch something off and the figure is
still mostly made of the seconds before you did, for as long as the window stays open. That is
what the Reset button was for, and needing a button to make a reading true is a poor instrument.

So the sample averages are folded into these every tenth of a second and the old value is decayed
away. Within a second or two the figures describe what is happening NOW, whatever changed and
whoever changed it — a switch here, a scope clipped on, a patch loaded. "Wait a few seconds after
changing a setting" is then simply true, and there is nothing to press. */
static double gTimeEma = 0.0;
static double gClockEma = 0.0;
/** How often a block is folded in, and how much of the old value survives it. A tenth of a
second and four fifths together settle in about a second and a half. */
static const double EMA_KEEP = 0.8;
static float gSampleTime = 1.f / 48000.f;
static double gStart = 0.0;
/** How often the clock's own cost is measured. Every sample would double the instrument's
footprint to learn something that does not change. */
static const uint64_t CLOCK_EVERY = 64;


void diagBegin() {
	if (!gMeasuring.load(std::memory_order_relaxed))
		return;
	gStart = rack::system::getTime();
}

void diagEnd(float sampleTime) {
	if (!gMeasuring.load(std::memory_order_relaxed))
		return;
	const double end = rack::system::getTime();
	gAccum += end - gStart;
	gSamples++;
	gSampleTime = sampleTime;

	// A tenth of a second's worth, folded in and started again.
	const uint64_t block = (uint64_t) std::fmax(1.0, 0.1 / (double) sampleTime);
	if (gSamples >= block) {
		const double avg = gAccum / (double) gSamples;
		gTimeEma = (gTimeEma <= 0.0) ? avg : gTimeEma * EMA_KEEP + avg * (1.0 - EMA_KEEP);
		gAccum = 0.0;
		gSamples = 0;
	}

	// THE CONTROL, and the number the whole exercise turns on. Rack's meter brackets every
	// module with two of these reads, so whatever a bare pair costs is in every module's
	// reading before any module has done anything at all.
	if ((gSamples % CLOCK_EVERY) == 0) {
		const double a = rack::system::getTime();
		const double b = rack::system::getTime();
		gClockAccum += b - a;
		gClockSamples++;
		if (gClockSamples >= 16) {
			const double avg = gClockAccum / (double) gClockSamples;
			gClockEma = (gClockEma <= 0.0) ? avg : gClockEma * EMA_KEEP + avg * (1.0 - EMA_KEEP);
			gClockAccum = 0.0;
			gClockSamples = 0;
		}
	}
}

/** Nanoseconds a sample of process() takes, averaged over what has been collected. */
static double nsPerSample() {
	return gTimeEma * 1e9;
}

static double nsPerClockPair() {
	return gClockEma * 1e9;
}

/** The same percentage Rack's meter shows: time spent against time available. */
static double percentOf(double ns) {
	const double periodNs = (double) gSampleTime * 1e9;
	return (periodNs > 0.0) ? (ns / periodNs) * 100.0 : 0.0;
}

/** Only on opening, so a window opened now is not showing what the last one was told. Nothing
else clears these: they decay to the truth on their own. */
static void diagReset() {
	gAccum = 0.0;
	gSamples = 0;
	gClockAccum = 0.0;
	gClockSamples = 0;
	gTimeEma = 0.0;
	gClockEma = 0.0;
}


// ---- the window ------------------------------------------------------------------------------

struct DiagWidget;
static DiagWidget* gDiag = NULL;

static std::shared_ptr<rack::window::Font> diagFont() {
	return APP->window->loadFont(rack::asset::system("res/fonts/DejaVuSans.ttf"));
}

/** A CHILD OF THE SCENE rather than of the rack, so it holds still while the rack is scrolled
and zoomed — the same choice the colour chooser makes and for the same reason. */
struct DiagWidget : rack::widget::OpaqueWidget {
	rack::math::Rect toggle[NUM_DIAG_PARTS];
	rack::math::Rect btnClose;
	/** Where the drag began, so the window can be moved out of the way of what it is measuring. */
	bool dragging = false;

	DiagWidget() {
		layout();
		box.pos = rack::math::Vec(60.f, 60.f);
		gMeasuring.store(true, std::memory_order_relaxed);
		diagReset();
	}

	~DiagWidget() {
		if (gDiag == this)
			gDiag = NULL;
		gMeasuring.store(false, std::memory_order_relaxed);
		// EVERYTHING BACK ON. A switch left off here would be a plugin quietly doing less than
		// it was asked to, with the window that explains why no longer on the screen.
		gGate.store((1u << NUM_DIAG_PARTS) - 1u, std::memory_order_relaxed);
	}

	void layout() {
		float y = DIAG_PAD + DIAG_TITLE + 10.f;
		y += DIAG_ROW * 3.f;               // the two measured lines and the note under them
		y += 8.f + DIAG_ROW * 2.f;         // what is running
		y += 10.f + DIAG_ROW + (DIAG_ROW - 3.f);       // the two-line heading over them
		for (int i = 0; i < NUM_DIAG_PARTS; i++) {
			toggle[i] = rack::math::Rect(rack::math::Vec(DIAG_PAD, y),
				rack::math::Vec(DIAG_W - 2.f * DIAG_PAD, DIAG_BTN_H - 2.f));
			y += DIAG_BTN_H;
			// A gap under the first, so it reads as the one to reach for rather than as one of
			// six alike. The rest only matter once something is clipped on.
			if (i == DIAG_BODY)
				y += 7.f;
		}
		y += 8.f;
		btnClose = rack::math::Rect(rack::math::Vec(DIAG_W - DIAG_PAD - 60.f, y),
			rack::math::Vec(60.f, DIAG_BTN_H));
		box.size = rack::math::Vec(DIAG_W, y + DIAG_BTN_H + DIAG_PAD);
	}

	void drawButton(const DrawArgs& args, const rack::math::Rect& r, const char* text, bool on,
			bool accent) {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 4.f);
		nvgFillColor(args.vg, on ? nvgRGB(0x24, 0x3a, 0x30) : DIAG_OFF);
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, on ? DIAG_EDGE : nvgRGB(0x4a, 0x52, 0x5e));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
		// White here too. A button's state is already said by its fill, its border and the word
		// at the front of it; using colour for the lettering as well only makes it harder to read.
		(void) accent;
		nvgFillColor(args.vg, DIAG_INK);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, r.pos.x + 8.f, r.getCenter().y, text, NULL);
	}

	void row(const DrawArgs& args, float y, const char* name, const std::string& value) {
		nvgFillColor(args.vg, DIAG_DIM);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, DIAG_PAD, y, name, NULL);
		nvgFillColor(args.vg, DIAG_INK);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, DIAG_W - DIAG_PAD, y, value.c_str(), NULL);
	}

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 5.f);
		nvgFillColor(args.vg, DIAG_BG);
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, DIAG_EDGE);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		std::shared_ptr<rack::window::Font> font = diagFont();
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);

		nvgFontSize(args.vg, DIAG_TITLE);
		nvgFillColor(args.vg, DIAG_INK);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, DIAG_PAD, DIAG_PAD + DIAG_TITLE / 2.f, "Test Gear diagnostics", NULL);

		nvgFontSize(args.vg, DIAG_TEXT);
		float y = DIAG_PAD + DIAG_TITLE + 10.f + DIAG_ROW / 2.f;

		const double ns = nsPerSample();
		const double clockNs = nsPerClockPair();
		row(args, y, "time in process()", rack::string::f("%.0f ns %.2f%%", ns, percentOf(ns)));
		y += DIAG_ROW;
		// THE CONTROL, and it is not two of anything the reader has to identify.
		//
		// To time a thing you read the clock before it and after it, and that pair of reads costs
		// something itself. Rack brackets every module with exactly such a pair, so whatever it
		// costs is inside every module's reading before the module has done anything at all. That
		// is what this line measures: not our work, but the price of asking what our work costs.
		//
		// "Two clock reads" described the method and left the meaning to be worked out. If this
		// figure is anywhere near the one above it, the meter is largely reporting itself.
		row(args, y, "cost of measuring",
			rack::string::f("%.0f ns %.2f%%", clockNs, percentOf(clockNs)));
		y += DIAG_ROW;
		// HOW LONG IT HAS BEEN AVERAGING, in seconds, rather than how many samples.
		//
		// The count was the same fact and unreadable: a number in the millions, changing forty
		// thousand times a second, which nobody can read and which does not say the one thing it
		// is there to say. What the reader needs to know is whether the average above is worth
		// copying down yet, and that is a length of time — a second or two is plenty, and until
		// then the figures jump about.
		// NO COUNTER FOR HOW LONG IT HAS BEEN AVERAGING. It was there so the reader could tell
		// whether the figures had settled, which is a real question — but the answer to it is
		// always the same, "give it a few seconds", and that is an instruction rather than a
		// number. A rapidly changing figure on screen to tell you that other figures are still
		// changing is one thing too many to read.
		nvgFillColor(args.vg, DIAG_INK);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		// SAYS WHAT TO WAIT FOR RATHER THAN HOW LONG. The figures roll, so they follow whatever
		// has just changed — a switch here, a scope clipped on, a patch loaded — and the reader
		// does not need to know which of those started the clock or how many seconds it takes.
		// Watching them stop moving is the whole instruction, and it fits on one line.
		nvgText(args.vg, DIAG_PAD, y, "Wait for readings to stabilise", NULL);
		y += DIAG_ROW + 8.f;

		// "WIDGETS", WHICH IS WHAT THEY ARE CALLED EVERYWHERE ELSE. The panel's list is headed
		// Widgets and the port menu offers Widgets; "clips" is the word for them in the source,
		// because clipping onto a terminal is what they do, and it means nothing to a reader.
		//
		// It is here because it turns "I have not created any widgets" from something the reporter
		// believes into something the window states: scopes, analysers, monitors, voltmeters,
		// frequency counters and injectors, counted.
		row(args, y, "widgets attached", rack::string::f("%d", clipCount()));
		y += DIAG_ROW;
		row(args, y, "sample rate",
			rack::string::f("%.0f Hz", (gSampleTime > 0.f) ? 1.f / gSampleTime : 0.f));
		y += DIAG_ROW + 10.f;

		nvgFillColor(args.vg, DIAG_DIM);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, DIAG_PAD, y, "Switch a part off and watch", NULL);
		nvgText(args.vg, DIAG_PAD, y + DIAG_ROW - 3.f, "Rack's own meter", NULL);

		for (int i = 0; i < NUM_DIAG_PARTS; i++) {
			const bool on = diagPartOn(i);
			drawButton(args, toggle[i], rack::string::f("%s  %s", on ? "ON " : "off",
				DIAG_NAMES[i]).c_str(), on, false);
		}

		drawButton(args, btnClose, "Close", false, false);
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT) {
			rack::widget::OpaqueWidget::onButton(e);
			return;
		}
		// Consumed whatever is hit, including the background: a window that lets clicks through
		// is a window that adjusts the knob behind it.
		e.consume(this);

		for (int i = 0; i < NUM_DIAG_PARTS; i++) {
			if (toggle[i].contains(e.pos)) {
				diagSetPart(i, !diagPartOn(i));
				return;
			}
		}
		if (btnClose.contains(e.pos)) {
			diagDismiss();
			return;
		}
		dragging = true;
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (dragging)
			box.pos = box.pos.plus(e.mouseDelta.div(APP->scene->rackScroll->getZoom()));
		rack::widget::OpaqueWidget::onDragMove(e);
	}

	void onDragEnd(const DragEndEvent& e) override {
		dragging = false;
		rack::widget::OpaqueWidget::onDragEnd(e);
	}

	void step() override {
		// Re-clamped rather than re-placed, so a window resize cannot leave it off screen.
		box.pos.x = rack::math::clamp(box.pos.x, 0.f,
			std::fmax(0.f, APP->scene->box.size.x - box.size.x));
		box.pos.y = rack::math::clamp(box.pos.y, 0.f,
			std::fmax(0.f, APP->scene->box.size.y - box.size.y));
		rack::widget::OpaqueWidget::step();
	}
};


void diagShow() {
	if (gDiag)
		return;
	gDiag = new DiagWidget;
	APP->scene->addChild(gDiag);
}

void diagDismiss() {
	if (!gDiag)
		return;
	// REQUESTED, NOT DONE HERE. Every caller is inside the window's own click handler, and
	// deleting a widget while event dispatch is still walking it returns into freed memory.
	gDiag->requestDelete();
	gDiag = NULL;
	gMeasuring.store(false, std::memory_order_relaxed);
	gGate.store((1u << NUM_DIAG_PARTS) - 1u, std::memory_order_relaxed);
}

bool diagVisible() {
	return gDiag != NULL;
}

bool diagCovers(rack::math::Vec scenePos) {
	return gDiag && gDiag->box.contains(scenePos);
}
