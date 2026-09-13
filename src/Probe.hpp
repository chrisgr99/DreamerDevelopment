#pragma once
/** WHAT A JACK ACTUALLY DOES, MEASURED RATHER THAN READ.

The Rack forum has spent a year asking how to know what an input port expects — a sensible
voltage range, continuous or stepped, whether it takes polyphony — and answering it with a scope
and a test rig, one module at a time. The help already says what a port is FOR, in words read off
each maker's manual. It cannot say whether the port takes sixteen channels, because no manual is
reliable about that and a great many say nothing at all.

But a module is a program, and the question can be put to it directly. This builds one of every
model in the installed library, feeds each input a polyphonic signal, runs the module, and writes
down what happened. It is the one source that needs no maker's cooperation: a closed plugin
answers exactly as readily as an open one, and the answer is true of the build actually installed
rather than of whatever is on the maker's main branch.

TWO QUESTIONS, BECAUSE ONE TEST MISSES HALF THE CASES.

  DOES THE COUNT FOLLOW? Set this input to sixteen channels and see whether any output goes
  polyphonic. That catches the ordinary case — an oscillator whose output width comes from its
  V/OCT jack — and misses a mixer, which reads all sixteen and sums them to one.

  IS ANYTHING BEYOND CHANNEL ONE READ AT ALL? Put a signal on channels two upwards, leave channel
  one exactly as the silent case had it, and see whether any output CHANGES. That catches the
  mixer, and is the stronger test of the two: it asks whether the module looked.

Either one answering yes means the port takes polyphony.

AND A MODULE THAT IS NOT THE SAME TWICE CANNOT BE ASKED THE SECOND QUESTION. Noise sources,
random sequencers and anything seeded from the clock give a different answer every run, so the
same measurement is taken twice before anything is concluded; where the two disagree the module
is marked as varying, and only the channel-count test is trusted for it.

WHAT IT COSTS, SAID PLAINLY. This calls process() on every module in the library from outside
Rack's engine. A module that assumes it is in a rack — that reaches for a sample rate nobody set,
or an expander that is not there — can take the application down with it, and that is a segfault
rather than an exception, so it cannot be caught. The run therefore saves as it goes and records
which model it was on: a crash costs a few models, the log names the one that did it, and
starting again carries on from where it stopped. That is a fair risk on your own machine and not
one to hand to a stranger, which is why this hides behind the same `census.enable` file the
census does.

Written to DreamerDevelopment/probe.json, beside Rack's settings. */
#include "plugin.hpp"

#include <string>

/** Start a run. `only` is matched against "plugin/model" as a prefix, so "Befaco" takes a whole
maker and "Befaco/EvenVCO" one module; empty takes the library. `skipDone` carries on from what
is already in the file rather than starting afresh — which is what you want after a crash. */
void probeStart(const std::string& only, bool skipDone);
bool probeBusy();
void probeTick(double seconds);
std::string probeStatus();
