#pragma once
#include <rack.hpp>

using namespace rack;

extern Plugin* pluginInstance;
extern Model* modelDRUI;

/** The pinch-zoom overlay, added to the Scene so it works in screen coordinates. */
widget::Widget* createPinchZoomOverlay(bool* enabled);

/** Gestures that must be seen before anything else: scroll-wheel adjustment of sliders, and
Option-click to clip a scope onto a jack. Added to the Scene and kept as its LAST child, so
it is offered events ahead of any open menu. */
widget::Widget* createInterceptOverlay(bool* sliderScroll, bool* clickCables,
	bool* offerScopes, bool* offerWidgets, bool* trace);

/** The plugin's knob face, shared so an injector's dial matches the knobs it sits among. */
void druiDrawKnob(NVGcontext* vg, math::Vec c, float r, float angle, int ticks);

/** A clip-on scope on this port, added to the rack. */
void scopeCreate(app::PortWidget* port);
/** Deposits a scope riding the pointer in follow mode. True if one was. */
bool scopeDepositFollowing();
/** Shows or hides every scope, without removing any. */
void scopeSetVisible(bool visible);
/** Every scope's attachment and settings, for saving with the patch. */
json_t* scopeToJson();
/** Queues saved scopes to be re-attached as their modules load. */
void scopeFromJson(json_t* arrayJ);
/** Re-attaches queued scopes. Cheap and does nothing once none are waiting. */
void scopeRestoreStep();


/** Following one cable through a tangle: hover an end for a pill, click it to hold that
cable bright while the rest dim. */
void cableFocusStep();
void cableFocusDraw(NVGcontext* vg);
bool cableFocusClick();
void cableFocusClear();
void cableFocusPrepareSave();
bool cableFocusActive();
bool cableFocusHidden(app::CableWidget* cw);
/** The cable whose pill is under the pointer, and the end the pill sits on. */
bool cableFocusPillAt(app::CableWidget*& cw, bool& atInput);
