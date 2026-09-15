#pragma once

#include <string>

/** What each jack carries, for Clarity's colouring.

THE JUDGEMENT IS NOT MADE HERE. Which family a jack belongs to — audio, cv, trigger, pitch — was
decided while the help text for that module was written, by somebody looking at the panel and the
code together. It lives in the help database and is edited there; src/PortFamilies.cpp is
generated from it by tools/families.py and must not be hand-edited.

Clarity does not need the help plugin installed, or its data on disk. The table is compiled in.
*/

struct PortFamilyEntry {
	const char* plugin;
	const char* model;
	int inCount;
	const signed char* in;
	int outCount;
	const signed char* out;
};

extern const PortFamilyEntry PORT_FAMILIES[];
extern const int PORT_FAMILY_COUNT;

/** The family of one jack, or -1 where nobody said.

-1 IS A REAL ANSWER AND NOT A FAILURE. Plenty of jacks were deliberately left uncoloured: a mult
carries whatever you give it, and a router's signal path is whatever is patched. Colouring those
would be asserting something nobody established. */
int portFamilyFor(const std::string& plugin, const std::string& model,
	bool isOutput, int port);
