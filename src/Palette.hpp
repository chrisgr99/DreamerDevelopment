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

enum Family {
	FAM_AUDIO,
	FAM_CV,
	FAM_TRIGGER,
	FAM_PITCH,
	NUM_FAMILIES
};

/** The colour for a family. Loads the saved palette the first time it is asked. */
NVGcolor paletteColor(int family);
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
