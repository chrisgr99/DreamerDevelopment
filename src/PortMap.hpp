#pragma once
/** WHAT A PORT IS, ON MODULES THAT DO NOT SAY.

Clarity decides a jack's colour from its NAME, and that works for two ports in three across the
installed library. It cannot work at all on a plugin that names none of them: NYSTHI has 3,104
ports and names 187, so every rule in the table is blind there.

What identifies those ports is not a word but a position: this plugin, this model, this input,
this number. That is what this table holds, read off the panels themselves with each port's
index drawn onto its own jack.

BENEATH THE USER'S OWN OVERRIDES AND ABOVE THE RULES. Somebody who right-clicks a jack and sets
its family has said something about their own rack and must win. The rules are guesses from
words; this is a reading of a panel, so it beats them.

AND IT CHECKS BEFORE IT SPEAKS. Every module records the number of ports it was read at. A
module with a different number now is a module whose numbering may have moved, and the whole of
it is ignored rather than half-trusted — a wrong colour is worse than no colour, because no
colour says "nobody knows" and a wrong one does not. */
#include "plugin.hpp"
#include "Palette.hpp"   // the families

#include <string>

/** The family for this port, or -1 if the table has nothing to say. */
int portMapFamily(const std::string& plugin, const std::string& model, bool isOutput, int port);

/** How many ports the model in front of us has, for the guard above. Implemented where the
module is known, since the table itself cannot see the rack. */
int portMapPortCount(const std::string& plugin, const std::string& model);
