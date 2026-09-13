/** In-rack help — see Help.hpp for what this is and why the text is written rather than derived. */
#include "Help.hpp"

#include <tag.hpp>
#include <ui/Menu.hpp>
#include <ui/MenuOverlay.hpp>
#include <ui/TextField.hpp>
#include <app/SvgScrew.hpp>

#include <algorithm>
#include <fstream>
#include <regex>
#include <cstdlib>
#include <cstdio>
#include <cstring>


// ---- the text ---------------------------------------------------------------------------------

std::vector<std::string> helpFor(const std::string& plugin, const std::string& model) {
	// BINARY SEARCH, because the table is meant to grow to the whole library and this runs on a
	// click. The generator writes it sorted by plugin then model, which is the order compared here.
	int lo = 0, hi = HELP_COUNT - 1;
	while (lo <= hi) {
		const int mid = (lo + hi) / 2;
		int c = std::strcmp(HELP[mid].plugin, plugin.c_str());
		if (c == 0)
			c = std::strcmp(HELP[mid].model, model.c_str());
		if (c == 0) {
			std::vector<std::string> out;
			for (int i = 0; i < HELP[mid].count; i++)
				out.push_back(HELP[mid].lines[i]);
			return out;
		}
		if (c < 0)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return std::vector<std::string>();
}


/** The entry for a module, or NULL. */
static const HelpEntry* helpEntryFor(const std::string& plugin, const std::string& model) {
	int lo = 0, hi = HELP_COUNT - 1;
	while (lo <= hi) {
		const int mid = (lo + hi) / 2;
		int c = std::strcmp(HELP[mid].plugin, plugin.c_str());
		if (c == 0)
			c = std::strcmp(HELP[mid].model, model.c_str());
		if (c == 0)
			return &HELP[mid];
		if (c < 0)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return NULL;
}

int helpFamilyFor(const std::string& plugin, const std::string& model,
		bool isOutput, int port) {
	const HelpEntry* e = helpEntryFor(plugin, model);
	if (!e || port < 0)
		return -1;
	const signed char* table = isOutput ? e->outFamilies : e->inFamilies;
	const int count = isOutput ? e->outFamilyCount : e->inFamilyCount;
	if (!table || port >= count)
		return -1;
	return table[port];
}

std::string helpPropsFor(const std::string& plugin, const std::string& model,
		bool isOutput, int port) {
	const HelpEntry* e = helpEntryFor(plugin, model);
	if (!e || port < 0)
		return "";
	const short* table = isOutput ? e->outProps : e->inProps;
	const int count = isOutput ? e->outPropCount : e->inPropCount;
	if (!table || port >= count)
		return "";
	const short at = table[port];
	if (at < 0 || at >= HELP_PROP_TEXT_COUNT)
		return "";
	return HELP_PROP_TEXT[at];
}

std::string helpForControl(const std::string& plugin, const std::string& model,
		HelpKind kind, int index) {
	const HelpEntry* e = helpEntryFor(plugin, model);
	if (!e || index < 0)
		return "";
	const short* table = NULL;
	int count = 0;
	switch (kind) {
		case HELP_INPUT:  table = e->inputs;  count = e->inputCount;  break;
		case HELP_OUTPUT: table = e->outputs; count = e->outputCount; break;
		case HELP_PARAM:  table = e->params;  count = e->paramCount;  break;
	}
	if (!table || index >= count)
		return "";
	const short line = table[index];
	if (line < 0 || line >= e->count)
		return "";
	return e->lines[line];
}

// ---- the panel --------------------------------------------------------------------------------

static const float HELP_PAD = 8.f;
static const float HELP_LEAD = 16.f;

/** SAYING IT OUT LOUD, BECAUSE NOTHING ELSE CAN.

Rack draws every pixel of its interface itself and publishes nothing to the accessibility API, so
there is no text object anywhere in its window for a screen reader to find — selected or not,
copied or not. A plugin cannot fix that from the inside. What it can do is read the text out
itself, which is what the demo system already does, with the same voice.

ONE AT A TIME. Opening a second module while the first is still being read stops the first: two
voices at once is worse than either.

Mac only. `say` is what is here, and this is a personal tool on a Mac; elsewhere the panel is
still there to be read with the eyes. */
static const char* HELP_VOICE = "Karen (Premium)";
static const int HELP_RATE = 198;

/** What a synthesiser needs, rather than what the panel shows.

Every one of these is a thing `say` gets wrong when read straight: it says the dash in "0-10V" as
a word, spells nothing out of "3HP", and reads "dB" as a syllable. The panel keeps the short
forms because they are what is printed on the module; this is a second copy for the voice. */
static std::string helpSpeech(std::string t) {
	// A MODIFIER SYMBOL IS SILENT. The panel says ⌘⇧ because a row has no room for the words;
	// `say` reads both as nothing, so the voice gets them back. Before everything else, so the
	// words that come out are then treated like any other words.
	t = std::regex_replace(t, std::regex("\u2325"), "option ");
	t = std::regex_replace(t, std::regex("\u2318"), "command ");
	t = std::regex_replace(t, std::regex("\u21E7"), "shift");
	// SYMBOLS A PANEL USES AND A VOICE CANNOT. Each was found by counting the non-ASCII
	// characters actually in the entries and reading them in context, rather than guessing at a
	// list: a degree sign in "a full 360 degree turn", cents in "100 cents", and the two arrows,
	// which mean different things — Befaco's menu prints "1 input ▸ 8 outputs", where the mark
	// is the word "to", while CountModula names a menu CHOICE with a bare arrow, where the
	// reader is looking for an arrow on screen and wants to hear that word.
	t = std::regex_replace(t, std::regex("\u00B1"), "plus or minus ");
	t = std::regex_replace(t, std::regex("\u00B0"), " degrees");
	t = std::regex_replace(t, std::regex("\u00A2"), " cents");
	t = std::regex_replace(t, std::regex("\u25B8"), " to ");
	t = std::regex_replace(t, std::regex("\u2192"), " arrow ");
	// A DIVISION SIGN, so a menu item can be quoted as it is printed. MindMeld's PatchMaster
	// prints "SR \u00f7 4 (default)", and the alternative was to write a slash and falsify the one
	// thing a menu line exists to get right.
	t = std::regex_replace(t, std::regex("\u00f7"), " divided by ");
	// A MIDDLE DOT SEPARATES THE FACTS A JACK EXPECTS — range, shape, polyphony. The eye reads
	// the gap; the voice needs a comma, or it runs the three together as one phrase.
	t = std::regex_replace(t, std::regex(" *· *"), ", ");
	// A PIPE IS A SEPARATOR, NOT A WORD. Menu items are quoted exactly as the menu prints them,
	// and some makers separate two names with a bar — MSM's "Espen's Treasure | Jedi". The eye
	// reads that as a break; the voice needs a comma.
	t = std::regex_replace(t, std::regex(" *\\| *"), ", ");
	// A bullet leads each menu item in a module's note; the voice does not need to say it.
	t = std::regex_replace(t, std::regex("\u2022 "), "");
	// An em dash is a pause, not a word. A leading bullet dash is not a word either.
	t = std::regex_replace(t, std::regex("\n- "), "\n");
	t = std::regex_replace(t, std::regex("—"), ",");
	// THE RANGE FIELD'S OWN SPELLING, which is not the one the lines use. A jack's properties are
	// generated as "1V per octave" — no slash — and the rule below only caught "1V/octave", so
	// this went to the voice as "one V per octave" and came out sounding like October. Both
	// spellings, because both are in the text.
	t = std::regex_replace(t, std::regex("1V per octave"), "one volt per octave");
	t = std::regex_replace(t, std::regex("1V/octave"), "one volt per octave");
	// "V/OCT" READ ALOUD IS "V OCTOBER", which is what the abbreviation deserves. The jacks that
	// name a pitch input in the same shorthand get the same treatment.
	t = std::regex_replace(t, std::regex("V/OCT"), "volts per octave");
	t = std::regex_replace(t, std::regex("V/O([0-9])"), "volts per octave $1");
	// A SLASH BETWEEN TWO LABELS IS A PAUSE, not the word "slash": FWD/REV, DRY/WET, RES/BW,
	// IN/SIDE, OFF/SM. Done twice, so a chain of three is caught as well.
	t = std::regex_replace(t, std::regex("([A-Z])/([A-Z])"), "$1 $2");
	t = std::regex_replace(t, std::regex("([A-Z])/([A-Z])"), "$1 $2");
	t = std::regex_replace(t, std::regex("([0-9]) *- *([0-9])"), "$1 to $2");
	t = std::regex_replace(t, std::regex("([0-9]) *HP"), "$1 H P");
	t = std::regex_replace(t, std::regex("([0-9]) *V\\b"), "$1 volts");
	t = std::regex_replace(t, std::regex("dB\\b"), " decibels");
	t = std::regex_replace(t, std::regex("(Hz|HZ|hz)\\b"), " hertz");
	t = std::regex_replace(t, std::regex("ms\\b"), " milliseconds");
	return t;
}

static bool gHelpSpeak = true;

void helpSetSpeak(bool on) {
	gHelpSpeak = on;
}

/** Whether `say` is running at this moment.

ASKED OF THE SYSTEM, because there is nothing to ask otherwise: `say` is a separate process with
no way to report back, and guessing from the length of the text would be a guess. One `pgrep` on
a click is nothing. */
static bool helpIsSpeaking() {
#if defined ARCH_MAC
	FILE* pipe = popen("/usr/bin/pgrep -x say >/dev/null 2>&1; echo $?", "r");
	if (!pipe)
		return false;
	char out[8] = {0};
	const bool read = fgets(out, sizeof(out), pipe) != NULL;
	pclose(pipe);
	return read && out[0] == '0';
#else
	return false;
#endif
}

static void helpSilence() {
#if defined ARCH_MAC
	std::system("/usr/bin/killall say >/dev/null 2>&1");
#endif
}

static void helpSay(const std::string& text) {
	// THE SWITCH ON THE MODULE SILENCES ALL OF IT, a clicked line included — somebody reading with
	// their eyes does not want a voice starting up because they touched a row.
	if (!gHelpSpeak)
		return;
#if defined ARCH_MAC
	const std::string path = system::getTempDirectory() + "/dreamer-help-speech.txt";
	std::ofstream file(path.c_str());
	if (!file)
		return;
	file << helpSpeech(text);
	file.close();
	// THROUGH A FILE RATHER THAN THE COMMAND LINE, so nothing in the text has to be escaped and
	// nothing in it can be read as a command. Detached, so the rack does not stop while it talks.
	std::string command = "/usr/bin/killall say >/dev/null 2>&1; /usr/bin/say -v \"";
	command += HELP_VOICE;
	command += "\" -r " + std::to_string(HELP_RATE) + " -f \"" + path + "\" >/dev/null 2>&1 &";
	std::system(command.c_str());
#else
	(void) text;
#endif
}

/** Opens the panel for one module at the pointer.

A MENU, DELIBERATELY. What was wanted is a floating panel that goes away when you click somewhere
else, stays inside the window, and looks like it belongs to Rack — which is a description of
Rack's own menu, so this is one, with a text field and a copy item in it rather than a list of
things to choose between. */
/** THE PANEL FOR ONE CONTROL, FLOATING BESIDE IT.

NOT A MENU, AND THAT IS THE WHOLE POINT. Rack's menus are modal: an overlay covers the window and
swallows the next click to dismiss itself. So the second click on a control never reached the
control, and clicking a thing twice — once to see it, again to hear it — was impossible. This is
an ordinary widget sitting on the rack beside the control, which takes a click only on itself.

It lives in the rack's own coordinates, so it sits beside the control at any zoom or scroll
position, and it is moved rather than recreated as one control after another is asked about. */
struct HelpPopup : widget::OpaqueWidget {
	std::string title;
	std::string line;
	bool missing = false;
	/** Enough to find this entry again in research/help: which maker, which model, and which
	control by kind and number. Carried for the copy button and nothing else. */
	std::string plugin;
	std::string model;
	std::string what;
	/** Set when the words are the maker's own rather than ours, so the note can say so. */
	bool fromMaker = false;
	/** WHETHER THE MAKER CALLS THE MODULE POLYPHONIC: 1 yes, 0 no, -1 they never say.

	A FLAG, BECAUSE IT IS THE ONE FACT THAT DECIDES WHETHER A PATCH IS POSSIBLE. The note says it
	in words as well, and the words are the ones that carry the attribution — but a person opening
	a note about a module is usually asking a different question, and having to read for this one
	is a poor way to answer "can I put sixteen voices through it". Colour answers that before the
	sentence is read, and the sentence is still there to be read, so nothing rests on colour
	alone. -1 draws nothing at all: a maker who never uses the tag has not said no. */
	int poly = -1;
	/** WHERE IT BELONGS, IN THE RACK'S OWN COORDINATES.
	
	The note is drawn on the SCENE rather than in the rack — see helpCatcherStep for why — so its
	own box is in window coordinates and has to be recomputed whenever the rack is scrolled or
	zoomed. This is the anchor it is placed against: the control's box, in rack space. */
	math::Rect anchor;
	/** WHERE THE POINTER WAS WHEN IT WAS ASKED, in the rack's own coordinates.

	The note is centred above this rather than beside the control, so that it never lands where
	Rack draws its own tooltip — which appears below and to the right of the cursor. Kept in rack
	coordinates, not window ones, so the note stays with the control when the rack is scrolled or
	zoomed under it. */
	math::Vec pointer;
	/** When the copy was last taken, so the button can show that it worked. */
	double copiedAt = -1.0;
	/** WHERE EACH PARAGRAPH ENDED UP, so a click can be answered with the one it landed on.

	A module's note is several paragraphs now — what it is, then anything true of the whole
	module rather than of a control. Read as one block it is a long listen with no way to skip,
	and the reader has no way to ask for the part they wanted. So the vertical span of each is
	recorded as it is drawn, and a click picks the paragraph it fell in. Filled by measure(),
	which is the only thing that knows where the text wrapped. */
	std::vector<float> paraTop;
	std::vector<float> paraBottom;

	/** The note broken at its blank lines. One paragraph for an ordinary control note; for a
	module, its first line and then each Note or Menu line. */
	std::vector<std::string> paragraphs() const {
		std::vector<std::string> out;
		size_t at = 0;
		while (at <= line.size()) {
			const size_t br = line.find("\n\n", at);
			out.push_back(line.substr(at, br == std::string::npos ? std::string::npos : br - at));
			if (br == std::string::npos)
				break;
			at = br + 2;
		}
		return out;
	}

	/** Where the pointer is, in this widget's coordinates, or below everything when it is away. */
	float hoverY = -1.f;

	void onHover(const HoverEvent& e) override {
		hoverY = e.pos.y;
		widget::OpaqueWidget::onHover(e);
	}

	void onLeave(const LeaveEvent& e) override {
		hoverY = -1.f;
		widget::OpaqueWidget::onLeave(e);
	}

	static float iconSize() { return 13.f; }

	/** The copy button, in this widget's own coordinates. */
	math::Rect iconBox() {
		return math::Rect(math::Vec(box.size.x - iconSize() - 6.f, 6.f),
			math::Vec(iconSize(), iconSize()));
	}

	/** WHAT LANDS ON THE CLIPBOARD: the note as read, and where it came from.

	The point of the button is to be able to say "this one is unclear" without typing out which
	one. So it carries the module, the control, the line itself, and the slugs and index that
	identify the entry in the source files — which is what makes the answer actionable rather
	than a search. */
	std::string forClipboard() {
		std::string out = title + "\n" + line + "\n";
		if (!plugin.empty())
			out += "[" + plugin + " / " + model + (what.empty() ? "" : " — " + what) + "]\n";
		return out;
	}

	void copyToClipboard() {
		const std::string text = forClipboard();
		glfwSetClipboardString(APP->window->win, text.c_str());
		copiedAt = system::getTime();
	}

	/** WHAT IS BEING TALKED ABOUT, BEFORE WHAT IS SAID ABOUT IT.

	On screen the title sits above the text and the eye takes both in at once. A voice has no
	above: it starts in the middle of a sentence about a thing it never named, and somebody
	listening to a jack's description has no way to tell which jack answered. So the name goes
	first, with a full stop after it, which is the pause `say` gives a sentence end.

	ONLY ON THE FIRST PARAGRAPH. A module's note is a dozen points and each is clicked on its own;
	repeating the module's name before every one of them would be the padding this whole project
	strips out of the lines themselves. The first paragraph — what the module is — is the one that
	needs the name, and the rest are plainly still about it. */
	std::string withTitle(bool first, const std::string& text) const {
		if (!first || title.empty())
			return text;
		return title + ". " + text;
	}

	/** Capitals, for the title row only.

	ASCII LETTERS AND NOTHING ELSE, deliberately. A byte-wise toupper across UTF-8 would corrupt
	the continuation bytes of any accented letter and put rubbish on the panel; module names such
	as Vult's Ferox and NYSTHI's Sussudio are plain ASCII, but a name from any maker may not be,
	and a title is not the place to find out. A letter this leaves alone is simply not capitalised,
	which is a good outcome rather than a broken one. */
	static std::string helpUpper(const std::string& s) {
		std::string out = s;
		for (size_t i = 0; i < out.size(); i++) {
			const unsigned char c = (unsigned char) out[i];
			if (c >= 'a' && c <= 'z')
				out[i] = (char) (c - 'a' + 'A');
		}
		return out;
	}

	float measure(NVGcontext* vg, bool drawing, const DrawArgs* args) {
		std::shared_ptr<window::Font> font =
			APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font || font->handle < 0)
			return 40.f;
		const float w = box.size.x;
		float y = HELP_PAD;

		nvgFontFaceId(vg, font->handle);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

		// THE NAME OF THE THING, AND ONLY IT. Larger, bold and in capitals: the note's one
		// heading, above a list whose own headings stay the size of the text they head. Caps get
		// a little tracking, because letters set in caps at their natural spacing crowd.
		nvgFontSize(vg, 15.f);
		if (drawing) {
			const std::string caps = helpUpper(title);
			nvgFillColor(args->vg, nvgRGB(0x7f, 0xb0, 0xe4));
			nvgTextLetterSpacing(args->vg, 0.6f);
			// Struck twice, a third of a pixel apart: no bold cut of this face ships with Rack,
			// and the one bold face in the bundle is a different typeface.
			nvgText(args->vg, HELP_PAD, y, caps.c_str(), NULL);
			const float after = nvgText(args->vg, HELP_PAD + 0.35f, y, caps.c_str(), NULL);
			// POLY OR MONO AS PART OF THE NAME, which is where somebody reading the title is
			// already looking. It was a pill on the right for a while; a bracket after the name
			// is read in the same glance as the name, and needs no shape to be learned. The
			// colour stays, so it is still answerable without reading, and the word stays, so
			// nothing rests on the colour.
			if (poly >= 0) {
				const char* word = poly ? "(POLY)" : "(MONO)";
				nvgFillColor(args->vg, poly ? nvgRGB(0x5f, 0xc8, 0x8b)
					: nvgRGB(0x87, 0x90, 0x9d));
				nvgText(args->vg, after + 5.f, y, word, NULL);
				nvgText(args->vg, after + 5.35f, y, word, NULL);
			}
			nvgTextLetterSpacing(args->vg, 0.f);
		}
		y += 20.f;

		nvgFontSize(vg, 12.f);
		nvgTextLineHeight(vg, HELP_LEAD / 12.f);
		paraTop.clear();
		paraBottom.clear();
		const std::vector<std::string> paras = paragraphs();
		for (size_t i = 0; i < paras.size(); i++) {
			// A POINT LOOKS LIKE A POINT. A module's note is a dozen separate facts, and run
			// together as plain paragraphs there is nothing for the eye to catch on: finding the
			// one you want means reading all of them. A bullet and a hanging indent give each
			// point an edge to scan down, which is the whole difference between a list and a
			// wall. The mark is put on by whoever assembles the note, so a one-fact note about a
			// single knob does not get a bullet it has no list to belong to.
			const bool bullet = paras[i].rfind("• ", 0) == 0;
			// A HEADING, which is any point that ends in a colon: "Right-click the panel for:".
			// It announces the points under it and is not one of them, so it is not bulleted and
			// it is set in bold.
			const bool heading = !bullet && !paras[i].empty()
				&& paras[i][paras[i].size() - 1] == ':';
			const std::string body = bullet ? paras[i].substr(std::strlen("• ")) : paras[i];
			const float x = bullet ? HELP_PAD + 9.f : HELP_PAD;
			const float tw = w - x - HELP_PAD;
			float bounds[4];
			nvgTextBoxBounds(vg, x, y, tw, body.c_str(), NULL, bounds);
			const float h = std::max(bounds[3] - bounds[1], HELP_LEAD);
			paraTop.push_back(y);
			paraBottom.push_back(y + h);
			if (drawing) {
				// The one under the pointer is lit, so it is plain that a paragraph is a thing
				// you can click rather than a wall of text.
				const bool hot = paras.size() > 1 && APP->event
					&& APP->event->hoveredWidget == this
					&& hoverY >= y && hoverY < y + h;
				nvgFillColor(args->vg, missing ? nvgRGB(0x8a, 0x92, 0x9e)
					: heading ? nvgRGB(0x9d, 0xc4, 0xf0)
					: hot ? nvgRGB(0xff, 0xff, 0xff) : nvgRGB(0xe4, 0xe8, 0xee));
				if (bullet)
					nvgText(args->vg, HELP_PAD, y, "•", NULL);
				nvgTextBox(args->vg, x, y, tw, body.c_str(), NULL);
				// BOLD WITHOUT A BOLD FONT. Rack ships DejaVuSans and no bold cut of it, and the
				// one bold face it does carry is Nunito — a different typeface, which at eleven
				// points reads as a mistake rather than as emphasis. Striking the same letters
				// twice a third of a pixel apart thickens the stems and keeps the face.
				if (heading)
					nvgTextBox(args->vg, x + 0.35f, y, tw, body.c_str(), NULL);
			}
			y += h;
			if (i + 1 < paras.size())
				y += heading ? 3.f : 6.f;
		}

		// WHOSE WORDS THESE ARE. Only where they are not ours: an entry we wrote needs no
		// attribution, and a note that says something on every reading says nothing.
		if (fromMaker) {
			y += 3.f;
			nvgFontSize(vg, 10.f);
			if (drawing) {
				nvgFillColor(args->vg, nvgRGB(0x7f, 0x86, 0x92));
				nvgText(args->vg, HELP_PAD, y, "the maker's own description", NULL);
			}
			y += 13.f;
		}
		return y + HELP_PAD;
	}

	void step() override {
		widget::OpaqueWidget::step();
		if (APP->window && APP->window->vg)
			box.size.y = measure(APP->window->vg, false, NULL);
	}

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.f, 0.f, box.size.x, box.size.y, 4.f);
		nvgFillColor(args.vg, nvgRGBA(0x16, 0x1a, 0x20, 0xf4));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGBA(0x5f, 0x9d, 0xd8, 0xc0));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
		measure(args.vg, true, &args);
		drawCopy(args);
	}

	/** TWO OVERLAPPING SHEETS, which is what a copy button looks like everywhere else. Drawn
	rather than loaded, because it is nine lines of nanovg and no file to ship. */
	void drawCopy(const DrawArgs& args) {
		const math::Rect r = iconBox();
		const bool done = copiedAt > 0.0 && system::getTime() - copiedAt < 1.2;
		const NVGcolor ink = done ? nvgRGB(0x7d, 0xe0, 0xa0) : nvgRGBA(0x9f, 0xc8, 0xf0, 0xc0);
		const float w = r.size.x * 0.62f, h = r.size.y * 0.72f;

		// The sheet behind, offset up and to the right.
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, r.pos.x + r.size.x - w, r.pos.y, w, h, 1.5f);
		nvgStrokeColor(args.vg, ink);
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		// The sheet in front, over the bottom-left of it, filled with the panel's own ground so
		// the line behind it stops where it is covered.
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, r.pos.x, r.pos.y + r.size.y - h, w, h, 1.5f);
		nvgFillColor(args.vg, nvgRGBA(0x16, 0x1a, 0x20, 0xff));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, ink);
		nvgStroke(args.vg);
	}

	/** A click on the panel itself reads it out, for when the mouse is already there. */
	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			// BOTH, and not just the first: consuming records the target, stopping propagation is
			// what keeps anything underneath from consuming afterwards and becoming the target
			// instead. Overriding onButton without calling the base means doing this by hand.
			e.consume(this);
			e.stopPropagating();
			// The copy button first: it sits inside the panel, so the panel's own click would
			// otherwise take it and start talking.
			if (iconBox().contains(e.pos)) {
				copyToClipboard();
				return;
			}
			// A CLICK WHILE IT IS TALKING IS A REQUEST TO STOP. Somebody who has heard enough
			// reaches for the thing that is talking, and the alternative — starting it again from
			// the top — is the opposite of what they wanted.
			if (helpIsSpeaking()) {
				helpSilence();
				return;
			}
			if (missing)
				return;
			// THE PARAGRAPH THAT WAS CLICKED, not the whole note. A module with eighteen menu
			// options read from the top is not an answer to anything.
			const std::vector<std::string> paras = paragraphs();
			for (size_t i = 0; i < paras.size() && i < paraTop.size(); i++) {
				if (e.pos.y >= paraTop[i] && e.pos.y < paraBottom[i]) {
					helpSay(withTitle(i == 0, paras[i]));
					return;
				}
			}
			helpSay(withTitle(true, line));
			return;
		}
		widget::OpaqueWidget::onButton(e);
	}
};

static HelpPopup* gPopup = NULL;

static void helpPopupHide() {
	if (gPopup)
		gPopup->hide();
}

/** PLACES THE NOTE BESIDE ITS CONTROL, converting the rack to the window.

The rack is inside a scrolling, zooming container, so a point on a panel and a point on the
screen are different things: scene = the rack's own origin on screen, plus the rack-space point
times the rack's zoom. That is the same arithmetic our carried widgets use in reverse, in
Clip.hpp.

The note keeps a constant size at any zoom, which is what you want of something being read.

BESIDE, NEVER OVER. To the right of the control where there is room and to the left where there
is not, so it never covers the thing being asked about. */
static void helpPopupPlace() {
	if (!gPopup || !gPopup->isVisible() || !APP->scene || !APP->scene->rack)
		return;
	widget::Widget* rack = APP->scene->rack;
	const float zoom = rack->getAbsoluteZoom();
	const math::Vec origin = rack->getAbsoluteOffset(math::Vec(0.f, 0.f));
	// CENTRED ABOVE THE POINTER. Rack puts its own tooltip below and to the right of the cursor,
	// so anything drawn there fights it; above is the one side that is always free. Far enough
	// up to clear the cursor itself.
	static const float GAP = 18.f;
	const math::Vec p = origin.plus(gPopup->pointer.mult(zoom));

	float x = p.x - gPopup->box.size.x / 2.f;
	float y = p.y - gPopup->box.size.y - GAP;
	// Near the top of the window there is no room above, so it goes below — which is where the
	// tooltip is, but a note the reader cannot see at all is worse than one they have to move off.
	if (y < 0.f)
		y = p.y + GAP;
	const float maxX = std::max(0.f, APP->scene->box.size.x - gPopup->box.size.x);
	const float maxY = std::max(0.f, APP->scene->box.size.y - gPopup->box.size.y);
	gPopup->box.pos = math::Vec(math::clamp(x, 0.f, maxX), math::clamp(y, 0.f, maxY));
}

/** The text as this machine should read it.

ONE PHRASE, AND IT IS OURS. Only our own entries name the gesture, and they name it the Mac way
because that is where they were written, in the symbols the Mac panel uses. Rather than keep two copies of a line, the one phrase is
swapped on the way to the screen — which is also the way to the voice, since the spoken copy is
computed from this. Narrow on purpose: a maker who writes "cmd" about something else of their own
is left alone, because only the exact gesture is matched. */
static std::string helpPlatformText(std::string t) {
#if !defined ARCH_MAC
	const std::string from = "\u2325\u21E7";
	const std::string to = "Alt+Shift";
	for (size_t at = t.find(from); at != std::string::npos; at = t.find(from, at + to.size()))
		t.replace(at, from.size(), to);
#endif
	return t;
}

/** Shows the note for one control, anchored to the control's box in rack coordinates. */
/** What the next note should flag, set just before it is shown. A parameter would have to be
threaded through six call sites that have nothing to do with polyphony. */
static int gPolyFlag = -1;

static void helpPopupShow(app::ModuleWidget* mw, math::Rect controlBox,
		const std::string& title, const std::string& line, bool missing,
		const std::string& what = "", bool fromMaker = false) {
	if (!gPopup)
		return;
	gPopup->plugin = mw->model && mw->model->plugin ? mw->model->plugin->slug : "";
	gPopup->model = mw->model ? mw->model->slug : "";
	gPopup->what = what;
	gPopup->fromMaker = fromMaker;
	gPopup->copiedAt = -1.0;
	gPopup->pointer = APP->scene && APP->scene->rack
		? APP->scene->rack->getMousePos() : math::Vec();
	// The flag belongs to the module note, so every other note clears it rather than inheriting
	// whatever the last one showed.
	gPopup->poly = gPolyFlag;
	gPolyFlag = -1;
	gPopup->title = title;
	gPopup->line = line.empty()
		? "Nothing here describes this one yet."
		: helpPlatformText(line);
	gPopup->missing = line.empty();
	gPopup->box.size.x = 290.f;
	if (APP->window && APP->window->vg)
		gPopup->box.size.y = gPopup->measure(APP->window->vg, false, NULL);
	gPopup->show();

	// The control's box is in the module's coordinates; the anchor is in the rack's.
	gPopup->anchor = math::Rect(mw->box.pos.plus(controlBox.pos), controlBox.size);
	helpPopupPlace();
}

/** WHAT IS UNDER THE POINTER, asked of the module rather than of the widget tree.

The obvious way — let the click fall through and see which widget takes it — cannot work: the
answer has to be known BEFORE deciding whether to consume, and a knob that has taken a click has
already started being turned. So the module's own lists of ports and parameters are walked and
their boxes tested, which is the same test Rack would apply and costs nothing on a click. */
static bool helpControlAt(app::ModuleWidget* mw, math::Vec pos, std::string& what,
		std::string& line, math::Rect& where, HelpKind& kind, int& index) {
	if (!mw->model)
		return false;
	const std::string plugin = mw->model->plugin ? mw->model->plugin->slug : "";
	const std::string model = mw->model->slug;

	// THE SMALLEST CONTROL UNDER THE POINTER WINS, not the first one found.
	//
	// TWO REASONS, both from real panels. A HIDDEN widget must never answer: a maker may build
	// two knobs at one spot and show whichever the mode calls for — StochasticTelegraph's
	// Fixation does exactly that with its LENGTH and note-length knobs — and the hidden one
	// would otherwise answer for the visible one every time.
	//
	// And CONCENTRIC controls must resolve to the one actually pointed at. PinkTrombone puts a
	// small attenuverter at the centre of a large knob, and Blamsoft does the same; both are
	// visible, both contain the click, and returning the first in the widget list answers about
	// whichever the maker happened to add first. Area is what tells them apart: the small knob
	// is wholly inside the large one, so the smaller box is the more specific answer, and on a
	// panel where nothing overlaps it changes nothing.
	float bestArea = 0.f;
	bool found = false;
	auto take = [&](math::Rect box) {
		const float area = box.size.x * box.size.y;
		if (found && area >= bestArea)
			return false;
		bestArea = area;
		found = true;
		return true;
	};

	for (app::PortWidget* p : mw->getInputs()) {
		if (!p->isVisible() || !p->box.contains(pos) || !take(p->box))
			continue;
		what = "input " + std::to_string(p->portId + 1);
		if (p->module) {
			const std::string named = p->module->getInputInfo(p->portId)
				? p->module->getInputInfo(p->portId)->getName() : "";
			if (!named.empty() && named[0] != '#')
				what = named + " input";
		}
		line = helpForControl(plugin, model, HELP_INPUT, p->portId);
		where = p->box;
		kind = HELP_INPUT;
		index = p->portId;
	}
	for (app::PortWidget* p : mw->getOutputs()) {
		if (!p->isVisible() || !p->box.contains(pos) || !take(p->box))
			continue;
		what = "output " + std::to_string(p->portId + 1);
		if (p->module) {
			const std::string named = p->module->getOutputInfo(p->portId)
				? p->module->getOutputInfo(p->portId)->getName() : "";
			if (!named.empty() && named[0] != '#')
				what = named + " output";
		}
		line = helpForControl(plugin, model, HELP_OUTPUT, p->portId);
		where = p->box;
		kind = HELP_OUTPUT;
		index = p->portId;
	}
	for (app::ParamWidget* p : mw->getParams()) {
		if (!p->isVisible() || !p->box.contains(pos) || !take(p->box))
			continue;
		what = "control " + std::to_string(p->paramId + 1);
		if (p->getParamQuantity() && !p->getParamQuantity()->name.empty())
			what = p->getParamQuantity()->name;
		line = helpForControl(plugin, model, HELP_PARAM, p->paramId);
		where = p->box;
		kind = HELP_PARAM;
		index = p->paramId;
	}
	return found;
}

// ---- catching the click -----------------------------------------------------------------------

/** CMD-SHIFT-CLICK A JACK OR A KNOB.

NO MODE, NO BADGE, NOTHING ADDED TO ANYBODY'S PANEL. Cmd-shift-click any control and its note
appears beside it; the same on bare panel gives what the module is. Alt on Windows and Linux is
Cmd on a Mac — RACK_MOD_CTRL is the platform's own modifier, so one test covers all three.

WHY NOT OPTION, WHICH WAS TRIED AND SHIPPED BRIEFLY. Rack's own ScrollWidget takes alt-click
BEFORE its children and consumes it, with the comment "most widgets consume Alt-click without
needing to" — alt-drag is how the rack is panned. So an alt-click never reaches a module at all,
whatever is listening. That, and not the selected-module problem, is what defeated it.

WHY CMD-SHIFT AND NOT CMD ALONE. Cmd-drag from a jack CREATES a cable and Cmd-drag on a knob is
fine adjust at a tenth speed; taking Cmd-click would sit on top of both. Cmd-shift-drag clones a
cable, but a Cmd-shift CLICK — pressed and released without travel — does nothing in Rack at all.
So this claims the one gesture that was going spare.

AND IT WAITS FOR THE RELEASE, OVER A JACK. Whether a press is a click or the start of a drag is
not knowable when it arrives, so over a jack the press is let through and the decision made on
release, once the travel is known. Under a few pixels is a click and the note appears; anything
more was a drag and Rack's clone has it. Over a knob or bare panel nothing is at stake, so those
answer on the press and feel immediate. */
static bool gHelpOn = true;

bool helpModeOn() {
	return gHelpOn;
}

/** WHAT THE MAKER CALLS IT, ASKED OF THE MODULE IN THE RACK.

Rack's tooltip is exactly this text: the name a maker passed to configInput, configParam or
configOutput, and the second line they may have added after it. Two thirds of the controls in an
installed library carry one, and for the modules nobody has written an entry for it is the only
description that exists — NYSTHI's Bitshifter names a jack "Pulse in to switch between RND or VCO
generators", which is a better line than silence by a distance.

SHOWN AS THEIRS, NOT OURS. It does not follow the rules the written entries follow: it names the
control, it says where things are, it is written to be read rather than heard. So the note marks
it as the maker's own words, and nobody is misled about which they are hearing.

The raw `name` field rather than getName(), which returns "#3" for an unnamed port and would give
the note something meaningless to say. */
static std::string helpMakerText(app::ModuleWidget* mw, HelpKind kind, int index) {
	if (!mw || !mw->module || index < 0)
		return "";
	engine::Module* m = mw->module;
	std::string name, desc;
	if (kind == HELP_INPUT && index < (int) m->inputInfos.size()) {
		if (engine::PortInfo* i = m->inputInfos[index]) {
			name = i->name;
			desc = i->description;
		}
	}
	else if (kind == HELP_OUTPUT && index < (int) m->outputInfos.size()) {
		if (engine::PortInfo* i = m->outputInfos[index]) {
			name = i->name;
			desc = i->description;
		}
	}
	else if (kind == HELP_PARAM && index < (int) m->paramQuantities.size()) {
		if (engine::ParamQuantity* q = m->paramQuantities[index]) {
			name = q->name;
			desc = q->description;
		}
	}
	if (name.empty() && desc.empty())
		return "";
	if (name.empty())
		return desc;
	if (desc.empty())
		return name;
	return name + ". " + desc;
}

/** WHETHER THE MAKER CALLS THIS MODULE POLYPHONIC, WHICH ONLY THE MAKER CAN SAY.

The question the Rack forum keeps asking about a jack — will it take sixteen channels — has no
answer anywhere in Rack's interface, and for most modules no answer in the manual either. But
every maker fills in a list of tags for the module browser, and one of the tags is Polyphonic.
That is a declaration, in their own words, and Rack has already loaded it: it is on the Model,
so nothing has to be scanned, stored or kept in step, and it is true of the build installed rather
than of whatever is on somebody's main branch.

IT IS ABOUT THE MODULE, NOT THE JACK, so it goes on the title band with the other things that are
true of the whole module. It does not say which inputs take polyphony — that stays a blank on the
ports until somebody establishes it — but "this module handles polyphony at all" is most of what
somebody wants to know before they patch a sixteen-channel cable into it.

AND SILENCE ONLY MEANS SOMETHING WHERE THE MAKER USES TAGS. Thirty-eight of the plugins here never
apply Polyphonic to anything, so an untagged module in one of those is not a monophonic module —
it is a maker who does not use the tag. Saying "the maker does not list this as polyphonic" there
would be inventing a statement nobody made. So the plugin is asked first whether it uses the tag
at all, and where it does not, this says nothing. */
/** WHAT WE ESTABLISHED OURSELVES, WHICH OUTRANKS A TAG.

A tag is one word about a whole module, written by a maker who may have meant it about the outputs
— Bogaudio's UNISON is tagged polyphonic and every one of its inputs reads channel one only,
because it is a mono-to-poly voicer. The port fields say which jacks actually take a polyphonic
cable, each cited to a file and a line, so where they exist they are the better answer.

They also reach where a tag cannot. Thirty-eight plugins never use the tag at all — Instruo,
Bidoo, JW-Modules and dBiz among them, 423 modules — and for those the tag can say nothing,
whereas a port that was read in the maker's source says as much as any other.

Returns 1 if any input was found to take polyphony, 0 if every input we settled does not, and -1
if nothing about this module's inputs has been established. */
static int helpPolyphonyFound(plugin::Model* model) {
	if (!model || !model->plugin)
		return -1;
	const HelpEntry* e = helpEntryFor(model->plugin->slug, model->slug);
	if (!e || !e->inProps)
		return -1;
	bool anyKnown = false;
	for (int i = 0; i < e->inPropCount; i++) {
		const short at = e->inProps[i];
		if (at < 0 || at >= HELP_PROP_TEXT_COUNT)
			continue;
		const std::string phrase = HELP_PROP_TEXT[at];
		if (phrase.find("polyphonic") != std::string::npos)
			return 1;
		if (phrase.find("one channel only") != std::string::npos)
			anyKnown = true;
	}
	return anyKnown ? 0 : -1;
}

static int helpPolyphonyFlag(plugin::Model* model) {
	const int found = helpPolyphonyFound(model);
	if (found >= 0)
		return found;
	if (!model || !model->plugin)
		return -1;
	const int want = tag::findId("Polyphonic");
	if (want < 0)
		return -1;
	for (int id : model->tagIds) {
		if (id == want)
			return 1;
	}
	for (plugin::Model* other : model->plugin->models) {
		if (!other)
			continue;
		for (int id : other->tagIds) {
			if (id == want)
				return 0;
		}
	}
	return -1;
}

static std::string helpPolyphony(plugin::Model* model) {
	// WHOSE STATEMENT IT IS, SAID IN THE SENTENCE. A fact read out of the maker's source is ours
	// and is about the jacks; a tag is theirs and is about the module. They are not the same claim
	// and the note does not pretend they are.
	switch (helpPolyphonyFound(model)) {
		case 1:  return "Inputs on this module take a polyphonic cable — "
			"option-click a jack to see which.";
		case 0:  return "Every input on this module reads one channel only.";
		default: break;
	}
	switch (helpPolyphonyFlag(model)) {
		case 1:  return "The maker lists this module as polyphonic.";
		case 0:  return "The maker lists other modules in this plugin as polyphonic "
			"and not this one.";
		default: return "";
	}
}

/** Whether this point in a module is one of its jacks.

Only jacks matter: they are the controls Rack might start a cable drag from, so they are the ones
whose press has to be left alone until the release settles what it was. */
static bool helpPortAt(app::ModuleWidget* mw, math::Vec pos) {
	for (app::PortWidget* p : mw->getInputs()) {
		if (p->box.contains(pos))
			return true;
	}
	for (app::PortWidget* p : mw->getOutputs()) {
		if (p->box.contains(pos))
			return true;
	}
	return false;
}

/** TAKING A CLICK, WHICH IS TWO THINGS AND NOT ONE.

Consuming an event only records which widget is to be treated as its target — it does NOT stop
the event being offered to everything else. Rack walks the rest of the children afterwards, and
the LAST widget to consume becomes the target. So a click taken here and then taken again by a
jack underneath belongs to the jack, and Rack starts dragging that jack's cable: exactly the
symptom, with this widget doing its half correctly the whole time.

Propagation has to be stopped as well, which is precisely what OpaqueWidget does and why this is
not one — an OpaqueWidget here would swallow every click in the rack, not the ones this is
about. */
static void helpTake(const widget::Widget::ButtonEvent& e, widget::Widget* by) {
	e.consume(by);
	e.stopPropagating();
}

/** THE TITLE BAND ACROSS THE TOP OF A MODULE, where nearly every maker puts its name.

Where the title is cannot be asked — a maker draws it wherever they like, and Rack's own SVG
renderer has no text at all, so a panel's name is either outlines or drawn in the maker's own
code. This is the top of the panel, which is where it nearly always is. */
static float titleBand() { return 40.f; }

/** Shows the note for whatever is at this point in the module's own coordinates, or puts the
note away where there is nothing to say. */
static void helpAnswer(app::ModuleWidget* mw, math::Vec local) {
	std::string what, line;
	math::Rect where;
	HelpKind kind = HELP_PARAM;
	int index = -1;
	if (helpControlAt(mw, local, what, line, where, kind, index)) {
		// WHAT TO SEND IT, UNDER WHAT IT IS FOR. Rack tells nobody what voltage a jack wants,
		// whether the signal is continuous or stepped, or whether it takes polyphony, and the
		// forum answers those with a scope and a test rig. Where we know, it goes on its own
		// paragraph so it can be clicked and heard on its own.
		if (kind == HELP_INPUT || kind == HELP_OUTPUT) {
			const std::string plugin = mw->model->plugin ? mw->model->plugin->slug : "";
			const std::string props = helpPropsFor(plugin, mw->model->slug,
				kind == HELP_OUTPUT, index);
			if (!props.empty())
				line += (line.empty() ? "" : "\n\n") + props;
		}
		if (!line.empty()) {
			helpPopupShow(mw, where, what, line, false, what);
			return;
		}
		// NOTHING WRITTEN FOR THIS ONE: ask the module itself. Better the maker's own words,
		// marked as theirs, than telling somebody nobody has got round to it.
		const std::string maker = helpMakerText(mw, kind, index);
		if (!maker.empty()) {
			helpPopupShow(mw, where, what, maker, false, what, true);
			return;
		}
		helpPopupShow(mw, where, what, "", true, what);
		return;
	}
	// THE TITLE IS THE MODULE ITSELF: what the thing is, which is the first line of its
	// entry, not a list of everything on it.
	if (local.y < titleBand()) {
		const std::string plugin = mw->model->plugin ? mw->model->plugin->slug : "";
		const std::vector<std::string> lines = helpFor(plugin, mw->model->slug);
		// THE MODULE'S OWN LINES, ALL OF THEM, and until now only the first was reachable.
		//
		// An entry's first line says what the module is. Everything after it describes one control
		// and is reached by clicking that control — except the lines belonging to no control:
		// `Note —` for something that changes how the module is used, and `Menu —` for a setting
		// with no knob. There are thousands of those and no click arrived at any of them.
		//
		// THE PREFIXES ARE FOR THE AUTHOR, NOT THE READER. `Note —` says nothing to somebody
		// seeing it for the first time, and `Menu —` is worse: it names a menu without saying
		// which, and Rack has four — the module's, a knob's, a port's, and whatever a display
		// carries. So the prefixes come off, and the menu settings are gathered under one heading
		// that says where to find them, as points beneath it.
		std::string idea = lines.empty() ? "" : lines[0];
		std::string menu;
		for (size_t i = 1; i < lines.size(); i++) {
			// rfind at 0 is a prefix test that needs no length: the em dash is three bytes in
			// UTF-8, so counting characters here would be counting the wrong thing.
			if (lines[i].rfind("Note \u2014 ", 0) == 0)
				idea += "\n\n\u2022 " + lines[i].substr(std::strlen("Note \u2014 "));
			else if (lines[i].rfind("Menu \u2014 ", 0) == 0)
				menu += "\n\n\u2022 " + lines[i].substr(std::strlen("Menu \u2014 "));
		}
		gPolyFlag = helpPolyphonyFlag(mw->model);
		const std::string poly = helpPolyphony(mw->model);
		if (!poly.empty())
			idea += "\n\n• " + poly;
		if (!menu.empty())
			idea += "\n\nRight-click the panel for:" + menu;
		const math::Rect at(math::Vec(local.x, titleBand()), math::Vec(0.f, 0.f));
		if (!idea.empty()) {
			helpPopupShow(mw, at, mw->model->name, idea, false);
			return;
		}
		// NOTHING WRITTEN FOR THIS MODULE: the maker's own one-line description, which is the
		// text the module browser shows. Marked as theirs, like the per-control fallback.
		// NYSTHI's Bitshifter, which nobody has written an entry for, describes itself as
		// "256 bits bitshifter with S&H and noise and inner LFO and VCO" — worth hearing.
		const std::string made = mw->model->description;
		helpPopupShow(mw, at, mw->model->name, made, made.empty(), "", !made.empty());
		return;
	}
	// ANYWHERE ELSE ON THE PANEL CLOSES IT. Bare panel has nothing of its own to say, and
	// somewhere harmless to click is worth more than one more thing to read.
	helpPopupHide();
	helpSilence();
}

static app::ModuleWidget* helpModuleAt(math::Vec pos) {
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		if (mw->box.contains(pos) && mw->model)
			return mw;
	}
	return NULL;
}


/** A HELP CLICK, WHEREVER IT IS DELIVERED FROM.

THE GESTURE IS HANDLED ABOVE THE RACK, NOT INSIDE IT, and that is not a preference — it is the
only place it can be. ScrollWidget consumes option-click BEFORE its children, because option-drag
pans the rack, so a catcher parented to the rack never saw one. Clarity's own overlay is a child
of the SCENE, added after the rack's scroll view, which is why the option-click menu on a port
has always worked. This is the same click, asked of the same overlay.

`rackPos` is the pointer in the RACK's coordinates, which is what the module boxes are in;
APP->scene->rack->getMousePos() hands it over with no conversion to do. Returns whether the click
was taken. */
bool helpClickAt(math::Vec rackPos) {
	if (!gHelpOn)
		return false;
	app::ModuleWidget* hit = helpModuleAt(rackPos);
	if (!hit) {
		// Bare rack: nothing to say, and the note goes away rather than hanging over nothing.
		helpPopupHide();
		helpSilence();
		return true;
	}
	helpAnswer(hit, rackPos.minus(hit->box.pos));
	return true;
}

/** Puts the note away, for any ordinary click elsewhere. */
void helpDismissNote() {
	if (gPopup && gPopup->isVisible()) {
		helpPopupHide();
		helpSilence();
	}
}

/** Keeps the note on the scene and watches for Escape.

THE NOTE LIVES ON THE SCENE, NOT IN THE RACK, and that is about being seen. RackWidget::draw
paints four layers in order: panels and modules, then lights and halos, then plugs, then cables.
Anything drawn as part of the first is repainted by the other three, so a note inside the rack was
covered by every lamp, plug and cable over it. Rack's own tooltips are worse still: they are on
the scene and drawn after the whole rack.

So the note is a child of the scene and is moved to the END of the scene's children every frame
while it is showing, which puts it after the rack and after any tooltip that has just appeared.
Its position is then in window coordinates and has to be recomputed as the rack scrolls and
zooms — see helpPopupPlace. */
static void helpCatcherStep() {
	if (!APP->scene || !APP->window)
		return;
	if (!gPopup) {
		gPopup = new HelpPopup;
		gPopup->box.size = math::Vec(290.f, 40.f);
		gPopup->hide();
		APP->scene->addChild(gPopup);
	}
	if (gPopup->isVisible()) {
		if (APP->scene->children.back() != gPopup) {
			APP->scene->removeChild(gPopup);
			APP->scene->addChild(gPopup);
		}
		helpPopupPlace();
	}

	// ESCAPE PUTS THE NOTE AWAY TOO, for a hand already on the keyboard. Polled rather than
	// handled as a key event, because a key event goes to whatever is focused and nothing here
	// takes focus.
	if (gPopup->isVisible()
			&& glfwGetKey(APP->window->win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		helpPopupHide();
		helpSilence();
	}
}

void helpRemoveAll() {
	helpPopupHide();
	helpSilence();
}

void helpStep(bool enabled) {
	if (!APP->scene || !APP->scene->rack)
		return;
	helpCatcherStep();
	if (enabled == gHelpOn)
		return;
	gHelpOn = enabled;
	// Switched off, the catcher stays where it is and simply stops acting; anything already on
	// the screen goes, and anything being read stops.
	if (!enabled)
		helpRemoveAll();
}
