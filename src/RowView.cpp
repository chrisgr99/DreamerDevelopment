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

/** Which row is at the top, and what we last put the view at, so a move made by anything else can
be told from our own and answered by settling on the nearest row. */
static bool gHave = false;
static int gRow = 0;
static float gLastY = 0.f;
/** How much of the current turn of the wheel has not yet been spent on a row, and when the last
movement of it came in. */
static float gScrollAcc = 0.f;
static double gVertAt = 0.0;
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


/** NEVER PAST THE LAST ROW THAT HAS ANYTHING ON IT.

Scrolling could carry the view off the end of the rack, until the window held one fifth of a row
of modules along one edge and nothing else. There is no reason to look at that: the rack is empty
in that direction and stays empty, and getting back to the patch meant scrolling the same distance
again. So the view stops where at least one whole row of modules is still on show — at the limit,
the last row of the patch sits alone at the top of the window, or the first row alone at the
bottom.

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
	// The topmost row of the window may be as high as the first occupied row less all but one of
	// the rows on show — which puts that row at the bottom — and as low as the last occupied row,
	// which puts that one at the top.
	const int lowest = first - (shown - 1);
	if (last < lowest)
		return row;   // fewer rows of modules than the window holds; leave it alone
	return math::clamp(row, lowest, last);
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
		if (rs->box.contains(APP->scene->mousePos))
			pivot = APP->scene->mousePos.minus(rs->box.pos);
		rs->setZoom(want, pivot);
		gRow = clampRow(rowAt(rs->getGridOffset().y));
		gLastY = topOf(gRow);
		rs->setGridOffset(math::Vec(rs->getGridOffset().x, gLastY));
		// AND THE SCROLL AREA WITH IT, or the offset just set is thrown away before it is drawn.
		//
		// THIS is what defeated doing it in one frame before. A module's step runs from inside
		// the scroll widget's own step, which works out how far the view may be scrolled BEFORE
		// stepping its children and clamps the offset to it AFTERWARDS — so the bound in force
		// when our new offset is clamped was worked out for the zoom we have just replaced. With
		// the zoom raised, the offset it wants is off the end of the old bound, and the clamp
		// drags the view to the edge: the corner this shot off to.
		//
		// The bound is worked out here the same way the scroll widget works it out, from the
		// modules at the new zoom, so the clamp at the end of this frame is the right one and
		// leaves the view alone.
		if (rs->zoomWidget && rs->rackWidget) {
			math::Rect moduleBox = rs->rackWidget->getModuleContainer()->getChildrenBoundingBox();
			if (!moduleBox.size.isFinite())
				moduleBox = math::Rect(RACK_OFFSET, math::Vec(0.f, 0.f));
			math::Rect scrollBox = moduleBox;
			scrollBox.pos = scrollBox.pos.mult(want);
			scrollBox.size = scrollBox.size.mult(want);
			scrollBox = scrollBox.grow(rs->box.size.mult(0.9f));
			rs->zoomWidget->box = scrollBox;
			rs->rackWidget->box.pos = scrollBox.pos.div(want).neg();
		}
		return;
	}
	gRowsShown = rowCount;
	// THE ZOOM HELD AGAINST ANYTHING ELSE THAT SETS IT, and asked for once rather than every
	// frame: asking again each frame re-pivots it on the middle of the window every time.
	if (std::fabs(rs->getZoom() - want) > 0.0005f
		&& std::fabs(want - gZoomAsked) > 0.0005f) {
		gZoomAsked = want;
		rs->setZoom(want);
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


extern "C" void drRowViewRows(int rows) {
	settingsSetRowViewRows(rows);
}


extern "C" void drRowViewTop(int row) {
	app::RackScrollWidget* rs = APP->scene ? APP->scene->rackScroll : NULL;
	if (!gOn || !rs)
		return;
	gRow = clampRow(row);
	gScrollAcc = 0.f;
	gLastY = topOf(gRow);
	gHave = true;
	rs->setGridOffset(math::Vec(rs->getGridOffset().x, gLastY));
}


/** How much of the current turn has not yet been spent on a row count, and when its last
movement came in. The same accounting as the scrolling below, and at the same cost per step, so
that turning the wheel to change the count feels like turning it to move the view. */
static float gZoomAcc = 0.f;
static double gZoomAt = 0.0;

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
	// SIDEWAYS IS RACK'S: only a movement more up and down than across is ours.
	if (dy == 0.f || std::fabs(dy) <= std::fabs(dx))
		return false;
	app::RackScrollWidget* rs = APP->scene ? APP->scene->rackScroll : NULL;
	if (!rs)
		return false;

	const double now = system::getTime();
	// A FRESH TURN STARTS FROM NOTHING. The wheel standing still for a moment ends the one
	// before it, and so does turning the other way.
	if (now - gVertAt > SCROLL_END || (gScrollAcc != 0.f && (gScrollAcc > 0.f) != (dy > 0.f)))
		gScrollAcc = 0.f;
	gVertAt = now;

	gScrollAcc += dy;
	if (std::fabs(gScrollAcc) >= SCROLL_PER_ROW) {
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
