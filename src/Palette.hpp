#pragma once
/** The colour a jack and its cable are drawn in, one per signal family, chosen by the user.

The four defaults are the ones the plugin has always used, and they are chosen for a particular
pair of eyes. Nobody else's eyes are those eyes: colour vision varies, monitors vary, and a
yellow that reads clearly on one screen is a beige on the next. So the four are settings.

BESIDE RACK'S OWN SETTINGS, NOT IN THE PATCH. This is a fact about the person looking, not about
the work. A palette saved in the patch would mean opening somebody else's file silently changed
how your whole rack is coloured, and Clarity colours every module in it — including the ones
that came from other plugins. Rack keeps its own cable palette the same way and for the same
reason.
*/
#include "plugin.hpp"

#include <app/PortWidget.hpp>
#include <string>
#include <vector>

enum Family {
	FAM_AUDIO,
	FAM_CV,
	FAM_TRIGGER,
	FAM_PITCH,
	/** MPX: a whole instrument on one cable, from the Modular Polyphonic Expression plugin.
	Not a signal family in the sense the others are — nothing about it is a voltage — which is
	exactly why it wants a colour of its own rather than being guessed at as one of them. */
	FAM_MPX,
	NUM_FAMILIES
};

/** The colour for a family. Loads the saved palette the first time it is asked. */
NVGcolor paletteColor(int family);

/** WHICH FAMILY A PORT BELONGS TO, which is the other half of the colour code and the half
that was not configurable.

Three things are asked in order, so the more particular always beats the more general:

  1. an override the user has set on THIS port of THIS module, which is exact and final
  2. the user's own name rules, read from the same file as the colours
  3. the built-in guess from the port's name

Rack has no concept of a signal family, so the guess is all there is to work from by default —
and a guess is wrong sometimes. One and two are how it gets put right, and both are kept
beside Rack's settings rather than in the patch: which family a module's port belongs to is a
fact about the module, true in every patch that uses it, so correcting it once corrects it
everywhere. */
int paletteFamilyForPort(app::PortWidget* port);

/** The family a bare port name guesses to, with the user's rules applied. For the ports of a
module that is not loaded, and for cables in flight. */
int paletteFamilyForName(const std::string& name);

/** The override on this port, or -1 for none. */
int palettePortOverride(app::PortWidget* port);
/** Sets or clears it, and writes the file. Pass -1 to go back to guessing. */
void paletteSetPortOverride(app::PortWidget* port, int family);

/** A number that changes whenever any colour or any categorisation does.

The cable colouring remembers which port each cable arrives at and leaves a cable alone while
that has not changed, since recolouring every cable in the rack on every frame is work for
nothing. That memory is about destinations, so it could not see a change of PALETTE: choosing
a new scheme left every cable in the rack painted in the old one until it was unplugged and
plugged back in. This is how it knows to forget what it decided. */
uint64_t paletteGeneration();

/** Replaces the colours AND the name rules with a named set, and saves.

A set is both halves together, because they are one answer: a scheme decides what the families
look like, and the rules decide what belongs to each. Choosing one and keeping the other's rules
would be a state nobody asked for. Per-port overrides are left alone — they name particular
ports of particular modules and are true whatever the colours are. */
void paletteApplyScheme(const char* key);
/** The sets on offer: keys and display names, ending in a NULL key. */
struct PaletteScheme { const char* key; const char* name; };
const PaletteScheme* paletteSchemes();

/** The names of the alternative sets sitting beside colours.json — any other .json file in the
same folder. Sharing a set is sending somebody a file. */
std::vector<std::string> paletteFileSets();
/** Loads one of those by name, as paletteApplyScheme loads a built-in one. */
void paletteApplyFileSet(const std::string& name);
/** Writes the colours, rules and overrides now in force to a file of that name beside
colours.json, so a set that has been worked out by hand can be kept and passed on. */
void paletteSaveAs(const std::string& name);
/** What the family is called on the panel and in the dialogue. */
const char* paletteName(int family);

/** Opens the chooser. */
void paletteShow();
/** True while the chooser is on screen and covers this scene position — the same guard the
hint dialogue needs, because our own overlay is offered every click before anything else and
would otherwise act on the jack underneath. */
bool paletteCovers(math::Vec scenePos);
/** Takes it down. Called when the last of our modules leaves. */
void paletteDismiss();
