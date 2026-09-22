/** See RowView.hpp. */
#include "plugin.hpp"
#include "RowView.hpp"
#include "Settings.hpp"

#include <app/RackScrollWidget.hpp>

#include <ui/MenuOverlay.hpp>
#include <ui/TextField.hpp>

#include <cmath>


/** A fifth of the row above and of the row below, so the window says there is more of the rack
in both directions and shows a little of what is there. */
static const float PEEK = 0.2f;

/** ONE GESTURE, ONE ROW. A turn of the wheel or a swipe arrives as a stream of movements, and
stepping by how far it went made how many rows you moved impossible to judge. The first movement
moves a row and the rest of the gesture is ignored; a quiet spell this long ends it, and the next
movement moves another row — including a movement the other way, so turning back moves nothing
until that long has passed since the row moved. A sideways scroll never counts, however much up
and down is mixed into it. */
static const double SCROLL_GAP = 0.4;

/** Which row is at the top, and what we last put the view at, so a move made by anything else can
be told from our own and answered by settling on the nearest row. */
static bool gHave = false;
static int gRow = 0;
static float gLastY = 0.f;
/** When the last wheel movement came in. A PAUSE ENDS A GESTURE: what is left over from one turn
of the wheel must not be waiting to be added to the next, or how far you have to turn depends on
what you did a minute ago. */
static double gScrollAt = 0.0;
/** And when the wheel last moved up or down at all, which ends one gesture and begins the next. */
static double gVertAt = 0.0;
/** When a drag last pushed the view past a row, and how long before it may again. */
static double gDragStepAt = 0.0;
static const double DRAG_STEP = 0.35;
static bool gOn = false;

bool rowViewOn() {
	return gOn;
}

/** The top of the view for a given row. IN ROWS, not in pixels: the scroll widget's grid offset
is counted in grid squares, one row tall and one HP wide, so a row is 1 and the peek is a tenth. */
static float topOf(int row) {
	return (float) row - PEEK;
}

static int rowAt(float y) {
	return (int) std::lround(y + PEEK);
}

/** THE ARROW KEYS, READ FROM THE KEYBOARD RATHER THAN WAITED FOR.

Rack answers the arrow keys itself, before any child of the scene is offered them, so a handler in
an overlay never sees one — tried, and nothing arrived. The keys are read directly instead, once a
frame, and a press is the moment one goes down. Holding a key repeats, after a pause, as a
keyboard does.

NOT WHILE SOMETHING ELSE IS LISTENING: a menu is open, or a text field has been clicked into, in
which case the arrows belong to whatever is being typed in. */
static const double KEY_FIRST = 0.35;
static const double KEY_REPEAT = 0.12;
static int gKeyHeld = 0;
static double gKeyAt = 0.0;
static bool gKeyRepeating = false;
static bool gKeyCounting = false;
/** How many rows were on show last frame, so a change can be noticed. */
static int gRowsShown = 0;
/** A change of row count waiting for Rack to settle the zoom, and where the pointer was. */
static bool gPending = false;
static math::Vec gPendingAt;
/** The zoom last asked of Rack, so it is asked once and not every frame on the way there. */
static float gZoomAsked = 0.f;

static bool somethingElseIsListening() {
	if (APP->event && dynamic_cast<ui::TextField*>(APP->event->selectedWidget))
		return true;
	if (!APP->scene)
		return false;
	for (widget::Widget* w : APP->scene->children) {
		ui::MenuOverlay* over = dynamic_cast<ui::MenuOverlay*>(w);
		if (over && over->isVisible())
			return true;
	}
	return false;
}

static void rowViewKeys() {
	GLFWwindow* win = APP->window ? APP->window->win : NULL;
	if (!win || somethingElseIsListening()) {
		gKeyHeld = 0;
		return;
	}
	const bool up = glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS;
	const bool down = glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS;
	// Both at once, or a modifier held, is not ours: Rack and other plugins use those.
	const int mods = APP->window->getMods() & RACK_MOD_MASK;
	const int key = (up == down || (mods != 0 && mods != RACK_MOD_CTRL))
		? 0 : (down ? 1 : -1);
	// COMMAND WITH THEM CHANGES HOW MANY ROWS ARE SHOWN, one to five, rather than moving.
	const bool counting = (mods == RACK_MOD_CTRL);
	const double now = system::getTime();
	if (key == 0) {
		gKeyHeld = 0;
		return;
	}
	// The count goes the other way round from the view: Command and Up shows one more row.
	auto act = [&]() {
		if (counting)
			settingsSetRowViewRows(settingsRowViewRows() + (key < 0 ? 1 : -1));
		else
			rowViewMove(key);
	};
	if (key != gKeyHeld || counting != gKeyCounting) {
		gKeyHeld = key;
		gKeyCounting = counting;
		gKeyAt = now;
		gKeyRepeating = false;
		act();
		return;
	}
	const double wait = gKeyRepeating ? KEY_REPEAT : KEY_FIRST;
	if (now - gKeyAt >= wait) {
		gKeyAt = now;
		gKeyRepeating = true;
		act();
	}
}

/** Puts the pointer at this place down the rack, keeping it where it is across the window. In
window coordinates, which are the scene's scaled by whatever Rack is drawing at. */
static void putPointerAtRow(app::RackScrollWidget* rs, float rackRow) {
	GLFWwindow* win = APP->window ? APP->window->win : NULL;
	if (!win)
		return;
	const float zoom = rs->getZoom();
	const float sceneY = rs->getAbsoluteOffset(math::Vec()).y
		+ (rackRow - topOf(gRow)) * zoom * RACK_GRID_HEIGHT;
	const float scale = (APP->window->windowRatio > 0.f)
		? APP->window->pixelRatio / APP->window->windowRatio : 1.f;
	double x = 0.0, y = 0.0;
	glfwGetCursorPos(win, &x, &y);
	glfwSetCursorPos(win, x, sceneY * (scale > 0.f ? scale : 1.f));
	// Rack reads the pointer from the events it is given, and a warp of its own making is one
	// it should know about at once rather than a frame later.
	APP->scene->mousePos.y = sceneY;
}


/** Where the pointer is on the rack, in rows and in HP across, which is what the offsets are
counted in. */
static math::Vec pointerOnRack(app::RackScrollWidget* rs) {
	// WORKED OUT FROM THE VIEW, not asked of the rack: the rack remembers where the pointer was
	// when it last had a hover event, which is a stale answer the moment the zoom changes.
	const math::Vec origin = rs->getAbsoluteOffset(math::Vec());
	const math::Vec grid = rs->getGridOffset();
	const float z = rs->getZoom();
	if (z <= 0.f)
		return grid;
	const math::Vec from = APP->scene->mousePos.minus(origin);
	return math::Vec(grid.x + from.x / (z * RACK_GRID_WIDTH),
		grid.y + from.y / (z * RACK_GRID_HEIGHT));
}


/** Whether something in the rack is being dragged: a cable in flight, or a widget belonging to
the rack rather than to the menu bar or a menu. */
static bool draggingInRack() {
	if (APP->scene->rack && !APP->scene->rack->getIncompleteCables().empty())
		return true;
	widget::Widget* dragged = APP->event ? APP->event->draggedWidget : NULL;
	for (widget::Widget* w = dragged; w; w = w->parent) {
		if (w == APP->scene->rack)
			return true;
	}
	return false;
}


void rowViewStep(bool on) {
	gOn = on;
	app::RackScrollWidget* rs = APP->scene ? APP->scene->rackScroll : NULL;
	if (!on || !rs || rs->box.size.y <= 0.f) {
		gHave = false;
			return;
	}

	// THE ZOOM THE ROW COUNT ASKS FOR, held against anything else that sets it.
	const int rowCount = settingsRowViewRows();
	const float want = rs->box.size.y
		/ (((float) rowCount + 2.f * PEEK) * RACK_GRID_HEIGHT);

	// SHOWING A DIFFERENT NUMBER OF ROWS KEEPS WHAT IS UNDER THE POINTER. The zoom pivots on the
	// middle of the window, so the module being looked at slid away as rows were added. Where the
	// pointer is on the rack is noted before the zoom changes, and put back under it afterwards,
	// across and down alike.
	//
	// AFTERWARDS IS NOT THE SAME FRAME. Rack carries a zoom of its own and applies it in its own
	// step, after ours, with its own pivot — an offset put right before that is thrown away, by a
	// long way. So the zoom is asked for, the view is left alone while Rack settles it, and the
	// pointer is put back on the first frame where the zoom is the one that was asked for.
	if (gHave && rowCount != gRowsShown) {
		gPendingAt = pointerOnRack(rs);
		gPending = true;
	}
	gRowsShown = rowCount;
	const bool zoomSettled = std::fabs(rs->getZoom() - want) <= 0.0005f;
	// ASKED FOR ONCE. Rack moves to a new zoom over several frames, and asking again each frame
	// re-pivots it on the middle of the window every time, which is the shudder this had.
	if (!zoomSettled && std::fabs(want - gZoomAsked) > 0.0005f) {
		gZoomAsked = want;
		rs->setZoom(want);
	}
	if (gPending) {
		// Nothing is touched until Rack has settled the zoom; then the place that was under the
		// pointer is put back under it, across and down alike, to the nearest row.
		if (!zoomSettled) {
			gLastY = rs->getGridOffset().y;
			return;
		}
		gPending = false;
		const math::Vec now = pointerOnRack(rs);
		const math::Vec grid = rs->getGridOffset();
		gRow = (int) std::lround(gPendingAt.y + PEEK - (now.y - grid.y));
		gLastY = topOf(gRow);
		rs->setGridOffset(math::Vec(grid.x + (gPendingAt.x - now.x), gLastY));
		return;
	}

	const math::Vec at = rs->getGridOffset();
	const float moved = gHave ? at.y - gLastY : 0.f;
	const double now = system::getTime();
	// A CABLE DRAGGED PAST THE EDGE MOVES A ROW. Rack edges the view along while something is
	// being dragged against the top or bottom of the window, a little at a time — held on a row,
	// that came to nothing, since each small push was put straight back. A push while a drag is
	// going on steps a whole row instead, and the drag carries on.
	//
	// WHERE THE POINTER IS, not how far Rack has managed to move the view: Rack edges the view
	// along only when the pointer is within a few pixels of the window, and the rows on show end
	// well inside that. A cable carried up out of the top row, or down out of the bottom one,
	// moves the view a row, so the row it is being carried into comes into view.
	// AND ONLY A DRAG IN THE RACK. A press on the menu bar makes that button the dragged widget,
	// and the pointer is then nowhere near the rows — which walked the view a row at a time for
	// as long as the button was held.
	const bool dragging = draggingInRack()
		&& rs->box.contains(APP->scene->mousePos);
	int push = 0;
	if (gHave && dragging) {
		const float pointerRow = (APP->scene->rack->getMousePos().y - RACK_OFFSET.y)
			/ RACK_GRID_HEIGHT;
		if (pointerRow < (float) gRow)
			push = -1;
		else if (pointerRow > (float) (gRow + rowCount))
			push = 1;
	}
	if (push != 0 && now - gDragStepAt >= DRAG_STEP) {
		gDragStepAt = now;
		gRow += push;
		// AND THE POINTER COMES WITH IT, to just inside the row that has come into view — the
		// foot of it when the drag was going up, the head of it going down. Left where it was,
		// the pointer would still be outside the rows on show, and the view would walk row after
		// row for as long as the cable was held there.
		const float land = (push < 0)
			? (float) gRow + 0.85f : (float) (gRow + rowCount) - 0.15f;
		putPointerAtRow(rs, land);
	}
	// A MOVE THAT WAS NOT OURS settles on the nearest row: that is the scroll bar, an
	// Option-drag, and Rack's own jumps, all answered the same way.
	else if (!gHave || std::fabs(moved) > 0.02f) {
		gRow = rowAt(at.y);
	}
	gHave = true;
	gLastY = topOf(gRow);
	if (std::fabs(at.y - gLastY) > 0.0005f)
		rs->setGridOffset(math::Vec(at.x, gLastY));

	rowViewKeys();
}

void rowViewMove(int rows) {
	app::RackScrollWidget* rs = APP->scene ? APP->scene->rackScroll : NULL;
	if (!gOn || !gHave || !rs || rows == 0)
		return;
	gRow += rows;
	gScrollAt = 0.0;
	gLastY = topOf(gRow);
	rs->setGridOffset(math::Vec(rs->getGridOffset().x, gLastY));
}

bool rowViewScroll(float dx, float dy) {
	if (!gOn || !gHave)
		return false;
	// SIDEWAYS IS RACK'S: only a movement more up and down than across is ours.
	if (dy == 0.f || std::fabs(dy) <= std::fabs(dx))
		return false;
	app::RackScrollWidget* rs = APP->scene ? APP->scene->rackScroll : NULL;
	if (!rs)
		return false;
	// ONE TURN OF THE WHEEL, ONE ROW. A turn arrives as a stream of movements: the first moves a
	// row and the rest are swallowed, rather than passed on, or the view would scroll freely for
	// the remainder of the turn. Another row needs both a fresh turn — the wheel still for a
	// while — and that long since the last row moved. Sideways movement never counts towards
	// either, so a wobble across cannot hold the next row off.
	const double now = system::getTime();
	const bool freshTurn = (now - gVertAt) > SCROLL_GAP;
	gVertAt = now;
	if (!freshTurn || now - gScrollAt <= SCROLL_GAP)
		return true;
	gScrollAt = now;
	// A wheel turned away from you goes UP the rack, which is what Rack does too.
	gRow -= (dy > 0.f) ? 1 : -1;
	gLastY = topOf(gRow);
	rs->setGridOffset(math::Vec(rs->getGridOffset().x, gLastY));
	return true;
}
