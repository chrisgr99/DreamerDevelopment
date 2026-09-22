/** See KnobArm.hpp. */
#include "plugin.hpp"
#include "KnobArm.hpp"

#include <app/ModuleWidget.hpp>
#include <app/SliderKnob.hpp>
#include <app/Knob.hpp>
#include <app/Switch.hpp>
#include <settings.hpp>
#include <cmath>


/** RACK'S OWN DIVISORS. Holding Cmd gives a tenth, Cmd and Shift a hundredth; these are the same
numbers, so an armed knob at its second step feels exactly like a knob dragged with Cmd down. */
static const float RATE[4] = {0.f, 1.f, 1.f / 10.f, 1.f / 100.f};
/** How long the highlight takes to go once the control is disarmed. */
static const double FADE = 0.25;

static bool gEnabled = false;
static WeakPtr<app::ParamWidget> gArmed;
/** 1 full rate, 2 a tenth, 3 a hundredth. Zero when nothing is armed. */
static int gLevel = 0;
/** Whether the armed control has been turned since it was armed: a click after turning starts
again at full rate rather than stepping to a finer one. */
static bool gTurned = false;
/** What is left of the highlight after the control was put away. */
static WeakPtr<app::ParamWidget> gFading;
static int gFadeLevel = 0;
static double gFadeUntil = 0.0;
/** Fractions of a step held back on a knob that snaps to whole numbers. */
static float gSnapAcc = 0.f;
/** When the last arming click was, so a DOUBLE CLICK is not read as two of them. Rack resets a
control to its default on a double click, and that second click must not also step the rate: the
control would go back to its default and be left in fine mode, which nobody asked for. */
static double gClickedAt = 0.0;
/** Two clicks closer together than this are one gesture. Rack has no double-click interval of its
own, so this is the usual one. */
static const double DOUBLE = 0.35;


/** A SLIDER RATHER THAN A KNOB. Rack builds a fader as a kind of knob, so they arrive here alike;
they are told apart because a slider does not carry the rates — see knobArmClick. */
bool knobArmIsSlider(app::ParamWidget* pw) {
	return dynamic_cast<app::SliderKnob*>(pw) != NULL;
}


static double now() {
	return (APP && APP->window) ? APP->window->getFrameTime() : 0.0;
}


static void disarm() {
	if (gArmed) {
		gFading = gArmed;
		gFadeLevel = gLevel;
		gFadeUntil = now() + FADE;
	}
	gArmed = NULL;
	gLevel = 0;
	gTurned = false;
	gSnapAcc = 0.f;
}


bool knobArmEnabled() {
	return gEnabled;
}


void knobArmStep(bool enabled) {
	if (enabled != gEnabled) {
		gEnabled = enabled;
		if (!enabled)
			disarm();
	}
	if (!gEnabled)
		return;
	// The control or its module has gone.
	if (gLevel > 0 && !gArmed)
		disarm();
	if (!gArmed)
		return;

	// THE POINTER HAS LEFT THE CONTROL: put it away, and let the mark fade. The control itself
	// rather than the module around it — the mark says "the wheel turns this", and it should stop
	// saying so the moment the pointer is somewhere the wheel would do something else.
	app::ModuleWidget* mw = gArmed->getAncestorOfType<app::ModuleWidget>();
	if (!mw)
		return;
	const math::Vec at = gArmed->getRelativeOffset(math::Vec(0.f, 0.f), APP->scene->rack);
	const math::Rect on = math::Rect(at, gArmed->box.size);
	if (!on.contains(APP->scene->rack->getMousePos()))
		disarm();
}


/** ONLY WHAT TURNS. A knob, or a slider — Rack builds its sliders as a kind of knob, so one test
covers both. A button, a toggle or a rotary switch is a ParamWidget as well, but nothing about it
is continuous, and arming one put the green disc on a push button, which said the wheel would turn
it. Reported from testing. */
bool knobArmAccepts(app::ParamWidget* pw) {
	if (!pw || !pw->getParamQuantity())
		return false;
	return dynamic_cast<app::Knob*>(pw) != NULL || knobArmIsStepped(pw);
}


/** A STEPPED CONTROL: a parameter control that is neither a knob nor a button — a numbered plate,
a column of lamps. It answers the wheel in whole steps of its own, so it is armed like a knob but
does its own stepping, and it has one rate, since a step is already as fine as it goes. Found by
what it is not, which is what lets this reach the plates on another maker's panel as well as ours. */
bool knobArmIsStepped(app::ParamWidget* pw) {
	return pw && pw->getParamQuantity() && dynamic_cast<app::Knob*>(pw) == NULL
		&& dynamic_cast<app::Switch*>(pw) == NULL;
}


bool knobArmClick(app::ParamWidget* pw) {
	if (!gEnabled || !knobArmAccepts(pw))
		return false;
	const double t = now();
	if (gArmed == pw) {
		const bool doubleClick = (t - gClickedAt) < DOUBLE;
		gClickedAt = t;
		// A double click is Rack resetting the control: the rate goes back to full and the second
		// click does not step it.
		// Otherwise: turned since it was armed, the click starts again at full rate; untouched,
		// it steps through the rates and round again.
		// A SLIDER HAS ONE RATE. Its mark is a bar down the middle of the track with nothing to
		// vary, so clicking it again says nothing — and a double click still reaches Rack, which
		// puts the control back to its default.
		gLevel = (doubleClick || gTurned || knobArmIsSlider(pw) || knobArmIsStepped(pw))
			? 1 : (gLevel % 3) + 1;
		gTurned = false;
		gSnapAcc = 0.f;
		return true;
	}
	gClickedAt = t;
	disarm();
	gArmed = pw;
	gLevel = 1;
	gTurned = false;
	gSnapAcc = 0.f;
	return true;
}


void knobArmClickedAway() {
	if (gEnabled)
		disarm();
}


int knobArmScroll(app::ParamWidget* under, float dy) {
	if (!gEnabled || !gArmed || dy == 0.f)
		return ARM_NONE;
	// THE POINTER HAS TO BE ON IT. Arming says which control the wheel is for; it does not follow
	// the wheel around the rack.
	if (under != gArmed.get())
		return ARM_NONE;
	// A stepped control steps itself: the wheel is let through to it.
	if (knobArmIsStepped(under)) {
		gTurned = true;
		return ARM_PASS;
	}
	engine::ParamQuantity* pq = gArmed->getParamQuantity();
	if (!pq)
		return ARM_NONE;

	// Rack's own arithmetic for a wheel over a knob: the sensitivity setting, the parameter's
	// range, and the rate this control is set to.
	float delta = dy * settings::knobScrollSensitivity * RATE[math::clamp(gLevel, 0, 3)];
	if (pq->isBounded())
		delta *= pq->getRange();
	if (pq->snapEnabled) {
		gSnapAcc += delta;
		delta = std::trunc(gSnapAcc);
		gSnapAcc -= delta;
	}
	if (delta != 0.f)
		pq->setValue(pq->getValue() + delta);
	gTurned = true;
	return ARM_TAKEN;
}


app::ParamWidget* knobArmWidget() {
	if (gArmed)
		return gArmed.get();
	if (gFading && now() < gFadeUntil)
		return gFading.get();
	return NULL;
}


int knobArmLevel() {
	return gArmed ? gLevel : gFadeLevel;
}


float knobArmAlpha() {
	if (gArmed)
		return 1.f;
	if (gFading && now() < gFadeUntil)
		return math::clamp((float) ((gFadeUntil - now()) / FADE), 0.f, 1.f);
	return 0.f;
}
