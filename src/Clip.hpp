#pragma once
/** Things that clip onto a terminal: the scope, and the signal injectors.

They share their whole attachment behaviour, so it lives here rather than being copied. A
clip is anchored to the PORT it is attached to, not to the screen — it is a child of the
RackWidget and its position is recomputed each frame from that port, so it scrolls and zooms
with the module and follows if the module is moved.

What each subclass supplies is only what it means to be attached: reattach() moves whatever
carries its signal, and detach() gives it up.
*/
#include "plugin.hpp"

#include <app/PortWidget.hpp>


/** Wcoast CALLOUT_COLOR, the same grey as a scope's frame, so the loop reads as part of the
instrument rather than as decoration. */
static const NVGcolor CLIP_CALLOUT_COLOR = nvgRGB(0xac, 0xb0, 0xb6);
/** Wcoast SCOPE_HANDLE, and also the shortest the loop-to-face line may get. */
static const float CLIP_HANDLE = 10.5f;


/** Declared here so the gates can tell a scope's handle from an injector's. */
struct ClipHandleWidget;
struct ClipCloseWidget;


struct ClipWidget : widget::OpaqueWidget {
	WeakPtr<app::PortWidget> port;
	/** Offset from the port's centre, which is what makes the clip follow its module. */
	math::Vec offset = math::Vec(30, -70);
	/** The face's own size, which is NOT always the widget's box.
	
	A scope's box grows downwards for its readout and sideways when that readout is wider than
	the trace, so measuring the callout against the box aimed it at the corner of something the
	user cannot see. The callout aims at the FACE.
	
	Zero width means "the same as the box", which is true of every injector. */
	float faceWidth = 0.f;
	float faceHeight = 110.f;

	float faceW() {
		return (faceWidth > 0.f) ? faceWidth : box.size.x;
	}

	/** The grab tab where the callout meets the jack. A separate widget because it is drawn
	OUTSIDE this widget's box, at the jack, and Rack offers a click only to a widget whose box
	contains it — as part of this widget the tab would be visible but ungrabbable. */
	ClipHandleWidget* handle = NULL;
	/** The X that removes this clip, drawn OUTSIDE it at the top left. Outside because a face
	forty pixels across has no room to spare for a control that is used once. */
	ClipCloseWidget* closeButton = NULL;

	/** While the tab is being dragged the callout points here, in rack coordinates, instead
	of at the port, so the loop follows the pointer to wherever it is being taken. */
	bool retargeting = false;
	math::Vec retargetPos;
	/** Newly made, and riding the pointer until a click puts it down.

	A widget dropped at a fixed offset from its jack lands wherever that offset happens to
	point — often over something you were looking at. Carrying it means the first thing you do
	is choose where it goes, which is what you were going to do anyway. */
	bool following = false;

	/** Moves whatever carries this clip's signal to a new port. Returns false to refuse, in
	which case the clip stays where it was. */
	virtual bool reattach(app::PortWidget* target) = 0;
	/** Gives up the attachment. The default marks the clip dead for the purge, which is the
	only safe way to remove a widget during an event. */
	virtual void detach() {
		port = NULL;
	}
	/** Whether this clip can attach to a port at all. Injectors refuse outputs: a signal is
	injected INTO something. */
	virtual bool acceptsPort(app::PortWidget* target) {
		return target != NULL;
	}

	/** Anchors to the port. Call from step(). */
	void followPort() {
		if (following) {
			followPointer();
			return;
		}
		if (!port)
			return;
		const math::Vec centre = port->getRelativeOffset(
			port->box.zeroPos().getCenter(), APP->scene->rack);
		box.pos = centre.plus(offset);
	}

	/** Rides the pointer, held by the middle of its left edge — or its right edge when there is
	no room to the right, so a widget made near the edge of the view is still fully visible. */
	void followPointer() {
		const math::Vec mouse = APP->scene->rack->getMousePos();
		const float zoom = APP->scene->rackScroll ? APP->scene->rackScroll->getAbsoluteZoom() : 1.f;
		const float viewRight = APP->scene->rackScroll
			? (APP->scene->rackScroll->offset.x + APP->scene->rackScroll->box.size.x) / zoom
			: mouse.x + box.size.x + 1.f;

		const bool room = (mouse.x + box.size.x + 8.f) < viewRight;
		box.pos = math::Vec(room ? mouse.x + 4.f : mouse.x - box.size.x - 4.f,
			mouse.y - box.size.y / 2.f);

		// Keep the offset in step, so putting it down leaves it exactly where it is drawn.
		if (port) {
			const math::Vec centre = port->getRelativeOffset(
				port->box.zeroPos().getCenter(), APP->scene->rack);
			offset = box.pos.minus(centre);
		}
	}

	bool ringPos(math::Vec& outCentre, float& outRadius) {
		if (retargeting) {
			outCentre = retargetPos.minus(box.pos);
			outRadius = 9.f;
			return true;
		}
		if (!port)
			return false;
		const math::Vec centre = port->getRelativeOffset(
			port->box.zeroPos().getCenter(), APP->scene->rack);
		outCentre = centre.minus(box.pos);
		outRadius = std::fmin(port->box.size.x, port->box.size.y) / 2.f + 2.f;
		return true;
	}

	/** The centre of whichever side of the FACE is nearest a given point.

	Every callout meets the face here — the trace's and the trigger's alike — so they behave
	the same way as the widget moves. Aiming at the nearest POINT on an edge instead lets the
	attachment slide along that edge, which reads as the line crawling round the frame.
	*/
	math::Vec nearestSideCentre(math::Vec ring) {
		const float fw = faceW();
		const math::Vec sides[4] = {
			math::Vec(fw / 2.f, 0.f),               // top
			math::Vec(fw / 2.f, faceHeight),        // bottom
			math::Vec(0.f, faceHeight / 2.f),       // left
			math::Vec(fw, faceHeight / 2.f),        // right
		};
		int best = 0;
		float bestDist = sides[0].minus(ring).norm();
		for (int i = 1; i < 4; i++) {
			const float d = sides[i].minus(ring).norm();
			if (d < bestDist) {
				bestDist = d;
				best = i;
			}
		}
		return sides[best];
	}

	/** Where the callout's ring, its grab tab, and the point on the face it aims at sit — all
	in this widget's coordinates.

	Worked out ONCE, here, and used by both the drawing and the handle widget. It used to be
	computed in two places from the same copied lines, which is how a tab could end up on a
	different side of the face from the line it belongs to.

	The face is met at the CENTRE of whichever side is nearest the jack, chosen by comparing
	the four side centres against the ring. Aiming at the nearest POINT on the edge instead
	lets the attachment slide along that edge as the face moves, which reads as the line
	crawling round the frame rather than being fixed to it.
	*/
	bool calloutGeometry(math::Vec& ring, float& rr, math::Vec& tab, math::Vec& target) {
		if (!ringPos(ring, rr))
			return false;

		target = nearestSideCentre(ring);

		math::Vec dir = target.minus(ring);
		const float dist = dir.norm();
		if (dist < 1e-3f) {
			// The jack is right where the face meets it — dragged onto its own terminal. Any
			// direction will do, and picking one is better than drawing nothing: returning
			// false here left the callout missing until the widget was moved away again.
			dir = math::Vec(0.f, -1.f);
		}
		else {
			dir = dir.div(dist);
		}
		tab = ring.plus(dir.mult(rr + CLIP_HANDLE / 2.f));
		return true;
	}

	bool calloutGeometry(math::Vec& ring, float& rr, math::Vec& tab) {
		math::Vec target;
		return calloutGeometry(ring, rr, tab, target);
	}

	/** Ring around the terminal, a line to this face, and the rounded tab where they meet.
	Draw BEFORE the face, so the face covers whatever the line runs under. */
	void drawCallout(NVGcontext* vg) {
		math::Vec ring, tab, target;
		float rr = 0.f;
		if (!calloutGeometry(ring, rr, tab, target))
			return;

		math::Vec dir = target.minus(ring);
		const float dist = dir.norm();
		dir = (dist < 1e-3f) ? math::Vec(0.f, -1.f) : dir.div(dist);

		// The line starts clear of the ring, leaving room for the tab.
		const math::Vec from = ring.plus(dir.mult(rr));
		nvgBeginPath(vg);
		nvgMoveTo(vg, from.x, from.y);
		nvgLineTo(vg, target.x, target.y);
		nvgStrokeColor(vg, CLIP_CALLOUT_COLOR);
		nvgStrokeWidth(vg, 1.6f);
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgCircle(vg, ring.x, ring.y, rr);
		nvgStrokeWidth(vg, 1.8f);
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRoundedRect(vg, tab.x - CLIP_HANDLE / 2.f, tab.y - CLIP_HANDLE / 2.f,
			CLIP_HANDLE, CLIP_HANDLE, 3.f);
		nvgFillColor(vg, CLIP_CALLOUT_COLOR);
		nvgFill(vg);
	}
};


/** Creates a clip's grab handle and adds it to the rack. Call right after adding the clip,
so the handle is offered the click first — the tab sits beside the jack, which would
otherwise take the press. */
void clipAddHandle(ClipWidget* clip);

/** Gives a clip an X centred on its top-left corner, always visible. */
void clipAddClose(ClipWidget* clip);

/** Shows or hides a clip AND its grab handle together. The handle is a separate widget owned
by the rack, so hiding a clip alone would leave its tab floating at the jack. */
void clipSetVisible(ClipWidget* clip, bool visible);

/** Removes clips that have been detached, and their handles. Called from the overlay's step,
because a widget cannot safely delete itself while the tree is being walked. */
void clipPurgeDead();

/** Whether any part of a clip is under this scene position — the face, its grab tab, or its
close button.

THE WHOLE FAMILY, not just the face. The tab sits ON the jack and the close button on the
corner, and both are separate widgets owned by the rack, so a test for the face alone said
"nothing of ours here" at exactly the two places most likely to be over a terminal. Every
gesture that searches the rack for a jack asks this first, so nothing done to one of our
widgets also reaches what is behind it.
*/
bool clipFamilyAt(math::Vec scenePos);

/** Puts down any clip that is riding the pointer. True if one was. */
bool clipDepositFollowing();

/** How many clips are riding the pointer. Diagnostic. */
int clipFollowingCount();
