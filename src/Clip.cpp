/** The grab handle shared by everything that clips onto a terminal — see Clip.hpp. */
#include "Clip.hpp"
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

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT || !clip)
			return;
		clip->retargeting = true;
		clip->retargetPos = box.pos.plus(box.size.div(2.f));
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (!clip || !clip->retargeting)
			return;
		// Rack reports drag movement in screen pixels, so it has to be divided by the rack's
		// zoom to become rack distance — otherwise the tab outruns the pointer when zoomed in.
		clip->retargetPos = clip->retargetPos.plus(e.mouseDelta.div(getAbsoluteZoom()));
	}

	void onDragEnd(const DragEndEvent& e) override {
		if (!clip || !clip->retargeting)
			return;
		clip->retargeting = false;

		app::PortWidget* target = widgetAt<app::PortWidget>(APP->scene->rack, clip->retargetPos);
		if (!target || !target->module || !clip->acceptsPort(target)) {
			// Dropped away from any jack it can use: it is no longer attached to anything,
			// which is the same as not being there.
			INFO("Clip: dropped off its terminal, removing");
			clip->detach();
			return;
		}
		if (target == clip->port)
			return;
		clip->reattach(target);
	}
};


void clipAddHandle(ClipWidget* clip) {
	if (!clip)
		return;
	ClipHandleWidget* handle = new ClipHandleWidget;
	handle->clip = clip;
	clip->handle = handle;
	APP->scene->rack->addChild(handle);
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
		APP->scene->rack->removeChild(clip);
		delete clip;
	}
}
