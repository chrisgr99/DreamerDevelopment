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
	return widgetAt<ClipWidget>(APP->scene, scenePos)
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


void clipPurgeDead() {
	std::vector<ClipWidget*> dead;
	for (widget::Widget* child : APP->scene->rack->children) {
		ClipWidget* clip = dynamic_cast<ClipWidget*>(child);
		if (clip && !clip->port && !clip->retargeting)
			dead.push_back(clip);
	}
	for (ClipWidget* clip : dead) {
		// The handle first, or it would be left pointing at freed memory for the rest of the
		// frame — and it is stepped every frame.
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
		APP->scene->rack->removeChild(clip);
		delete clip;
	}
}
