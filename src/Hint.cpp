/** The one-time notes — see Hint.hpp for why they exist. */
#include "Hint.hpp"

#include <set>


static const float HINT_W = 420.f;
static const float HINT_PAD = 16.f;
static const float HINT_LINE = 20.f;
static const float HINT_TITLE = 16.f;
static const float HINT_TEXT = 14.5f;
static const float HINT_BOX = 15.f;      /**< The tick box. */
static const float HINT_BTN_W = 58.f;
static const float HINT_BTN_H = 24.f;
/** How far from the thing it is about, and how much room is left for the pointer. */
static const float HINT_GAP = 34.f;

static const NVGcolor HINT_BG = nvgRGB(0x1c, 0x21, 0x29);
static const NVGcolor HINT_INK = nvgRGB(0xe6, 0xe8, 0xec);
/** Bright enough to READ. A note is not chrome — it is on screen for a few seconds and has to
be legible in that time, so it is set in the same ink as the heading. */
static const NVGcolor HINT_DIM = nvgRGB(0xd2, 0xd7, 0xdd);
static const NVGcolor HINT_EDGE = nvgRGB(0x3d, 0xe0, 0x7a);
static const NVGcolor HINT_FIELD = nvgRGB(0x10, 0x14, 0x1a);

/** PROPORTIONAL, and Rack's own. The monospaced face this plugin uses on its instrument faces
is right for numbers that must not shift as they change; it is the wrong face for a paragraph,
where it reads as a terminal printout and is hard work at any size. */
static std::shared_ptr<window::Font> bodyFont() {
	return APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
}

static std::shared_ptr<window::Font> titleFont() {
	return APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf"));
}

/** Ids the user has silenced. NOTHING ELSE holds a note back.

It used to appear once per session, which meant a note about a gesture could be dismissed by
accident and not be seen again that day — and the box that was there to make the choice
explicit was moot, since the note was going away either way. It now shows every time the thing
it is about happens, until someone says otherwise. */
static std::set<std::string> silenced;
static bool loaded = false;


static std::string hintFilePath() {
	return asset::user("DreamerDevelopment/hints.json");
}


static void hintLoad() {
	if (loaded)
		return;
	loaded = true;

	FILE* file = std::fopen(hintFilePath().c_str(), "r");
	if (!file)
		return;
	json_error_t error;
	json_t* rootJ = json_loadf(file, 0, &error);
	std::fclose(file);
	if (!rootJ)
		return;

	if (json_t* arrayJ = json_object_get(rootJ, "silenced")) {
		size_t i;
		json_t* idJ;
		json_array_foreach(arrayJ, i, idJ) {
			if (const char* id = json_string_value(idJ))
				silenced.insert(id);
		}
	}
	json_decref(rootJ);
}


static void hintSave() {
	// Beside Rack's own settings, not in the patch: whether someone wants to be told a thing
	// again is about them, and would otherwise arrive with a patch they were sent.
	system::createDirectories(asset::user("DreamerDevelopment"));

	json_t* rootJ = json_object();
	json_t* arrayJ = json_array();
	for (const std::string& id : silenced)
		json_array_append_new(arrayJ, json_string(id.c_str()));
	json_object_set_new(rootJ, "silenced", arrayJ);

	FILE* file = std::fopen(hintFilePath().c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(2));
		std::fclose(file);
	}
	json_decref(rootJ);
}


/** The note itself: a small panel at the top of the window.

A child of the SCENE rather than of the rack, so it holds still while the rack is scrolled and
zoomed — it is a message to the person, not a label on the patch.
*/
struct HintWidget : widget::OpaqueWidget {
	std::string id;
	std::string title;
	std::vector<std::string> lines;
	/** In scene coordinates: the thing this note is about. */
	math::Vec anchor = math::Vec(-1.f, -1.f);
	/** Which way the hand was going when it appeared. */
	math::Vec away;
	/** WHERE IT WENT, decided once. Choosing a spot every frame meant the note fled from the
	pointer and could never be clicked: keeping clear of the hand is a decision made when it
	appears, not a rule it goes on enforcing. */
	bool placed = false;
	math::Vec placedPos;

	/** Ticked by the user, and acted on when the note is closed — NOT the moment it is
	clicked. A note that vanished for good on one click would be gone before anyone had decided
	that is what they wanted; a box you tick and then close is a choice you can see yourself
	making, and change your mind about. */
	bool silenceTicked = false;

	math::Rect tickBox;
	math::Rect tickLabel;
	math::Rect button;

	void layout() {
		const float h = HINT_PAD + HINT_TITLE + 8.f
			+ HINT_LINE * lines.size() + 14.f + HINT_BTN_H + HINT_PAD;
		box.size = math::Vec(HINT_W, h);

		if (anchor.x < 0.f) {
			box.pos = math::Vec(std::floor((APP->scene->box.size.x - HINT_W) / 2.f), 60.f);
		}
		else {
			if (!placed) {
				placedPos = placeNearAnchor();
				placed = true;
			}
			// Re-clamped rather than re-chosen, so a window resize cannot leave it off screen.
			box.pos = math::Vec(
				math::clamp(placedPos.x, 8.f,
					std::fmax(8.f, APP->scene->box.size.x - box.size.x - 8.f)),
				math::clamp(placedPos.y, 8.f,
					std::fmax(8.f, APP->scene->box.size.y - box.size.y - 8.f)));
		}

		const float rowY = box.size.y - HINT_PAD - HINT_BTN_H;
		button = math::Rect(math::Vec(HINT_W - HINT_PAD - HINT_BTN_W, rowY),
			math::Vec(HINT_BTN_W, HINT_BTN_H));
		tickBox = math::Rect(
			math::Vec(HINT_PAD, rowY + (HINT_BTN_H - HINT_BOX) / 2.f),
			math::Vec(HINT_BOX, HINT_BOX));
		// The label is part of the target: a fifteen-pixel square is a small thing to hit, and
		// the words beside it belong to the same control.
		tickLabel = math::Rect(math::Vec(HINT_PAD, rowY), math::Vec(190.f, HINT_BTN_H));
	}

	/** Somewhere beside the jack, and NEVER where the hand is.

	A dialogue that lands on the very spot someone is working reads as an interruption they have
	to fight, so a candidate that would cover the pointer is not used even if it is the tidiest
	one. The far side from the pull is preferred, since that is the space the hand has just left
	and is not about to cross; the other three sides follow, ordered by how far each keeps the
	note from the pointer. Only if all four would fall off the window does it settle for the one
	furthest from the hand, clamped into view.
	*/
	math::Vec placeNearAnchor() {
		const math::Vec scene = APP->scene->box.size;
		const math::Vec size = box.size;
		const math::Vec mouse = APP->scene->getMousePos();
		/** Room left around the pointer: the hand, the cable end, and a little to spare. */
		const float KEEP_CLEAR = 40.f;

		const math::Vec left(anchor.x - HINT_GAP - size.x, anchor.y - size.y / 2.f);
		const math::Vec right(anchor.x + HINT_GAP, anchor.y - size.y / 2.f);
		const math::Vec above(anchor.x - size.x / 2.f, anchor.y - HINT_GAP - size.y);
		const math::Vec below(anchor.x - size.x / 2.f, anchor.y + HINT_GAP);

		// The far side from the pull comes first; the rest are sorted below by distance.
		std::vector<math::Vec> candidates;
		if (std::fabs(away.x) >= std::fabs(away.y))
			candidates.push_back((away.x >= 0.f) ? left : right);
		else
			candidates.push_back((away.y >= 0.f) ? above : below);
		for (const math::Vec& c : {left, right, above, below}) {
			if (c.x != candidates[0].x || c.y != candidates[0].y)
				candidates.push_back(c);
		}

		auto onScreen = [&](math::Vec p) {
			return p.x >= 8.f && p.y >= 8.f
				&& p.x + size.x <= scene.x - 8.f && p.y + size.y <= scene.y - 8.f;
		};
		auto clearOfHand = [&](math::Vec p) {
			return !math::Rect(p.minus(math::Vec(KEEP_CLEAR, KEEP_CLEAR)),
				size.plus(math::Vec(KEEP_CLEAR * 2.f, KEEP_CLEAR * 2.f))).contains(mouse);
		};

		// First choice: on screen and clear of the hand, in preference order.
		for (const math::Vec& p : candidates) {
			if (onScreen(p) && clearOfHand(p))
				return p;
		}
		// Then: anywhere on screen that is clear of the hand, sliding along the window instead
		// of hugging the jack. The note is still about that jack; being readable matters more
		// than being beside it.
		const math::Vec corners[4] = {
			math::Vec(8.f, 8.f),
			math::Vec(scene.x - size.x - 8.f, 8.f),
			math::Vec(8.f, scene.y - size.y - 8.f),
			math::Vec(scene.x - size.x - 8.f, scene.y - size.y - 8.f),
		};
		const math::Vec* best = NULL;
		float bestDist = -1.f;
		for (const math::Vec& p : corners) {
			if (!onScreen(p))
				continue;
			const float d = p.plus(size.div(2.f)).minus(mouse).norm();
			if (d > bestDist) {
				bestDist = d;
				best = &p;
			}
		}
		if (best && clearOfHand(*best))
			return *best;

		// Last resort on a window too small to avoid anything: clamped into view.
		return math::Vec(
			math::clamp(candidates[0].x, 8.f, std::fmax(8.f, scene.x - size.x - 8.f)),
			math::clamp(candidates[0].y, 8.f, std::fmax(8.f, scene.y - size.y - 8.f)));
	}

	/** NO TIMER. A note that takes itself away is a note you look up at and find gone, and the
	only way to see it again is to repeat whatever summoned it. It stays until it is closed. */
	void step() override {
		layout();
		widget::OpaqueWidget::step();
	}

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 5);
		nvgFillColor(args.vg, HINT_BG);
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, HINT_EDGE);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> body = bodyFont();
		std::shared_ptr<window::Font> heading = titleFont();
		if (!body || body->handle < 0)
			return;
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

		float y = HINT_PAD + HINT_TITLE / 2.f;
		nvgFontFaceId(args.vg, (heading && heading->handle >= 0) ? heading->handle : body->handle);
		nvgFontSize(args.vg, HINT_TITLE);
		nvgFillColor(args.vg, HINT_INK);
		nvgText(args.vg, HINT_PAD, y, title.c_str(), NULL);

		y += HINT_TITLE / 2.f + 8.f + HINT_LINE / 2.f;
		nvgFontFaceId(args.vg, body->handle);
		nvgFontSize(args.vg, HINT_TEXT);
		nvgFillColor(args.vg, HINT_DIM);
		for (const std::string& line : lines) {
			nvgText(args.vg, HINT_PAD, y, line.c_str(), NULL);
			y += HINT_LINE;
		}

		// The tick box.
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, tickBox.pos.x, tickBox.pos.y, HINT_BOX, HINT_BOX, 3.f);
		nvgFillColor(args.vg, HINT_FIELD);
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, silenceTicked ? HINT_EDGE : nvgRGB(0x5a, 0x62, 0x6c));
		nvgStrokeWidth(args.vg, 1.2f);
		nvgStroke(args.vg);
		if (silenceTicked) {
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, tickBox.pos.x + 3.f, tickBox.pos.y + HINT_BOX / 2.f);
			nvgLineTo(args.vg, tickBox.pos.x + 6.f, tickBox.pos.y + HINT_BOX - 4.f);
			nvgLineTo(args.vg, tickBox.pos.x + HINT_BOX - 3.f, tickBox.pos.y + 4.f);
			nvgStrokeColor(args.vg, HINT_EDGE);
			nvgStrokeWidth(args.vg, 2.f);
			nvgLineCap(args.vg, NVG_ROUND);
			nvgLineJoin(args.vg, NVG_ROUND);
			nvgStroke(args.vg);
		}
		nvgFillColor(args.vg, HINT_DIM);
		nvgText(args.vg, tickBox.pos.x + HINT_BOX + 8.f, tickLabel.getCenter().y,
			"Don't show this again", NULL);

		// The button.
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, button.pos.x, button.pos.y, button.size.x, button.size.y, 4.f);
		nvgFillColor(args.vg, nvgRGB(0x2a, 0x31, 0x3b));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGB(0x5a, 0x62, 0x6c));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, HINT_INK);
		// "OK", not "Got it": the button only closes the note. A button that sounds like an
		// acknowledgement invites the reading that pressing it also means "and don't tell me
		// again", which is the tick box's job and nothing else's.
		nvgText(args.vg, button.getCenter().x, button.getCenter().y, "OK", NULL);

		widget::OpaqueWidget::draw(args);
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT) {
			widget::OpaqueWidget::onButton(e);
			return;
		}
		if (tickLabel.contains(e.pos)) {
			silenceTicked = !silenceTicked;
			e.consume(this);
			return;
		}
		if (button.contains(e.pos)) {
			// The choice is acted on HERE, when the note is closed, so ticking the box is
			// something you can see before it takes effect.
			if (silenceTicked) {
				silenced.insert(id);
				hintSave();
			}
			requestDelete();
			e.consume(this);
			return;
		}
		// Anywhere else on the note is swallowed rather than passed through: the note sits over
		// the rack, and a click meant for it must not also land on whatever is beneath.
		e.consume(this);
	}
};


static WeakPtr<HintWidget> current;


void hintShow(const std::string& id, const std::vector<std::string>& lines, math::Vec anchor,
	math::Vec away) {
	hintLoad();
	if (id.empty() || lines.empty())
		return;
	if (silenced.count(id))
		return;
	if (!APP->scene)
		return;
	// Already up, and the same note: MOVE it to the new jack rather than leaving it beside the
	// last one. A note that is about the cable in your hand should be beside the cable in your
	// hand; left where it was, it is a note about something you have finished with.
	if (current && current->id == id) {
		current->anchor = anchor;
		current->away = away;
		current->placed = false;   // Chosen once more, for the new jack.
		return;
	}

	if (current) {
		current->requestDelete();
		current = NULL;
	}

	HintWidget* hint = new HintWidget;
	hint->id = id;
	hint->title = lines[0];
	hint->lines.assign(lines.begin() + 1, lines.end());
	hint->anchor = anchor;
	hint->away = away;
	hint->layout();
	APP->scene->addChild(hint);
	current = hint;
}


bool hintCovers(math::Vec scenePos) {
	return current && current->box.contains(scenePos);
}


void hintResetAll() {
	hintLoad();
	silenced.clear();
	hintSave();
}


void hintDismiss() {
	if (current) {
		current->requestDelete();
		current = NULL;
	}
}
