/** Scroll wheel adjusts sliders, in menus and anywhere else they appear.

WHY THIS IS NOT SIMPLY A SCROLL HANDLER ON THE SLIDER. A plugin cannot change ui::Slider,
and Slider inherits OpaqueWidget, whose scroll handler calls stopPropagating(). A scroll
over a slider therefore DIES at the slider — not consumed, but never offered to anything
else — so no widget placed anywhere in the scene can pick it up afterwards.

The way round it is to be offered the event before the menu rather than after. Events start
at the Scene and are offered to its children in REVERSE order, so the last child gets first
refusal. Rack appends the menu overlay when a menu opens, which is why the menu currently
wins; this widget moves itself back to the end of that list whenever it is not already
there, and so gets first refusal instead.

That is a strong position to hold, so this handles exactly one case and passes everything
else through untouched: if the widget under the pointer is a Slider, its value moves and the
event stops here; otherwise nothing happens and the event carries on to whatever would have
received it — a long menu's own scrolling, knob scroll, the rack view.

It matters because dragging a slider calls cursorLock(), which fights a screen magnifier
that follows the pointer, leaving the wheel as the only practical route.
*/
#include "plugin.hpp"

#include <ui/Slider.hpp>

#include <algorithm>


/** The deepest Slider under `pos`, searching in the same reverse-child order the event
system uses so the topmost one wins. `pos` is in `w`'s coordinate space. */
static ui::Slider* sliderAt(widget::Widget* w, math::Vec pos) {
	for (auto it = w->children.rbegin(); it != w->children.rend(); it++) {
		widget::Widget* child = *it;
		if (!child->isVisible())
			continue;
		if (!child->box.contains(pos))
			continue;
		const math::Vec childPos = pos.minus(child->box.pos);
		// Depth first: a slider nested inside this child is nearer the front than the child.
		if (ui::Slider* found = sliderAt(child, childPos))
			return found;
		if (ui::Slider* slider = dynamic_cast<ui::Slider*>(child))
			return slider;
	}
	return NULL;
}


struct SliderScrollOverlay : widget::Widget {
	bool* enabled = NULL;

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
		if (!enabled || !*enabled || e.scrollDelta.y == 0.f) {
			widget::Widget::onHoverScroll(e);
			return;
		}
		// We have no children, so this searches from the scene rather than from ourselves.
		// Our box sits at the scene's origin, so the event position needs no adjustment.
		ui::Slider* slider = sliderAt(APP->scene, e.pos);
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
};


widget::Widget* createSliderScrollOverlay(bool* enabled) {
	SliderScrollOverlay* overlay = new SliderScrollOverlay;
	overlay->enabled = enabled;
	return overlay;
}
