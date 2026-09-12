/** In-rack help — see Help.hpp for what this is and why the text is written rather than derived. */
#include "Help.hpp"

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
	// An em dash is a pause, not a word. A leading bullet dash is not a word either.
	t = std::regex_replace(t, std::regex("\n- "), "\n");
	t = std::regex_replace(t, std::regex("—"), ",");
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
	/** WHERE IT BELONGS, IN THE RACK'S OWN COORDINATES.
	
	The note is drawn on the SCENE rather than in the rack — see helpCatcherStep for why — so its
	own box is in window coordinates and has to be recomputed whenever the rack is scrolled or
	zoomed. This is the anchor it is placed against: the control's box, in rack space. */
	math::Rect anchor;
	/** When the copy was last taken, so the button can show that it worked. */
	double copiedAt = -1.0;

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

	float measure(NVGcontext* vg, bool drawing, const DrawArgs* args) {
		std::shared_ptr<window::Font> font =
			APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font || font->handle < 0)
			return 40.f;
		const float w = box.size.x;
		float y = HELP_PAD;

		nvgFontFaceId(vg, font->handle);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

		nvgFontSize(vg, 11.f);
		if (drawing) {
			nvgFillColor(args->vg, nvgRGB(0x7f, 0xb0, 0xe4));
			nvgText(args->vg, HELP_PAD, y, title.c_str(), NULL);
		}
		y += 15.f;

		nvgFontSize(vg, 12.f);
		nvgTextLineHeight(vg, HELP_LEAD / 12.f);
		float bounds[4];
		nvgTextBoxBounds(vg, HELP_PAD, y, w - HELP_PAD * 2.f, line.c_str(), NULL, bounds);
		if (drawing) {
			nvgFillColor(args->vg, missing ? nvgRGB(0x8a, 0x92, 0x9e) : nvgRGB(0xe4, 0xe8, 0xee));
			nvgTextBox(args->vg, HELP_PAD, y, w - HELP_PAD * 2.f, line.c_str(), NULL);
		}
		y += std::max(bounds[3] - bounds[1], HELP_LEAD);

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
			if (helpIsSpeaking())
				helpSilence();
			else if (!missing)
				helpSay(line);
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
	const math::Rect a = gPopup->anchor;

	float x = origin.x + (a.pos.x + a.size.x) * zoom + 8.f;
	const float y = origin.y + a.pos.y * zoom - 4.f;
	if (x + gPopup->box.size.x > APP->scene->box.size.x)
		x = origin.x + a.pos.x * zoom - gPopup->box.size.x - 8.f;
	gPopup->box.pos = math::Vec(std::max(x, 0.f),
		math::clamp(y, 0.f, std::max(0.f, APP->scene->box.size.y - gPopup->box.size.y)));
}

/** Shows the note for one control, anchored to the control's box in rack coordinates. */
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
	gPopup->title = title;
	gPopup->line = line.empty()
		? "Nothing here describes this one yet."
		: line;
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

	for (app::PortWidget* p : mw->getInputs()) {
		if (!p->box.contains(pos))
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
		return true;
	}
	for (app::PortWidget* p : mw->getOutputs()) {
		if (!p->box.contains(pos))
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
		return true;
	}
	for (app::ParamWidget* p : mw->getParams()) {
		if (!p->box.contains(pos))
			continue;
		what = "control " + std::to_string(p->paramId + 1);
		if (p->getParamQuantity() && !p->getParamQuantity()->name.empty())
			what = p->getParamQuantity()->name;
		line = helpForControl(plugin, model, HELP_PARAM, p->paramId);
		where = p->box;
		kind = HELP_PARAM;
		index = p->paramId;
		return true;
	}
	return false;
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

/** Whether option-click help is switched on, for the rest of the plugin's own gestures. */
bool helpModeOn() {
	return gHelpOn;
}

bool helpClaimsClick(int mods) {
	return gHelpOn && (mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT);
}

/** A press we are waiting to see the end of, because it landed on a jack. */
static app::ModuleWidget* gPressModule = NULL;
static math::Vec gPressAt;
static bool gPressPending = false;
/** How far the mouse may travel and still count as a click rather than a drag. */
static const float HELP_CLICK_SLOP = 3.f;

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

struct HelpCatcher : widget::Widget {
	/** THE TITLE BAND ACROSS THE TOP OF A MODULE, where nearly every maker puts its name.

	Where the title is cannot be asked — a maker draws it wherever they like, and Rack's own SVG
	renderer has no text at all, so a panel's name is either outlines or drawn in the maker's own
	code. This is the top of the panel, which is where it nearly always is. */
	static float titleBand() { return 40.f; }

	/** Shows the note for whatever is at this point in the module's own coordinates, or puts the
	note away where there is nothing to say. */
	void answer(app::ModuleWidget* mw, math::Vec local) {
		std::string what, line;
		math::Rect where;
		HelpKind kind = HELP_PARAM;
		int index = -1;
		if (helpControlAt(mw, local, what, line, where, kind, index)) {
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
			const std::string idea = lines.empty() ? "" : lines[0];
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

	app::ModuleWidget* moduleAt(math::Vec pos) {
		for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
			if (mw->box.contains(pos) && mw->model)
				return mw;
		}
		return NULL;
	}

	bool isOurs(const ButtonEvent& e) {
		return gHelpOn && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT);
	}

	void onButton(const ButtonEvent& e) override {
		// THE NOTE ITSELF FIRST, whatever the modifiers. It is a child of this widget, and a
		// plain click on it reads it out — see HelpPopup::onButton.
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			widget::Widget::onButton(e);
			if (e.isConsumed())
				return;
		}

		if (!isOurs(e)) {
			// AN ORDINARY CLICK PUTS THE NOTE AWAY. It is a transient answer to a question, not
			// a window, so getting on with anything dismisses it — and a click ON the note has
			// already been handled above, so that one reads it out instead of closing it.
			if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
					&& gPopup && gPopup->isVisible()) {
				helpPopupHide();
				helpSilence();
			}
			if (e.action == GLFW_PRESS)
				gPressPending = false;
			widget::Widget::onButton(e);
			return;
		}

		if (e.action == GLFW_PRESS) {
			app::ModuleWidget* hit = moduleAt(e.pos);
			gPressPending = false;
			if (!hit) {
				helpPopupHide();
				return;
			}
			const math::Vec local = e.pos.minus(hit->box.pos);
			// OVER A JACK, WAIT. Rack may be about to clone a cable from it, and only the
			// release says whether this was a click or the start of that drag.
			if (helpPortAt(hit, local)) {
				gPressModule = hit;
				gPressAt = e.pos;
				gPressPending = true;
				widget::Widget::onButton(e);
				return;
			}
			helpTake(e, this);
			answer(hit, local);
			return;
		}

		if (e.action == GLFW_RELEASE && gPressPending) {
			gPressPending = false;
			const bool moved = e.pos.minus(gPressAt).norm() > HELP_CLICK_SLOP;
			if (!moved && gPressModule) {
				answer(gPressModule, gPressAt.minus(gPressModule->box.pos));
				// Rack may have begun cloning a cable on the press. A clone that was never
				// dragged anywhere is not wanted, and it is always a NEW cable, so removing it
				// cannot cost the user a connection they had.
				for (app::CableWidget* cw : APP->scene->rack->getIncompleteCables()) {
					APP->scene->rack->removeCable(cw);
					delete cw;
				}
				helpTake(e, this);
				return;
			}
		}
		widget::Widget::onButton(e);
	}
};

/** Puts the catcher on the rack and keeps it last, so it is offered a click before the modules
under it. Also watches for Escape, which leaves help mode. */
static void helpCatcherStep() {
	if (!APP->scene || !APP->scene->rack || !APP->window)
		return;
	app::RackWidget* rack = APP->scene->rack;
	static HelpCatcher* catcher = NULL;
	if (!catcher) {
		catcher = new HelpCatcher;
		rack->addChild(catcher);
	}
	catcher->box.pos = math::Vec(0.f, 0.f);
	catcher->box.size = rack->box.size;

	// THE NOTE LIVES ON THE SCENE, NOT IN THE RACK, and that is about being seen.
	//
	// RackWidget::draw paints four layers in order: panels and modules, then lights and halos,
	// then plugs, then cables. Anything drawn as part of the first is repainted by the other
	// three, so a note inside the rack was covered by every lamp, plug and cable over it. Rack's
	// own tooltips are worse still: they are on the scene and drawn after the whole rack.
	//
	// So the note is a child of the scene and is moved to the END of the scene's children every
	// frame while it is showing, which puts it after the rack and after any tooltip that has just
	// appeared. Its position is then in window coordinates and has to be recomputed as the rack
	// scrolls and zooms — see helpPopupPlace.
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

	// LAST, EVERY FRAME, WITHOUT CONDITION. Anything added to the rack afterwards would otherwise
	// be asked about a click before it.
	if (rack->children.back() != catcher) {
		rack->removeChild(catcher);
		rack->addChild(catcher);
	}

	// ESCAPE PUTS THE NOTE AWAY TOO, for a hand already on the keyboard. Polled rather than
	// handled as a key event, because a key event goes to whatever is focused and nothing here
	// takes focus.
	if (gPopup && gPopup->isVisible()
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
