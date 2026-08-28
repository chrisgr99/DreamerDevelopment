#pragma once
#include <rack.hpp>

using namespace rack;

extern Plugin* pluginInstance;
extern Model* modelDRUI;

/** The pinch-zoom overlay, added to the Scene so it works in screen coordinates. */
widget::Widget* createPinchZoomOverlay(bool* enabled);
