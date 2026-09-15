/** Finding a module in the generated family table.

Separate from the generated file so that the lookup can be read and changed by a person while
the table itself stays a build product nobody edits. */
#include <rack.hpp>
#include <string>
#include <cstring>
#include "PortFamilies.hpp"


int portFamilyFor(const std::string& plugin, const std::string& model,
		bool isOutput, int port) {
	if (port < 0)
		return -1;
	// LINEAR, AND THAT IS FINE. This is asked once per port while a panel is drawn, over a
	// few thousand rows, and the strings differ in their first character nearly every time.
	// A binary search would need the generator to guarantee an ordering, which is one more
	// thing that can silently stop being true.
	for (int i = 0; i < PORT_FAMILY_COUNT; i++) {
		const PortFamilyEntry& e = PORT_FAMILIES[i];
		if (plugin != e.plugin || model != e.model)
			continue;
		const signed char* table = isOutput ? e.out : e.in;
		const int count = isOutput ? e.outCount : e.inCount;
		if (!table || port >= count)
			return -1;
		return table[port];
	}
	return -1;
}
