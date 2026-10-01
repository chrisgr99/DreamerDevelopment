/** See RowView.hpp. */
#include "plugin.hpp"
#include "RowView.hpp"
#include "Settings.hpp"

#include <app/RackScrollWidget.hpp>

#include <ui/MenuOverlay.hpp>
#include <ui/TextField.hpp>
#include <widget/TransparentWidget.hpp>

#include <GL/glew.h>

#include <cmath>
#include <vector>


/** A fifth of the row above and of the row below, so the window says there is more of the rack
in both directions and shows a little of what is there. */
static const float PEEK = 0.2f;

/** KEEP SCROLLING, KEEP MOVING. A turn of the wheel or a swipe arrives as a stream of movements,
and this is how much of it one row costs. Four notches of an ordinary wheel, so one
turned steadily walks the rack a row at a time for as long as it is turned, rather than moving one
row and then asking to be let go of and turned again.

ONE ROW PER MOVEMENT, WHATEVER ITS SIZE. Acceleration makes a fast turn arrive as a few large
deltas rather than many small ones, and a glide on a trackpad as very large ones; counting the
distance would then send the view flying by ten rows on one flick of a finger. So what is left
over after a row is capped below the cost of the next one: a movement can pay for one row and
never for two, and how fast the rack walks follows how fast the wheel is turned rather than how
hard.

A SIDEWAYS SCROLL NEVER COUNTS, however much up and down is mixed into it. */
static const float SCROLL_PER_ROW = 40.f;

/** A QUIET SPELL THIS LONG ENDS A GESTURE, and what was left over is thrown away with it.

This is what makes the first row of every turn cost the same. Carrying the remainder across meant
a turn that had stopped a hair short of a row left that hair banked, so the next turn moved a row
almost at once — and the one after it took a full notch. Neither was wrong by much, and the
inconsistency was the whole of the annoyance: how far you had to turn to move the first row
depended on what you had done a moment before. Reversing direction clears it for the same
reason. */
static const double SCROLL_END = 0.25;

/** WHAT THE FIRST ROW OF A TURN COSTS, which is less than a row and more than nothing.

It was nothing at all, so that a scroll did not begin with a stretch where the wheel turned and
the rack stood still. That was right about the waiting and wrong about the twitching: a small
movement, of the kind a hand makes while resting on a wheel, was enough to be judged vertical and
therefore enough to step a row nobody asked for.

Just under a whole row is where it settled, by ear: a turn anybody means still moves before the
wheel has gone a full row, and a hand jogging it does not move the rack at all. */
static const float SCROLL_FIRST_ROW = 37.5f;

/** HOW MUCH OF ONE MOVEMENT COUNTS TOWARDS THAT FIRST ROW.

macOS scales a scroll by how fast the wheel is turning, so the same distance arrives as one large
number when it is flicked and as several small ones when it is turned. A threshold counting those
numbers is therefore crossed sooner by a fast twitch than by a slow one, which is the opposite of
what anybody means: a jog of the wheel is a jog however quickly it happened.

So no single movement counts for more than a notch towards the first row. What the threshold then
measures is how LONG the wheel was turned rather than how hard it was hit, and the system's
acceleration stops having a say in whether a row moves. A deliberate flick is several movements
and still moves at once; a single sharp jog is one, and does not.

Nothing after the first row is capped this way. By then the turn has been established, and there
the acceleration is welcome: it is what lets a long flick walk the rack quickly. */
static const float SCROLL_NOTCH = 10.f;

/** Which row is at the top, and what we last put the view at, so a move made by anything else can
be told from our own and answered by settling on the nearest row. */
static bool gHave = false;
static int gRow = 0;
static float gLastY = 0.f;
/** How much of the current turn of the wheel has not yet been spent on a row, and when the last
movement of it came in. */
static float gScrollAcc = 0.f;
static double gVertAt = 0.0;
/** Whether this turn of the wheel has already moved a row. The first movement of a turn moves
one at once; everything after it is paid for. */
static bool gScrollTurning = false;
/** WHICH WAY THE TURN IS GOING, kept for itself rather than read off what is left over. The
leftover is nothing just after a row moves, and a reversal tested against nothing was not seen
as one: a slight turn back then counted as the same turn, with no notch limit, and stepped a row
back at once. */
static int gScrollDir = 0;
/** When the last row moved. */
static double gStepAt = 0.0;
/** A ROW SETTLES BEFORE THE NEXT ONE IS PAID FOR. The movement that carried one row over its
threshold carries on for a moment — a wheel slowing down, and macOS sending it in large pieces
because it was turning fast — and counted, it paid for the next row a few hundredths of a second
later. So what arrives in the first eighth of a second after a row moves is spent on nothing, and
the next row is earned by turning on. */
static const double ROW_SETTLE = 0.125;
/** HOW FAR A CABLE MUST BE PULLED TOWARDS AN EDGE before the row follows it there.

A cable is picked up where its jack is, and a jack in the fraction of a row peeking in at the top
or the foot of the window is already outside the whole rows — so the row stepped the instant the
cable was taken, before the hand had moved at all. What was meant by carrying a cable off the
bottom of the window is a movement, not a position.

Twenty pixels, the same as the window's own edges use: a deliberate pull and not a tremor. Once an
edge has been asked for it stays asked for until the cable is put down, so a cable carried back
and forth across a boundary does not keep having to ask again. */
static const float DRAG_PULL = 20.f;

/** When a drag last pushed the view past a row, and how long before it may again. */
/** THE PHOTOGRAPH THAT HIDES A ROW-COUNT CHANGE — see the overlay at the foot of this file. */
static bool gFreezeWant = false;
static bool gFrozen = false;
static double gFreezeAsked = 0.0;
static double gFreezeUntil = 0.0;
/** How long the photograph stays up once the change has been made. Two or three frames: long
enough for the view to have settled, short enough that nothing appears to have stopped. */
static const double FREEZE_HOLD = 0.06;
/** And how long to wait for the photograph before going ahead without one, for a window that is
not being drawn at all. */
static const double FREEZE_WAIT = 0.25;

/** A top row asked for from outside, waiting to be applied. ASKED FOR RATHER THAN SET, because
the caller may be a plugin whose own step runs before this one's, in the same frame as a patch is
loaded — with the view not yet held on rows, and about to be moved again by whatever framed the
patch. Remembered here and applied on the next step, which is the frame after the load and so
the first frame anybody sees. */
/** WHETHER THE NEXT COUNT CHANGE PIVOTS ON THE POINTER. A change made by a hand — the wheel, or
Command and an arrow — keeps what is under the pointer under it, which is the whole point of
pivoting there. A change asked for from outside has no hand behind it and no reason to believe
the pointer is anywhere in particular; it pivots on the middle of the window instead. Set at the
load of a patch, the pointer was wherever it had been left and the rack was thrown off to one
side of the window. */
static bool gPivotOnPointer = true;

static bool gWantTop = false;
static int gWantTopRow = 0;

static double gDragStepAt = 0.0;
/** Where the pointer was when this drag began, and which way it has since been pulled. */
static bool gDragHave = false;
static float gDragFromY = 0.f;
static bool gDragUp = false, gDragDown = false;
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


/** ONE ROW PAST THE END OF THE PATCH, AND NO FURTHER.

Scrolling could carry the view off into empty rack for as far as anybody cared to turn, and
getting back meant turning the same distance again. But stopping while a whole row of modules was
still on show stopped too soon: with one row in the window there was then no way to look at the
empty row below the last one, which is where the next module goes.

So the view may go one row beyond the rows the patch occupies, in either direction. At the limit
the last row of the patch is the fifth of a row peeking in at the top edge, with empty rack below
it — which is exactly the view you want when adding to the end of a patch — and the first row
peeks in at the bottom.

The rows the patch occupies, not the rows that exist: an empty row in the middle of a patch is
scrolled past like any other, because what is above and below it is worth reaching. A rack with
nothing in it is not clamped at all. */
static bool occupiedRows(int* first, int* last) {
	if (!APP->scene || !APP->scene->rack)
		return false;
	bool any = false;
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		const int row = (int) std::floor((mw->box.pos.y - RACK_OFFSET.y)
			/ RACK_GRID_HEIGHT + 0.5f);
		if (!any || row < *first)
			*first = row;
		if (!any || row > *last)
			*last = row;
		any = true;
	}
	return any;
}

static int clampRow(int row) {
	int first = 0, last = 0;
	if (!occupiedRows(&first, &last))
		return row;
	const int shown = settingsRowViewRows();
	// The topmost row of the window may be as high as one row past the first occupied one, which
	// leaves that row peeking in at the bottom edge, and as low as one row past the last, which
	// leaves that one peeking in at the top.
	const int lowest = first - shown;
	const int highest = last + 1;
	if (highest < lowest)
		return row;
	return math::clamp(row, lowest, highest);
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
	// COMMAND AND UP MAKES THE MODULES BIGGER, which is one row FEWER: the keys read as a
	// zoom, since that is what changing the count amounts to. Turned round in the menu for
	// anyone who reads them as up for more rather than up for closer.
	auto act = [&]() {
		if (counting) {
			const int bigger = (key < 0) ? -1 : 1;   // key < 0 is Up
			settingsSetRowViewRows(settingsRowViewRows()
				+ (settingsRowKeysReversed() ? -bigger : bigger));
		}
		else {
			rowViewMove(key);
		}
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
	// NOT WHEN THE POINTER IS NOT THE HAND'S. A scripted demonstration drives Rack by feeding
	// it pointer positions of its own while the real cursor lies wherever it was left; warping
	// that cursor then moves nothing the viewer can see, but Rack takes it as the pointer from
	// the next frame on — and the cable being carried jumps across the window to it.
	//
	// Told apart by asking where the real cursor is: when it is not where Rack believes the
	// pointer to be, the pointer belongs to something else and is that thing's to move. The
	// view still steps; only the warp is skipped, and the pointer, being held at the edge,
	// simply steps it again.
	{
		const float scale = (APP->window->windowRatio > 0.f)
			? APP->window->pixelRatio / APP->window->windowRatio : 1.f;
		double rx = 0.0, ry = 0.0;
		glfwGetCursorPos(win, &rx, &ry);
		const math::Vec real = math::Vec((float) rx, (float) ry)
			.div(scale > 0.f ? scale : 1.f);
		if (real.minus(APP->scene->mousePos).square() > 4.f * 4.f)
			return;
	}
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


/** THE SCROLL AREA, WORKED OUT AGAIN FOR A ZOOM WE HAVE JUST SET.

The scroll widget works out how far the view may be scrolled at the top of its own step and
clamps the offset to it at the end — and a module's step runs between those two. So a zoom set
from here is followed, in the same frame, by a clamp against the bound belonging to the zoom it
replaced. Zoomed in, the offset the view wants is off the end of that bound and the clamp drags
the rack to the edge of the window.

The same sum the scroll widget does, so the clamp that follows is the right one and leaves the
view where it was put. */
static void refitScrollArea(app::RackScrollWidget* rs, float zoom) {
	if (!rs || !rs->zoomWidget || !rs->rackWidget || zoom <= 0.f)
		return;
	math::Rect moduleBox = rs->rackWidget->getModuleContainer()->getChildrenBoundingBox();
	if (!moduleBox.size.isFinite())
		moduleBox = math::Rect(RACK_OFFSET, math::Vec(0.f, 0.f));
	math::Rect scrollBox = moduleBox;
	scrollBox.pos = scrollBox.pos.mult(zoom);
	scrollBox.size = scrollBox.size.mult(zoom);
	scrollBox = scrollBox.grow(rs->box.size.mult(0.9f));
	rs->zoomWidget->box = scrollBox;
	rs->rackWidget->box.pos = scrollBox.pos.div(zoom).neg();
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

	// SHOWING A DIFFERENT NUMBER OF ROWS KEEPS WHAT IS UNDER THE POINTER, and does it in ONE
	// FRAME.
	//
	// It was two, and it showed. The zoom was asked for, and because Rack's own step recomputes
	// the scroll area and clamps the offset to it, anything put right in the same frame was
	// thrown away — so the correction waited for the next frame. That is the shudder: one frame
	// zoomed about the middle of the window, the next frame jumped the view back to where the
	// pointer said it should be.
	//
	// Rack will pivot a zoom wherever it is told to, and it adjusts the offset for the pivot as
	// part of the same call. Pivoting on the pointer is exactly the thing the correction was
	// trying to achieve, so there is nothing left to put right afterwards and nothing to see.
	// The row is then settled in the same frame, which moves the view by at most half a row.
	if (gHave && rowCount != gRowsShown) {
		// NOTHING MOVES UNTIL THE PHOTOGRAPH HAS BEEN TAKEN. The view is left exactly as it is
		// for one frame, which is the frame the overlay photographs; the change is made on the
		// next one, under the picture.
		if (!gFrozen) {
			if (!gFreezeWant) {
				gFreezeWant = true;
				gFreezeAsked = system::getTime();
			}
			if (system::getTime() - gFreezeAsked < FREEZE_WAIT)
				return;
			// Nobody is drawing us. Go ahead without a picture rather than never at all.
			gFreezeWant = false;
		}
		gFreezeUntil = system::getTime() + FREEZE_HOLD;
		gRowsShown = rowCount;
		gZoomAsked = want;
		// ON THE MIDDLE OF THE WINDOW when the pointer is somewhere else — a menu, the toolbar,
		// another display — since there is nothing under it to keep.
		math::Vec pivot = rs->box.size.div(2.f);
		const bool onRack = rs->box.contains(APP->scene->mousePos);
		if (gPivotOnPointer && onRack)
			pivot = APP->scene->mousePos.minus(rs->box.pos);
		gPivotOnPointer = true;
		// WHICH ROW THE POINTER IS OVER, before the zoom moves everything.
		const float zoomWas = rs->getZoom();
		const int under = (zoomWas > 0.f && onRack)
			? (int) std::floor(rs->getGridOffset().y
				+ (APP->scene->mousePos.y - rs->box.pos.y) / (zoomWas * RACK_GRID_HEIGHT))
			: 0;
		rs->setZoom(want, pivot);
		gRow = rowAt(rs->getGridOffset().y);
		// AND IT IS STILL ON SHOW AFTERWARDS. The pivot keeps the point under the pointer where
		// it was, but the view is then settled on the nearest row, and half a row of rounding is
		// enough to put the row you were looking at off the top — with one row on show, that
		// leaves you looking at the empty row above the module you were working on. The rounding
		// is allowed to settle anywhere that still has that row in the window.
		if (onRack)
			gRow = math::clamp(gRow, under - (rowCount - 1), under);
		gRow = clampRow(gRow);
		gLastY = topOf(gRow);
		rs->setGridOffset(math::Vec(rs->getGridOffset().x, gLastY));
		// AND THE SCROLL AREA WITH IT, or the offset just set is thrown away before it is
		// drawn — see refitScrollArea.
		refitScrollArea(rs, want);
		return;
	}
	gRowsShown = rowCount;
	// THE ZOOM HELD AGAINST ANYTHING ELSE THAT SETS IT, every frame it is wrong.
	//
	// It used to be asked for once and then not again until the wanted zoom itself changed, to
	// keep from re-pivoting on the middle of the window frame after frame while Rack settled
	// it. Rack does not settle it — a zoom is applied the moment it is set — and the one-shot
	// rule meant that anything else which moved the zoom afterwards was simply left alone. A
	// patch loaded by a program that frames the whole rack came up showing four rows, or two,
	// depending on which of the two ran first that time.
	if (std::fabs(rs->getZoom() - want) > 0.0005f) {
		gZoomAsked = want;
		rs->setZoom(want);
		refitScrollArea(rs, want);
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
	if (!dragging) {
		gDragHave = false;
		gDragUp = gDragDown = false;
	}
	if (gHave && dragging) {
		// WHERE THE HAND STARTED, so that carrying a cable off an edge can be told from having
		// picked one up near it. See DRAG_PULL.
		const float pointerY = APP->scene->getMousePos().y;
		if (!gDragHave) {
			gDragHave = true;
			gDragFromY = pointerY;
			gDragUp = gDragDown = false;
		}
		if (pointerY < gDragFromY - DRAG_PULL)
			gDragUp = true;
		if (pointerY > gDragFromY + DRAG_PULL)
			gDragDown = true;

		const float pointerRow = (APP->scene->rack->getMousePos().y - RACK_OFFSET.y)
			/ RACK_GRID_HEIGHT;
		if (pointerRow < (float) gRow && gDragUp)
			push = -1;
		else if (pointerRow > (float) (gRow + rowCount) && gDragDown)
			push = 1;
	}
	// NOTHING AT ALL WHEN THE VIEW CANNOT GO THAT WAY. At the end of the patch the row does not
	// change, and moving the pointer to a row that has not come into view threw the cable a row
	// down the window for no reason a hand could see. Carrying a cable off the top of the first
	// row, or the foot of the last, simply holds it there.
	const int wasRow = gRow;
	if (push != 0 && now - gDragStepAt >= DRAG_STEP
		&& clampRow(gRow + push) != wasRow) {
		gDragStepAt = now;
		gRow = clampRow(gRow + push);
		// AND THE POINTER COMES WITH IT, to just inside the row that has come into view — the
		// foot of it when the drag was going up, the head of it going down. Left where it was,
		// the pointer would still be outside the rows on show, and the view would walk row after
		// row for as long as the cable was held there.
		// The row that has come into view is the top one going up and the bottom one going
		// down; it is that row's own head or foot that is wanted, not the window's. Written as
		// the window's edge, going down put the cable at the foot of the row it had just
		// crossed into rather than at its head — a whole row from where the hand was.
		const float land = (push < 0)
			? (float) gRow + 0.85f
			: (float) (gRow + rowCount - 1) + 0.15f;
		putPointerAtRow(rs, land);
	}
	// A MOVE THAT WAS NOT OURS settles on the nearest row: that is the scroll bar, an
	// Option-drag, and Rack's own jumps, all answered the same way.
	else if (!gHave || std::fabs(moved) > 0.02f) {
		gRow = clampRow(rowAt(at.y));
	}
	// A TOP ROW ASKED FOR FROM OUTSIDE WINS over wherever the view happens to have landed.
	if (gWantTop) {
		gWantTop = false;
		gRow = clampRow(gWantTopRow);
		gScrollAcc = 0.f;
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
	gRow = clampRow(gRow + rows);
	gScrollAcc = 0.f;
	gLastY = topOf(gRow);
	rs->setGridOffset(math::Vec(rs->getGridOffset().x, gLastY));
}

extern "C" bool drRowViewCommand(int move, int count) {
	if (!gOn)
		return false;
	if (count != 0)
		settingsSetRowViewRows(settingsRowViewRows() + count);
	else if (move != 0)
		rowViewMove(move);
	return true;
}


extern "C" bool drRowViewWhere(int* topRow, int* rows) {
	if (topRow)
		*topRow = gRow;
	if (rows)
		*rows = settingsRowViewRows();
	return gOn && gHave;
}


extern "C" void drRowViewRows(int rows) {
	if (rows != settingsRowViewRows())
		gPivotOnPointer = false;
	settingsSetRowViewRows(rows);
}


extern "C" void drRowViewTop(int row) {
	gWantTop = true;
	gWantTopRow = row;
}


/** How much of the current turn has not yet been spent on a row count, and when its last
movement came in. The same accounting as the scrolling below, and at the same cost per step, so
that turning the wheel to change the count feels like turning it to move the view. */
static float gZoomAcc = 0.f;
static double gZoomAt = 0.0;

/** WHICH WAY A SCROLLING HAND IS GOING, judged over the last half second or so.

A SINGLE MOVEMENT CANNOT BE JUDGED. A gesture arrives as a stream of small deltas and the ratio
between them wobbles from one to the next, so even a firmly vertical swipe throws off a few that
are nearly horizontal. Judged one at a time, each of those was declined — and declining hands it
to Rack, which slides the rack a few pixels sideways. That is the sideways drift while scrolling
rows: not a wrong angle, but a test with no memory.

So how much has gone across and how much up and down are each kept as a running total that
decays with a half-life of a quarter of a second: roughly the last half second counts, and what
came before fades out rather than dropping off a cliff. The axis is whichever total is larger.

CLAIMED, THEN HELD AGAINST A REVERSAL. The opening movement of a gesture is not worth much, so
the axis it suggests stands only provisionally until enough movement has arrived to be worth
judging; after that it takes a two-to-one lead the other way to change it. That is what stops it
flapping about at the boundary. It CAN still change — a gesture that starts down the rack and
turns into a sideways one is a real thing a hand does, and this follows it a few events later
rather than refusing to.

A quarter of a second with no scrolling ends the gesture, and the next one starts from nothing.

THE SIDEWAYS PART OF A VERTICAL GESTURE IS SWALLOWED, by the caller: being vertical means the
whole movement is ours, and only its up and down is used. Passing the sideways part on is the
very thing this exists to stop. */
static const double AXIS_HALF_LIFE = 0.25;
static const double AXIS_END = 0.25;
static const float AXIS_CLAIM = 20.f;
static float gAxisAcross = 0.f, gAxisDown = 0.f;
static double gAxisAt = 0.0;
static bool gAxisClaimed = false;
static bool gAxisVertical = true;

static bool axisIsVertical(float dx, float dy) {
	const double now = system::getTime();
	const double gap = now - gAxisAt;
	gAxisAt = now;
	if (gap > AXIS_END) {
		gAxisAcross = gAxisDown = 0.f;
		gAxisClaimed = false;
	}
	else {
		const float fade = (float) std::pow(0.5, gap / AXIS_HALF_LIFE);
		gAxisAcross *= fade;
		gAxisDown *= fade;
	}
	gAxisAcross += std::fabs(dx);
	gAxisDown += std::fabs(dy);

	if (!gAxisClaimed) {
		// VERTICAL UNTIL PROVED OTHERWISE. The opening movement of a gesture is a few pixels
		// and its direction means nothing; taking it at face value meant a swipe that began
		// with a hair more across than down was handed to Rack, which panned the rack before
		// the gesture had said what it was. Held as ours until there is enough movement to
		// judge, a sideways gesture loses about a notch of panning at its start, which is not
		// a thing anybody can see.
		gAxisVertical = true;
		if (gAxisAcross + gAxisDown >= AXIS_CLAIM) {
			gAxisClaimed = true;
			gAxisVertical = gAxisDown >= gAxisAcross;
		}
	}
	else if (gAxisVertical) {
		if (gAxisAcross > 2.f * gAxisDown)
			gAxisVertical = false;
	}
	else if (gAxisDown > 2.f * gAxisAcross) {
		gAxisVertical = true;
	}
	return gAxisVertical;
}


bool rowViewZoom(float dx, float dy) {
	if (!gOn)
		return false;
	// SIDEWAYS IS SWALLOWED RATHER THAN PASSED ON. A gesture meant as a zoom is not a request to
	// move the view sideways, and a hand on a trackpad or a tilting wheel puts some across into
	// every movement — which slid the rack about while the row count was being chosen. So the
	// whole gesture belongs to the count: what is not up and down does nothing at all.
	const bool vertical = (dy != 0.f && std::fabs(dy) > std::fabs(dx));
	if (!vertical)
		return true;
	const double now = system::getTime();
	// A FRESH TURN STARTS FROM NOTHING: the wheel standing still for a moment ends the one
	// before it, and so does turning the other way. See SCROLL_END.
	if (now - gZoomAt > SCROLL_END || (gZoomAcc != 0.f && (gZoomAcc > 0.f) != (dy > 0.f)))
		gZoomAcc = 0.f;
	gZoomAt = now;

	gZoomAcc += dy;
	if (std::fabs(gZoomAcc) >= SCROLL_PER_ROW) {
		// THE WAY RACK ZOOMS. Turning the wheel the way that magnifies the rack shows FEWER
		// rows, because fewer rows in the window is what magnifying it amounts to here — and
		// Rack's "invert zoom" setting turns this round with everything else, so the gesture
		// always goes the way that person's zoom goes.
		bool zoomIn = gZoomAcc > 0.f;
		if (settings::invertZoom)
			zoomIn = !zoomIn;
		settingsSetRowViewRows(settingsRowViewRows() + (zoomIn ? -1 : 1));
		gZoomAcc -= (gZoomAcc > 0.f) ? SCROLL_PER_ROW : -SCROLL_PER_ROW;
		// Never more than one step banked, so one movement changes the count by one at most
		// however large it was.
		gZoomAcc = math::clamp(gZoomAcc, -SCROLL_PER_ROW * 0.99f, SCROLL_PER_ROW * 0.99f);
	}
	// Taken either way: left to Rack a movement that has not yet added up to a step would zoom
	// the rack, and the row count would put it straight back.
	return true;
}


bool rowViewScroll(float dx, float dy) {
	if (!gOn || !gHave)
		return false;
	// WHICH WAY THE HAND IS GOING, TAKEN OVER THE LAST HALF SECOND rather than from this one
	// movement. See axisIsVertical.
	if (!axisIsVertical(dx, dy)) {
		// Sideways: Rack's, and nothing of this turn is kept.
		gScrollAcc = 0.f;
		return false;
	}
	if (dy == 0.f)
		return true;
	app::RackScrollWidget* rs = APP->scene ? APP->scene->rackScroll : NULL;
	if (!rs)
		return false;

	const double now = system::getTime();
	// A FRESH TURN STARTS FROM NOTHING. The wheel standing still for a moment ends the one
	// before it, and so does turning the other way — however little, and whatever is left over.
	const int dir = (dy > 0.f) ? 1 : -1;
	if (now - gVertAt > SCROLL_END || (gScrollDir != 0 && dir != gScrollDir)) {
		gScrollAcc = 0.f;
		gScrollTurning = false;
	}
	gScrollDir = dir;
	gVertAt = now;

	// THE FIRST ROW OF A TURN IS CHEAP. A row costs four notches so that a continuous turn walks
	// the rack at a readable speed — but making the FIRST row cost that too meant every scroll
	// began with a stretch where the wheel turned and nothing happened, and a stretch where
	// nothing happens is a stretch in which a hand can wander. So the first row costs a little
	// less than a whole one rather than nothing at all — see SCROLL_FIRST_ROW, which was found
	// by turning the wheel rather than by reasoning about it.
	if (!gScrollTurning) {
		// BUT NOT UNTIL THE GESTURE HAS SAID WHAT IT IS. A free first row meant a sideways
		// scroll that opened with a hair of vertical in it stepped a row before the axis had
		// been judged — a row you did not ask for, and the harder of the two mistakes to undo.
		// Until there is enough movement to judge, the gesture is held as ours and counted, and
		// the row waits.
		// A NOTCH AT MOST FROM ANY ONE MOVEMENT, so that how fast the wheel was hit does not
		// decide whether a row moves. See SCROLL_NOTCH.
		gScrollAcc += math::clamp(dy, -SCROLL_NOTCH, SCROLL_NOTCH);
		if (!gAxisClaimed)
			return true;
		// AND NOT UNTIL THE TURN IS WORTH A ROW. See SCROLL_FIRST_ROW: being judged vertical is
		// not the same as having been turned.
		if (std::fabs(gScrollAcc) < SCROLL_FIRST_ROW)
			return true;
		gScrollTurning = true;
		gScrollAcc = 0.f;
		gStepAt = now;
		gRow = clampRow(gRow + ((dy > 0.f) ? -1 : 1));
		gLastY = topOf(gRow);
		rs->setGridOffset(math::Vec(rs->getGridOffset().x, gLastY));
		return true;
	}

	// See ROW_SETTLE. Taken, so it does not reach Rack either.
	if (now - gStepAt < ROW_SETTLE)
		return true;
	gScrollAcc += dy;
	if (std::fabs(gScrollAcc) >= SCROLL_PER_ROW) {
		gStepAt = now;
		// A wheel turned away from you goes UP the rack, which is what Rack does too.
		gRow = clampRow(gRow + ((gScrollAcc > 0.f) ? -1 : 1));
		gScrollAcc -= (gScrollAcc > 0.f) ? SCROLL_PER_ROW : -SCROLL_PER_ROW;
		// Never more than one row's worth banked, so one movement can pay for one row and no
		// more however large it was.
		gScrollAcc = math::clamp(gScrollAcc, -SCROLL_PER_ROW * 0.99f, SCROLL_PER_ROW * 0.99f);
		gLastY = topOf(gRow);
		rs->setGridOffset(math::Vec(rs->getGridOffset().x, gLastY));
	}
	// TAKEN EITHER WAY. A movement that has not yet added up to a row must not be passed on to
	// Rack, or the view would slide off the boundary it is being held on and be pulled back.
	return true;
}


/** THE PHOTOGRAPH OVER THE WINDOW WHILE THE ROW COUNT CHANGES.

Changing how many rows are on show changes the zoom, and a zoom is a thing the eye follows: even
made in a single frame it reads as Rack zooming rather than as the rack snapping from three rows
to two. What should be seen is the number of rows before and the number after, and nothing in
between.

So the frame before the change is photographed out of the picture that has already been drawn
beneath this overlay, the change is made underneath it, and the photograph is taken away a few
frames later. The same trick the pinch zoom uses, and for the same reason.

ONLY THE RACK AREA is covered, so the menu bar and anything floating over it are untouched. */
struct RowViewFreeze : widget::TransparentWidget {
	int image = -1;
	math::Rect shot;

	void step() override {
		// Cover the scene, or the frame this is drawn in will skip it as being outside the
		// window.
		if (parent)
			box.size = parent->box.size;
		widget::TransparentWidget::step();
	}

	/** The picture is a texture on the card; it goes back when the freeze ends rather than
	being left to pile up one per change. */
	void release(NVGcontext* vg) {
		if (image >= 0 && vg)
			nvgDeleteImage(vg, image);
		image = -1;
		shot = math::Rect();
	}

	/** Photographs the rack out of the frame already drawn beneath us. */
	void capture(const DrawArgs& args) {
		app::RackScrollWidget* rs = APP->scene ? APP->scene->rackScroll : NULL;
		if (!rs)
			return;
		const float ratio = APP->window->pixelRatio;
		const math::Vec winSize = APP->window->getSize();
		shot = math::Rect(rs->box.pos, rs->box.size);
		const int px = (int) (shot.pos.x * ratio);
		const int pw = (int) (shot.size.x * ratio);
		const int ph = (int) (shot.size.y * ratio);
		// OpenGL counts from the bottom of the window, the widget tree from the top.
		const int py = (int) ((winSize.y - shot.pos.y - shot.size.y) * ratio);
		if (pw <= 0 || ph <= 0)
			return;
		std::vector<uint8_t> pixels((size_t) pw * ph * 4);
		glReadPixels(px, py, pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		release(args.vg);
		// FLIPY because glReadPixels hands back its rows bottom-up.
		image = nvgCreateImageRGBA(args.vg, pw, ph, NVG_IMAGE_FLIPY, pixels.data());
	}

	void draw(const DrawArgs& args) override {
		if (gFreezeWant) {
			gFreezeWant = false;
			capture(args);
			gFrozen = (image >= 0);
			// AND THE END OF THE LAST FREEZE FORGOTTEN. Left set from the change before, it is
			// a moment in the past, so the test below took this picture down in the very frame
			// it was taken — and the change waiting under it was then put off for ever, each
			// frame asking for a fresh picture and having it cancelled again. Every change
			// after the first did nothing at all. It is set when the change is actually made.
			gFreezeUntil = 0.0;
		}
		if (gFrozen && (system::getTime() > gFreezeUntil && gFreezeUntil > 0.0)) {
			gFrozen = false;
			release(args.vg);
		}
		if (gFrozen && image >= 0) {
			nvgBeginPath(args.vg);
			nvgRect(args.vg, shot.pos.x, shot.pos.y, shot.size.x, shot.size.y);
			nvgFillPaint(args.vg, nvgImagePattern(args.vg, shot.pos.x, shot.pos.y,
				shot.size.x, shot.size.y, 0.f, image, 1.f));
			nvgFill(args.vg);
		}
		widget::TransparentWidget::draw(args);
	}
};


widget::Widget* createRowViewOverlay() {
	return new RowViewFreeze;
}
