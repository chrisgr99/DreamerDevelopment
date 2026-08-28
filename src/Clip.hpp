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
static const NVGcolor CLIP_CALLOUT_COLOR = nvgRGB(0x8a, 0x8d, 0x92);
/** Wcoast SCOPE_HANDLE, and also the shortest the loop-to-face line may get. */
static const float CLIP_HANDLE = 10.5f;


/** Declared here so the gates can tell a scope's handle from an injector's. */
struct ClipHandleWidget;


struct ClipWidget : widget::OpaqueWidget {
	WeakPtr<app::PortWidget> port;
	/** Offset from the port's centre, which is what makes the clip follow its module. */
	math::Vec offset = math::Vec(30, -70);
	/** The face's own height. The callout aims at the face, which for a scope is shorter than
	the whole widget once its values box is open. */
	float faceHeight = 110.f;

	/** The grab tab where the callout meets the jack. A separate widget because it is drawn
	OUTSIDE this widget's box, at the jack, and Rack offers a click only to a widget whose box
	contains it — as part of this widget the tab would be visible but ungrabbable. */
	ClipHandleWidget* handle = NULL;
	/** While the tab is being dragged the callout points here, in rack coordinates, instead
	of at the port, so the loop follows the pointer to wherever it is being taken. */
	bool retargeting = false;
	math::Vec retargetPos;

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
		if (!port)
			return;
		const math::Vec centre = port->getRelativeOffset(
			port->box.zeroPos().getCenter(), APP->scene->rack);
		box.pos = centre.plus(offset);
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

	/** Where the callout's ring and grab tab sit, in this widget's coordinates. Shared by the
	drawing and by the handle widget, so what you see is exactly what you can grab. */
	bool calloutGeometry(math::Vec& ring, float& rr, math::Vec& tab) {
		if (!ringPos(ring, rr))
			return false;

		// Attach to the CENTRE of the side nearest the jack. Aiming at the nearest POINT on
		// the edge instead lets the attachment slide along that edge as the face moves, which
		// reads as the line crawling round the frame rather than being fixed to it. Which side
		// is nearest is judged in units of the face's own half-width and half-height, so a wide
		// face does not always answer "left or right".
		const math::Vec faceCentre = math::Vec(box.size.x / 2.f, faceHeight / 2.f);
		const math::Vec away = ring.minus(faceCentre);
		math::Vec target;
		if (std::fabs(away.x) / (box.size.x / 2.f) > std::fabs(away.y) / (faceHeight / 2.f))
			target = math::Vec(away.x > 0.f ? box.size.x : 0.f, faceCentre.y);
		else
			target = math::Vec(faceCentre.x, away.y > 0.f ? faceHeight : 0.f);

		math::Vec dir = target.minus(ring);
		const float dist = dir.norm();
		if (dist < 1e-3f)
			return false;
		dir = dir.div(dist);
		tab = ring.plus(dir.mult(rr + CLIP_HANDLE / 2.f));
		return true;
	}

	/** Ring around the terminal, a line to this face, and the rounded tab where they meet.
	Draw BEFORE the face, so the face covers whatever the line runs under. */
	void drawCallout(NVGcontext* vg) {
		math::Vec ring, tab;
		float rr = 0.f;
		if (!calloutGeometry(ring, rr, tab))
			return;

		const math::Vec faceCentre = math::Vec(box.size.x / 2.f, faceHeight / 2.f);
		const math::Vec away = ring.minus(faceCentre);
		math::Vec target;
		if (std::fabs(away.x) / (box.size.x / 2.f) > std::fabs(away.y) / (faceHeight / 2.f))
			target = math::Vec(away.x > 0.f ? box.size.x : 0.f, faceCentre.y);
		else
			target = math::Vec(faceCentre.x, away.y > 0.f ? faceHeight : 0.f);

		math::Vec dir = target.minus(ring);
		const float dist = dir.norm();
		if (dist < 1e-3f)
			return;
		dir = dir.div(dist);

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

/** Shows or hides a clip AND its grab handle together. The handle is a separate widget owned
by the rack, so hiding a clip alone would leave its tab floating at the jack. */
void clipSetVisible(ClipWidget* clip, bool visible);

/** Removes clips that have been detached, and their handles. Called from the overlay's step,
because a widget cannot safely delete itself while the tree is being walked. */
void clipPurgeDead();
