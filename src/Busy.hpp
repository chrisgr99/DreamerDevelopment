#pragma once
/** WHETHER THERE IS ANY AUDIO-RATE WORK TO DO AT ALL, in one number.

Five subsystems do work on the audio thread — the taps, the injectors, the monitors, the
voltmeters and the frequency counters — and each already knew whether it had anything active. So
Test Gear's process() asked all five, every sample, and every one of them said no.

FIVE QUESTIONS IS NOT THE SAME AS ONE. Each count lives in its own translation unit, which means
its own static, which means its own cache line: an idle module was touching five separate lines
of memory forty-four thousand times a second to be told five times that there was nothing to do.
The arithmetic in that path is a handful of instructions and the memory traffic is not, and it is
the memory traffic that varies from machine to machine — which is the shape of a cost that shows
up on somebody else's computer and not on yours.

One counter, one line, one branch. Each subsystem keeps its own count as well, because each still
needs to know about itself; this is only the question asked first.
*/

/** Called alongside each subsystem's own count, with the same sign. */
void busyAdd(int delta);
/** True if anything at all wants the audio thread. */
bool busyAny();
