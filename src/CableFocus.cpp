/** Following one cable through a tangle.

Hover either end of a cable and a pill swells out of it, just clear of the jack. Click the
pill and that cable stays fully bright while every other cable drops to a quarter of its
opacity, so the one you are tracing is the only one your eye follows. Click it again, press
Escape, or click the pill on a different cable, to change what is lit.

HOW THE DIMMING IS DONE, and why it is safe. Each cable's own colour carries an alpha, and
Rack multiplies it by the global cable opacity when drawing — so lowering that alpha dims a
cable without touching how Rack draws anything. The originals are kept here and put back the
moment the focus is released.

The one hazard in that is a dimmed alpha being SAVED, because a cable's colour is written
into the patch. So the focus is cleared before any save: DRUI drops it in dataToJson, which
runs for a manual save, for the autosave, and on quit.
*/
#include "plugin.hpp"
#include "WidgetAt.hpp"

#include <app/CableWidget.hpp>

#include <map>
#include <set>
#include <vector>
#include <algorithm>


/** The rest are HIDDEN, not merely faded.

Alpha alone could not do this. A cable's shadow is drawn in fixed black rather than in the
cable's colour, so a fully transparent cable still cast a visible shadow; and our own flow
dashes are drawn by us, not by Rack, so they would have kept crawling along cables that had
otherwise vanished. Hiding the widget removes the cable, its outline and its shadow together,
and the dashes are skipped for anything hidden. */
/** How near the pointer must be to a pill's centre for it to be offered, in rack pixels.

Kept smaller than the gap between the pill and the jack. At 13 the claimed area reached back
over the jack itself, so a press meant for the terminal could be taken by a pill instead. */
static const float PILL_REACH = 8.f;
/** Where the pill sits, measured along the cable from where the cable is DRAWN to start —
which is already 14 px out from the jack's centre, because Rack insets both ends. So zero
here puts the pill against the jack without reaching into the jack's own click radius. */
static const float PILL_START = 0.f;
static const float PILL_LENGTH = 12.f;
static const float PILL_WIDTH = 11.f;

static int64_t focusedId = -1;
static std::map<int64_t, NVGcolor> originalColors;
/** Exactly which cables THIS hid, so exactly those are put back.

Recording "what its visibility was" instead was wrong in both directions: a cable already
hidden by us from an earlier frame recorded itself as hidden and stayed invisible for good
once it became the traced one, and an injector's deliberately hidden cable could be recorded
as visible and then put on screen. Owning only what we hid has neither failure. */
static std::set<int64_t> hiddenByUs;
/** The global cable opacity, held while a cable is being traced.

Setting the traced cable's own alpha to 1 was not enough to make it FULLY bright: Rack
multiplies every cable by settings::cableOpacity, which is 0.5 by default, so the lit cable
could never be brighter than half. The global goes to 1 for the duration, which makes the
traced cable genuinely full strength and leaves the others at a true quarter. */
static float savedCableOpacity = -1.f;

/** Every cable end whose pill lies under the pointer, not just the nearest one.

Two cables leaving the same jack put their pills almost on top of each other, so picking only
the closest made the one underneath unreachable. They are collected instead, and a click
takes the one on top and then moves the turn on to the next — so clicking repeatedly in the
same spot rotates through everything that shares it.
*/
struct HoverCandidate {
	app::CableWidget* cw = NULL;
	bool atInput = false;
	int64_t id = -1;
};
static std::vector<HoverCandidate> candidates;
/** Which candidate the next click takes. Reset whenever the set under the pointer changes,
so the rotation only ever applies to the pills actually being pointed at. */
static size_t turn = 0;
static std::vector<int64_t> lastCandidateKey;


/** Rack's own cable curve. Reproduced rather than called because the function that computes
it is not exported to plugins; the shape has to match exactly or the pill would sit beside
the cable instead of on it. */
static math::Vec slumpOf(math::Vec p0, math::Vec p1) {
	const float dist = p0.minus(p1).norm();
	math::Vec avg = p0.plus(p1).div(2);
	avg.y += (1.0 - settings::cableTension) * (150.0 + 1.0 * dist);
	return avg;
}

static math::Vec quadAt(math::Vec p0, math::Vec c, math::Vec p1, float t) {
	const float u = 1.f - t;
	return p0.mult(u * u).plus(c.mult(2.f * u * t)).plus(p1.mult(t * t));
}

/** A cable's two ends and control point, in rack coordinates, with the same inset Rack
applies so the curve starts where the drawn cable starts. */
static bool cableCurve(app::CableWidget* cw, math::Vec& p0, math::Vec& c, math::Vec& p1) {
	if (!cw || !cw->inputPort || !cw->outputPort)
		return false;
	p0 = cw->outputPort->getRelativeOffset(
		cw->outputPort->box.zeroPos().getCenter(), APP->scene->rack);
	p1 = cw->inputPort->getRelativeOffset(
		cw->inputPort->box.zeroPos().getCenter(), APP->scene->rack);
	c = slumpOf(p0, p1);
	p0 = p0.plus(c.minus(p0).normalize().mult(14.f));
	p1 = p1.plus(c.minus(p1).normalize().mult(14.f));
	return true;
}

/** Walks in from one end until it has travelled `along` pixels of curve. Stepping along the
curve rather than measuring straight-line distance matters on a deeply slumped cable, where
the two differ by a lot. */
static math::Vec pointAlong(math::Vec p0, math::Vec c, math::Vec p1, bool fromInput,
	float along) {

	const int steps = 48;
	math::Vec prev = fromInput ? p1 : p0;
	float travelled = 0.f;
	for (int i = 1; i <= steps; i++) {
		const float t = (float) i / steps;
		const math::Vec pt = quadAt(p0, c, p1, fromInput ? 1.f - t : t);
		travelled += pt.minus(prev).norm();
		prev = pt;
		if (travelled >= along)
			return pt;
	}
	return prev;
}


/** Takes the global cable opacity up to full, remembering what it was. Called every frame
while a trace is held, not only when one is started: a save puts the setting back, and
without retaking it here the traced cable dropped to half brightness at the first autosave
and stayed there — the colours were being restored correctly, the opacity was not. */
static void holdOpacity() {
	if (savedCableOpacity < 0.f)
		savedCableOpacity = settings::cableOpacity;
	settings::cableOpacity = 1.f;
}


static void restoreColors() {
	for (app::CableWidget* cw : APP->scene->rack->getCompleteCables()) {
		const int64_t id = cw->cable ? cw->cable->id : -1;
		auto it = originalColors.find(id);
		if (it != originalColors.end())
			cw->color = it->second;
		if (hiddenByUs.count(id))
			cw->visible = true;
	}
	originalColors.clear();
	hiddenByUs.clear();
}


void cableFocusClear() {
	if (focusedId < 0)
		return;
	focusedId = -1;
	restoreColors();
	if (savedCableOpacity >= 0.f) {
		settings::cableOpacity = savedCableOpacity;
		savedCableOpacity = -1.f;
	}
}


/** Puts the true colours and the global opacity back for the duration of a save, WITHOUT
giving up the trace.

Clearing the focus outright would have been wrong: dataToJson runs for the periodic autosave
too, so tracing a cable would have gone dark every fifteen seconds for no visible reason.
Nothing else is needed to restore the effect — the dimming is re-applied on the next frame
like any other, and colours are re-read from what is there now, which is the truth again.

Rack serialises its modules before its cables, so by the time the cables are written the real
colours are back. Quitting writes the patch and then the settings, with no frame in between,
so this protects the saved cable opacity as well.
*/
void cableFocusPrepareSave() {
	restoreColors();
	if (savedCableOpacity >= 0.f) {
		settings::cableOpacity = savedCableOpacity;
		savedCableOpacity = -1.f;
	}
}


/** The cable whose pill is currently under the pointer, and which of its ends the pill is on.

Exposed so a right-click on the pill can lift THAT cable. The pill is the only thing in Rack
that says which of several cables converging on a jack you mean, so it is the natural place
to take one from.
*/
bool cableFocusPillAt(app::CableWidget*& cw, bool& atInput) {
	if (candidates.empty())
		return false;
	cw = candidates[turn].cw;
	atInput = candidates[turn].atInput;
	return cw != NULL;
}


bool cableFocusActive() {
	return focusedId >= 0;
}


/** Whether this cable is one of the ones being kept out of the way, so its flow dashes are
skipped along with it. */
bool cableFocusHidden(app::CableWidget* cw) {
	if (focusedId < 0 || !cw || !cw->cable)
		return false;
	return cw->cable->id != focusedId;
}


/** Keeps the dimming true as cables are added, removed or recoloured while a focus is held. */
void cableFocusStep() {
	// What the pointer is near. Read from the rack rather than from an event, because this
	// needs to be true every frame whether or not the pointer has moved.
	candidates.clear();
	const math::Vec mouse = APP->scene->rack->getMousePos();

	for (app::CableWidget* cw : APP->scene->rack->getCompleteCables()) {
		if (!cw->cable)
			continue;
		// A cable hidden by anything other than this trace — an injector's, which is hidden on
		// purpose — is not something to offer. Without this test one could be picked up by the
		// rotation and then made visible, putting a lead across the rack that is meant to be
		// invisible.
		if (!cw->visible && !hiddenByUs.count(cw->cable->id))
			continue;
		math::Vec p0, c, p1;
		if (!cableCurve(cw, p0, c, p1))
			continue;
		for (int end = 0; end < 2; end++) {
			const bool atInput = (end == 1);
			const math::Vec pill = pointAlong(p0, c, p1, atInput,
				PILL_START + PILL_LENGTH / 2.f);
			if (pill.minus(mouse).norm() < PILL_REACH) {
				HoverCandidate hc;
				hc.cw = cw;
				hc.atInput = atInput;
				hc.id = cw->cable->id;
				candidates.push_back(hc);
			}
		}
	}

	// Sorted by cable id so the rotation has a fixed order. Drawing order would not do: it
	// changes as cables are made and removed, and the order a click steps through must not.
	std::sort(candidates.begin(), candidates.end(),
		[](const HoverCandidate& a, const HoverCandidate& b) {
			return std::make_pair(a.id, a.atInput) < std::make_pair(b.id, b.atInput);
		});

	std::vector<int64_t> key;
	for (const HoverCandidate& hc : candidates)
		key.push_back(hc.id);
	if (key != lastCandidateKey) {
		lastCandidateKey = key;
		turn = 0;
	}
	if (!candidates.empty() && turn >= candidates.size())
		turn = 0;

	// Keep the turn ON the cable that is lit, whenever the lit one is among those under the
	// pointer. The drawn pill then always belongs to the cable you are looking at, instead of
	// to the one the next click would take — which read as every click lagging one behind.
	for (size_t i = 0; i < candidates.size(); i++) {
		if (candidates[i].id == focusedId) {
			turn = i;
			break;
		}
	}

	if (focusedId < 0)
		return;

	holdOpacity();

	// Enforce the dimming. Done every frame rather than once, so a cable made or recoloured
	// while a focus is held is dimmed too rather than standing out for no reason.
	bool focusStillExists = false;
	for (app::CableWidget* cw : APP->scene->rack->getCompleteCables()) {
		if (!cw->cable)
			continue;
		const int64_t id = cw->cable->id;
		// Leave alone anything hidden for its own reasons — an injector's cable is hidden on
		// purpose, and must not be swept up by this.
		if (!cw->visible && !hiddenByUs.count(id))
			continue;

		if (originalColors.find(id) == originalColors.end())
			originalColors[id] = cw->color;
		const NVGcolor base = originalColors[id];
		if (id == focusedId) {
			focusStillExists = true;
			cw->color = nvgRGBAf(base.r, base.g, base.b, 1.f);
			cw->visible = true;
			hiddenByUs.erase(id);
		}
		else {
			cw->visible = false;
			hiddenByUs.insert(id);
		}
	}

	// The lit cable was pulled out: nothing is being traced any more, so let the rack back up
	// rather than leaving every cable dim with no way to tell why.
	if (!focusStillExists)
		cableFocusClear();
}


/** Draws the pill on the hovered end, in the cable's own colour so it reads as a swelling of
the cable rather than a separate object stuck to it. */
void cableFocusDraw(NVGcontext* vg) {
	if (candidates.empty())
		return;
	const HoverCandidate& hc = candidates[turn];
	math::Vec p0, c, p1;
	if (!cableCurve(hc.cw, p0, c, p1))
		return;

	const math::Vec a = pointAlong(p0, c, p1, hc.atInput, PILL_START);
	const math::Vec b = pointAlong(p0, c, p1, hc.atInput, PILL_START + PILL_LENGTH);

	NVGcolor col = hc.cw->color;
	// Drawn at full strength even when everything is dim, or the pill on a dimmed cable would
	// be nearly invisible exactly when you were reaching for it.
	col.a = 1.f;

	nvgBeginPath(vg);
	nvgMoveTo(vg, a.x, a.y);
	nvgLineTo(vg, b.x, b.y);
	nvgStrokeColor(vg, col);
	nvgStrokeWidth(vg, PILL_WIDTH);
	nvgLineCap(vg, NVG_ROUND);
	nvgStroke(vg);

	// A dark outline, so a pale pill still reads against a pale panel.
	nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 120));
	nvgStrokeWidth(vg, 1.2f);
	nvgLineCap(vg, NVG_ROUND);
	nvgStroke(vg);

	// How many pills are stacked here, so it is clear that clicking again reaches another
	// cable rather than doing nothing.
	if (candidates.size() > 1) {
		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (font && font->handle >= 0) {
			nvgFontFaceId(vg, font->handle);
			nvgFontSize(vg, 9.f);
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, nvgRGB(0xff, 0xff, 0xff));
			nvgText(vg, (a.x + b.x) / 2.f, (a.y + b.y) / 2.f - PILL_WIDTH,
				string::f("%d/%d", (int) turn + 1, (int) candidates.size()).c_str(), NULL);
		}
	}

	// A ring on the lit cable's pill, so it is obvious which one is being traced.
	if (hc.cw->cable && hc.cw->cable->id == focusedId) {
		nvgBeginPath(vg);
		nvgCircle(vg, (a.x + b.x) / 2.f, (a.y + b.y) / 2.f, PILL_WIDTH * 0.75f);
		nvgStrokeColor(vg, nvgRGB(0xff, 0xff, 0xff));
		nvgStrokeWidth(vg, 1.4f);
		nvgStroke(vg);
	}
}


/** A plain click on the pill lights that cable, or puts it out if it was already lit.
Returns whether the click was ours. */
bool cableFocusClick() {
	if (candidates.empty())
		return false;
	const int64_t id = candidates[turn].id;

	// With one pill under the pointer, clicking the cable already being traced puts it out.
	// With several, that click belongs to the rotation instead — there is a panel click and
	// Escape for releasing, and losing a step of the rotation would be the greater cost.
	if (id == focusedId && candidates.size() == 1) {
		cableFocusClear();
		INFO("Cable focus: released");
		return true;
	}

	// Clicking the pill of the cable already lit steps to the next one sharing this spot.
	// Clicking any other pill simply takes it.
	size_t target = turn;
	if (id == focusedId)
		target = (turn + 1) % candidates.size();

	// Colours and visibility are captured fresh for the new focus, so switching from one cable
	// to another cannot record an already-hidden state as that cable's original.
	restoreColors();
	holdOpacity();
	focusedId = candidates[target].id;
	turn = target;
	INFO("Cable focus: following cable %lld (%d of %d here)",
		(long long) focusedId, (int) turn + 1, (int) candidates.size());
	return true;
}
