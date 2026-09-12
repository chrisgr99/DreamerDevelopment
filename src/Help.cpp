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
	}

	/** A click on the panel itself reads it out, for when the mouse is already there. */
	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			// BOTH, and not just the first: consuming records the target, stopping propagation is
			// what keeps anything underneath from consuming afterwards and becoming the target
			// instead. Overriding onButton without calling the base means doing this by hand.
			e.consume(this);
			e.stopPropagating();
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

/** Puts the panel beside a control, in the rack's coordinates.

BESIDE, AND INSIDE THE RACK. It is placed to the right of the control where there is room and to
the left where there is not, so it never covers the thing being asked about. */
static void helpPopupShow(app::ModuleWidget* mw, math::Rect controlBox,
		const std::string& title, const std::string& line, bool missing) {
	if (!gPopup)
		return;
	gPopup->title = title;
	gPopup->line = line.empty()
		? "Nothing here describes this one yet."
		: line;
	gPopup->missing = line.empty();
	gPopup->box.size.x = 290.f;
	if (APP->window && APP->window->vg)
		gPopup->box.size.y = gPopup->measure(APP->window->vg, false, NULL);
	gPopup->show();

	// The control's box is in the module's coordinates; the popup lives in the rack's.
	const math::Vec at = mw->box.pos.plus(controlBox.pos);
	float x = at.x + controlBox.size.x + 8.f;
	if (x + gPopup->box.size.x > APP->scene->rack->box.size.x)
		x = at.x - gPopup->box.size.x - 8.f;
	gPopup->box.pos = math::Vec(x, at.y - 4.f);
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

/** THE BADGE, AT THE TOP RIGHT OF SOMEBODY ELSE'S MODULE.

A PLAIN CLICK, NOT A MODIFIED ONE, and that was decided by measurement rather than taste. The
first build took option-click on the title; on this machine an option-click never reaches Rack at
all — the log shows not one mouse event carrying any modifier — because something above it takes
them. Shift was the obvious second choice and is worse: Rack uses shift-click to extend a
selection, so claiming it would break selecting modules to add help to them.

So the gesture is a target of its own, which nothing else can be holding. It is small, it sits
where no maker puts a control, and it is the LAST child of the module so it draws over the panel
and everything on it. */
struct HelpBadge : widget::OpaqueWidget {
	/** A millimetre smaller than a screw, so it reads as a mark rather than a fitting. */
	static float size() { return 15.f - mm2px(1.f); }
	bool hovered = false;

	HelpBadge() {
		box.size = math::Vec(size(), size());
	}

	void onEnter(const EnterEvent& e) override {
		hovered = true;
		widget::OpaqueWidget::onEnter(e);
	}
	void onLeave(const LeaveEvent& e) override {
		hovered = false;
		widget::OpaqueWidget::onLeave(e);
	}

	/** Set from outside, when this module is the one in help mode. */
	bool active = false;

	/** NO BUTTON HANDLING HERE, deliberately. A widget inside a module never sees a click while
	that module is selected — ModuleWidget::onButton returns before it dispatches to its children,
	so it can drag the selection. The badge is therefore drawn here and clicked on the rack, by
	the catcher, which nothing can get in front of. */

	void draw(const DrawArgs& args) override {
		const float r = box.size.x / 2.f;
		const bool lit = hovered || active;
		// QUIET UNTIL IT IS POINTED AT. It is on every module in the rack at once, so at rest it
		// has to read as a mark on the panel rather than as one more control competing with the
		// maker's own.
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, r, r, r - 1.f);
		nvgFillColor(args.vg, active ? nvgRGBA(0x2f, 0x7d, 0xc4, 0xff)
			: lit ? nvgRGBA(0x2b, 0x5c, 0x8a, 0xff) : nvgRGBA(0x18, 0x1c, 0x22, 0xa0));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, lit ? nvgRGBA(0x9f, 0xc8, 0xf0, 0xff)
			: nvgRGBA(0x8a, 0x92, 0x9e, 0xb0));
		nvgStrokeWidth(args.vg, active ? 2.f : 1.f);
		nvgStroke(args.vg);

		// A RING AROUND IT WHILE THE MODE IS ON, so it reads as switched on from across the rack
		// rather than as merely hovered.
		if (active) {
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, r, r, r + 2.f);
			nvgStrokeColor(args.vg, nvgRGBA(0x9f, 0xc8, 0xf0, 0x90));
			nvgStrokeWidth(args.vg, 1.f);
			nvgStroke(args.vg);
		}

		std::shared_ptr<window::Font> font =
			APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 11.f);
		nvgFillColor(args.vg, lit ? nvgRGB(0xff, 0xff, 0xff) : nvgRGBA(0xd8, 0xdd, 0xe4, 0xd0));
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, r, r + 0.5f, "?", NULL);
	}
};

/** HELP MODE, TURNED ON BY THE BADGE AND OFF BY THE BADGE OR ESCAPE.

Click a question mark and the rack goes into help mode: every badge lights, and a click on any
jack or knob on any module tells you what that one is, instead of doing what it would normally
do. Click any badge again, or press Escape, and the rack goes back to normal. The click that
turns the mode on says nothing — it is the way in, not a question.

WHY A MODE RATHER THAN A MODIFIER. Option-click was tried first and works in Rack, which binds
Alt nowhere. What defeated it is that a modified click still has to reach the control, and a
widget inside a module never sees a click while that module is SELECTED: ModuleWidget::onButton
returns before dispatching to its children so it can drag the selection. A module you have been
clicking on is usually selected, so the case that failed is the ordinary one.

ON THE RACK, ABOVE EVERY MODULE, for the same reason. This one widget works out for itself which
module, which badge and which control the pointer is over, so nothing in a module's own handling
can get in front of it. It consumes a click only when the mode is on or a badge was hit;
otherwise every click — plain, shift, cmd, right — passes straight through untouched. */
static bool gHelpMode = false;

bool helpModeOn() {
	return gHelpMode;
}

/** TAKING A CLICK, WHICH IS TWO THINGS AND NOT ONE.

Consuming an event only records which widget is to be treated as its target — it does NOT stop
the event being offered to everything else. Rack walks the rest of the children afterwards, and
the LAST widget to consume becomes the target. So a click taken here and then taken again by a
port underneath belongs to the port, and Rack starts dragging that port's cable: exactly the
symptom, with the catcher doing its half correctly the whole time.

Propagation has to be stopped as well, which is precisely what OpaqueWidget does and why this is
not one — an OpaqueWidget here would swallow every click in the rack, not the ones this mode is
about. */
static void helpTake(const widget::Widget::ButtonEvent& e, widget::Widget* by) {
	e.consume(by);
	e.stopPropagating();
	INFO("help: took the click — consumed=%d propagating=%d",
		e.isConsumed() ? 1 : 0, e.isPropagating() ? 1 : 0);
}

struct HelpCatcher : widget::Widget {
	/** The badge on this module, if it has one, in the module's own coordinates. */
	static HelpBadge* badgeIn(app::ModuleWidget* mw) {
		for (widget::Widget* child : mw->children) {
			if (HelpBadge* b = dynamic_cast<HelpBadge*>(child))
				return b;
		}
		return NULL;
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT
				|| (e.mods & RACK_MOD_MASK) != 0) {
			widget::Widget::onButton(e);
			return;
		}

		INFO("help: press at %g,%g mode=%d", e.pos.x, e.pos.y, gHelpMode ? 1 : 0);

		// THE PANEL ITSELF FIRST. It is a child of this widget, and a click on it is a click on
		// it, not on whatever module happens to be behind it.
		widget::Widget::onButton(e);
		if (e.isConsumed())
			return;

		app::ModuleWidget* hit = NULL;
		for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
			if (mw->box.contains(e.pos)) {
				hit = mw;
				break;
			}
		}

		// THE BADGE IS THE WAY IN AND THE WAY OUT, and it says nothing either way.
		if (hit && hit->model) {
			HelpBadge* badge = badgeIn(hit);
			if (badge && badge->box.contains(e.pos.minus(hit->box.pos))) {
				gHelpMode = !gHelpMode;
				INFO("help: badge clicked, mode now %d", gHelpMode ? 1 : 0);
				if (!gHelpMode)
					helpPopupHide();
				helpTake(e, this);
				return;
			}
		}

		if (!gHelpMode) {
			widget::Widget::onButton(e);
			return;
		}

		// EVERY CLICK IS TAKEN WHILE THE MODE IS ON, whether or not it lands on anything that has
		// something to say. A mode that answers some clicks and lets others through is a mode
		// that picks up a cable when you meant to ask about the jack.
		helpTake(e, this);
		if (!hit || !hit->model) {
			helpPopupHide();
			return;
		}

		const math::Vec local = e.pos.minus(hit->box.pos);
		std::string what, line;
		math::Rect where;
		HelpKind kind = HELP_PARAM;
		int index = -1;
		if (!helpControlAt(hit, local, what, line, where, kind, index)) {
			// BARE PANEL IS THE MODULE ITSELF, and what is wanted there is what the thing is —
			// the first line of the entry — not a list of everything on it.
			const std::string plugin = hit->model->plugin ? hit->model->plugin->slug : "";
			const std::vector<std::string> lines = helpFor(plugin, hit->model->slug);
			const std::string idea = lines.empty() ? "" : lines[0];
			// CLICKING THE PANEL PUTS THE NOTE AWAY, when there is one to put away. With none
			// showing it says what the module is, which is the other thing bare panel is for.
			if (gPopup && gPopup->isVisible()) {
				helpPopupHide();
				helpSilence();
				return;
			}
			helpPopupShow(hit, math::Rect(local, math::Vec(0.f, 0.f)),
				hit->model->name, idea, idea.empty());
			return;
		}

		// SHOWN, NEVER SPOKEN FROM HERE. Clicking a control puts its line on the screen without a
		// word; the note itself is what speaks, and clicking it again stops it. Nothing on the
		// module makes a sound, so moving around a panel looking at things is silent.
		helpPopupShow(hit, where, what, line, line.empty());
	}
};

/** A QUESTION MARK FOLLOWING THE POINTER WHILE THE MODE IS ON.

DRAWN, NOT A SYSTEM CURSOR. Rack draws its own window and hands the operating system no cursor to
swap, and a plugin that reached around it would be fighting whatever Rack does with the pointer
while a knob is being turned. Drawing one costs a circle and a glyph, sits exactly where the
pointer is, and disappears with the mode.

Beside the pointer rather than on it, so the thing being aimed at stays visible. */
struct HelpPointer : widget::Widget {
	void draw(const DrawArgs& args) override {
		if (!gHelpMode || !APP->scene)
			return;
		const math::Vec at = APP->scene->mousePos.plus(math::Vec(11.f, 12.f));
		const float r = 7.f;
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, at.x, at.y, r);
		nvgFillColor(args.vg, nvgRGBA(0x2f, 0x7d, 0xc4, 0xf0));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xd0));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> font =
			APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 11.f);
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, at.x, at.y + 0.5f, "?", NULL);
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

	// THE PANEL IS A CHILD OF THE CATCHER, not a sibling of it.
	//
	// As siblings they competed for the last place in the rack's children, which is the place
	// that is offered a click first: showing the panel took that place, and the catcher never got
	// it back — so the next click fell through to whatever was under it and picked up a cable.
	// One widget to keep on top, and the panel inside it, cannot get into that argument. The
	// catcher's box starts at the rack's origin, so the panel's coordinates do not change.
	if (!gPopup) {
		gPopup = new HelpPopup;
		gPopup->box.size = math::Vec(290.f, 40.f);
		gPopup->hide();
		catcher->addChild(gPopup);
	}

	// The pointer mark lives on the scene rather than the rack, so it is not scrolled or zoomed
	// with the modules: it belongs to the pointer, which is in window coordinates.
	static HelpPointer* pointer = NULL;
	if (!pointer) {
		pointer = new HelpPointer;
		APP->scene->addChild(pointer);
	}
	pointer->box.pos = math::Vec(0.f, 0.f);
	pointer->box.size = APP->scene->box.size;
	if (gHelpMode && APP->scene->children.back() != pointer) {
		APP->scene->removeChild(pointer);
		APP->scene->addChild(pointer);
	}
	// LAST, EVERY FRAME, WITHOUT CONDITION. Anything added to the rack after it would otherwise be
	// asked about a click first.
	if (rack->children.back() != catcher) {
		rack->removeChild(catcher);
		rack->addChild(catcher);
	}

	// THE BADGES FOLLOW THE MODE THE MOMENT IT CHANGES. Placing them is done only when the rack
	// changes, which is almost never, so the lighting is done here instead.
	static bool wasMode = false;
	if (gHelpMode != wasMode) {
		wasMode = gHelpMode;
		for (app::ModuleWidget* mw : rack->getModules()) {
			if (HelpBadge* b = HelpCatcher::badgeIn(mw))
				b->active = gHelpMode;
		}
	}

	// WATCHING FOR A CABLE IN FLIGHT, while one fault is being tracked down.
	static bool hadCable = false;
	if (gHelpMode) {
		const std::vector<app::CableWidget*> loose = rack->getIncompleteCables();
		if (!loose.empty() && !hadCable) {
			app::CableWidget* cw = loose[0];
			INFO("help: a cable is in flight — in=%p out=%p",
				(void*) cw->inputPort, (void*) cw->outputPort);
		}
		hadCable = !loose.empty();
	}

	// ESCAPE LEAVES, and is polled rather than handled as an event: a key event goes to whatever
	// is focused, and in help mode that is nothing in particular.
	if (gHelpMode && glfwGetKey(APP->window->win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		gHelpMode = false;
		INFO("help: escape, mode off");
		helpPopupHide();
	}
}

static bool gWasEnabled = false;
static bool gDirty = true;
static size_t gLastCount = 0;

static HelpBadge* badgeOf(app::ModuleWidget* mw, bool make) {
	for (widget::Widget* child : mw->children) {
		if (HelpBadge* h = dynamic_cast<HelpBadge*>(child))
			return h;
	}
	if (!make)
		return NULL;
	HelpBadge* h = new HelpBadge;
	mw->addChild(h);
	return h;
}

void helpRemoveAll() {
	if (!APP->scene || !APP->scene->rack)
		return;
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		if (HelpBadge* h = badgeOf(mw, false)) {
			mw->removeChild(h);
			delete h;
		}
	}
	gLastCount = 0;
}

void helpStep(bool enabled) {
	if (!APP->scene || !APP->scene->rack)
		return;
	helpCatcherStep();
	if (enabled != gWasEnabled) {
		gWasEnabled = enabled;
		gDirty = true;
		if (!enabled) {
			// No badges means no way back out of the mode, so it goes with them.
			gHelpMode = false;
			INFO("help: badges switched off, mode off");
			helpPopupHide();
			helpRemoveAll();
			return;
		}
	}
	if (!enabled)
		return;
	const std::vector<app::ModuleWidget*> modules = APP->scene->rack->getModules();
	if (!gDirty && modules.size() == gLastCount)
		return;
	gDirty = false;
	gLastCount = modules.size();

	for (app::ModuleWidget* mw : modules) {
		if (!mw->model)
			continue;
		HelpBadge* h = badgeOf(mw, true);
		// LAST CHILD, AND PLACED EVERY TIME. Last because children are drawn in order and offered
		// a click in reverse, so this draws over the panel and is asked about the click first.
		// Placed every time because a module that changes width — an expander being attached, a
		// themed panel swapping — would otherwise leave the badge off its own corner.
		h->box.size = math::Vec(HelpBadge::size(), HelpBadge::size());
		// ON THE LINE THE SCREWS SIT ON, taken from a screw rather than assumed: the standard
		// position is the top of the panel, but a maker is free to put theirs elsewhere and the
		// badge should line up with whatever is actually drawn.
		float cy = 15.f / 2.f;
		for (widget::Widget* child : mw->children) {
			app::SvgScrew* screw = dynamic_cast<app::SvgScrew*>(child);
			if (screw && screw->box.pos.y < mw->box.size.y / 2.f) {
				cy = screw->box.getCenter().y;
				break;
			}
		}
		h->box.pos = math::Vec(mw->box.size.x - h->box.size.x - 1.5f, cy - h->box.size.y / 2.f);
		h->active = gHelpMode;
	}
}
