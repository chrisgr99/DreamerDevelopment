#pragma once
/** Trackpad pinch, macOS.

GLFW has no gesture API on any platform — not one mention of magnify, pinch or gesture in its
header — so this cannot arrive through Rack's normal input path. On macOS it comes as an
NSEvent of type magnify, picked up by a local event monitor.
*/

namespace drui {

/** Installs the magnify monitor. Idempotent, and a no-op where there is none. */
void pinchInit();

/** Magnification accumulated since the last call, cleared by reading. */
float pinchTake();

/** Whether pinch events are taken away from everything else. A local monitor passes the event on
by default, and Rack's own window sees it too; while the view is held on rows, or while this
plugin is acting on pinches itself, nobody else should act on the same gesture. */
void pinchSetSwallow(bool swallow);

/** Seconds since the last magnify event arrived, or a large number if none ever has. Used to
tell when a gesture has ended, since there is no gesture-end event to rely on. */
double pinchIdleTime();

}
