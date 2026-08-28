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
#include <app/PortWidget.hpp>
#include "Injector.hpp"
#include "WidgetAt.hpp"

#include <algorithm>


struct InterceptOverlay : widget::Widget {
	bool* sliderScroll = NULL;

	void step() override {
		// Cover the scene, or the event system will not offer us events outside our box.
		if (parent)
			box.size = parent->box.size;

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

	void onHoverScroll(const HoverScrollEvent& e) override {
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
	void onButton(const ButtonEvent& e) override {
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
		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel("Clip on"));
		menu->addChild(createMenuItem("Scope", "", [weakPort]() {
			if (weakPort)
				scopeCreate(weakPort);
		}));

		// Injectors drive a port, so they are offered on inputs only. An output is written by
		// its own module and nothing else may write to it.
		if (injectorAcceptsPort(port)) {
			menu->addChild(createMenuItem("Gate button", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_GATE);
			}));
			menu->addChild(createMenuItem("Trigger button", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_PULSE);
			}));
			menu->addChild(createMenuItem("DC level", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_DC);
			}));
			menu->addChild(createMenuItem("Waveform", "", [weakPort]() {
				if (weakPort)
					injectorCreate(weakPort, INJECT_LFO);
			}));
		}

		e.consume(this);
		e.stopPropagating();
	}

	/** Escape deposits a scope riding the pointer. It has to be reachable from here because a
	following scope is click-through, so it is never the hovered widget itself. */
	void onHoverKey(const HoverKeyEvent& e) override {
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


widget::Widget* createInterceptOverlay(bool* sliderScroll) {
	InterceptOverlay* overlay = new InterceptOverlay;
	overlay->sliderScroll = sliderScroll;
	return overlay;
}
