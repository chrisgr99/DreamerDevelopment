#pragma once
/** Finding the widget under a point, the way the event system would.

Needed because several of our gestures are handled by a widget that is not the one being
pointed at — the intercept overlay, which sits over the whole scene, and a scope's grab
handle, which is dropped onto someone else's jack.
*/
#include "plugin.hpp"

#include <widget/ZoomWidget.hpp>


/** The deepest widget of type T under `pos`, searching in the same reverse-child order the
event system uses so the topmost one wins. `pos` is in `w`'s coordinate space. */
template <typename T>
static T* widgetAt(widget::Widget* w, math::Vec pos) {
	for (auto it = w->children.rbegin(); it != w->children.rend(); it++) {
		widget::Widget* child = *it;
		if (!child->isVisible())
			continue;
		if (!child->box.contains(pos))
			continue;
		math::Vec childPos = pos.minus(child->box.pos);
		// The rack sits inside a ZoomWidget, which divides event positions by its zoom before
		// handing them to its children. Miss this and every hit test inside the rack is wrong
		// at any zoom but 100%.
		if (widget::ZoomWidget* zoomer = dynamic_cast<widget::ZoomWidget*>(child))
			childPos = childPos.div(zoomer->zoom);
		// Depth first: one nested inside this child is nearer the front than the child.
		if (T* found = widgetAt<T>(child, childPos))
			return found;
		if (T* hit = dynamic_cast<T*>(child))
			return hit;
	}
	return NULL;
}
