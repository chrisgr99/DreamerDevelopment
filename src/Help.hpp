#pragma once
/** WHAT A MODULE IS, WITHOUT LEAVING THE RACK.

A maker's manual is a web page, and reaching it means leaving what you were doing, finding the
right page among a hundred modules, and reading six hundred words to learn what three jacks do.
The question being answered here is smaller than that: what is this thing, and what goes in its
ports. A badge on the module answers it where the module is.

POINT FORM, FROM THE MAKER'S OWN MANUAL, IN OUR WORDS. Each entry is a first line saying what the
module is, then one line per port or control, each led by the name printed on the panel, with
menu options gathered at the end. Written by reading the manual rather than generated from the
module, because a generated sheet can only repeat the port names that are already on the panel —
which on the modules that most need explaining are numbers.

The text lives in research/help/<maker>.json and is flattened into HelpText.cpp by
research/make_help.py. Edit the JSON, never the generated table.

NOT ON EVERY MODULE, AND THAT IS VISIBLE. A maker with no entry yet gets a panel saying so rather
than a guess. Nothing here is inferred: an entry exists because their manual was read.

ON THE DARK MODULE FOR NOW. It belongs in Clarity eventually — it is a thing a rack has once, not
a thing a module does — but Dark is where the tools that walk everybody else's widgets already
live, and moving it later is moving a call. */
#include "plugin.hpp"

#include <string>
#include <vector>

/** One module's text, in the generated table.

AND WHICH LINE BELONGS TO WHICH JACK. The lines are written one per control, so a click on a
control can be answered with the line that describes it — but only if something records that this
input, by number, is the one line four is about. That is what the three index tables are: one
entry per input, output and parameter, holding the line number that covers it, or -1 where
nothing does.

Written by hand in the JSON beside the text, because it cannot be derived: the makers who most
need explaining are exactly the ones who leave every port unnamed. */
struct HelpEntry {
	const char* plugin;
	const char* model;
	const char* const* lines;
	int count;
	const short* inputs;
	int inputCount;
	const short* outputs;
	int outputCount;
	const short* params;
	int paramCount;
};

/** What kind of thing was clicked. */
enum HelpKind { HELP_INPUT, HELP_OUTPUT, HELP_PARAM };
extern const HelpEntry HELP[];
extern const int HELP_COUNT;

/** The lines for a module, or an empty vector if nobody has written any. */
std::vector<std::string> helpFor(const std::string& plugin, const std::string& model);

/** The one line covering this jack or knob, or empty if nothing does. */
std::string helpForControl(const std::string& plugin, const std::string& model,
	HelpKind kind, int index);

/** Puts a help badge on every module in the rack, or takes them all away.

Cheap to call every frame: it does nothing unless the rack has changed or the switch has moved.

HOW IT IS ASKED. Clicking a badge puts the rack into help mode — every badge lights, and a click
on any jack or knob says what that one is rather than doing what it normally would. Clicking a
badge again, or pressing Escape, leaves. The click that turns it on says nothing: it is the way
in, not a question.

A modified click was tried first and could not be made to work. See the note in Help.cpp: a
widget inside a module never sees a click while that module is selected, which is the ordinary
state of a module somebody is working on. */
void helpStep(bool enabled);

/** Whether opening the panel also reads it out.

SPEECH IS THE ONLY WAY THE TEXT CAN BE HEARD. Rack publishes nothing to the accessibility API, so
no reader can find this text however it is selected or copied; the plugin says it itself, with
`say`, in the same voice the demo system uses. Mac only, deliberately — this is a personal tool
on a Mac, and everywhere else the panel is still there to be read with the eyes. */
void helpSetSpeak(bool on);

/** Whether the rack is in help mode at this moment.

FOR EVERY OTHER GESTURE WE OWN. Clarity has gestures of its own on the rack — clicking a jack to
carry a cable, most of all — and those run from their own handler high in the scene, where the
help catcher cannot get in front of them. A mode that answers a click about a jack while another
part of the same plugin picks a cable up off it is not a mode. Anything of ours that acts on a
click asks this first and stands down. */
bool helpModeOn();

/** Takes every badge away. Called when the last module that asked for them leaves, because a
widget added to somebody else's module must not outlive the reason for it. */
void helpRemoveAll();
