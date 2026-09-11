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
thousand modules is not what somebody starting Rack asked for. */
#include "plugin.hpp"

#include <string>

/** Writes the census to DreamerDevelopment/census.json, and returns how many models were read.
`only` narrows it to plugins whose slug begins with that text — empty for all of them. */
int censusWrite(const std::string& only);
