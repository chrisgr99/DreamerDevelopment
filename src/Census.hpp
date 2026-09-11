#pragma once
/** What every installed module calls its ports.

A port's name lives in PortInfo, which exists only once a module has been instantiated — so it
cannot be read from the files on disk, and the rules that colour a jack by its name cannot be
written against anything but guesswork until it has been. This walks the plugin registry,
creates one of every model, writes down what it calls its ports and parameters, and throws it
away again.

NAMES ONLY, NOT POSITIONS. A position lives on the widget, and creating a widget for every model
in the library loads every panel and runs every maker's constructor against a module that is in
no rack — which is exactly the path that crashed Rack from the module browser. The names are what
a colouring rule matches on, so this stops short of that.

It is a menu item rather than something that happens on its own: instantiating a couple of
thousand modules is not what somebody starting Rack asked for.

AND THE MENU ITEM IS NOT THERE UNLESS IT IS ASKED FOR. Creating every module in an installed
library runs every maker's constructor against a module that is in no rack, and one that
misbehaves takes Rack down with it. That is a fair risk to take on your own machine and not one
to hand to somebody who found the module in the browser — so the items appear only when a file
called `census.enable` sits in the plugin's own folder beside Rack's settings. An empty file is
enough. Kept rather than deleted before a release, because a tool that has to be re-added every
time it is wanted is a tool that gets lost. */
#include "plugin.hpp"

#include <string>

/** Whether census.enable is there, and the menu should offer this at all. */
bool censusOffered();

/** WHERE EVERY JACK AND KNOB SITS, which the census above cannot tell you.

A position lives on the WIDGET, so this creates each model's widget as well as its module —
loading its panel and running its author's constructor against a module that is in no rack. That
is the path that crashes, which is why it takes a filter and is meant to be pointed at one
plugin, or one model, at a time. Rack does the same walk itself when it screenshots every module,
which is fair evidence that it survives.

Written to DreamerDevelopment/census-positions.json. `only` is matched against "plugin/model",
so "NYSTHI" takes a whole maker and "NYSTHI/Model277" takes one module. */
int censusPositions(const std::string& only);

/** THE SAME WALK, A SLICE AT A TIME.

Done in one go it holds the UI thread for as long as it takes — the window stops redrawing, and
a scan that is working looks exactly like a scan that has hung. So it is started, ticked once a
frame with a time budget, and finished; the module it was started from shows how far along it is.

Start it, then call tick every frame while busy is true. Status is a line of text to draw. */
void censusStart(const std::string& only);
bool censusBusy();
void censusTick(double seconds);
std::string censusStatus();

/** Writes the census to DreamerDevelopment/census.json, and returns how many models were read.
`only` narrows it to plugins whose slug begins with one of the comma-separated prefixes given —
empty for all of them. */
int censusWrite(const std::string& only);
