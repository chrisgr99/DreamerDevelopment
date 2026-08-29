/** A frequency analyser that clips onto a terminal.

What a bench scope's FFT gives you is a spectrum and a peak marker. This adds the one reading
a modular rack actually asks for: the peak NAMED AS A NOTE. Patch it to an oscillator and it
is a tuner; patch it after a filter and the harmonics show you what the filter is doing.

Three things are deliberate.

LOG FREQUENCY, because this is a musical instrument. Octaves are then equal distances and a
harmonic series lands in a recognisable pattern, instead of everything below middle C being
crushed into the left-hand edge.

AVERAGED, always, unless switched off. A single frame's spectrum flickers so badly that the
shape is hard to read at all — the average is what makes it an instrument rather than a light
show. It is an exponential average, so it settles quickly and still follows a change.

AND ITS OWN WINDOW LENGTH. A bench scope analyses whatever its acquisition happened to
capture, so its resolution is whatever the time base left it. The tap here holds eleven
seconds at the engine's rate, so the analyser simply takes the window it wants: 4096 samples,
about ten hertz per bin at 48 kHz, regardless of what any scope on the same signal is doing.
*/
#include "plugin.hpp"
#include "Clip.hpp"
#include "SignalTap.hpp"
#include "WidgetAt.hpp"

#include <dsp/fft.hpp>

#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <string>


/** Samples per transform. About 6 Hz per bin at 48 kHz.

A LOW-END COMPROMISE. Bins are evenly spaced in hertz while the display is logarithmic, so
the bottom of the range is where resolution is scarce: at 4096 samples a bass note came out
as a wide smear, because a dozen hertz either side of it is a large distance down there.
Doubling the window halves that. Doubling again would halve it once more, but each row of the
waterfall then covers a third of a second, and events shorter than that stop being separate
events — which is the other half of what the picture is for.
*/
static const int FFT_SIZE = 8192;
/** The range drawn: 20 Hz to whatever Nyquist is, and 80 dB of it. */
static const float ANA_MIN_HZ = 20.f;
static const float ANA_DB_FLOOR = -80.f;
/** The waterfall floors higher than the spectrum does, and squares what is left.

Eighty decibels of range spread over a colour ramp meant the room noise thirty decibels down
was still a definite green, so the whole face carried a wash that said nothing. Cutting the
range and bending it towards the dark end leaves the quiet parts genuinely dark, and the
bands that remain are the partials that are actually there.
*/
static const float WF_DB_FLOOR = -60.f;
/** How much of the new spectrum each frame contributes. Low enough to steady the display,
high enough that a change is visible immediately. */
static const float ANA_AVERAGE = 0.25f;

/** The waterfall's own resolution, independent of the face size.

Held as a texture rather than as thousands of little rectangles: a face this size would be
some eleven thousand quads per frame, and one image the graphics card scales costs nothing.
Fixed dimensions mean resizing the analyser stretches the history rather than throwing it
away.
*/
static const int WF_COLS = 256;
static const int WF_ROWS = 160;

/** Resize borders and the smallest useful face, as on the scope — the same numbers, so a
grab that works on one works on the other. */
static const float ANA_RESIZE_EDGE = 6.f;
static const float ANA_MIN_W = 90.f, ANA_MIN_H = 50.f;

static const NVGcolor ANA_GREEN = nvgRGB(0x3d, 0xe0, 0x7a);
static const NVGcolor ANA_AMBER = nvgRGB(0xe0, 0xa0, 0x3b);


struct AnalyserWidget : ClipWidget {
	int tapSlot = -1;
	bool averaging = true;
	bool harmonics = true;

	dsp::RealFFT fft{FFT_SIZE};
	std::vector<float> input;
	std::vector<float> spectrum;   // interleaved output from the FFT
	std::vector<float> mags;       // magnitude per bin, averaged
	std::vector<float> raw;        // this frame's magnitudes, unaveraged
	std::vector<float> window;     // Hann, computed once
	float peakHz = 0.f;
	float peakMag = 0.f;

	/** The waterfall: the same spectrum, but with time as the second axis.

	A spectrum answers "what is in this sound"; a waterfall answers "what is it DOING" — a
	filter sweeping, an envelope opening, a sequence changing note. Neither replaces the
	other, so the W switches between them rather than the analyser being one or the whole
	thing being two widgets.
	*/
	/** The frequency range on show. Zero means "everything", which is what a new analyser
	shows and what the menu puts it back to.

	Held in hertz rather than as a zoom factor and a centre, because hertz is what the axis is
	made of: the range survives a change of sample rate, and a saved patch reopens looking at
	the same frequencies rather than at the same fraction of a different span.
	*/
	float viewMinHz = 0.f;
	float viewMaxHz = 0.f;

	bool waterfall = false;
	std::vector<uint8_t> wfPixels;
	int wfImage = -1;
	bool wfDirty = false;

	AnalyserWidget() {
		faceWidth = 150.f;
		faceHeight = 74.f;
		box.size = math::Vec(faceWidth, faceHeight);
		input.resize(FFT_SIZE, 0.f);
		spectrum.resize(FFT_SIZE * 2, 0.f);
		mags.assign(FFT_SIZE / 2, 0.f);
		raw.assign(FFT_SIZE / 2, 0.f);
		window.resize(FFT_SIZE);
		for (int i = 0; i < FFT_SIZE; i++)
			window[i] = 0.5f - 0.5f * std::cos(2.f * M_PI * i / (FFT_SIZE - 1));
		wfPixels.assign(WF_COLS * WF_ROWS * 4, 0);
	}

	~AnalyserWidget() {
		destroyTooltip();
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
		if (wfImage >= 0 && APP->window && APP->window->vg)
			nvgDeleteImage(APP->window->vg, wfImage);
	}

	bool reattach(app::PortWidget* target) override {
		const int slot = tapCreate(target->module->id, target->portId,
			target->type == engine::Port::OUTPUT);
		if (slot < 0) {
			WARN("Analyser: no tap slots available");
			return false;
		}
		if (tapSlot >= 0)
			tapDestroy(tapSlot);
		tapSlot = slot;
		port = target;
		mags.assign(FFT_SIZE / 2, 0.f);
		return true;
	}

	void step() override {
		followPort();
		analyse();
		ClipWidget::step();
	}

	/** One transform per frame, windowed and averaged into the running spectrum. */
	void analyse() {
		if (tapSlot < 0 || retargeting)
			return;
		const int have = tapRead(tapSlot, input.data(), FFT_SIZE);
		if (have < FFT_SIZE)
			return;

		// Hann window: without one, a tone that does not sit exactly on a bin smears across the
		// whole spectrum and the picture is useless. Computed once, in the constructor.
		for (int i = 0; i < FFT_SIZE; i++)
			input[i] *= window[i];
		fft.rfft(input.data(), spectrum.data());

		const int bins = FFT_SIZE / 2;
		const float norm = 2.f / FFT_SIZE;
		float best = 0.f;
		int bestBin = 0;

		for (int i = 1; i < bins; i++) {
			const float re = spectrum[i * 2];
			const float im = spectrum[i * 2 + 1];
			const float m = std::sqrt(re * re + im * im) * norm;
			raw[i] = m;
			mags[i] = averaging ? (mags[i] + (m - mags[i]) * ANA_AVERAGE) : m;
			if (mags[i] > best) {
				best = mags[i];
				bestBin = i;
			}
		}

		// The peak, refined by fitting a parabola through its neighbours — a bin is 12 Hz wide
		// and a tuner that could only ever be right to 12 Hz would not be worth having.
		const float sr = tapSampleRate();
		float refined = (float) bestBin;
		if (bestBin > 1 && bestBin < bins - 1) {
			const float a = mags[bestBin - 1], b = mags[bestBin], c = mags[bestBin + 1];
			const float denom = a - 2.f * b + c;
			if (std::fabs(denom) > 1e-9f)
				refined += 0.5f * (a - c) / denom;
		}
		peakHz = refined * sr / FFT_SIZE;
		peakMag = best;

		if (waterfall)
			pushWaterfallRow();
	}

	/** Heat for a magnitude: dark, then green, then amber, then white at the top.

	Green through amber because those are already this plugin's two colours — a reading and a
	control — so a waterfall sits in the same instrument rather than looking like a heat map
	borrowed from somewhere else. White only at the very top, which is what makes a loud
	partial stand out from a merely present one.
	*/
	static void heat(float t, uint8_t& r, uint8_t& g, uint8_t& b) {
		t = math::clamp(t, 0.f, 1.f);
		t = t * t;   // Dark stays dark; see WF_DB_FLOOR.
		auto mix = [](float a, float bb, float u) { return a + (bb - a) * u; };
		float rf, gf, bf;
		if (t < 0.35f) {
			const float u = t / 0.35f;
			rf = mix(8.f, 30.f, u); gf = mix(14.f, 140.f, u); bf = mix(22.f, 90.f, u);
		}
		else if (t < 0.75f) {
			const float u = (t - 0.35f) / 0.40f;
			rf = mix(30.f, 224.f, u); gf = mix(140.f, 160.f, u); bf = mix(90.f, 59.f, u);
		}
		else {
			const float u = (t - 0.75f) / 0.25f;
			rf = mix(224.f, 255.f, u); gf = mix(160.f, 245.f, u); bf = mix(59.f, 235.f, u);
		}
		r = (uint8_t) rf; g = (uint8_t) gf; b = (uint8_t) bf;
	}

	/** Newest row at the top, everything else pushed down — which is what "waterfall" means,
	and puts the moment you are listening to where the eye already is. */
	void pushWaterfallRow() {
		const int stride = WF_COLS * 4;
		std::memmove(wfPixels.data() + stride, wfPixels.data(), stride * (WF_ROWS - 1));

		const float sr = tapSampleRate();
		const float maxHz = std::fmax(1000.f, sr / 2.f);
		const int bins = FFT_SIZE / 2;

		for (int col = 0; col < WF_COLS; col++) {
			// The column's own slice of the spectrum, not a single bin. High up, one column
			// covers hundreds of bins, and sampling just one of them makes a loud partial
			// flicker in and out as it drifts between them.
			const float t0 = (float) col / WF_COLS;
			const float t1 = (float) (col + 1) / WF_COLS;
			const float hz0 = ANA_MIN_HZ * std::pow(maxHz / ANA_MIN_HZ, t0);
			const float hz1 = ANA_MIN_HZ * std::pow(maxHz / ANA_MIN_HZ, t1);
			int b0 = (int) std::floor(hz0 * FFT_SIZE / sr);
			int b1 = (int) std::ceil(hz1 * FFT_SIZE / sr);
			b0 = math::clamp(b0, 1, bins - 1);
			b1 = math::clamp(b1, b0 + 1, bins);

			// THE RAW SPECTRUM, not the averaged one. The average exists to stop a live
			// spectrum flickering, and it works by blurring a tenth of a second together —
			// which in a waterfall, whose whole subject is change over time, smeared every
			// event into the haze that made this hard to read.
			float m = 0.f;
			for (int i = b0; i < b1; i++)
				m = std::fmax(m, raw[i]);

			const float db = 20.f * std::log10(std::fmax(m, 1e-6f) / 5.f);
			uint8_t r, g, b;
			heat((db - WF_DB_FLOOR) / -WF_DB_FLOOR, r, g, b);
			uint8_t* px = wfPixels.data() + col * 4;
			px[0] = r; px[1] = g; px[2] = b; px[3] = 0xff;
		}
		wfDirty = true;
	}

	/** Clear of the bottom-left resize corner, so the corner is still grabbable. */
	math::Rect wfBox() {
		return math::Rect(math::Vec(7.f, faceHeight - 21.f), math::Vec(13.f, 13.f));
	}

	/** Which edge or corner the pointer is on, exactly as the scope decides it: any of the
	four, or a corner for two at once. */
	math::Vec resizeZoneAt(math::Vec pos) {
		math::Vec dir;
		if (pos.x <= ANA_RESIZE_EDGE)
			dir.x = -1;
		else if (pos.x >= faceWidth - ANA_RESIZE_EDGE)
			dir.x = 1;
		if (pos.y <= ANA_RESIZE_EDGE)
			dir.y = -1;
		else if (pos.y >= faceHeight - ANA_RESIZE_EDGE)
			dir.y = 1;
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

	/** The whole range the analyser could show: everything the sample rate makes real. */
	float fullMaxHz() {
		return std::fmax(1000.f, tapSampleRate() / 2.f);
	}

	float viewLow() {
		return (viewMinHz > 0.f) ? viewMinHz : ANA_MIN_HZ;
	}

	float viewHigh() {
		return (viewMaxHz > 0.f) ? std::fmin(viewMaxHz, fullMaxHz()) : fullMaxHz();
	}

	/** Where a frequency falls across the face, on a log scale, within the view. */
	float xForHz(float hz, float w) {
		const float lo = viewLow(), hi = viewHigh();
		if (hz <= lo)
			return 0.f;
		const float t = std::log(hz / lo) / std::log(hi / lo);
		return math::clamp(t, 0.f, 1.f) * w;
	}

	/** Zooms about a point on the face, keeping the frequency under the pointer where it is.

	Zooming about the CENTRE would push whatever you were looking at off to one side, and the
	thing under the pointer is by definition the thing you are interested in — this is the
	same rule the rack's own zoom follows.
	*/
	void zoomView(float factor, float frac) {
		const float lo = viewLow(), hi = viewHigh();
		const float fullSpan = std::log(fullMaxHz() / ANA_MIN_HZ);
		// A quarter of an octave is as close as it goes: enough to separate two partials a
		// semitone apart, and short of the point where the bins themselves become the picture.
		const float minSpan = std::log(2.f) / 4.f;

		const float span = std::log(hi / lo);
		const float hzAt = lo * std::exp(span * frac);
		const float newSpan = math::clamp(span / factor, minSpan, fullSpan);

		float newLo = hzAt * std::exp(-newSpan * frac);
		newLo = math::clamp(newLo, ANA_MIN_HZ, fullMaxHz() / std::exp(newSpan));
		viewMinHz = newLo;
		viewMaxHz = newLo * std::exp(newSpan);
		// Back to "everything" when it is everything, so the menu entry and the saved patch
		// agree with what is on screen.
		if (newSpan >= fullSpan - 1e-4f) {
			viewMinHz = 0.f;
			viewMaxHz = 0.f;
		}
	}

	/** Slides the view along the axis. The shift is in log units, so a given scroll moves the
	same visual distance whether you are looking at the bass or the top octave. */
	void panView(float dLog) {
		const float lo = viewLow(), hi = viewHigh();
		const float span = std::log(hi / lo);
		if (span >= std::log(fullMaxHz() / ANA_MIN_HZ) - 1e-4f)
			return;   // Nothing to pan: the whole range is already on show.
		float newLo = lo * std::exp(dLog);
		newLo = math::clamp(newLo, ANA_MIN_HZ, fullMaxHz() / std::exp(span));
		viewMinHz = newLo;
		viewMaxHz = newLo * std::exp(span);
	}

	/** Accepts a pinch aimed at this face. Returns false if the pointer is elsewhere. */
	bool pinch(float mag) {
		const math::Vec pos = APP->scene->rack->getMousePos().minus(box.pos);
		if (pos.x < 0.f || pos.y < 0.f || pos.x > faceWidth || pos.y > faceHeight)
			return false;
		zoomView(1.f + mag, math::clamp(pos.x / faceWidth, 0.f, 1.f));
		return true;
	}

	static std::string noteFor(float hz) {
		if (hz < 8.f)
			return "";
		static const char* names[12] = {"C", "C#", "D", "D#", "E", "F",
			"F#", "G", "G#", "A", "A#", "B"};
		const int n = (int) std::lround(69.f + 12.f * std::log2(hz / 440.f));
		if (n < 0 || n > 127)
			return "";
		return string::f("%s%d", names[n % 12], n / 12 - 1);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 3)
			drawFace(args);
		widget::OpaqueWidget::drawLayer(args, layer);
	}

	void draw(const DrawArgs& args) override {}

	void drawFace(const DrawArgs& args) {
		drawCallout(args.vg);

		const float w = faceWidth, h = faceHeight;

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, w, h, 3);
		nvgFillColor(args.vg, nvgRGB(0x10, 0x12, 0x16));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, ANA_GREEN);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		nvgSave(args.vg);
		nvgScissor(args.vg, 0, 0, w, h);

		// An octave grid, which on a log scale is evenly spaced and is the only division a
		// musical spectrum wants.
		nvgBeginPath(args.vg);
		for (float hz = 31.25f; hz < viewHigh(); hz *= 2.f) {
			if (hz < viewLow())
				continue;
			const float x = xForHz(hz, w);
			nvgMoveTo(args.vg, x, 0.f);
			nvgLineTo(args.vg, x, h);
		}
		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x18));
		nvgStrokeWidth(args.vg, 0.6f);
		nvgStroke(args.vg);

		if (waterfall)
			drawWaterfall(args);

		// The spectrum itself, as a filled curve: area reads as "how much is here" far better
		// than a line does at this size.
		const int bins = FFT_SIZE / 2;
		const float sr = tapSampleRate();
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, 0.f, h);
		for (int i = 1; i < bins; i++) {
			const float hz = i * sr / FFT_SIZE;
			if (hz < viewLow())
				continue;
			if (hz > viewHigh())
				break;
			const float db = 20.f * std::log10(std::fmax(mags[i], 1e-6f) / 5.f);
			const float y = h - math::clamp((db - ANA_DB_FLOOR) / -ANA_DB_FLOOR, 0.f, 1.f) * h;
			nvgLineTo(args.vg, xForHz(hz, w), y);
		}
		nvgLineTo(args.vg, w, h);
		nvgClosePath(args.vg);
		// Over a waterfall the fill would bury the history, so only the line is drawn there —
		// enough to read the current shape against the picture of how it got there.
		if (!waterfall) {
			nvgFillColor(args.vg, nvgRGBA(0x3d, 0xe0, 0x7a, 0x66));
			nvgFill(args.vg);
		}
		nvgStrokeColor(args.vg, ANA_GREEN);
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		// Harmonics of the peak: ticks along the top, so it is obvious at a glance whether a
		// filter is taking them away or a waveshaper is adding them.
		if (harmonics && peakHz > ANA_MIN_HZ) {
			nvgBeginPath(args.vg);
			for (int k = 2; k <= 8; k++) {
				const float x = xForHz(peakHz * k, w);
				if (x <= 0.f || x >= w)
					continue;
				nvgMoveTo(args.vg, x, 0.f);
				nvgLineTo(args.vg, x, 5.f);
			}
			nvgStrokeColor(args.vg, nvgRGBA(0xe0, 0xa0, 0x3b, 0x99));
			nvgStrokeWidth(args.vg, 1.2f);
			nvgStroke(args.vg);

			// The peak itself.
			const float px = xForHz(peakHz, w);
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, px, 0.f);
			nvgLineTo(args.vg, px, h);
			nvgStrokeColor(args.vg, nvgRGBA(0xe0, 0xa0, 0x3b, 0x77));
			nvgStrokeWidth(args.vg, 1.f);
			nvgStroke(args.vg);
		}

		nvgRestore(args.vg);

		drawWaterfallButton(args.vg);

		// The reading: the note first, because that is the answer to the question anyone
		// patches this in to ask, and the frequency beside it for the ones who want it exact.
		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (font && font->handle >= 0 && peakMag > 1e-4f) {
			const std::string note = noteFor(peakHz);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

			nvgFontSize(args.vg, 13.f);
			nvgFillColor(args.vg, ANA_GREEN);
			for (int i = 0; i < 2; i++)
				nvgText(args.vg, 4.f + i * 0.35f, 3.f, note.c_str(), NULL);

			nvgFontSize(args.vg, 9.f);
			nvgFillColor(args.vg, nvgRGBA(0x3d, 0xe0, 0x7a, 0xcc));
			nvgText(args.vg, 4.f, 17.f, string::f("%.1f Hz", peakHz).c_str(), NULL);
		}

		widget::OpaqueWidget::draw(args);
	}

	ui::Tooltip* tooltip = NULL;

	void updateTooltip(math::Vec pos) {
		const bool over = wfBox().contains(pos);
		if (!over) {
			destroyTooltip();
			return;
		}
		if (tooltip)
			return;
		tooltip = new ui::Tooltip;
		tooltip->text = "Waterfall: the spectrum over time";
		// A child of the SCENE, not of this widget: a tooltip places itself against the
		// pointer in screen space, and would be dragged about by the analyser otherwise.
		APP->scene->addChild(tooltip);
	}

	void destroyTooltip() {
		if (!tooltip)
			return;
		APP->scene->removeChild(tooltip);
		delete tooltip;
		tooltip = NULL;
	}

	/** One axis at a time, claimed from the whole opening movement — the same rule as the
	scope, and for the same reason: a horizontal glide starts with a pixel or two of vertical,
	and judging on the first event alone locks the gesture to the wrong axis. */
	int scrollAxis = 0;
	math::Vec axisClaim;
	double lastScrollTime = 0.0;

	void onHoverScroll(const HoverScrollEvent& e) override {
		math::Vec delta = e.scrollDelta;
#if !defined ARCH_MAC
		if ((APP->window->getMods() & RACK_MOD_MASK) & GLFW_MOD_SHIFT)
			delta = delta.flip();
#endif

		const double now = APP->window->getFrameTime();
		if (now - lastScrollTime > 0.7) {
			scrollAxis = 0;
			axisClaim = math::Vec();
		}
		lastScrollTime = now;
		if (scrollAxis == 0) {
			axisClaim = axisClaim.plus(delta);
			if (std::fabs(axisClaim.x) + std::fabs(axisClaim.y) >= 6.f)
				scrollAxis = (std::fabs(axisClaim.x) > std::fabs(axisClaim.y)) ? 1 : 2;
		}

		// Sideways moves the view along the axis. Negated, so the axis follows the hand:
		// scrolling left pulls the spectrum left, as dragging paper under a pen would.
		if (scrollAxis == 1)
			panView(-delta.x / 200.f);
		// Upwards zooms in, about the pointer.
		else if (scrollAxis == 2)
			zoomView(1.f + delta.y / 200.f, math::clamp(e.pos.x / faceWidth, 0.f, 1.f));

		e.consume(this);
	}

	void onHover(const HoverEvent& e) override {
		if (following) {
			destroyTooltip();
			return;
		}
		updateTooltip(e.pos);
		druiSetCursorShape(cursorForZone(resizeZoneAt(e.pos)));
		widget::OpaqueWidget::onHover(e);
	}

	void onLeave(const LeaveEvent& e) override {
		destroyTooltip();
		widget::OpaqueWidget::onLeave(e);
	}

	void drawWaterfall(const DrawArgs& args) {
		if (wfImage < 0)
			wfImage = nvgCreateImageRGBA(args.vg, WF_COLS, WF_ROWS, 0, wfPixels.data());
		if (wfImage < 0)
			return;
		if (wfDirty) {
			nvgUpdateImage(args.vg, wfImage, wfPixels.data());
			wfDirty = false;
		}
		// THE HISTORY IS STORED OVER THE WHOLE RANGE, always, and the view is drawn as a crop
		// of it. Both mappings are logarithmic, so a frequency range is a straight slice of
		// the stored columns — which means zooming re-frames the rows already collected
		// instead of throwing them away and starting the picture again.
		const float fullSpan = std::log(fullMaxHz() / ANA_MIN_HZ);
		const float l0 = std::log(viewLow() / ANA_MIN_HZ) / fullSpan;
		const float l1 = std::log(viewHigh() / ANA_MIN_HZ) / fullSpan;
		const float frac = std::fmax(1e-4f, l1 - l0);
		const float ex = faceWidth / frac;
		const NVGpaint paint = nvgImagePattern(args.vg, -l0 * ex, 0.f, ex, faceHeight,
			0.f, wfImage, 1.f);
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.f, 0.f, faceWidth, faceHeight);
		nvgFillPaint(args.vg, paint);
		nvgFill(args.vg);
	}

	/** The letter IS the control, as on the scope — but ALWAYS at full strength here.

	Elsewhere a dimmed letter says "not doing anything", which is worth stating when the
	difference is invisible. The waterfall is the opposite case: the face either has a picture
	of history on it or it does not, so dimming the W said nothing the display was not already
	saying, and made the one control on the face hard to find.
	*/
	void drawWaterfallButton(NVGcontext* vg) {
		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return;
		const math::Rect r = wfBox();
		nvgFontFaceId(vg, font->handle);
		nvgFontSize(vg, 12.f);
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, ANA_AMBER);
		nvgText(vg, r.pos.x + r.size.x / 2, r.pos.y + r.size.y / 2, "W", NULL);
	}

	/** Dragged by its face, resized from its right and bottom edges. */
	void onDragMove(const DragMoveEvent& e) override {
		const math::Vec d = e.mouseDelta.div(getAbsoluteZoom());
		if (!resizing) {
			offset = offset.plus(d);
			return;
		}

		// Dragging the left or top edge has to move the face as well as resize it, or the far
		// edge would walk across the rack while you pull the near one.
		if (resizeDir.x > 0.f) {
			faceWidth = std::fmax(ANA_MIN_W, faceWidth + d.x);
		}
		else if (resizeDir.x < 0.f) {
			const float newW = std::fmax(ANA_MIN_W, faceWidth - d.x);
			offset.x += faceWidth - newW;
			faceWidth = newW;
		}
		if (resizeDir.y > 0.f) {
			faceHeight = std::fmax(ANA_MIN_H, faceHeight + d.y);
		}
		else if (resizeDir.y < 0.f) {
			const float newH = std::fmax(ANA_MIN_H, faceHeight - d.y);
			offset.y += faceHeight - newH;
			faceHeight = newH;
		}
		box.size = math::Vec(faceWidth, faceHeight);
	}

	bool resizing = false;
	math::Vec resizeDir;

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			ui::Menu* menu = createMenu();
			menu->addChild(createMenuLabel("Analyser"));
			menu->addChild(createBoolPtrMenuItem("Average", "", &averaging));
				menu->addChild(createBoolPtrMenuItem("Harmonic markers", "", &harmonics));
			menu->addChild(createBoolPtrMenuItem("Waterfall", "", &waterfall));
			menu->addChild(new ui::MenuSeparator);
			menu->addChild(createMenuItem("Full range", "", [this]() {
				viewMinHz = 0.f;
				viewMaxHz = 0.f;
			}));
			menu->addChild(createMenuItem("Remove", "", [this]() { detach(); }));
			e.consume(this);
			return;
		}
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& wfBox().contains(e.pos)) {
			waterfall ^= true;
			if (waterfall)
				std::fill(wfPixels.begin(), wfPixels.end(), 0);   // No stale history.
			wfDirty = true;
			e.consume(this);
			return;
		}
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			resizeDir = resizeZoneAt(e.pos);
			resizing = (resizeDir.x != 0.f || resizeDir.y != 0.f);
			e.consume(this);
			return;
		}
		widget::OpaqueWidget::onButton(e);
	}

	void onDragStart(const DragStartEvent& e) override {
		e.consume(this);
	}

	void onDragEnd(const DragEndEvent& e) override {
		resizing = false;
		resizeDir = math::Vec();
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
		json_object_set_new(rootJ, "width", json_real(faceWidth));
		json_object_set_new(rootJ, "height", json_real(faceHeight));
		json_object_set_new(rootJ, "average", json_boolean(averaging));
		json_object_set_new(rootJ, "harmonics", json_boolean(harmonics));
		json_object_set_new(rootJ, "waterfall", json_boolean(waterfall));
		json_object_set_new(rootJ, "viewMinHz", json_real(viewMinHz));
		json_object_set_new(rootJ, "viewMaxHz", json_real(viewMaxHz));
		return rootJ;
	}

	void fromJson(json_t* rootJ) {
		auto num = [&](const char* key, float& target) {
			if (json_t* j = json_object_get(rootJ, key))
				target = json_number_value(j);
		};
		auto boolean = [&](const char* key, bool& target) {
			if (json_t* j = json_object_get(rootJ, key))
				target = json_boolean_value(j);
		};
		num("offsetX", offset.x);
		num("offsetY", offset.y);
		num("width", faceWidth);
		num("height", faceHeight);
		boolean("average", averaging);
		boolean("harmonics", harmonics);
		boolean("waterfall", waterfall);
		num("viewMinHz", viewMinHz);
		num("viewMaxHz", viewMaxHz);
		box.size = math::Vec(faceWidth, faceHeight);
	}
};


void analyserCreate(app::PortWidget* port, bool place) {
	if (!port || !port->module)
		return;

	AnalyserWidget* a = new AnalyserWidget;
	a->port = port;
	a->tapSlot = tapCreate(port->module->id, port->portId,
		port->type == engine::Port::OUTPUT);
	if (a->tapSlot < 0) {
		WARN("Analyser: no tap slots available");
		delete a;
		return;
	}
	a->following = place;
	APP->scene->rack->addChild(a);
	clipAddHandle(a);
	clipAddClose(a);
	INFO("Analyser: attached to port %d", port->portId);
}


bool analyserPinch(float mag) {
	for (auto it = APP->scene->rack->children.rbegin();
		it != APP->scene->rack->children.rend(); it++) {
		AnalyserWidget* a = dynamic_cast<AnalyserWidget*>(*it);
		if (a && a->visible && a->pinch(mag))
			return true;
	}
	return false;
}


void analyserSetVisible(bool visible) {
	for (widget::Widget* child : APP->scene->rack->children) {
		if (AnalyserWidget* a = dynamic_cast<AnalyserWidget*>(child))
			clipSetVisible(a, visible);
	}
}


// ---- Saving with the patch, the same way the scopes do ----

struct PendingAnalyser {
	int64_t moduleId = -1;
	int portId = 0;
	bool isOutput = true;
	json_t* stateJ = NULL;
	int budget = 300;
};

static std::vector<PendingAnalyser> pending;


json_t* analyserToJson() {
	json_t* arrayJ = json_array();
	for (widget::Widget* child : APP->scene->rack->children) {
		AnalyserWidget* a = dynamic_cast<AnalyserWidget*>(child);
		if (a && a->port)
			json_array_append_new(arrayJ, a->toJson());
	}
	for (const PendingAnalyser& p : pending) {
		if (p.stateJ)
			json_array_append(arrayJ, p.stateJ);
	}
	return arrayJ;
}


void analyserFromJson(json_t* arrayJ) {
	for (PendingAnalyser& p : pending) {
		if (p.stateJ)
			json_decref(p.stateJ);
	}
	pending.clear();
	if (!arrayJ || !json_is_array(arrayJ))
		return;

	size_t i;
	json_t* aJ;
	json_array_foreach(arrayJ, i, aJ) {
		json_t* moduleIdJ = json_object_get(aJ, "moduleId");
		if (!moduleIdJ)
			continue;
		PendingAnalyser p;
		p.moduleId = json_integer_value(moduleIdJ);
		if (json_t* j = json_object_get(aJ, "portId"))
			p.portId = json_integer_value(j);
		if (json_t* j = json_object_get(aJ, "isOutput"))
			p.isOutput = json_boolean_value(j);
		p.stateJ = json_incref(aJ);
		pending.push_back(p);
	}
}


void analyserRestoreStep() {
	if (pending.empty())
		return;

	for (size_t i = 0; i < pending.size();) {
		PendingAnalyser& p = pending[i];
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
			analyserCreate(found, false);
			for (auto it = APP->scene->rack->children.rbegin();
				it != APP->scene->rack->children.rend(); it++) {
				AnalyserWidget* a = dynamic_cast<AnalyserWidget*>(*it);
				if (a && a->port == found) {
					a->fromJson(p.stateJ);
					break;
				}
			}
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		if (--p.budget <= 0) {
			WARN("Analyser: module %lld never appeared, dropping it",
				(long long) p.moduleId);
			json_decref(p.stateJ);
			pending.erase(pending.begin() + i);
			continue;
		}
		i++;
	}
}
