/** Trackpad pinch on every platform that is not macOS — see pinch.hpp and pinch.mm.

There is nothing to hook. GLFW reports no gesture events on any platform, and neither Windows
nor Linux offers an equivalent of the macOS magnify event. These exist so that the rest of the
plugin does not have to ask which platform it is running on: they report that no pinch has
happened and that none ever has.

They live in a .cpp rather than in pinch.mm because a Windows or Linux compiler handed an .mm
file either refuses it or wants an Objective-C runtime that is not installed. The Makefile
compiles .mm only on macOS, so this file is what the other platforms link against.
*/
#include "pinch.hpp"
#include <arch.hpp>

#if !defined ARCH_MAC

namespace drui {

void pinchInit() {}

float pinchTake() {
	return 0.f;
}

double pinchIdleTime() {
	return 1e9;
}

}

#endif
