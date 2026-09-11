#pragma once
/** Dark panels for makers who ship none.

Rack's "prefer dark panels" chooses between two drawings a plugin has shipped. A plugin that
ships one drawing has nothing to choose from, and a great many do not. This darkens those, in
memory, from the artwork already loaded.

HOW IT DECIDES WHAT TO CHANGE, in two numbers and two colours:

  A shape painted near-white is BACKGROUND and becomes near-black, whatever its size — small
  white detail on a white panel would be invisible, so every white shape is either the ground or
  a mask over it. A shape painted near-black and smaller than a lettering-sized fraction of the
  panel is LETTERING or a rule, and becomes light so it survives on the new ground. Everything
  else is left exactly as it was, which is what keeps a maker's reds red.

  SIZE IS A PERCENTAGE OF THE PANEL rather than a measurement, so one threshold works on a 4 HP
  module and a 60 HP one alike.

  AND THE PANEL HAS TO BE LIGHT TO BEGIN WITH. A drawing with no large near-white area is left
  entirely alone — otherwise a mid-toned panel keeps its ground and loses its lettering, which
  is worse than doing nothing.

IN PLACE, AND REVERSIBLE. Rack parses each panel once and hands the same drawing to every
instance of that model, so changing it changes all of them at once. Every colour changed is
written down first, so switching off puts the panel back without a reparse and without a
restart.

NOT WRITTEN TO DISK, EVER. The artwork belongs to whoever drew it. Transforming what is already
in memory, on the machine of somebody who already has it, is the same posture as a dark mode in
a browser; shipping the result would not be.

NO SETTINGS, AND NOT PART OF CLARITY. What counts as background, what counts as lettering, and
which families need their titles written back are decided in Dark.cpp, where they can be argued
with in code. It lives in a module of its own so that the shipped Clarity is the same Clarity
everybody else has. */
#include "plugin.hpp"

#include <string>

/** Applies the darkening to whatever is in the rack, or puts every panel back when `enabled` is
false. Cheap to call every frame: it does nothing unless the rack has changed or the switch has
moved. */
void darkStep(bool enabled);

/** Puts every panel back. Called when the last Darkener leaves, so nothing outlives the module
that asked for it. */
void darkRestoreAll();
