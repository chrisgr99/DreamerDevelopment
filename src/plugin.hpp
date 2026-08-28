#pragma once
#include <rack.hpp>

using namespace rack;

extern Plugin* pluginInstance;
extern Model* modelDRUI;

/** The pinch-zoom overlay, added to the Scene so it works in screen coordinates. */
widget::Widget* createPinchZoomOverlay(bool* enabled);

/** Scroll-wheel adjustment for sliders. Also added to the Scene, and kept as its LAST child
so it is offered events before any open menu. */
widget::Widget* createSliderScrollOverlay(bool* enabled);
