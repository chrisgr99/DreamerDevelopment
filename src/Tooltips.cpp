/** TOOLTIP READABILITY: Rack's own tooltips, drawn large, in high contrast, and below the control.

WHAT IS SHOWN IS RACK'S. A tooltip already says the most useful things about whatever is under
the pointer — a control's name, value and the description its maker wrote, a port's name, its
live voltage and what it is connected to — and Rack updates that text every frame. This does not
repeat any of that work. It reads the text of the tooltip Rack has made, moves Rack's own off the
screen, and draws the same words again: larger, light on black or in the classic light yellow,
and directly beneath the control, where Rack puts its tooltip down and to the right of the
pointer, which can be out of view.

WHY MOVE IT RATHER THAN HIDE IT. A hidden widget may not be stepped, and a tooltip's text is
written in its step. Moved, it goes on updating where nobody can see it, and this reads it.

EITHER ORDER WORKS. Rack adds a tooltip to the scene when one is needed, so it may come before
this in the scene's children or after it. Before, its step has already put it at the pointer
when this one moves it; after, its step puts it back — so it is moved again in this one's draw,
which comes before the tooltip's own draw in that order. Either way Rack's is never drawn where
it can be seen, and nothing has to fight over being the scene's last child.
*/
// Before Rack's headers, whose Rect would otherwise clash with the system's.
#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif
#include "plugin.hpp"
#include "Settings.hpp"

#include <ui/Tooltip.hpp>

#include <cctype>
#include <vector>


/** HOW LARGE THE SYSTEM DRAWS THE POINTER, as a multiple of its normal size. macOS lets the
pointer be enlarged in its accessibility settings, and the tooltip has to clear the tail of the
pointer as it is actually drawn. Read when a tooltip appears, so a change takes effect at the
next one. Elsewhere, 1. */
static float pointerScale() {
#ifdef ARCH_MAC
	CFStringRef app = CFSTR("com.apple.universalaccess");
	CFPreferencesAppSynchronize(app);
	CFPropertyListRef v = CFPreferencesCopyAppValue(CFSTR("mouseDriverCursorSize"), app);
	float scale = 1.f;
	if (v) {
		if (CFGetTypeID(v) == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef) v, kCFNumberFloatType, &scale);
		CFRelease(v);
	}
	return math::clamp(scale, 1.f, 4.f);
#else
	return 1.f;
#endif
}


struct TooltipReadabilityOverlay : widget::Widget {
	bool* on = NULL;
	/** Rack's tooltip this frame and what it says, and where the control it belongs to is. */
	ui::Tooltip* tip = NULL;
	std::string text;
	math::Rect anchor;
	bool haveAnchor = false;
	/** FADING. In over half a second when a tooltip appears, out over a quarter when it goes —
	drawn where it last was, from the words it last said, since Rack's own is gone by then. */
	ui::Tooltip* lastTip = NULL;
	double shownAt = -1.0;
	double hiddenAt = -1.0;
	float alphaAtHide = 0.f;
	float lastAlpha = 0.f;
	math::Rect lastBox;
	/** THE WIDEST AND TALLEST THIS CONTROL'S TOOLTIP HAS BEEN. Rack writes a value to a number
	of figures rather than of decimal places, so turning a knob walks through 9.99, 10.0, 10.05,
	and a box that followed the text grew and shrank — and, being centred on the control, moved
	its words left and right with every step. It widens the first time a value needs the room and
	then holds, until the pointer moves to another control. */
	float widest = 0.f;
	float tallest = 0.f;
	/** How far below the pointer's tip its tail reaches, in the scene's units. */
	float tailBelow = 24.f;

	static constexpr double FADE_IN = 0.5;
	static constexpr double FADE_OUT = 0.25;
	/** The margin between the letters and the frame, at Rack's own size. */
	static constexpr float PAD = 1.25f;

	void step() override {
		widget::Widget::step();
		tip = NULL;
		haveAnchor = false;
		if (APP->scene)
			box.size = APP->scene->box.size;
		if (!on || !*on || !APP->scene)
			return;
		for (widget::Widget* w : APP->scene->children) {
			if (ui::Tooltip* t = dynamic_cast<ui::Tooltip*>(w)) {
				if (t->isVisible())
					tip = t;
			}
		}
		const double now = APP->window ? APP->window->getFrameTime() : 0.0;
		if (tip != lastTip) {
			if (tip) {
				// A NEW ONE, even straight after another: each control's tooltip fades in, at its
				// own size.
				shownAt = now;
				hiddenAt = -1.0;
				widest = tallest = 0.f;
				// The arrow reaches about 16 points below its tip at the normal size. A scene unit
				// is a point unless Rack's own interface scale is changed.
				const float pointsPerUnit = (APP->window && APP->window->windowRatio > 0.f)
					? APP->window->pixelRatio / APP->window->windowRatio : 1.f;
				tailBelow = 16.f * pointerScale() / (pointsPerUnit > 0.f ? pointsPerUnit : 1.f);
				widestWhole.clear();
				widestFrac.clear();
				linePrefix.clear();
			}
			else {
				hiddenAt = now;
				alphaAtHide = lastAlpha;
			}
			lastTip = tip;
		}
		if (!tip)
			return;
		text = tip->text;
		// OFF THE SCREEN, where it goes on updating its text for us to read.
		tip->box.pos = math::Vec(-100000.f, -100000.f);
		// WHAT IT IS ABOUT: the widget under the pointer, in the scene's coordinates, which are
		// ours. The zoom is applied on the way up, so the box is the size it is drawn.
		widget::Widget* hovered = APP->event ? APP->event->hoveredWidget : NULL;
		if (hovered) {
			const math::Vec a = hovered->getAbsoluteOffset(math::Vec());
			const math::Vec b = hovered->getAbsoluteOffset(hovered->box.size);
			anchor = math::Rect(a, b.minus(a));
			haveAnchor = true;
		}
	}

	/** A LINE'S NUMBER, IN PARTS, so it can be held still while it changes.

	Rack formats a knob's value to a number of figures, not of decimal places, so turning it walks
	through 9.99, 10, 10.05; and a port's voltage is written with a space where a minus would go,
	so it changes width as the sign changes and when it gains a digit. Drawn as one string, the
	unit after the number — dB, V — jumped in and out with every step. Drawn in parts, the whole
	number is set right-aligned against the decimal point, the decimal places start from it, and
	the unit sits after the widest the number has been, so nothing moves back.

	WHICH NUMBER: the first with a decimal point in it, so a polyphonic port's "1: 0.123V" holds
	the voltage still rather than the channel; failing that, the first number standing on its own.
	The spaces and sign just before it go with the number, so a sign appearing does not push it. */
	struct LineParts {
		bool num = false;
		std::string prefix, whole, frac, suffix;
	};

	static LineParts splitLine(const std::string& line) {
		LineParts lp;
		lp.suffix = line;
		size_t start = std::string::npos, end = 0, dot = std::string::npos;
		for (int pass = 0; pass < 2 && start == std::string::npos; pass++) {
			for (size_t i = 0; i < line.size(); i++) {
				if (!std::isdigit((unsigned char) line[i]))
					continue;
				// A digit inside a word — IN1, 2x — is part of a name, not a value.
				if (i > 0 && std::isalpha((unsigned char) line[i - 1]))
					continue;
				size_t j = i;
				while (j < line.size() && std::isdigit((unsigned char) line[j]))
					j++;
				size_t d = std::string::npos;
				if (j + 1 < line.size() && line[j] == '.'
						&& std::isdigit((unsigned char) line[j + 1])) {
					d = j;
					j++;
					while (j < line.size() && std::isdigit((unsigned char) line[j]))
						j++;
				}
				if (pass == 0 && d == std::string::npos) {
					i = j;
					continue;
				}
				start = i;
				end = j;
				dot = d;
				break;
			}
		}
		if (start == std::string::npos)
			return lp;
		// The sign and the spaces before it belong with the number.
		size_t from = start;
		if (from > 0 && (line[from - 1] == '-' || line[from - 1] == '+'))
			from--;
		while (from > 0 && line[from - 1] == ' ')
			from--;
		lp.num = true;
		lp.prefix = line.substr(0, from);
		const size_t split = dot == std::string::npos ? end : dot;
		lp.whole = line.substr(from, split - from);
		lp.frac = line.substr(split, end - split);
		lp.suffix = line.substr(end);
		return lp;
	}

	/** Where everything goes inside the box, worked out when shown and kept for the fade out. */
	struct Line {
		LineParts parts;
		std::string text;
		float prefixW = 0.f, wholeW = 0.f, fracW = 0.f;
		float h = 0.f;
		/** The width a line of words was measured wrapping at, and so must be drawn wrapping at. */
		float wrapW = 0.f;
		/** Where the line's last row starts, down from the line's top. */
		float lastRowTop = 0.f;
	};
	std::vector<Line> lines;
	/** HOW FAR BELOW THE LINE'S TOP THE LETTERS BEGIN. Text set from the top of its line is set
	from the font's ascender, which leaves room above the capitals for accents; the box starts at
	the capitals instead. */
	float inkTop = 0.f;
	/** THE WIDEST EACH LINE'S NUMBER HAS BEEN, by line, for as long as the pointer stays on one
	control and the words before the number stay the same. */
	std::vector<float> widestWhole, widestFrac;
	std::vector<std::string> linePrefix;

	void draw(const DrawArgs& args) override {
		if (text.empty() || !APP->window)
			return;
		const double now = APP->window->getFrameTime();
		// GOING: the last box, fading out where it was.
		if (!tip) {
			if (hiddenAt < 0.0 || now - hiddenAt >= FADE_OUT || !on || !*on)
				return;
			const float a = alphaAtHide * (float) (1.0 - (now - hiddenAt) / FADE_OUT);
			paint(args, lastBox, a);
			return;
		}
		// AGAIN, for a tooltip after this one in the scene, whose step has put it back.
		tip->box.pos = math::Vec(-100000.f, -100000.f);
		std::shared_ptr<window::Font> font =
			APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font || font->handle < 0)
			return;
		const float scale = settingsTooltipScale();
		// RACK'S OWN SIZE, multiplied. Thirteen is the size Rack draws a tooltip at.
		const float size = 13.f * scale;
		// A THIN MARGIN, so the frame sits close around the words and covers as little as
		// possible of what is behind it.
		const float pad = PAD * scale;
		const float maxW = std::min(420.f * scale, box.size.x * 0.6f);

		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, size);
		nvgTextLineHeight(args.vg, 1.2f);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		float ascender = 0.f, descender = 0.f, fontLineH = 0.f;
		nvgTextMetrics(args.vg, &ascender, &descender, &fontLineH);
		// The capitals and the tall lower-case letters reach this far above the baseline, in
		// DejaVu Sans. In ems, not in the font size: the font is scaled so that its ascender and
		// descender together are the size, and DejaVu's span 1.164 ems.
		const float em = (ascender - descender) / 1.164f;
		const float capTop = 0.76f * em;
		// THE FOOT OF THE FRAME CROSSES THE TAILS of g, p and y, which reach 0.24 em below the
		// baseline, rather than leaving room under them that most lines never use. The margin is
		// taken off so the foot is here whatever the margin.
		const float tailBottom = 0.15f * em - pad;
		inkTop = std::max(0.f, ascender - capTop);

		// EVERY LINE IN TURN: one with a number in it is laid out in parts, and one without is
		// wrapped as Rack's own would be.
		lines.clear();
		const float lineH = size * 1.2f;
		float contentW = 0.f, contentH = 0.f;
		size_t at0 = 0;
		for (size_t n = 0; ; n++) {
			const size_t br = text.find('\n', at0);
			Line ln;
			ln.text = text.substr(at0, br == std::string::npos ? std::string::npos : br - at0);
			ln.parts = splitLine(ln.text);
			if (widestWhole.size() <= n) {
				widestWhole.resize(n + 1, 0.f);
				widestFrac.resize(n + 1, 0.f);
				linePrefix.resize(n + 1);
			}
			if (ln.parts.num) {
				// Different words before the number are a different line: start its measure again.
				if (linePrefix[n] != ln.parts.prefix) {
					linePrefix[n] = ln.parts.prefix;
					widestWhole[n] = widestFrac[n] = 0.f;
				}
				ln.prefixW = nvgTextBounds(args.vg, 0.f, 0.f, ln.parts.prefix.c_str(), NULL, NULL);
				widestWhole[n] = std::max(widestWhole[n],
					nvgTextBounds(args.vg, 0.f, 0.f, ln.parts.whole.c_str(), NULL, NULL));
				widestFrac[n] = std::max(widestFrac[n],
					nvgTextBounds(args.vg, 0.f, 0.f, ln.parts.frac.c_str(), NULL, NULL));
				ln.wholeW = widestWhole[n];
				ln.fracW = widestFrac[n];
				const float suffixW =
					nvgTextBounds(args.vg, 0.f, 0.f, ln.parts.suffix.c_str(), NULL, NULL);
				contentW = std::max(contentW, ln.prefixW + ln.wholeW + ln.fracW + suffixW);
				ln.h = lineH;
			}
			else if (!ln.text.empty()) {
				float bounds[4];
				nvgTextBoxBounds(args.vg, 0.f, 0.f, maxW, ln.text.c_str(), NULL, bounds);
				// From the left edge the words are drawn at, not from where the first glyph's ink
				// begins: the ink can start to the right of it, and a box that narrow was short by
				// that much.
				contentW = std::max(contentW, bounds[2]);
				ln.wrapW = maxW;
				ln.h = bounds[3] - bounds[1];
				// The measure runs to the last row's descender, a row's full height below its top.
				ln.lastRowTop = std::max(0.f, ln.h - (ascender - descender));
			}
			else {
				ln.h = lineH;
			}
			lines.push_back(ln);
			if (br == std::string::npos) {
				// THE LAST LINE ENDS AT ITS TAILS, not at the foot of its line.
				contentH += ln.lastRowTop + ascender + tailBottom - inkTop;
				break;
			}
			contentH += ln.h;
			at0 = br + 1;
		}
		widest = std::max(widest, contentW + 2.f * pad);
		tallest = std::max(tallest, contentH + 2.f * pad);
		const float w = widest, h = tallest;

		// CENTRED UNDER THE CONTROL and just below the tail of the pointer, which would otherwise
		// cover its first line. Under the pointer rather than under the control: a tall slider's
		// foot can be a long way below the hand, and the eye is where the pointer is.
		const math::Vec mouse = APP->scene->mousePos;
		// Slightly over the tail, so the box reads as belonging to the pointer.
		const float underTail = mouse.y + tailBelow - 1.5f;
		math::Vec at = math::Vec(mouse.x - w / 2.f, underTail);
		if (haveAnchor) {
			at.x = anchor.pos.x + anchor.size.x / 2.f - w / 2.f;
			at.y = underTail;
			// NO ROOM BELOW: directly above the control, never on top of it.
			if (at.y + h > box.size.y - 2.f)
				at.y = anchor.pos.y - h - 2.f;
		}
		at.x = math::clamp(at.x, 2.f, std::max(2.f, box.size.x - w - 2.f));
		at.y = math::clamp(at.y, 2.f, std::max(2.f, box.size.y - h - 2.f));

		lastBox = math::Rect(at, math::Vec(w, h));
		lastAlpha = (float) std::min(1.0, (now - shownAt) / FADE_IN);
		paint(args, lastBox, lastAlpha);
	}

	/** The box and its words, at `alpha`, from the layout last worked out — so the fade out
	draws exactly what was last on the screen. */
	void paint(const DrawArgs& args, math::Rect r, float alpha) {
		std::shared_ptr<window::Font> font =
			APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font || font->handle < 0 || alpha <= 0.f)
			return;
		const float scale = settingsTooltipScale();
		const bool classic = settingsTooltipClassic();
		const float pad = PAD * scale;
		nvgSave(args.vg);
		nvgGlobalAlpha(args.vg, alpha);
		nvgBeginPath(args.vg);
		// SLIGHTLY ROUNDED, with a thin light frame on the black, which marks the box off from a
		// dark panel behind it.
		nvgRoundedRect(args.vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 1.5f * scale);
		nvgFillColor(args.vg, classic ? nvgRGB(0xff, 0xff, 0xe1) : nvgRGB(0x00, 0x00, 0x00));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, classic ? nvgRGB(0x76, 0x76, 0x76) : nvgRGB(0xc8, 0xc8, 0xc8));
		// Half a unit, which is one pixel on a high-density screen.
		nvgStrokeWidth(args.vg, 0.5f);
		nvgStroke(args.vg);
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 13.f * scale);
		nvgTextLineHeight(args.vg, 1.2f);
		nvgFillColor(args.vg, classic ? nvgRGB(0x00, 0x00, 0x00) : nvgRGB(0xff, 0xff, 0xff));
		float y = r.pos.y + pad - inkTop;
		const float x = r.pos.x + pad;
		for (const Line& ln : lines) {
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			if (ln.parts.num) {
				nvgText(args.vg, x, y, ln.parts.prefix.c_str(), NULL);
				// The whole number against the decimal point, the decimals from it, and what
				// follows after the widest the number has been.
				const float point = x + ln.prefixW + ln.wholeW;
				nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
				nvgText(args.vg, point, y, ln.parts.whole.c_str(), NULL);
				nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
				nvgText(args.vg, point, y, ln.parts.frac.c_str(), NULL);
				nvgText(args.vg, point + ln.fracW, y, ln.parts.suffix.c_str(), NULL);
			}
			else if (!ln.text.empty()) {
				// WRAPPED WHERE IT WAS MEASURED. Wrapping at the box's own width broke a line the
				// measure had kept whole, and the extra line ran out of the bottom of the box.
				nvgTextBox(args.vg, x, y, ln.wrapW, ln.text.c_str(), NULL);
			}
			y += ln.h;
		}
		nvgRestore(args.vg);
	}
};

widget::Widget* createTooltipOverlay(bool* on) {
	TooltipReadabilityOverlay* o = new TooltipReadabilityOverlay;
	o->on = on;
	return o;
}
