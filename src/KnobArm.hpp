#pragma once
/** CLICK A KNOB TO ARM IT, and the wheel turns that knob and nothing else.

A wheel over the rack means two things at once: move the view, or turn whatever the pointer
happens to be over. Scrolling carries the rack under a still pointer, so a run that began over bare
panel ends over a knob — and the rest of the run turns it. Timers and grace periods paper over
that; saying which control the wheel is for does not.

So: a click that does not travel ARMS the control under the pointer. While something is armed the
wheel turns it. While nothing is armed the wheel moves the view, wherever the pointer is.

AND THE CLICK IS FREE. A plain left click on a knob does nothing in Rack — the value moves only
when the pointer does — so this takes a gesture away from nobody.

Clicking again, WITHOUT having turned it, steps the rate: full, a tenth, a hundredth, and back to
full. Those are Rack's own fine and ultra-fine divisors, the ones Cmd and Cmd-Shift give, so the
feel is the one people already know. Turning it puts it back to full.

The armed control is marked with a green disc at its centre — the green of our buttons — full
size at full rate, two thirds at a tenth, one third at a hundredth. The size says the rate; there is nothing to
read. It fades over half a second when the control is disarmed. */
#include "plugin.hpp"

#include <app/ParamWidget.hpp>

/** Called every frame from the overlay's step. Does nothing while off. */
void knobArmStep(bool enabled);

/** Whether the feature is on right now. */
bool knobArmEnabled();

/** Whether this control can be armed at all: a knob, a slider or a stepped control, never a button
or a switch. */
bool knobArmAccepts(app::ParamWidget* pw);

/** A parameter control that is neither a knob nor a button: a numbered plate, a lamp column. */
bool knobArmIsStepped(app::ParamWidget* pw);

/** A click that did not travel, on this control. True if it armed or stepped it. */
bool knobArmClick(app::ParamWidget* pw);

/** A click on something that is not a control: puts the armed one away. */
void knobArmClickedAway();

/** What happened to a wheel over this point: nothing of ours, adjusted here, or — for an armed
stepped control — to be let through so the control steps itself. */
enum { ARM_NONE, ARM_TAKEN, ARM_PASS };
int knobArmScroll(app::ParamWidget* under, float dy);

/** Whether this control is a slider, which is marked with a bar rather than a disc and has one
rate. */
bool knobArmIsSlider(app::ParamWidget* pw);

/** The control the highlight is drawn on, or nothing. Its rate step is 1, 2 or 3, and the alpha
carries the fade out. */
app::ParamWidget* knobArmWidget();
int knobArmLevel();
float knobArmAlpha();
