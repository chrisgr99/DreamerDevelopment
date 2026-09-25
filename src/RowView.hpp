#pragma once
/** WHOLE ROWS IN THE WINDOW.

The rack is a wall of rows and the window shows whatever part of it the scrolling has landed on,
usually the bottom of one row and the top of the next. This sets the zoom so that a chosen number
of rows fills the window, with a tenth of a row showing above and below to say what is there, and
holds the view on that boundary: every way the view moves — the wheel, the scroll bar, an
Option-drag, a cable dragged past the edge, Rack moving the view itself — ends on a row.

WHILE IT IS ON, ZOOMING DOES NOTHING. The zoom belongs to the row count, and a view zoomed out and
back again did not land where it started, which made the zoom a thing you could not undo.

Only up and down. Scrolling left and right is untouched. */

/** Called every frame while a Clarity is in the rack. */
void rowViewStep(bool on);

/** A wheel turned over the rack. True if it moved the view, which is whenever this is on and the
wheel is being turned up or down rather than sideways. */
bool rowViewScroll(float dx, float dy);

/** A row up (negative) or down (positive), for the arrow keys. */
void rowViewMove(int rows);

/** Whether the view is being held on rows at this moment. */
bool rowViewOn();
