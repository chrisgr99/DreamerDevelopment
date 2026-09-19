/** The grab handle shared by everything that clips onto a terminal — see Clip.hpp. */
#include "Clip.hpp"
#include <ui/ScrollWidget.hpp>
#include "WidgetAt.hpp"

#include <vector>


/** The tab at the jack end of a clip's callout.

Dragging it re-attaches: drop it on another jack and the clip moves there; drop it anywhere
that is not a jack and the clip is removed. Both are the same gesture because they are the
same idea — the tab is where the thing is attached, so taking it off a jack is what
detaching means.

A child of the RACK rather than of the clip it belongs to, because it is drawn out at the
jack, well outside the clip's own box, and Rack offers a click only to a widget whose box
contains the point.
*/
struct ClipHandleWidget : widget::OpaqueWidget {
	ClipWidget* clip = NULL;

	ClipHandleWidget() {
		// The drawn tab's size exactly. Bigger would be easier to hit but would reach back
		// over the jack and swallow the clicks that make cables.
		box.size = math::Vec(CLIP_HANDLE, CLIP_HANDLE);
	}

	/** Sits on the tab each frame, since the clip, its module and the zoom all move. */
	void step() override {
		if (!clip || !clip->parent) {
			visible = false;
			widget::OpaqueWidget::step();
			return;
		}
		math::Vec ring, tab;
		float rr = 0.f;
		visible = clip->calloutGeometry(ring, rr, tab);
		if (visible)
			box.pos = clip->box.pos.plus(tab).minus(box.size.div(2.f));
		widget::OpaqueWidget::step();
	}

	/** Nothing of its own: the tab you see is the one the clip draws. Two tabs a pixel apart
	would look like a rendering fault. */
	void draw(const DrawArgs& args) override {}

	/** DRAGGED, not clicked. Click-carry was tried and taken out again: the clicks around a
	terminal are already spoken for — picking a cable up, stepping through the pills that
	choose one — and a loop that follows the pointer until the next click anywhere turns every
	one of those into a decision about the loop instead.
	*/
	/** Where the tab was held, relative to the pointer, so it does not jump on the first move. */
	math::Vec grabOffset;

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT || !clip)
			return;
		clip->retargeting = true;
		clip->retargetPos = box.pos.plus(box.size.div(2.f));
		grabOffset = clip->retargetPos.minus(APP->scene->rack->getMousePos());
	}

	/** FROM THE POINTER, not by accumulating deltas.

	Deltas describe how far the mouse moved, and while the view is auto-scrolling at the edge
	the rack also moves underneath it — so a tab positioned by adding up deltas drifts away
	from the pointer exactly when it is being taken somewhere off screen. Reading the pointer's
	position in rack coordinates each time is right under any scroll or zoom.
	*/
	void onDragMove(const DragMoveEvent& e) override {
		if (!clip || !clip->retargeting)
			return;
		clip->retargetPos = APP->scene->rack->getMousePos().plus(grabOffset);
	}

	void onDragEnd(const DragEndEvent& e) override {
		if (!clip || !clip->retargeting)
			return;
		clip->retargeting = false;

		app::PortWidget* target = widgetAt<app::PortWidget>(APP->scene->rack, clip->retargetPos);
		if (!target || !target->module || !clip->acceptsPort(target)) {
			// Dropped away from any jack it can use: it is no longer attached to anything,
			// which is the same as not being there.
			INFO("Clip: dropped off its port, removing");
			clip->detach();
			return;
		}
		if (target == clip->port)
			return;

		// STAY WHERE IT IS. The position is held as an offset from the port, so re-attaching to
		// a different terminal would otherwise fling the widget across the rack to keep that
		// offset — often landing on top of something the user was looking at. The offset is
		// recomputed instead, so the connection moves and the face does not.
		const math::Vec keep = clip->box.pos;
		if (!clip->reattach(target))
			return;
		const math::Vec centre = target->getRelativeOffset(
			target->box.zeroPos().getCenter(), APP->scene->rack);
		clip->offset = keep.minus(centre);
	}
};


/** The X that removes a clip, sitting just outside its top-left corner.

Its own widget, and a child of the rack, for the same reason the grab tab is: Rack offers a
click only to a widget whose box contains the point, so anything drawn outside a clip's box
would be visible and unclickable. Its centre sits ON the clip's top-left corner, so it overlaps the frame rather than floating
away from it. Nothing of the face is there to cover, which is what lets it stay up all the
time: no appearing and disappearing to chase, and no moment where it is not yet drawn.
*/
struct ClipCloseWidget : widget::OpaqueWidget {
	ClipWidget* clip = NULL;

	ClipCloseWidget() {
		box.size = math::Vec(13.f, 13.f);
	}

	void step() override {
		if (!clip || !clip->parent) {
			visible = false;
			widget::OpaqueWidget::step();
			return;
		}
		visible = clip->visible;
		box.pos = clip->box.pos.minus(box.size.div(2.f));
		widget::OpaqueWidget::step();
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && clip) {
			clip->detach();
			e.consume(this);
			return;
		}
		widget::OpaqueWidget::onButton(e);
	}

	void draw(const DrawArgs& args) override {
		const float r = box.size.x / 2.f;
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, r, r, r);
		nvgFillColor(args.vg, nvgRGB(0xe0, 0x3b, 0x3b));
		nvgFill(args.vg);

		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, r - 3.f, r - 3.f);
		nvgLineTo(args.vg, r + 3.f, r + 3.f);
		nvgMoveTo(args.vg, r + 3.f, r - 3.f);
		nvgLineTo(args.vg, r - 3.f, r + 3.f);
		nvgStrokeColor(args.vg, nvgRGB(0x10, 0x12, 0x16));
		nvgStrokeWidth(args.vg, 2.f);
		nvgLineCap(args.vg, NVG_ROUND);
		nvgStroke(args.vg);
	}
};


/** ONE RESIZE HANDLE, just outside an edge or a corner of a clip.

Its own widget, and a child of the rack, for the reason the X and the grab tab are: Rack offers
a click only to a widget whose box contains it, and these sit outside the clip's box on purpose.

OUTSIDE, so that making them easy to grab costs the clip nothing. A band inside the edge — which
is what the scope had — takes a strip of the face away from dragging and scrolling, and is
invisible, so it has to be hunted for. These are drawn where they can be seen, and only the
handles answer a click: the gaps between them belong to whatever is behind the clip.

Seven of them, as a marquee has: the middle of each edge and three corners. NOT the top left,
which is where the X that removes the clip already sits.
*/
struct ClipGripWidget : widget::OpaqueWidget {
	ClipWidget* clip = NULL;
	/** Which edge or corner, as (x, y) in {-1, 0, 1}. */
	math::Vec dir;

	/** Drawn: how far a handle stands out from the edge, how long an edge handle is, and how long
	each arm of a corner bracket is. Grabbed: how far out from the edge, and how far in. All in
	SCREEN pixels — divided by the zoom below, so a handle is the same size to the hand however
	far the rack is zoomed out. */
	static constexpr float OUT = 5.f;
	static constexpr float LONG = 16.f;
	static constexpr float ARM = 10.f;
	static constexpr float REACH = 10.f;
	static constexpr float IN = 4.f;
	/** A grab area a little longer than the mark, so a near miss along the edge still catches. */
	static constexpr float GRAB_LONG = 20.f;

	float zoom() {
		const float z = APP->scene->rack ? APP->scene->rack->getAbsoluteZoom() : 1.f;
		return (z > 0.f) ? z : 1.f;
	}

	/** The face's frame: a rounded rectangle stroked one and a half wide, so the OUTSIDE of the
	line lies three quarters of a unit beyond the face's own edge. The scope and the analyser both
	draw it that way. A handle is placed against THAT edge and not against the face, or it sits
	under the line on one side and away from it on the next. In rack units, since the frame is
	part of the picture and scales with it. */
	static constexpr float FRAME_HALF = 0.75f;
	/** The frame's corner radius. A bracket set flush with both straight edges stands off the
	curve at the corner, so it is tucked in by as much as the curve falls away. */
	static constexpr float FRAME_ROUND = 3.f;

	/** The face in THIS widget's coordinates, worked out in step so the drawing and the grab
	area can never disagree about where the edge is. */
	math::Rect face;

	void step() override {
		if (!clip || !clip->parent || !clip->gripsShowing()) {
			visible = false;
			widget::OpaqueWidget::step();
			return;
		}
		visible = true;
		const float z = zoom();
		const float reach = REACH / z, in = IN / z, grab = GRAB_LONG / z;
		// THE FACE, and nothing else. A scope's readout hangs below the face and can be wider
		// than it, but the face is what a handle drags: one out at the corner of the readout
		// would point at an edge that does not move.
		const math::Rect c(clip->box.pos, math::Vec(clip->faceW(), clip->faceHeight));
		const float l = c.pos.x - FRAME_HALF, r = c.pos.x + c.size.x + FRAME_HALF;
		const float t = c.pos.y - FRAME_HALF, b = c.pos.y + c.size.y + FRAME_HALF;
		math::Vec pos, size;
		if (dir.x == 0.f) {
			pos.x = (l + r) / 2.f - grab / 2.f;
			size.x = grab;
		}
		else {
			pos.x = (dir.x < 0.f) ? l - reach : r - in;
			size.x = reach + in;
		}
		if (dir.y == 0.f) {
			pos.y = (t + b) / 2.f - grab / 2.f;
			size.y = grab;
		}
		else {
			pos.y = (dir.y < 0.f) ? t - reach : b - in;
			size.y = reach + in;
		}
		box.pos = pos;
		box.size = size;
		face = math::Rect(math::Vec(l, t).minus(box.pos), math::Vec(r - l, b - t));
		widget::OpaqueWidget::step();
	}

	static int cursorForDir(math::Vec dir) {
		if (dir.x != 0.f && dir.y != 0.f)
			return (dir.x * dir.y > 0.f) ? GLFW_RESIZE_NWSE_CURSOR : GLFW_RESIZE_NESW_CURSOR;
		if (dir.x != 0.f)
			return GLFW_RESIZE_EW_CURSOR;
		return GLFW_RESIZE_NS_CURSOR;
	}

	void onHover(const HoverEvent& e) override {
		if (clip)
			clip->showGrips();
		druiSetCursorShape(cursorForDir(dir));
		widget::OpaqueWidget::onHover(e);
	}

	void onButton(const ButtonEvent& e) override {
		// The LEFT button only. A right-click here belongs to whatever is behind the handle —
		// usually the port the clip is attached to.
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && clip) {
			e.consume(this);
			return;
		}
	}

	void onDragStart(const DragStartEvent& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT)
			e.consume(this);
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (!clip)
			return;
		clip->showGrips();
		druiSetCursorShape(cursorForDir(dir));
		clip->resizeBy(e.mouseDelta.div(zoom()), dir);
	}

	/** In front of the cables, where the clip itself is drawn: a handle painted over by a cable
	running past would be the one thing on the face you cannot see to grab. */
	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 3)
			paint(args.vg);
		widget::OpaqueWidget::drawLayer(args, layer);
	}

	void draw(const DrawArgs& args) override {}

	void paint(NVGcontext* vg) {
		if (!clip)
			return;
		const float z = zoom();
		const float out = OUT / z, len = LONG / z, arm = ARM / z;
		// The outer edge of the frame's line, on each side, and the middle of each run.
		const float l = face.pos.x, r = face.pos.x + face.size.x;
		const float t = face.pos.y, b = face.pos.y + face.size.y;
		const float midX = (l + r) / 2.f, midY = (t + b) / 2.f;
		// How far the rounded corner falls away from the two straight edges that meet there.
		const float tuck = FRAME_ROUND * (1.f - 0.70710678f);

		// ROUNDED WHERE IT FACES OUT, square where it meets the face: the rounded side is the
		// side you see against the rack, and a handle square there looked like a chip off
		// something rather than a control.
		const float round = out * 0.4f;

		nvgBeginPath(vg);
		if (dir.x != 0.f && dir.y != 0.f) {
			// A BRACKET round the outside of the corner: two arms meeting at the corner itself.
			// A shape rather than another rectangle, so a corner is told from an edge at a
			// glance — and its two arms say it moves in two directions at once.
			//
			// Six points: along the outside from one arm's end, round the corner, to the other's,
			// then straight back along the inside. The three on the outside are rounded.
			const float ex = ((dir.x < 0.f) ? l + tuck : r - tuck);
			const float ey = ((dir.y < 0.f) ? t + tuck : b - tuck);
			const float ox = ex + dir.x * out, oy = ey + dir.y * out;
			const float ax = ex - dir.x * arm, ay = ey - dir.y * arm;
			nvgMoveTo(vg, ax, ey);
			nvgArcTo(vg, ax, oy, ox, oy, round);
			nvgArcTo(vg, ox, oy, ox, ay, round);
			nvgArcTo(vg, ox, ay, ex, ay, round);
			nvgLineTo(vg, ex, ay);
			nvgLineTo(vg, ex, ey);
			nvgClosePath(vg);
		}
		else if (dir.x != 0.f) {
			const float x = (dir.x < 0.f) ? l - out : r;
			nvgRoundedRectVarying(vg, x, midY - len / 2.f, out, len,
				(dir.x < 0.f) ? round : 0.f, (dir.x < 0.f) ? 0.f : round,
				(dir.x < 0.f) ? 0.f : round, (dir.x < 0.f) ? round : 0.f);
		}
		else {
			const float y = (dir.y < 0.f) ? t - out : b;
			nvgRoundedRectVarying(vg, midX - len / 2.f, y, len, out,
				(dir.y < 0.f) ? round : 0.f, (dir.y < 0.f) ? round : 0.f,
				(dir.y < 0.f) ? 0.f : round, (dir.y < 0.f) ? 0.f : round);
		}
		nvgFillColor(vg, clip->gripColor());
		nvgFill(vg);
		// A dark outline, so a green handle is still a handle over a green cable.
		nvgStrokeColor(vg, nvgRGBA(0x10, 0x12, 0x16, 0xc0));
		nvgStrokeWidth(vg, 1.f / z);
		nvgStroke(vg);
	}
};


void clipAddGrips(ClipWidget* clip) {
	if (!clip || !clip->resizable())
		return;
	static const math::Vec DIRS[7] = {
		math::Vec(0, -1), math::Vec(0, 1), math::Vec(-1, 0), math::Vec(1, 0),
		math::Vec(1, -1), math::Vec(1, 1), math::Vec(-1, 1),
	};
	for (const math::Vec& d : DIRS) {
		ClipGripWidget* g = new ClipGripWidget;
		g->clip = clip;
		g->dir = d;
		clip->grips.push_back(g);
		APP->scene->rack->addChild(g);
	}
}


void clipAddClose(ClipWidget* clip) {
	if (!clip)
		return;
	ClipCloseWidget* x = new ClipCloseWidget;
	x->clip = clip;
	clip->closeButton = x;
	APP->scene->rack->addChild(x);
}


void clipAddHandle(ClipWidget* clip) {
	if (!clip)
		return;
	ClipHandleWidget* handle = new ClipHandleWidget;
	handle->clip = clip;
	clip->handle = handle;
	APP->scene->rack->addChild(handle);
}


void clipSetVisible(ClipWidget* clip, bool visible) {
	if (!clip)
		return;
	clip->visible = visible;
	if (clip->handle)
		clip->handle->visible = visible;
	if (clip->closeButton)
		clip->closeButton->visible = visible;
	// AND IT STOPS WORKING, not merely showing. Hiding used to leave every scope capturing at
	// audio rate behind the panel, so switching the widgets off looked like removing them and
	// cost exactly as much as leaving them on.
	clip->setSuspended(!visible);
}


bool clipDepositFollowing() {
	bool any = false;
	for (widget::Widget* child : APP->scene->rack->children) {
		ClipWidget* clip = dynamic_cast<ClipWidget*>(child);
		if (clip && clip->following) {
			clip->following = false;
			any = true;
		}
	}
	return any;
}


bool clipFamilyAt(math::Vec scenePos) {
	// Clips by hand rather than by widgetAt, which goes by the box alone: see onVisiblePart.
	for (auto it = APP->scene->rack->children.rbegin(); it != APP->scene->rack->children.rend();
		it++) {
		ClipWidget* clip = dynamic_cast<ClipWidget*>(*it);
		if (!clip || !clip->isVisible())
			continue;
		math::Vec local = scenePos.minus(clip->getRelativeOffset(math::Vec(), APP->scene))
			.div(clip->getAbsoluteZoom());
		if (clip->box.zeroPos().contains(local) && clip->onVisiblePart(local))
			return true;
	}
	return widgetAt<ClipGripWidget>(APP->scene, scenePos)
		|| widgetAt<ClipHandleWidget>(APP->scene, scenePos)
		|| widgetAt<ClipCloseWidget>(APP->scene, scenePos);
}


bool clipRetargeting() {
	for (widget::Widget* child : APP->scene->rack->children) {
		ClipWidget* clip = dynamic_cast<ClipWidget*>(child);
		if (clip && clip->retargeting)
			return true;
	}
	return false;
}


int clipFollowingCount() {
	int n = 0;
	for (widget::Widget* child : APP->scene->rack->children) {
		ClipWidget* clip = dynamic_cast<ClipWidget*>(child);
		if (clip && clip->following)
			n++;
	}
	return n;
}


/** IS THERE STILL A RACK TO WALK? On the way out of the application the scene is destroyed and
takes the rack with it, and anything of ours that runs from a destructor after that point is
walking freed memory. Every function here that reaches for the rack asks this first. */
static bool rackAlive() {
	return APP && APP->scene && APP->scene->rack;
}


/** How many clips are on the rack at all, for the diagnostics window: the difference between
"I removed the widgets" and "the widgets are gone" said as a number. */
int clipCount() {
	if (!rackAlive())
		return 0;
	int n = 0;
	for (widget::Widget* child : APP->scene->rack->children) {
		if (dynamic_cast<ClipWidget*>(child))
			n++;
	}
	return n;
}


/** Takes one clip and its two loose parts off the rack. The clip's own destructor gives up
whatever it was holding — its tap, its slot in whichever table it belongs to — so nothing else
has to be told that it has gone. */
static void clipDestroy(ClipWidget* clip) {
	// The handle first, or it would be left pointing at freed memory for the rest of the frame
	// — and it is stepped every frame.
	if (clip->handle) {
		APP->scene->rack->removeChild(clip->handle);
		delete clip->handle;
		clip->handle = NULL;
	}
	if (clip->closeButton) {
		APP->scene->rack->removeChild(clip->closeButton);
		delete clip->closeButton;
		clip->closeButton = NULL;
	}
	for (widget::Widget* g : clip->grips) {
		APP->scene->rack->removeChild(g);
		delete g;
	}
	clip->grips.clear();
	APP->scene->rack->removeChild(clip);
	delete clip;
}

/** WHETHER A PORT IS ACTUALLY ON SCREEN, which is not what isVisible() answers: that reports
the widget's own flag and nothing else, so a jack inside a hidden container still calls itself
visible. The whole chain up to the rack is asked instead. Clarity's drawing asks the same
question of the controls it draws over. */
static bool portReallyVisible(app::PortWidget* p) {
	for (widget::Widget* w = p; w && w != APP->scene->rack; w = w->parent) {
		if (!w->isVisible())
			return false;
	}
	return true;
}


/** HOW LONG A PORT MAY BE OUT OF SIGHT before the clip on it goes. Long enough that a module
redrawing or rebuilding its panel for a frame or two does not cost anybody a scope. */
static const int CLIP_HIDDEN_FRAMES = 60;


void clipPurgeDead() {
	if (!rackAlive())
		return;
	std::vector<ClipWidget*> dead;
	for (widget::Widget* child : APP->scene->rack->children) {
		ClipWidget* clip = dynamic_cast<ClipWidget*>(child);
		if (!clip)
			continue;
		// A JACK THAT HAS BEEN HIDDEN takes its clip with it. Venom's Envelope Factory hides the
		// stages above the number in use, and a scope left on one of those jacks went on sitting
		// in mid air reading a port that is no longer part of the module. Reported from testing.
		//
		// The clip is detached rather than hidden: the port it names may never come back, and a
		// mute detaching is what puts the cables it was holding back into the patch.
		if (clip->port && !clip->retargeting) {
			if (portReallyVisible(clip->port)) {
				clip->portHidden = 0;
			}
			else if (++clip->portHidden > CLIP_HIDDEN_FRAMES) {
				clip->detach();
			}
		}
		if (!clip->port && !clip->retargeting)
			dead.push_back(clip);
	}
	for (ClipWidget* clip : dead)
		clipDestroy(clip);
}

/** EVERYTHING GOES, which is what the last Test Gear leaving the rack means.

A clip belongs to Test Gear even though it is not inside it: the module is what captures the
signal, what mixes the monitors and what saves them all with the patch. With the module gone they
are attached to nothing, cannot be reopened, and are not saved — so leaving them on the rack
leaves furniture nobody can move or get rid of.

Nothing is lost by it. The scopes and the rest are written into the module's own JSON, so
undoing the deletion brings the module back with its data and the clips are made again from it. */
void clipRemoveAll() {
	if (!rackAlive())
		return;
	std::vector<ClipWidget*> all;
	for (widget::Widget* child : APP->scene->rack->children) {
		if (ClipWidget* clip = dynamic_cast<ClipWidget*>(child))
			all.push_back(clip);
	}
	for (ClipWidget* clip : all)
		clipDestroy(clip);
}
