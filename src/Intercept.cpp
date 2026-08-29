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
#include <app/ParamWidget.hpp>
#include <ui/ScrollWidget.hpp>
#include <app/CableWidget.hpp>
#include <history.hpp>
#include <ui/ScrollWidget.hpp>
#include <app/PortWidget.hpp>
#include "Injector.hpp"
#include "WidgetAt.hpp"
#include "Clip.hpp"

#include <cmath>
#include <vector>
#include <string>

#include <algorithm>


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
	double pressTime = -1e9;
	bool pressed = false;
	int pressedButton = 0;
	/** Where the pointer has been while a button was held, for the drag trail. */
	std::vector<math::Vec> trail;
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
	/** True between the click that picks a cable up and the click that puts it down. Rack
	believes a drag is in progress the whole time. */
	bool carrying = false;


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

		history::CableRemove* h = new history::CableRemove;
		h->setCable(cw);
		APP->history->push(h);
		cw->getPort(atInput ? engine::Port::INPUT : engine::Port::OUTPUT) = NULL;
		cw->updateCable();
		carried = cw;
		carrying = true;
	}

	/** Picks up the top cable on this port, or starts a new one from it. */
	void pickUp(app::PortWidget* port) {
		if (!port || !port->module)
			return;

		app::CableWidget* cw = APP->scene->rack->getTopCable(port);
		if (cw) {
			// Taking an existing cable off: it becomes incomplete at this end, and the removal
			// is recorded so undo puts it back where it was.
			history::CableRemove* h = new history::CableRemove;
			h->setCable(cw);
			APP->history->push(h);
			cw->getPort(port->type) = NULL;
			cw->updateCable();
		}
		else {
			cw = new app::CableWidget;
			cw->getPort(port->type) = port;
			cw->updateCable();
			APP->scene->rack->addCable(cw);
		}
		carried = cw;
		carrying = true;
	}

	/** Drops what is being carried onto this port, or discards it if the port cannot take it. */
	void dropOn(app::PortWidget* port) {
		carrying = false;
		app::CableWidget* cw = carried;
		carried = NULL;
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

	void cancelCarry() {
		carrying = false;
		if (carried)
			discard(carried);
		carried = NULL;
	}

	/** Scrolls the rack when a carried cable is taken to the edge of the view.

	Needed because the pointer cannot leave the window: without it a cable could only ever be
	dropped on something already on screen. Only while carrying — a rack that slid about
	whenever the pointer neared an edge would be unusable.
	*/
	void autoScrollWhileCarrying() {
		if (!carrying)
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

		autoScrollWhileCarrying();

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
		if (widgetAt<ClipWidget>(APP->scene, e.pos))
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
		if (!claimed) {
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
		scrollParam = widgetAt<app::ParamWidget>(APP->scene, e.pos);

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
			trail.clear();
		}
		else if (e.action == GLFW_RELEASE) {
			pressed = false;
			trail.clear();
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
	static std::string paramText(app::ParamWidget* pw) {
		if (!pw)
			return "";
		engine::ParamQuantity* pq = pw->getParamQuantity();
		if (!pq)
			return "";
		const std::string label = pq->getLabel();
		const std::string value = pq->getDisplayValueString() + pq->getUnit();
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
			origin = math::Vec(p.x, p.y - 24.f);       // above the pointer
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

	/** A label in a dark plate, beside the pointer. */
	void drawPointerLabel(const DrawArgs& args, math::Vec p, const std::string& text,
		NVGcolor ink, float dy) {

		std::shared_ptr<window::Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0 || text.empty())
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 26.f);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		const float w = nvgTextBounds(args.vg, 0, 0, text.c_str(), NULL, NULL);

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, p.x + 16.f, p.y + dy, w + 18.f, 34.f, 6.f);
		nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 0xc8));
		nvgFill(args.vg);
		nvgFillColor(args.vg, ink);
		nvgText(args.vg, p.x + 25.f, p.y + dy + 17.f, text.c_str(), NULL);
	}

	void drawPointer(const DrawArgs& args) {
		if (!demoPointer || !*demoPointer)
			return;
		// Our box sits at the scene's origin, so the scene's mouse position is ours.
		const math::Vec p = APP->scene->mousePos;
		const double now = APP->window->getFrameTime();
		const double age = now - pressTime;

		// THE DRAG TRAIL. Where the pointer has been while the button was down, fading with
		// distance back — movement, rather than a pointer that appears to teleport.
		if (pressed) {
			if (trail.empty() || trail.back().minus(p).norm() > 2.f)
				trail.push_back(p);
			if (trail.size() > 40)
				trail.erase(trail.begin());
			for (size_t i = 1; i < trail.size(); i++) {
				const float t = (float) i / trail.size();
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, trail[i - 1].x, trail[i - 1].y);
				nvgLineTo(args.vg, trail[i].x, trail[i].y);
				NVGcolor c = pointerAccent();
				c.a = 0.55f * t;
				nvgStrokeColor(args.vg, c);
				nvgStrokeWidth(args.vg, 3.f * t + 1.f);
				nvgLineCap(args.vg, NVG_ROUND);
				nvgStroke(args.vg);
			}
		}

		// HELD. A steady halo for as long as the button is down, which is what separates a
		// press-and-hold from a click: the click's ring is gone in a quarter second, this is
		// not.
		if (pressed) {
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
		const double scrollAge = now - scrollTime;
		if (scrollAge < 0.5)
			drawScrollRun(args, p, (float) (1.0 - scrollAge / 0.5), now);

		// CLICK. A ring that snaps out and is gone in a quarter second — which is exactly what
		// distinguishes it from the halo of a held button, which stays.
		if (age < 0.25) {
			const float t = (float) (age / 0.25);
			NVGcolor c = pointerAccent();
			c.a = 0.9f * (1.f - t);
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, p.x, p.y, 6.f + 16.f * t);
			nvgStrokeColor(args.vg, c);
			nvgStrokeWidth(args.vg, 2.5f);
			nvgStroke(args.vg);
		}

		// The pointer itself: the familiar arrow, white with a dark outline so it reads over
		// both a pale panel and a dark scope face.
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

		// WHAT IS BEING TURNED. A knob moves too little to see on video; its name and value do
		// not. Shown above the modifier line so both can be up at once.
		// What is being changed, however it is being changed: dragged, or scrolled.
		std::string param = draggedParamText();
		if (param.empty() && scrollAge < 0.9 && scrollParam)
			param = paramText(scrollParam);
		if (!param.empty())
			drawPointerLabel(args, p, param, nvgRGB(0x3d, 0xe0, 0x7a), -10.f);

		// Whatever modifier is held, named. A viewer cannot see a key being pressed, and half
		// of what this plugin does hangs off Option.
		// ONLY Option. It is the modifier this plugin's own gestures use, and it is the one a
		// viewer needs told. The others are not shown deliberately: a screen magnifier holds
		// keys of its own, and a recording captioned SHIFT or CONTROL every few seconds would
		// be describing the accessibility tooling rather than the software.
		if (APP->window->getMods() & GLFW_MOD_ALT)
			drawPointerLabel(args, p, "OPTION", nvgRGB(0xff, 0xd8, 0x66), 26.f);
	}

	void draw(const DrawArgs& args) override {
		widget::Widget::draw(args);
		// Last child of the scene, so this lands on top of everything — menus included.
		drawPointer(args);
	}

	void onButton(const ButtonEvent& e) override {
		notePointerButton(e);
		// A pill under the pointer takes a right-click: that lifts the cable it belongs to.
		// Part of trace assist rather than of click-to-pull, since the pill is what names the
		// cable and the two only make sense together.
		if (!carrying && trace && *trace
			&& e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT
			&& (e.mods & RACK_MOD_MASK) == 0) {
			app::CableWidget* pillCable = NULL;
			bool atInput = false;
			if (cableFocusPillAt(pillCable, atInput)) {
				pickUpCable(pillCable, atInput);
				e.consume(this);
				e.stopPropagating();
				return;
			}
		}

		// CARRYING: the next click puts the cable down, wherever it lands.
		if (carrying && e.action == GLFW_PRESS) {
			if (e.button == GLFW_MOUSE_BUTTON_RIGHT)
				cancelCarry();
			else if (e.button == GLFW_MOUSE_BUTTON_LEFT)
				dropOn(widgetAt<app::PortWidget>(APP->scene, e.pos));
			e.consume(this);
			e.stopPropagating();
			return;
		}
		// The carried cable was removed from under us — by an undo, or by its module going
		// away. Stop carrying rather than pointing at nothing.
		if (carrying && !carried)
			carrying = false;

		// PICKING UP: a plain click on a jack takes its cable, or starts a new one.
		if (clickCables && *clickCables && !carrying
			&& e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0) {
			if (app::PortWidget* port = widgetAt<app::PortWidget>(APP->scene, e.pos)) {
				pickUp(port);
				e.consume(this);
				e.stopPropagating();
				return;
			}
		}

		// A click anywhere puts down a scope that is riding the pointer. It has to be caught
		// here: a following scope is click-through, so the click lands on whatever is beneath
		// it — a panel, another module — and would drag that instead of dropping the scope.
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0 && scopeDepositFollowing()) {
			e.consume(this);
			e.stopPropagating();
			return;
		}

		// A plain click on a cable's pill takes that cable — but NEVER a press that is on a
		// jack. Claiming those broke dragging a cable off a terminal, which is the single most
		// common thing anyone does in Rack: the port never saw the press, so the drag became a
		// module drag. The pill sits outside the jack, so a press over a jack is always meant
		// for the jack.
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT
			&& (e.mods & RACK_MOD_MASK) == 0
			&& !widgetAt<app::PortWidget>(APP->scene, e.pos)
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
		app::PortWidget* port = widgetAt<app::PortWidget>(APP->scene, e.pos);
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

		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel("Clip on"));
		if (scopesOn) {
			menu->addChild(createMenuItem("Oscilloscope", "", [weakPort]() {
				if (weakPort)
					scopeCreate(weakPort);
			}));
		}

		// Injectors drive a port, so they are offered on inputs only. An output is written by
		// its own module and nothing else may write to it.
		if (widgetsOn && injectorAcceptsPort(port)) {
			menu->addChild(createMenuItem("Gate button", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_GATE);
			}));
			menu->addChild(createMenuItem("Pulse button", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_PULSE);
			}));
			menu->addChild(createMenuItem("Clock", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_CLOCK);
			}));
			menu->addChild(createMenuItem("DC level", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_DC);
			}));
			menu->addChild(createMenuItem("LFO", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_LFO);
			}));
			menu->addChild(createMenuItem("VFO", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_AUDIO);
			}));
			// The same oscillator, opened in note mode. Two entries because the two uses are
			// different — a test tone at 440 Hz, or a note to play something with — and either
			// can be switched to the other from its own menu.
			menu->addChild(createMenuItem("Musical note", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_AUDIO, true);
			}));
			menu->addChild(createMenuItem("Volt/oct", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_NOTE);
			}));
			menu->addChild(createMenuItem("Noise", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_NOISE);
			}));
			menu->addChild(createMenuItem("Attenuverter", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_AV);
			}));
		}

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
		if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ESCAPE && scopeDepositFollowing()) {
			e.consume(this);
			e.stopPropagating();
			return;
		}
		widget::Widget::onHoverKey(e);
	}
};


widget::Widget* createInterceptOverlay(bool* sliderScroll, bool* clickCables,
	bool* offerScopes, bool* offerWidgets, bool* trace, bool* demoPointer) {

	InterceptOverlay* overlay = new InterceptOverlay;
	overlay->sliderScroll = sliderScroll;
	overlay->clickCables = clickCables;
	overlay->offerScopes = offerScopes;
	overlay->offerWidgets = offerWidgets;
	overlay->trace = trace;
	overlay->demoPointer = demoPointer;
	return overlay;
}
