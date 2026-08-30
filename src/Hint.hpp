#pragma once
/** One-time notes, for features nobody would find on their own.

A switch on a panel advertises a feature to someone who reads the panel. Click-to-pull cables
is the one feature that changes a gesture people already have, so it ships off — and a feature
that is off and unmentioned may as well not exist. The note appears at the moment the gesture
it replaces is being used, which is the one moment the alternative is worth hearing about.

Each note carries "Don't show this again", and that answer is remembered next to Rack's own
settings rather than in the patch: it is a fact about the person, not about the work.
*/
#include "plugin.hpp"

#include <string>
#include <vector>

/** Shows a note, unless this id has been silenced or has already been shown this session.
The first line is the heading.

`anchor` is a point in SCENE coordinates the note should appear beside — the thing it is about.
A note at the top of the window is a note nobody reads, because the eye is wherever the hand
is. It never covers the anchor itself. Leave it out to centre the note at the top.

`away` is the direction the hand is travelling. The note goes to the OPPOSITE side of the
anchor, so it never lands under the pointer or in the path it is heading down.
*/
void hintShow(const std::string& id, const std::vector<std::string>& lines,
	math::Vec anchor = math::Vec(-1.f, -1.f), math::Vec away = math::Vec());

/** Forgets every "don't show this again", so the notes come back. */
void hintResetAll();

/** Whether a note is on screen and covers this scene position.

Our own overlay is offered every click before anything else in the scene, so without this it
acts on the jack UNDER the note — the note is opaque to Rack but not to us. */
bool hintCovers(math::Vec scenePos);

/** Takes down any note on screen. Called when the last of our modules leaves. */
void hintDismiss();
