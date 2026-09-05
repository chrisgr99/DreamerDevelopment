/** Gestures that have to be seen before anything else gets them.

Two so far: the scroll wheel adjusting sliders, and Option-click on a jack offering the
things that can be clipped onto it.

WHY THIS IS NOT SIMPLY A SCROLL HANDLER ON THE SLIDER. A plugin cannot change ui::Slider,
and Slider inherits OpaqueWidget, whose scroll handler calls stopPropagating(). A scroll
over a slider therefore DIES at the slider — not consumed, but never offered to anything
else — so no widget placed anywhere in the scene can pick it up afterwards.

The way round it is to be offered the event before the menu rather than after. Events start
at the Scene and are offered to its children in REVERSE order, so the last child gets first
refusal. Rack appends the menu overlay when a menu opens, which is why the menu currently
wins; this widget moves itself back to the end of that list whenever it is not already
there, and so gets first refusal instead.

That is a strong position to hold, so this handles only the two gestures above and passes
everything else through untouched. A scroll acts only when the widget under the pointer is a
Slider; otherwise the event carries on to whatever would have received it — a long menu's own
scrolling, knob scroll, the rack view. A press acts only when Option is held over a jack.

It matters because dragging a slider calls cursorLock(), which fights a screen magnifier
that follows the pointer, leaving the wheel as the only practical route.
*/
#include "plugin.hpp"

#include <ui/Slider.hpp>
#include <ui/Menu.hpp>
#include <ui/MenuOverlay.hpp>
#include <app/ParamWidget.hpp>
#include <ui/ScrollWidget.hpp>
#include <app/CableWidget.hpp>
#include <history.hpp>
#include <ui/ScrollWidget.hpp>
#include <app/PortWidget.hpp>
#include "Injector.hpp"
#include "WidgetAt.hpp"
#include "Clip.hpp"
#include "Monitor.hpp"
#include "Meter.hpp"
#include "Palette.hpp"

#include <algorithm>
#include <list>
#include <vector>

#include <cmath>
#include <vector>
#include <string>

#include <algorithm>


/** Whether a press at `pos` is meant for a cable's pill rather than for a terminal under it.

True when there is no terminal under the press at all, and true as well when there is one but
the pill is drawn nearer to the press than the terminal's own centre — measured in the rack's
coordinates, so it holds at any zoom. */
static bool pillBeatsPortAt(math::Vec pos) {
	app::PortWidget* port = widgetAt<app::PortWidget>(APP->scene, pos);
	if (!port)
		return true;
	math::Vec pill;
	if (!cableFocusPillPos(pill))
		return false;
	const math::Vec mouse = APP->scene->rack->getMousePos();
	const math::Vec centre = port->getRelativeOffset(port->box.size.div(2.f),
		APP->scene->rack);
	return pill.minus(mouse).norm() < centre.minus(mouse).norm();
}


struct InterceptOverlay : widget::Widget {
	bool* sliderScroll = NULL;
	bool* clickCables = NULL;
	/** What the Option-click menu is allowed to offer, and whether a cable's pill appears at
	all. Switched off on the face, these simply stop being offered; anything already attached
	is hidden by its own gate rather than destroyed here. */
	bool* offerScopes = NULL;
	bool* offerWidgets = NULL;
	bool* trace = NULL;
	/** Draws a pointer into the rack itself, for recordings made with VCV Recorder.

	The Recorder captures Rack's own framebuffer, and the mouse cursor is not in it — macOS
	composites the cursor over the window afterwards. So a video recorded that way shows cables
	leaping about with nothing causing them. This draws a pointer where the real one is, flashes
	it when a button goes down, and names any modifier being held, which is the difference
	between a demonstration and a conjuring trick.
	*/
	bool* demoPointer = NULL;
	/** The value readout, switched separately from both. */
	bool* demoValues = NULL;
	double pressTime = -1e9;
	bool pressed = false;
	int pressedButton = 0;
	/** When the wheel last moved, and which way, for the scroll chevrons. */
	double scrollTime = -1e9;
	math::Vec scrollDir;
	/** When the run last changed axis, so a settled gesture may claim a new one. */
	double scrollDirTime = -1e9;
	/** The opening movement of a gesture, summed until it is decisive. */
	math::Vec claim;
	bool claimed = false;
	/** The control the wheel was last used on. Rack's knob scroll changes a param WITHOUT
	dragging anything, so the dragged-widget readout never saw it — turning a knob by wheel
	showed no value at all, which is the one case where the value matters most. */
	WeakPtr<app::ParamWidget> scrollParam;
	/** What that parameter read before the wheel reached it, and whether it has moved since.

	THE READOUT IS FOR A VALUE BEING CHANGED, and a wheel over a knob does not always change
	one: Rack only lets the wheel turn a knob when "knob scroll" is switched on, and scrolling
	the RACK moves the modules under a pointer that has not moved at all — so parameter after
	parameter passed beneath it and each was announced as though it had been turned. Reported
	from the forum.

	Asking whether the value actually moved answers it exactly, and keeps answering it whatever
	Rack decides the wheel should do to a knob in some later version. */
	float scrollParamWas = 0.f;
	bool scrollParamMoved = false;
	/** The readout that lingers: what it said, when it was last true, and where it sat. */
	std::string valueText;
	double valueTime = -1e9;
	math::Vec valuePos;
	/** THE WIDEST IT HAS BEEN for the control being turned, and which control that is.

	A unit lives on the end of the value — "1.2 V" — so padding the part after the point to a
	fixed number of characters does not hold the width: two decimals and a unit is already wider
	than the pad, and the plate grew and shrank as Rack changed its mind about how many figures
	to show. A high-water mark holds it. The plate widens once, the first time a value needs the
	room, and then stays there for as long as you are on that control. */
	WeakPtr<app::ParamWidget> valueOwner;
	size_t valueWidth = 0;
	/** True between the click that picks a cable up and the click that puts it down. Rack
	believes a drag is in progress the whole time. */
	bool carrying = false;
	/** Where the carry began, so a RELEASE can be told apart from a click.

	This is what makes holding the button and not holding it the same gesture. Rack's way is
	press, drag, release; ours is click, move, click. They differ only in what the release
	means — and if the pointer has travelled since the press, the release plainly ends a drag,
	so it lands the cable. If it has not, the press was a click and the cable stays in hand.
	Nobody has to know which mode they are in, because both are true at once.
	*/
	math::Vec carryStart;
	/** A jack that was just right-clicked, and how many frames we will wait for Rack's own
	menu to appear so ours can be added to it. */
	WeakPtr<app::PortWidget> menuPort;
	int menuWait = 0;
	/** A chooser we opened, waiting to be positioned once its height is known. */
	WeakPtr<ui::Menu> chooser;
	math::Vec chooserAt;


	/** The cable being carried, if any. Owned by the rack like any other; what makes it
	"carried" is simply that one of its ends is not connected to anything.

	Injecting a press and withholding the release was the obvious way to do this and it does
	not work. Rack sets the dragged widget to WHOEVER CONSUMED the press — so consuming the
	click to stop the port acting on it also replaced the port's drag with ours, and no cable
	appeared. Not consuming it means the ordinary drag starts and the ordinary release ends it.

	None of that machinery is needed. An incomplete cable draws its loose end at the rack's
	mouse position all by itself, so carrying one is just a matter of leaving an end unset. The
	cable is made, moved and completed here, with Rack's own history actions so undo behaves as
	it always does.
	*/
	WeakPtr<app::CableWidget> carried;

	/** THE UNDO ENTRY FOR TAKING A CABLE OFF, MADE BUT NOT YET PUSHED.

	It has to be made at the moment the cable is lifted, since that is when its old ports are
	still known — but it must not be pushed until the gesture has decided what it is. Clicking
	round the cycle below lifts and replaces cables several times over, and pushing as we went
	would leave an undo entry per click for a gesture the user thinks of as one act.
	*/
	history::CableRemove* pendingRemove = NULL;
	/** The port the carried cable was taken off, so it can be put back on it. */
	WeakPtr<app::PortWidget> carriedFrom;
	/** True while the carried cable is one WE made, so putting it back means deleting it. */
	bool carriedIsNew = false;

	/** Clicking the same jack again reaches past the cable on top of it.

	An output can hold several cables and a click could only ever take the one on top, so the
	other cables under it — and starting a NEW cable from an output that already has one, which
	is ordinary practice — were unreachable. Rack answers this with a modifier key. Repeated
	clicks answer it without one: each click on the same jack, with the pointer still on it,
	swaps what is in your hand for the next thing that jack can offer — the cables on it in
	turn, then a new cable, then nothing, then round again.

	Nothing has to be labelled, because the states already look different: a held cable is
	drawn at full strength while the rest are at half, an existing cable still runs to wherever
	its far end is plugged, and a new one hangs from the jack you clicked.
	*/
	WeakPtr<app::PortWidget> cyclePort;
	std::vector<app::CableWidget*> cycleCables;
	int cycleIndex = -1;
	math::Vec cycleAt;

	/** Lifts ONE named cable off the end its pill sits on.

	Right-click, because the left clicks are already spoken for: with several cables converging
	on a jack their pills stack, and clicking steps through them to choose. A second left click
	could not both rotate and lift, and the case where you most need to name a cable is exactly
	the case where the rotation matters. So choosing stays on the left button and taking moves
	to the right.
	*/
	void pickUpCable(app::CableWidget* cw, bool atInput) {
		if (!cw || !cw->cable)
			return;
		// The trace goes with it: the cable being carried is no longer one of the ones on
		// screen to compare against, and leaving the rest hidden would be baffling.
		cableFocusClear();

		// Provisional, like every other pickup: the undo entry is made now, while the cable's
		// old ports are still known, and pushed only if the cable is actually put somewhere.
		history::CableRemove* h = new history::CableRemove;
		h->setCable(cw);
		delete pendingRemove;
		pendingRemove = h;
		carriedIsNew = false;
		carriedFrom = atInput ? cw->inputPort : cw->outputPort;
		cw->getPort(atInput ? engine::Port::INPUT : engine::Port::OUTPUT) = NULL;
		cw->updateCable();
		carried = cw;
		carrying = true;
		carryStart = APP->scene->getMousePos();
		endCycle();
	}

	/** Picks up the top cable on this port, or starts a new one from it. */
	void pickUp(app::PortWidget* port) {
		if (!port || !port->module)
			return;

		// THE LIST IS READ FIRST, before anything is lifted. Reading it afterwards left the
		// cable now in the hand out of it — so the second cable sat at index 0 while the first
		// was being held, and the next click stepped past it to a new cable. The cable on top
		// has to be index 0 of the same list the cycle walks.
		cyclePort = port;
		cycleCables = APP->scene->rack->getCompleteCablesOnPort(port);
		// Top first: Rack keeps them in the order they were made, and the last is the one a
		// click lands on.
		std::reverse(cycleCables.begin(), cycleCables.end());
		cycleIndex = 0;
		cycleAt = APP->scene->getMousePos();

		if (!cycleCables.empty())
			liftExisting(cycleCables[0], port);
		else
			liftNew(port);
	}

	/** Takes a cable off this end of the port, keeping the undo entry back until the gesture
	is over. */
	void liftExisting(app::CableWidget* cw, app::PortWidget* port) {
		history::CableRemove* h = new history::CableRemove;
		h->setCable(cw);
		delete pendingRemove;
		pendingRemove = h;
		carriedIsNew = false;
		carriedFrom = port;

		cw->getPort(port->type) = NULL;
		cw->updateCable();
		carried = cw;
		carrying = true;
		carryStart = APP->scene->getMousePos();
	}

	/** Starts a new cable at this port, with its other end in the hand. */
	void liftNew(app::PortWidget* port) {
		app::CableWidget* cw = new app::CableWidget;
		cw->getPort(port->type) = port;
		cw->updateCable();
		APP->scene->rack->addCable(cw);
		delete pendingRemove;
		pendingRemove = NULL;
		carriedIsNew = true;
		carriedFrom = port;
		carried = cw;
		carrying = true;
		carryStart = APP->scene->getMousePos();
	}

	/** Puts back whatever is in the hand, leaving no trace in the undo history: a cable we
	made is deleted, and one we lifted goes back on the port it came from. */
	void returnHeld() {
		app::CableWidget* cw = carried;
		carried = NULL;
		carrying = false;
		if (!cw) {
			delete pendingRemove;
			pendingRemove = NULL;
			return;
		}
		if (carriedIsNew) {
			discard(cw);
		}
		else if (carriedFrom) {
			cw->getPort(carriedFrom->type) = carriedFrom;
			cw->updateCable();
		}
		else {
			// Nowhere to put it back: the module it came from has gone. Letting go of it is
			// the only honest thing left, and the withheld undo entry describes exactly that.
			if (pendingRemove) {
				APP->history->push(pendingRemove);
				pendingRemove = NULL;
			}
			discard(cw);
		}
		delete pendingRemove;
		pendingRemove = NULL;
	}

	/** One click further round: the cables on the jack in turn, then a new cable, then an
	empty hand, then back to the first. The jack is read afresh each time round, so a cable
	added or removed meanwhile is accounted for. */
	void advanceCycle() {
		app::PortWidget* port = cyclePort;
		if (!port)
			return;
		returnHeld();

		const int cables = (int) cycleCables.size();
		cycleIndex++;
		if (cycleIndex > cables + 1) {
			cycleCables = APP->scene->rack->getCompleteCablesOnPort(port);
			std::reverse(cycleCables.begin(), cycleCables.end());
			cycleIndex = 0;
		}

		if (cycleIndex < (int) cycleCables.size())
			liftExisting(cycleCables[cycleIndex], port);
		else if (cycleIndex == (int) cycleCables.size())
			liftNew(port);
		// Otherwise the hand is empty, which is a state of the cycle rather than the end of it.
	}

	void endCycle() {
		cyclePort = NULL;
		cycleCables.clear();
		cycleIndex = -1;
	}

	/** Drops what is being carried onto this port, or discards it if the port cannot take it. */
	void dropOn(app::PortWidget* port) {
		carrying = false;
		endCycle();
		app::CableWidget* cw = carried;
		carried = NULL;
		// NOW the removal is recorded: the gesture has decided what it was, so undo has one
		// entry for it rather than one per click of the cycle.
		if (pendingRemove) {
			APP->history->push(pendingRemove);
			pendingRemove = NULL;
		}
		if (!cw)
			return;

		const engine::Port::Type wanted = cw->inputPort ? engine::Port::OUTPUT
			: engine::Port::INPUT;
		if (!port || !port->module || port->type != wanted) {
			discard(cw);
			return;
		}

		// An input takes one cable. Rack replaces what is there when you drop on an occupied
		// input, so this does the same rather than inventing a third behaviour.
		if (port->type == engine::Port::INPUT) {
			for (app::CableWidget* other : APP->scene->rack->getCompleteCablesOnPort(port)) {
				history::CableRemove* h = new history::CableRemove;
				h->setCable(other);
				APP->history->push(h);
				APP->scene->rack->removeCable(other);
				delete other;
			}
		}

		cw->getPort(port->type) = port;
		cw->updateCable();
		if (!cw->isComplete()) {
			discard(cw);
			return;
		}
		history::CableAdd* h = new history::CableAdd;
		h->setCable(cw);
		APP->history->push(h);
	}

	void discard(app::CableWidget* cw) {
		if (!cw)
			return;
		APP->scene->rack->removeCable(cw);
		delete cw;
	}

	/** Right-click while carrying: put back what is in the hand and leave the patch as it was.
	Nothing reaches the undo history, because nothing happened. */
	void cancelCarry() {
		returnHeld();
		endCycle();
	}

	/** Scrolls the rack when a carried cable, or a widget's connection, is taken to the edge of
	the view.

	Needed because the pointer cannot leave the window: without it a cable could only ever be
	dropped on something already on screen, and a scope could only ever be moved to a terminal
	that happened to be in view. Only while something is in hand — a rack that slid about
	whenever the pointer neared an edge would be unusable.
	*/
	void autoScrollWhileCarrying() {
		if (!carrying && !clipRetargeting())
			return;
		ui::ScrollWidget* scroll = APP->scene->rackScroll;
		if (!scroll)
			return;

		const math::Vec mouse = APP->scene->mousePos;
		const math::Rect view = scroll->box;
		const float margin = 45.f;
		const float speed = 14.f;
		math::Vec push;

		if (mouse.x < view.pos.x + margin)
			push.x = -(margin - (mouse.x - view.pos.x));
		else if (mouse.x > view.pos.x + view.size.x - margin)
			push.x = margin - (view.pos.x + view.size.x - mouse.x);
		if (mouse.y < view.pos.y + margin)
			push.y = -(margin - (mouse.y - view.pos.y));
		else if (mouse.y > view.pos.y + view.size.y - margin)
			push.y = margin - (view.pos.y + view.size.y - mouse.y);

		if (push.x == 0.f && push.y == 0.f)
			return;
		// Proportional to how far into the margin the pointer is, so easing up to the edge
		// creeps and pressing into it travels.
		scroll->offset = scroll->offset.plus(push.div(margin).mult(speed));
	}

	void step() override {
		// Cover the scene, or the event system will not offer us events outside our box.
		if (parent)
			box.size = parent->box.size;

		// WHETHER A BUTTON IS ACTUALLY DOWN, asked of the window rather than counted from
		// events.
		//
		// Counting releases did not work and could not be made to. Rack LOCKS THE CURSOR while
		// a knob is being turned, and while it is locked Rack dispatches no button events at
		// all — so the release that ended a knob drag never arrived, the pointer stayed drawn
		// as held, and it took a click somewhere else to clear it. Reported by DaveVenom.
		//
		// The window knows, so the window is asked. Nothing can get this stuck.
		if (APP->window && APP->window->win) {
			const bool down =
				glfwGetMouseButton(APP->window->win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS
				|| glfwGetMouseButton(APP->window->win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
			if (!down && pressed)
				pressed = false;
		}

		// Whether the wheel actually turned the parameter it was over. Watched here rather than
		// decided in the scroll handler, because Rack changes the value after we have seen the
		// event — so the answer is only known on the frame after.
		if (scrollParam) {
			engine::ParamQuantity* pq = scrollParam->getParamQuantity();
			if (pq && pq->getValue() != scrollParamWas) {
				scrollParamMoved = true;
				scrollParamWas = pq->getValue();
			}
		}

		autoScrollWhileCarrying();
		addToPortMenu();
		placeChooser();

		// THE CYCLE ENDS WHEN THE POINTER LEAVES THE JACK, and it has to be noticed here
		// rather than at the next click. Taking a cable away and bringing it back to where it
		// came from is how anyone undoes a pickup by hand — and that click has to put the
		// cable down, not carry on stepping through the jack's cables as though the pointer
		// had never left.
		if (cyclePort && widgetAt<app::PortWidget>(APP->scene, APP->scene->getMousePos())
			!= cyclePort)
			endCycle();

		// Retake the last place whenever something else has taken it — which is exactly what
		// happens each time a menu opens. Moving our position in the child list only; the
		// parent link is untouched, so this is not an add or a remove.
		//
		// It MUST be a splice. The parent is part-way through iterating this very list to
		// call our step(), holding an iterator to us, and will do ++it the moment we return.
		// Erasing ourselves — which remove() then push_back() does — invalidates precisely
		// that iterator, and the app crashed on opening any menu. splice() moves the element
		// without invalidating anything: the parent's iterator follows us to the end, and its
		// loop simply finishes a frame early.
		if (parent && parent->children.back() != this) {
			auto self = std::find(parent->children.begin(), parent->children.end(),
				static_cast<widget::Widget*>(this));
			if (self != parent->children.end())
				parent->children.splice(parent->children.end(), parent->children, self);
		}

		widget::Widget::step();
	}

	/** Sideways scroll pans the rack, which "mouse wheel zooms" mode otherwise makes
	impossible.

	In that mode the rack scroll widget consumes EVERY scroll its children decline: it zooms by
	the vertical delta — zero for a sideways glide — and then swallows the event, so it never
	reaches the panning code below it. A horizontal gesture is left doing nothing at all.

	This runs before the rack sees the event, so it can pan and consume first. It acts only on
	a clearly sideways gesture, only in zoom mode — Rack's own panning is fine in the other —
	and never over one of our own faces, since a scope steps its time base with sideways
	scroll.
	*/
	bool panSideways(const HoverScrollEvent& e) {
		if (!settings::mouseWheelZoom)
			return false;
		if (std::fabs(e.scrollDelta.x) <= std::fabs(e.scrollDelta.y))
			return false;
		ui::ScrollWidget* scroll = APP->scene->rackScroll;
		if (!scroll)
			return false;
		if (clipFamilyAt(e.pos))
			return false;

		// Rack's own step, so panning feels the same in either wheel mode.
		scroll->offset = scroll->offset.minus(e.scrollDelta);
		return true;
	}

	void onHoverScroll(const HoverScrollEvent& e) override {
		scrollTime = APP->window->getFrameTime();
		// The direction is STICKY. A trackpad glide tails off into small ragged deltas, and
		// taking every one of them made the run flip axis at the end of each gesture — a
		// horizontal scroll finishing with a flash of vertical. A new axis has to be both
		// decisive and clearly dominant, and until the gesture has been still for a while the
		// old one stands.
		// The direction comes from the WHOLE opening movement, not from its first event.
		//
		// A gesture almost never starts cleanly: a horizontal glide begins with a pixel or two
		// of vertical, and judging from that first delta locked the run — and the scope — to
		// the wrong axis. Deltas are gathered until they add up to something worth judging, and
		// the axis is then taken from the sum, which is whichever way the hand was actually
		// going. Once claimed it holds until the gesture stops.
		const double now = APP->window->getFrameTime();
		if (now - scrollDirTime > 0.7) {
			claim = math::Vec();
			claimed = false;
		}
		scrollDirTime = now;
		// A MOUSE WHEEL HAS NO SIDEWAYS COMPONENT AT ALL, so there is nothing to be sticky
		// about: a delta with no x is vertical, now, rather than after enough movement has
		// been gathered to be sure. Waiting left the axis at whatever a previous trackpad
		// gesture had claimed, which is how a plain wheel came to be drawn as a sideways
		// scroll.
		if (e.scrollDelta.x == 0.f && e.scrollDelta.y != 0.f) {
			scrollDir = math::Vec(0.f, e.scrollDelta.y >= 0.f ? 1.f : -1.f);
			claimed = true;
			claim = math::Vec();
		}
		else if (!claimed) {
			claim = claim.plus(e.scrollDelta);
			if (std::fabs(claim.x) + std::fabs(claim.y) >= 6.f) {
				scrollDir = claim;
				claimed = true;
			}
		}
		else if (std::fabs(scrollDir.x) > std::fabs(scrollDir.y)) {
			scrollDir = math::Vec(e.scrollDelta.x >= 0.f ? 1.f : -1.f, 0.f);
		}
		else {
			scrollDir = math::Vec(0.f, e.scrollDelta.y >= 0.f ? 1.f : -1.f);
		}
		// The parameter under the pointer, and what it reads BEFORE this event goes any
		// further. Our overlay is offered the scroll first, so this is the value as it was.
		{
			app::ParamWidget* under = widgetAt<app::ParamWidget>(APP->scene, e.pos);
			if (under != scrollParam) {
				scrollParam = under;
				scrollParamMoved = false;
				engine::ParamQuantity* pq = under ? under->getParamQuantity() : NULL;
				scrollParamWas = pq ? pq->getValue() : 0.f;
			}
		}

		if (panSideways(e)) {
			e.consume(this);
			e.stopPropagating();
			return;
		}
		if (!sliderScroll || !*sliderScroll || e.scrollDelta.y == 0.f) {
			widget::Widget::onHoverScroll(e);
			return;
		}
		// We have no children, so this searches from the scene rather than from ourselves.
		// Our box sits at the scene's origin, so the event position needs no adjustment.
		ui::Slider* slider = widgetAt<ui::Slider>(APP->scene, e.pos);
		if (!slider || !slider->quantity) {
			widget::Widget::onHoverScroll(e);
			return;
		}

		// A hundred notches across the full range, a thousand with Shift held — the same
		// resolution Rack gives a knob for coarse and fine.
		float step = 0.01f;
		if ((APP->window->getMods() & RACK_MOD_MASK) == GLFW_MOD_SHIFT)
			step = 0.001f;
		slider->quantity->moveScaledValue(step * ((e.scrollDelta.y > 0.f) ? 1.f : -1.f));

		// Both are needed: consume names us as the handler, stopPropagating keeps the menu
		// underneath from also scrolling itself by the same wheel movement.
		e.consume(this);
		e.stopPropagating();
	}

	/** Option-click on a jack offers the things that can be clipped onto it.

	Option is the one modifier stock Rack leaves free on a port: it already takes plain click
	and drag, Shift-click, Cmd-drag, Cmd-Shift-drag and right-click. The only Alt binding
	anywhere in Rack is Alt-drag to pan the rack view, and that is on the scroll area behind
	the modules, which never sees a press we have claimed on a port.
	*/
	/** Records the click for the drawn pointer. Never consumes: this only watches. */
	void notePointerButton(const ButtonEvent& e) {
		if (e.action == GLFW_PRESS) {
			pressed = true;
			pressedButton = e.button;
			pressTime = APP->window->getFrameTime();
			// Whatever the last value was, it is not what this click is about.
			valueText.clear();
		}
		else if (e.action == GLFW_RELEASE) {
			pressed = false;
		}
	}

	/** Colour by button, so a right-click reads as a different act from a left one. */
	NVGcolor pointerAccent() {
		return (pressedButton == GLFW_MOUSE_BUTTON_RIGHT)
			? nvgRGB(0x6c, 0xb8, 0xff) : nvgRGB(0xff, 0xd8, 0x66);
	}

	/** The param being turned, named and valued.

	A knob moving three degrees is invisible on video, so tutorials put the number beside the
	pointer instead. Rack will format it for us — the same string its own tooltip shows — so
	this is the real value rather than an approximation of it.
	*/
	/** PADS A NUMBER SO ITS DECIMAL POINT DOES NOT MOVE.

	Rack formats a value to a number of significant figures, not to a number of decimal places,
	so turning one knob walks through 9.99, 10.0, 10.05 — three different widths. The readout is
	centred over the pointer, so every one of those re-centred the whole plate: the number
	shuffled left and right while you were trying to read it, which is the one thing it must not
	do.

	The face is ShareTechMono, in which every character is the same width, so this is a matter
	of counting rather than of measuring.

	AND THE COUNT COMES FROM THE CONTROL, not from a number picked once and applied to
	everything. Padding every value to six characters left of the point suited a knob that runs
	to thousands and pushed a knob that runs from nought to one a long way off to the right for
	no reason at all. The parameter knows its own ends, and the same arithmetic Rack uses to
	turn a value into a displayed one turns those ends into the widest integer part this control
	can ever show. */
	static float displayValueOf(engine::ParamQuantity* pq, float v) {
		if (pq->displayBase == 0.f) {
			// Linear.
		}
		else if (pq->displayBase < 0.f)
			v = std::log(v) / std::log(-pq->displayBase);
		else
			v = std::pow(pq->displayBase, v);
		return v * pq->displayMultiplier + pq->displayOffset;
	}

	/** How many characters the part before the point can ever need, sign included. */
	static int integerWidthOf(engine::ParamQuantity* pq) {
		const float ends[2] = {
			displayValueOf(pq, pq->getMinValue()),
			displayValueOf(pq, pq->getMaxValue()),
		};
		int width = 1;
		for (int i = 0; i < 2; i++) {
			if (!std::isfinite(ends[i]))
				continue;
			int digits = 1;
			float a = std::fabs(ends[i]);
			while (a >= 10.f && digits < 8) {
				a /= 10.f;
				digits++;
			}
			if (ends[i] < 0.f)
				digits++;      // room for the sign
			width = std::max(width, digits);
		}
		return width;
	}

	static std::string alignAroundPoint(const std::string& value, int headWidth) {
		size_t dot = std::string::npos;
		for (size_t i = 0; i + 1 < value.size(); i++) {
			if (value[i] == '.' && std::isdigit((unsigned char) value[i + 1])) {
				dot = i;
				break;
			}
		}
		if (dot == std::string::npos)
			return value;
		std::string head = value.substr(0, dot);
		std::string tail = value.substr(dot);
		while ((int) head.size() < headWidth)
			head = " " + head;
		// The right side only has to stop the string changing length; four covers a point and
		// three decimals, and a unit simply makes it longer for every value alike.
		while (tail.size() < 4)
			tail += " ";
		return head + tail;
	}

	static std::string paramText(app::ParamWidget* pw) {
		if (!pw)
			return "";
		engine::ParamQuantity* pq = pw->getParamQuantity();
		if (!pq)
			return "";
		const std::string label = pq->getLabel();
		const std::string value = alignAroundPoint(
			pq->getDisplayValueString() + pq->getUnit(), integerWidthOf(pq));
		return label.empty() ? value : (label + "  " + value);
	}

	std::string draggedParamText() {
		return paramText(dynamic_cast<app::ParamWidget*>(APP->event->getDraggedWidget()));
	}

	/** One chevron, pointing along `dir`, centred at `c`. */
	static void chevron(NVGcontext* vg, math::Vec c, math::Vec dir, float size, NVGcolor col) {
		// Perpendicular to the direction, which is where the two arms go.
		const math::Vec perp = math::Vec(-dir.y, dir.x);
		const math::Vec tip = c.plus(dir.mult(size * 0.5f));
		const math::Vec a = c.minus(dir.mult(size * 0.5f)).plus(perp.mult(size));
		const math::Vec b = c.minus(dir.mult(size * 0.5f)).minus(perp.mult(size));

		nvgBeginPath(vg);
		nvgMoveTo(vg, a.x, a.y);
		nvgLineTo(vg, tip.x, tip.y);
		nvgLineTo(vg, b.x, b.y);
		nvgStrokeColor(vg, col);
		nvgStrokeWidth(vg, 3.4f);
		nvgLineCap(vg, NVG_ROUND);
		nvgLineJoin(vg, NVG_ROUND);
		nvgStroke(vg);
	}

	/** A run of chevrons travelling in the scroll direction, fading in at the back and out at
	the front, so the group reads as moving rather than as three static marks. */
	void drawScrollRun(const DrawArgs& args, math::Vec p, float fade, double now) {
		const bool horizontal = std::fabs(scrollDir.x) > std::fabs(scrollDir.y);
		math::Vec dir, origin;
		if (horizontal) {
			// Rack's horizontal delta is positive when the content moves RIGHT, which sends the
			// view left — so the run follows the view, which is what the eye is tracking.
			dir = math::Vec((scrollDir.x >= 0.f) ? -1.f : 1.f, 0.f);
			// BELOW the pointer, clear of the arrow. It used to run above, which is where the
			// value readout now sits — so a sideways scroll marched a line of chevrons straight
			// across the digits, and they flickered in and out as the run travelled. The
			// readout is the thing being read, so it keeps the space above and the chevrons
			// move. Thirty-two clears the drawn arrow, which reaches twenty down.
			origin = math::Vec(p.x, p.y + 32.f);
		}
		else {
			// Rack's scroll is positive upwards, which is the way the CONTENT moves.
			dir = math::Vec(0.f, (scrollDir.y >= 0.f) ? -1.f : 1.f);
			origin = math::Vec(p.x - 20.f, p.y + 6.f); // to its left
		}

		const float spacing = 14.f;
		const float span = 36.f;
		// One spacing per third of a second, so the run travels at a readable pace.
		const float march = (float) std::fmod(now * 33.0, (double) spacing);

		// No caption. The run was labelled at first, but placing that plate correctly against a
		// group of chevrons that moves, changes axis and sits on either side of the pointer was
		// more fuss than it was worth — the demo can say once that the wheel is being used, and
		// the animation carries it from there.

		for (int i = -3; i <= 3; i++) {
			const float along = i * spacing + march;
			if (along < -span || along > span)
				continue;
			// Brightest in the middle of the run, gone at either end.
			const float edge = 1.f - std::fabs(along) / span;
			const math::Vec c = origin.plus(dir.mult(along));
			// Brighter than the rest of the pointer furniture: on a busy panel this is the only
			// sign that the wheel is doing anything at all.
			chevron(args.vg, c, dir, 6.5f,
				nvgRGBAf(1.f, 0.92f, 0.55f, std::fmin(1.f, fade * edge * 1.6f)));
		}
	}

	/** A label in a dark plate. CENTRED ABOVE THE TIP when `above`, and beside the pointer
	otherwise.

	Above and centred for the value being changed, because that is the one you are reading: the
	tip of the pointer is where the eye already is, and a plate hanging off to the right meant
	looking away from the control being turned to read what it now says. Above it also cannot
	be under the hand, whichever way round the mouse is being held.

	Beside is kept for the modifier line, which is a caption rather than something you read. */
	void drawPointerLabel(const DrawArgs& args, math::Vec p, const std::string& text,
		NVGcolor ink, float dy, bool above = false, float alpha = 1.f) {

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0 || text.empty())
			return;
		nvgFontFaceId(args.vg, font->handle);
		// HALF THE SIZE IT WAS. Twenty-six point was a caption for a video watched across a
		// room; read at a desk it was larger than anything else on the panel and took more of
		// the rack than the control it was describing.
		nvgFontSize(args.vg, 13.f);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		const float w = nvgTextBounds(args.vg, 0, 0, text.c_str(), NULL, NULL);
		const float plateW = w + 10.f;
		const float plateH = 18.f;

		const float x = above ? (p.x - plateW / 2.f) : (p.x + 16.f);
		// Six pixels of air over the control, and never off the top of the window: a readout
		// that has gone above the edge of the screen is a readout nobody can read.
		const float y = above ? std::fmax(2.f, p.y - plateH - 6.f) : (p.y + dy);

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, x, y, plateW, plateH, 3.5f);
		nvgFillColor(args.vg, nvgRGBA(0, 0, 0, (int) (0xc8 * alpha)));
		nvgFill(args.vg);
		ink.a = alpha;
		nvgFillColor(args.vg, ink);
		nvgText(args.vg, x + 5.f, y + plateH / 2.f, text.c_str(), NULL);
	}

	void drawPointer(const DrawArgs& args) {
		// TWO SWITCHES, NOT ONE. The drawn pointer with its rings, and the readout of what is
		// being turned, are two different things that shared a switch because both were built
		// for the same afternoon's video. Only the first is about a recording; the second
		// answers "what did I just set that to", which is worth having with nothing being
		// filmed at all.
		//
		// There was a third — a trail behind the pointer — and it is gone. Every operating
		// system offers pointer trails already, and doing the same thing worse inside one
		// plugin is not worth the switch it would need.
		const bool clicks = demoPointer && *demoPointer;
		const bool values = demoValues && *demoValues;
		if (!clicks && !values)
			return;
		// Our box sits at the scene's origin, so the scene's mouse position is ours.
		const math::Vec p = APP->scene->mousePos;
		const double now = APP->window->getFrameTime();
		const double age = now - pressTime;

		// HELD. A steady halo for as long as the button is down, which is what separates a
		// press-and-hold from a click: the click's ring is gone in a quarter second, this is
		// not.
		if (pressed && clicks) {
			NVGcolor c = pointerAccent();
			c.a = 0.28f;
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, p.x, p.y, 13.f);
			nvgFillColor(args.vg, c);
			nvgFill(args.vg);
			c.a = 0.9f;
			nvgStrokeColor(args.vg, c);
			nvgStrokeWidth(args.vg, 1.6f);
			nvgStroke(args.vg);
		}

		// SCROLL. A marching run of chevrons, pointing and travelling the way the wheel is
		// going. Vertical scrolling shows them to the LEFT of the pointer and horizontal
		// scrolling ABOVE it — never under it, where the arrow and Rack's own hover cursors
		// would cover them. Movement is what says "scrolling" rather than "something flashed".
		// SCROLL. Only when the wheel is actually turning something. The chevrons were shown
		// for any wheel movement at all, so scrolling the rack — where the wheel moves the view
		// and changes nothing — marched them across the screen for no reason. The same fault as
		// the value readout had, and it takes the same answer: did the parameter move.
		const double scrollAge = now - scrollTime;
		if (scrollAge < 0.5 && clicks && scrollParam && scrollParamMoved)
			drawScrollRun(args, p, (float) (1.0 - scrollAge / 0.5), now);

		// CLICK. A ring that snaps out and is gone in a third of a second — which is exactly
		// what distinguishes it from the halo of a held button, which stays.
		//
		// The size it was, drawn better. Reaching fifty pixels was too much of the screen for
		// a click: what wanted improving was how the ring MOVES, not how much room it takes.
		// So it is back to about twenty-six across, and keeps the things that made it easier to
		// follow — it opens fast and slows the way something struck does, thins as it grows so
		// the ring reads as spreading rather than as a circle being drawn, and holds its
		// brightness a little past the middle of its life instead of fading from the start.
		if (age < 0.35 && clicks) {
			const float t = (float) (age / 0.35);
			// Fast out, slowing: the shape of something struck.
			const float grow = 1.f - (1.f - t) * (1.f - t);
			const float fade = std::pow(1.f - t, 0.7f);
			const float r = 6.f + 20.f * grow;

			NVGcolor c = pointerAccent();
			c.a = 0.16f * fade;
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, p.x, p.y, r);
			nvgFillPaint(args.vg, nvgRadialGradient(args.vg, p.x, p.y, r * 0.35f, r,
				nvgRGBA(0, 0, 0, 0), c));
			nvgFill(args.vg);

			c.a = 0.85f * fade;
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, p.x, p.y, r);
			nvgStrokeColor(args.vg, c);
			nvgStrokeWidth(args.vg, 3.f * (1.f - t) + 1.2f);
			nvgStroke(args.vg);
		}

		// The pointer itself: the familiar arrow, white with a dark outline so it reads over
		// both a pale panel and a dark scope face. Only when the clicks are being animated —
		// the readout on its own is for somebody who can see their real cursor perfectly well.
		if (clicks) {
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, p.x, p.y);
		nvgLineTo(args.vg, p.x, p.y + 17.f);
		nvgLineTo(args.vg, p.x + 4.5f, p.y + 13.f);
		nvgLineTo(args.vg, p.x + 7.5f, p.y + 19.5f);
		nvgLineTo(args.vg, p.x + 10.5f, p.y + 18.f);
		nvgLineTo(args.vg, p.x + 7.5f, p.y + 11.5f);
		nvgLineTo(args.vg, p.x + 12.5f, p.y + 11.f);
		nvgClosePath(args.vg);
		nvgFillColor(args.vg, pressed ? pointerAccent() : nvgRGB(0xff, 0xff, 0xff));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGBA(0, 0, 0, 0xcc));
		nvgStrokeWidth(args.vg, 1.2f);
		nvgStroke(args.vg);
		}

		// WHAT IS BEING TURNED. A knob moves too little to see on video; its name and value do
		// not. Shown above the modifier line so both can be up at once.
		//
		// NOT WHILE A MENU IS OPEN, and not for the right button. A right press on a knob makes
		// it the dragged widget for as long as the button is held, and Rack opens the knob's own
		// menu under the pointer at the same moment — so the readout was laid over the menu it
		// was competing with. Reported by DaveVenom on Windows. Neither case is one the readout
		// is for: it is there to show a value being changed, and a menu being opened changes
		// nothing.
		// IT STAYS FOR A SECOND AND THEN FADES, rather than vanishing the instant the knob is
		// let go. Letting go is when you look at what you set it to, and a readout that is gone
		// by then has been shown for exactly the part of the gesture where you were watching
		// your hand instead. A second at full strength, then half a second going.
		//
		// UNLESS SOMETHING ELSE HAPPENS. Turning another control replaces it at once, and any
		// press anywhere clears it — a click is somebody attending to something else, and a
		// value hanging over the thing they are now doing is in the way.
		//
		// It also stops following the pointer once the hand has left: it stays over the control
		// it belongs to rather than drifting across the rack with the cursor.
		if (values && !menuIsOpen()) {
			app::ParamWidget* active = NULL;
			if (pressedButton != GLFW_MOUSE_BUTTON_RIGHT)
				active = dynamic_cast<app::ParamWidget*>(APP->event->getDraggedWidget());
			if (!active && scrollAge < 0.5 && scrollParam && scrollParamMoved)
				active = scrollParam;
			if (active) {
				if (active != valueOwner) {
					valueOwner = active;
					valueWidth = 0;   // A different control starts its own measure.
				}
				std::string text = paramText(active);
				if (text.size() > valueWidth)
					valueWidth = text.size();
				while (text.size() < valueWidth)
					text += " ";
				valueText = text;
				valueTime = now;
				// ABOVE THE CONTROL, not above the pointer. What you are reading belongs to the
				// knob, and a plate that hangs over the cursor moves with every twitch of the
				// hand while the thing it describes stays still. Anchored to the middle of the
				// control's top edge it sits in one place for the whole gesture, and it is
				// obvious which control it is talking about when several are close together.
				//
				// Asked of the widget rather than worked out, so the rack's zoom is already in
				// it: the offset comes back in the scene's own coordinates, which are ours.
				valuePos = active->getAbsoluteOffset(
					math::Vec(active->box.size.x / 2.f, 0.f));
			}
			const double age = now - valueTime;
			if (!valueText.empty() && age < 1.5) {
				const float alpha = (age <= 1.0) ? 1.f
					: (float) ((1.5 - age) / 0.5);
				drawPointerLabel(args, valuePos, valueText,
					nvgRGB(0x3d, 0xe0, 0x7a), -10.f, true, alpha);
			}
		}

		// Whatever modifier is held, named. A viewer cannot see a key being pressed, and half
		// of what this plugin does hangs off Option.
		// ONLY Option. It is the modifier this plugin's own gestures use, and it is the one a
		// viewer needs told. The others are not shown deliberately: a screen magnifier holds
		// keys of its own, and a recording captioned SHIFT or CONTROL every few seconds would
		// be describing the accessibility tooling rather than the software.
		if (clicks && (APP->window->getMods() & GLFW_MOD_ALT))
			drawPointerLabel(args, p, "OPTION", nvgRGB(0xff, 0xd8, 0x66), 26.f);
	}

	void draw(const DrawArgs& args) override {
		widget::Widget::draw(args);
		// Last child of the scene, so this lands on top of everything — menus included.
		drawPointer(args);
	}

	/** The things that can be clipped onto a jack. Shared by the Option-click menu and by the
	entry added to Rack's own right-click menu, so the two can never drift apart. */
	static void addClipOnItems(ui::Menu* menu, app::PortWidget* port, bool scopesOn,
		bool widgetsOn) {

		WeakPtr<app::PortWidget> weakPort = port;
		if (scopesOn) {
			menu->addChild(createMenuItem("Scope", "", [weakPort]() {
				if (weakPort)
					scopeCreate(weakPort);
			}));
			menu->addChild(createMenuItem("Analyser", "", [weakPort]() {
				if (weakPort)
					analyserCreate(weakPort);
			}));
		}
		if (widgetsOn) {
			// Before the sources, and outside the test below: a monitor listens, so it goes on
			// an output as readily as on an input.
			menu->addChild(createMenuItem("Audio monitor", "", [weakPort]() {
				if (weakPort)
					monitorCreate(weakPort);
			}));
			// A voltmeter reads, so it belongs here too: either end of a cable will do.
			menu->addChild(createMenuItem("Voltmeter", "", [weakPort]() {
				if (weakPort)
					meterCreate(weakPort);
			}));
		}
		if (!widgetsOn || !injectorAcceptsPort(port))
			return;

		struct Entry { const char* name; InjectorType type; bool noteMode; };
		// THE ORDER IS THE ORDER THEY ARE REACHED FOR. The switch follows the instruments
		// because, like them, it is about a signal that is already there rather than one being
		// made; then the two oscillators, which are the sources wanted most often. The rest
		// follow in the order they always have.
		static const Entry entries[] = {
			{"Switch", INJECT_SWITCH, false},
			{"LFO", INJECT_LFO, false},
			{"VCO", INJECT_AUDIO, false},
			{"Gate button", INJECT_GATE, false},
			{"Pulse button", INJECT_PULSE, false},
			{"Clock", INJECT_CLOCK, false},
			{"DC level", INJECT_DC, false},
			{"Note", INJECT_AUDIO, true},
			{"Volt/oct", INJECT_NOTE, false},
			{"Noise", INJECT_NOISE, false},
			{"Attenuverter", INJECT_AV, false},
		};
		for (const Entry& entry : entries) {
			const InjectorType type = entry.type;
			const bool noteMode = entry.noteMode;
			menu->addChild(createMenuItem(entry.name, "", [weakPort, type, noteMode]() {
				if (weakPort)
					injectorCreate(weakPort, type, noteMode);
			}));
		}
	}

	/** Adds a Widgets submenu to the port menu Rack has just opened.

	Rack gives a plugin no hook into another module's port menu, so this works by inference: a
	right-click on a jack is noted, and the menu that appears within the next frame or two is
	taken to be that jack's. Menus are ordinary widgets and anything added to one is deleted
	with it, so nothing is left behind.

	If Rack ever changed when that menu is built, our entry would simply stop appearing —
	visible, and harmless.
	*/
	void addToPortMenu() {
		if (!menuPort || menuWait <= 0)
			return;
		menuWait--;

		for (auto it = APP->scene->children.rbegin(); it != APP->scene->children.rend(); it++) {
			ui::MenuOverlay* overlay = dynamic_cast<ui::MenuOverlay*>(*it);
			if (!overlay)
				continue;
			for (widget::Widget* child : overlay->children) {
				ui::Menu* menu = dynamic_cast<ui::Menu*>(child);
				if (!menu)
					continue;
				const bool scopesOn = offerScopes && *offerScopes;
				const bool widgetsOn = offerWidgets && *offerWidgets;
				if (scopesOn || widgetsOn) {
					WeakPtr<app::PortWidget> port = menuPort;
					// NOT a submenu. A submenu opens beside its parent and our list is long
					// enough to run off the bottom of the window from a jack low in the rack.
					// This opens the same chooser Option-click gives, which Rack positions and
					// fits to the window itself.
					InterceptOverlay* self = this;
					ui::MenuItem* item = createMenuItem("Widgets…", "",
						[self, port, scopesOn, widgetsOn]() {
							if (!port)
								return;
							ui::Menu* m = createMenu();
							addClipOnItems(m, port, scopesOn, widgetsOn);
							// Positioned on the next frame, once it knows how tall it is.
							self->chooser = m;
							self->chooserAt = APP->scene->mousePos;
						});
					ui::MenuSeparator* separator = new ui::MenuSeparator;
					menu->addChild(item);
					menu->addChild(separator);

					// AT THE TOP, above Rack's own entries and divided from them. This menu
					// belongs to the port rather than to us, so adding to the foot was the
					// polite place — but the foot is where the entries nobody reaches live, and
					// the whole point of this one is that it is the way in. Widgets are added
					// far more often than a port's colour is changed.
					//
					// Rack's menu lays its children out in list order, so moving the two nodes
					// to the front is all this takes; splice keeps the widgets themselves and
					// their ownership untouched.
					std::list<widget::Widget*>& entries = menu->children;
					auto itemAt = std::find(entries.begin(), entries.end(),
						(widget::Widget*) item);
					auto separatorAt = std::find(entries.begin(), entries.end(),
						(widget::Widget*) separator);
					if (itemAt != entries.end() && separatorAt != entries.end()) {
						entries.splice(entries.begin(), entries, itemAt);
						entries.splice(std::next(entries.begin()), entries, separatorAt);
					}
				}
				// AND WHICH FAMILY THIS PORT IS, at the foot.
				//
				// The colour code guesses from the port's name, and a guess is wrong sometimes:
				// a port called "Rate" is modulation on one module and a clock on the next, and
				// no rule reads both correctly. This is where it gets put right, on the port
				// itself, where the person who can see that it is wrong already is. The choice
				// is remembered against the model rather than against the patch, so a port
				// corrected once is correct in every patch that uses that module.
				//
				// At the foot deliberately, unlike the widgets entry above: this is done once
				// for a module and then never again, so it should not be in the way.
				{
					WeakPtr<app::PortWidget> port = menuPort;
					menu->addChild(new ui::MenuSeparator);
					menu->addChild(createSubmenuItem("Signal family", "",
						[port](ui::Menu* sub) {
							sub->addChild(createCheckMenuItem("Automatic", "",
								[port]() {
									return port && palettePortOverride(port) < 0;
								},
								[port]() {
									if (port)
										paletteSetPortOverride(port, -1);
								}));
							for (int family = 0; family < NUM_FAMILIES; family++) {
								sub->addChild(createCheckMenuItem(paletteName(family), "",
									[port, family]() {
										return port && palettePortOverride(port) == family;
									},
									[port, family]() {
										if (port)
											paletteSetPortOverride(port, family);
									}));
							}
						}));
				}

				menuPort = NULL;
				menuWait = 0;
				return;
			}
		}
	}

	/** Whether Rack is dragging a cable out of a port right now, and what the switch was set to
	last frame — the two things the notes are triggered by. */
	/** A note owed for the carry in progress, waiting for the pull to declare a direction. */
	bool wasClickCables = false;
	bool seenClickCables = false;

	/** The note, offered as a cable is picked up and dragged.

	AN OFFER, NOT A CORRECTION. Both gestures are right — the note exists only because nobody
	would guess that letting go of the button is allowed, and it says so while a cable is in
	the hand, which is the one moment that is worth knowing.
	*/

	/** Centres our chooser on the pointer, and lifts it clear of the bottom of the window.

	A menu opens with its top at the pointer, so a long list opened from low in the rack runs
	off the bottom. Centring it vertically puts the middle of the list under the pointer — half
	the entries are then a shorter reach — and clamping keeps the whole list on screen whatever
	happens. Done a frame later because a menu does not know its own height until it has laid
	its children out.
	*/
	void placeChooser() {
		if (!chooser)
			return;
		ui::Menu* m = chooser;
		if (m->box.size.y <= 0.f)
			return;   // Not laid out yet; try again next frame.

		const float windowH = APP->scene->box.size.y;
		float y = chooserAt.y - m->box.size.y / 2.f;
		y = math::clamp(y, 4.f, std::fmax(4.f, windowH - m->box.size.y - 4.f));
		m->box.pos = math::Vec(chooserAt.x, y);
		chooser = NULL;
	}

	/** Whether a menu is open. Ours searches the whole scene for jacks and pills, and a menu
	drawn over the rack does not hide them from that search — so a click on a menu item that
	happened to sit over a jack picked up that jack's cable as well as choosing the item. A menu
	is modal: while one is up, none of the gestures below apply. */
	/** Whether a menu is actually open.

	VISIBILITY IS THE TEST, not the presence of an overlay. Rack keeps a MenuOverlay in the
	scene at all times — the module browser lives in one, hidden until it is summoned — so
	"is there a MenuOverlay?" is true from the moment Rack starts. Asking that question
	silently switched off everything this overlay does below the guard.
	*/
	static bool menuIsOpen() {
		for (widget::Widget* child : APP->scene->children) {
			ui::MenuOverlay* overlay = dynamic_cast<ui::MenuOverlay*>(child);
			if (overlay && overlay->visible && !overlay->requestedDelete)
				return true;
		}
		return false;
	}

	void onButton(const ButtonEvent& e) override {
		notePointerButton(e);

		// A note on screen owns its own clicks, like a menu does.
		if (paletteCovers(e.pos)) {
			widget::Widget::onButton(e);
			return;
		}

		// A right-click on a jack: Rack is about to open its port menu, and ours is added to it
		// on the next frame or two. NOT consumed — the port's own menu is the point.
		//
		// Noted BEFORE the modal check below, because right-clicking a second jack while the
		// first menu is still open is an ordinary thing to do, and returning early there meant
		// the note was never taken and our entry never appeared on that menu.
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT
			&& !clipFamilyAt(e.pos)) {
			if (app::PortWidget* p = widgetAt<app::PortWidget>(APP->scene, e.pos)) {
				menuPort = p;
				menuWait = 3;
			}
		}

		// A menu is modal for everything that follows: our gestures search the whole scene, and
		// a menu drawn over the rack does not hide a jack from that search.
		if (menuIsOpen()) {
			widget::Widget::onButton(e);
			return;
		}

		// A pill under the pointer takes a right-click: that lifts the cable it belongs to.
		// Part of trace assist rather than of click-to-pull, since the pill is what names the
		// cable and the two only make sense together.
		if (!carrying && trace && *trace
			&& e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT
			&& (e.mods & RACK_MOD_MASK) == 0
			&& !clipFamilyAt(e.pos)) {
			app::CableWidget* pillCable = NULL;
			bool atInput = false;
			if (cableFocusPillAt(pillCable, atInput)) {
				pickUpCable(pillCable, atInput);
				e.consume(this);
				e.stopPropagating();
				return;
			}
		}

		// CARRYING, and the button comes UP after moving: that was a drag, so the cable lands
		// wherever it was let go — over a port it connects, and anywhere else it stays in hand
		// rather than being thrown away, since letting go halfway across the rack is not a
		// decision to discard a cable.
		if (carrying && e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			const float travelled = APP->scene->getMousePos().minus(carryStart).norm();
			if (travelled >= 4.f) {
				app::PortWidget* target = clipFamilyAt(e.pos)
					? NULL : widgetAt<app::PortWidget>(APP->scene, e.pos);
				if (target)
					dropOn(target);
			}
			e.consume(this);
			e.stopPropagating();
			return;
		}

		// CARRYING: the next click puts the cable down, wherever it lands — except on the jack
		// the cycle belongs to, where it reaches the next thing that jack can offer. Dropping a
		// cable back exactly where it came from does nothing, so that click was free to mean
		// something else.
		if (carrying && e.action == GLFW_PRESS) {
			if (e.button == GLFW_MOUSE_BUTTON_RIGHT) {
				cancelCarry();
			}
			else if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
				app::PortWidget* under = clipFamilyAt(e.pos)
					? NULL : widgetAt<app::PortWidget>(APP->scene, e.pos);
				// STILL ON THE JACK is the whole test. Six pixels of tolerance was the first
				// attempt and it is too mean: a new cable's loose end sits at the pointer, so
				// with the pointer pinned to one spot the cable has no length and its loop
				// hangs inside the jack, where it cannot be seen. Anywhere on the jack, and
				// the state you are in is visible.
				const bool stillThere = cyclePort && under == cyclePort;
				if (stillThere)
					advanceCycle();
				else
					dropOn(under);
			}
			e.consume(this);
			e.stopPropagating();
			return;
		}
		// The carried cable was removed from under us — by an undo, or by its module going
		// away. Stop carrying rather than pointing at nothing.
		if (carrying && !carried)
			carrying = false;

		// A click anywhere puts down a widget that is riding the pointer. It has to be caught
		// here: a following widget is click-through, so the click lands on whatever is beneath
		// it — a panel, another module — and would drag that instead of dropping the widget.
		//
		// BEFORE the pickup below, and that ordering is the fix for a real bug. A widget rides
		// just to one side of the pointer, so the pointer is not inside it, so the guard that
		// keeps clicks off jacks under a widget does not apply — and placing one over a jack
		// pulled that jack's cable out at the same moment.
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0
			&& (clipDepositFollowing() || scopeDepositFollowing())) {
			e.consume(this);
			e.stopPropagating();
			return;
		}

		// PICKING UP: a plain click on a jack takes its cable, or starts a new one.
		// NOT through a widget. A scope or an injector sitting over a jack is what the pointer is
		// on; searching for a PortWidget alone found the jack underneath it and picked up its
		// cable, so clicking a widget to drag it pulled a cable out from beneath.
		if (clickCables && *clickCables && !carrying
			&& e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0
			&& !clipFamilyAt(e.pos)) {
			if (app::PortWidget* port = widgetAt<app::PortWidget>(APP->scene, e.pos)) {
				// Empty-handed but still on the jack we were cycling: this is the next step
				// round, not a new gesture, or the cycle could never come back to its start.
				if (cyclePort && port == cyclePort) {
					advanceCycle();
					e.consume(this);
					e.stopPropagating();
					return;
				}
				endCycle();
				pickUp(port);
				e.consume(this);
				e.stopPropagating();
				return;
			}
		}

		// A plain click on a cable's pill takes that cable — unless the press belongs to a
		// terminal instead.
		//
		// WHICHEVER IS NEARER, rather than the terminal always. Refusing every press that landed
		// on a terminal at all was the first rule, and it was too broad: a terminal's clickable
		// box is square and larger than the jack drawn in it, so where modules are packed
		// closely a pill can lie over the corner of a neighbouring terminal's box and the trace
		// then does nothing at all, with the terminal it was refused to being one the pointer is
		// nowhere near. Reported by DaveVenom.
		//
		// The terminal still wins a press that is genuinely on it, which is what matters:
		// dragging a cable off a terminal is the commonest gesture in Rack and must never be
		// taken by a pill.
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0
			&& !clipFamilyAt(e.pos)
			&& pillBeatsPortAt(e.pos)
			&& cableFocusClick()) {
			e.consume(this);
			e.stopPropagating();
			return;
		}
		// A plain click on a bare part of a module's panel puts the traced cable out. The test
		// is what Rack itself considers hovered: a ModuleWidget is opaque and its controls sit
		// on top of it, so the hovered widget IS the module only when the pointer is on panel
		// rather than on a knob, a jack, a screen or a button. NOT consumed — the click goes on
		// to do whatever it would have done, which on a panel is to drag the module.
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0 && cableFocusActive()
			&& dynamic_cast<app::ModuleWidget*>(APP->event->hoveredWidget)) {
			cableFocusClear();
		}

		if (e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT
			|| (e.mods & RACK_MOD_MASK) != GLFW_MOD_ALT) {
			widget::Widget::onButton(e);
			return;
		}
		app::PortWidget* port = clipFamilyAt(e.pos)
			? NULL : widgetAt<app::PortWidget>(APP->scene, e.pos);
		if (!port) {
			widget::Widget::onButton(e);
			return;
		}

		WeakPtr<app::PortWidget> weakPort = port;
		const bool scopesOn = offerScopes && *offerScopes;
		const bool widgetsOn = offerWidgets && *offerWidgets;
		if (!scopesOn && !widgetsOn) {
			widget::Widget::onButton(e);
			return;
		}

		// No heading: the list is self-explanatory, and a label only makes it taller.
		ui::Menu* menu = createMenu();
		addClipOnItems(menu, port, scopesOn, widgetsOn);

		e.consume(this);
		e.stopPropagating();
	}

	/** Escape deposits a scope riding the pointer. It has to be reachable from here because a
	following scope is click-through, so it is never the hovered widget itself. */
	void onHoverKey(const HoverKeyEvent& e) override {
		if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ESCAPE && carrying) {
			cancelCarry();
			e.consume(this);
			e.stopPropagating();
			return;
		}
		if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ESCAPE && cableFocusActive()) {
			cableFocusClear();
			e.consume(this);
			e.stopPropagating();
			return;
		}
		if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ESCAPE
			&& (clipDepositFollowing() || scopeDepositFollowing())) {
			e.consume(this);
			e.stopPropagating();
			return;
		}
		widget::Widget::onHoverKey(e);
	}
};


widget::Widget* createInterceptOverlay(bool* sliderScroll, bool* clickCables,
	bool* offerScopes, bool* offerWidgets, bool* trace, bool* demoPointer,
	bool* demoValues) {

	InterceptOverlay* overlay = new InterceptOverlay;
	overlay->sliderScroll = sliderScroll;
	overlay->clickCables = clickCables;
	overlay->offerScopes = offerScopes;
	overlay->offerWidgets = offerWidgets;
	overlay->trace = trace;
	overlay->demoPointer = demoPointer;
	overlay->demoValues = demoValues;
	return overlay;
}
