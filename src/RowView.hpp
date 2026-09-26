#pragma once
/** WHOLE ROWS IN THE WINDOW.

The rack is a wall of rows and the window shows whatever part of it the scrolling has landed on,
usually the bottom of one row and the top of the next. This sets the zoom so that a chosen number
of rows fills the window, with a tenth of a row showing above and below to say what is there, and
holds the view on that boundary: every way the view moves — the wheel, the scroll bar, an
Option-drag, a cable dragged past the edge, Rack moving the view itself — ends on a row.

IT STOPS AT THE END OF THE PATCH. The view goes no further than leaves one whole row of modules
on show, so it cannot be scrolled out into empty rack with a fifth of a row along one edge and
nothing else in the window.

WHILE IT IS ON, ZOOMING DOES NOTHING. The zoom belongs to the row count, and a view zoomed out and
back again did not land where it started, which made the zoom a thing you could not undo.

Only up and down. Scrolling left and right is untouched. */

/** THE OVERLAY THAT HIDES A ROW-COUNT CHANGE. Added to the scene above everything else in the
rack; does nothing at all except in the few frames a change takes. See the foot of RowView.cpp. */
widget::Widget* createRowViewOverlay();

/** Called every frame while a Clarity is in the rack. */
void rowViewStep(bool on);

/** A wheel turned over the rack. True if the movement was ours, which is whenever this is on and
the wheel is being turned up or down rather than sideways — whether or not it has added up to a
row yet, since a movement passed on would slide the view off the boundary it is held on. */
bool rowViewScroll(float dx, float dy);

/** THE WHEEL GESTURE RACK ZOOMS WITH, turned over the rack: one action of it shows one row more
or one row fewer.

ZOOM MEANS SOMETHING ELSE HERE. Held on rows, the magnification is not a thing to be set — it
belongs to the row count. So the gesture that would have changed how big the modules are changes
how many rows are in the window instead, which is the same question asked the other way round.

IT GOES THE WAY RACK'S ZOOM GOES. Turning the wheel the way that magnifies the rack shows fewer
rows, since fewer rows in the window is what magnifying it amounts to; the other way shows more.
Rack's "invert zoom" setting is followed too, so the gesture always agrees with that person's
zoom. This is the opposite way round from Command with the arrow keys, where up shows one more
row: the keys are not a zoom gesture and read as up for more, and the wheel stands in for the
zoom and reads as in for closer.

KEEP TURNING, KEEP CHANGING, and at the same cost per step as scrolling the view: a row's worth
of wheel adds or removes a row, so the two things the wheel does over the rack answer to the hand
the same way. One movement can pay for one step and never two, however large it is, and what is
left over is thrown away when the wheel stops or turns back — the same accounting, for the same
reason, as the scrolling below.

Always taken, whether or not it changed anything, because left to Rack it would zoom the rack and
the row count would put it straight back. */
bool rowViewZoom(float dx, float dy);

/** A row up (negative) or down (positive), for the arrow keys. */
void rowViewMove(int rows);

/** Whether the view is being held on rows at this moment. */
bool rowViewOn();

/** WHAT THE ARROW KEYS DO, REACHABLE FROM ANOTHER PLUGIN.

The keys themselves cannot be. Rack's scene answers the arrows before any widget in it is
offered them, so they are read straight from the keyboard instead — which means a key sent into
the scene by a program rather than pressed by a hand never arrives here. That is what a scripted
demonstration sends, so a demo could show every other part of this and not the two keys.

This is the same two actions with the keyboard taken out of them: `move` is a row up (negative)
or down (positive), and `count` is one more row on show (positive) or one fewer (negative). One
or the other, not both. Returns false when the view is not being held on rows, in which case
neither means anything and nothing was done.

Declared extern "C" so it can be found by name at run time with dlsym. Nothing in this plugin
calls it. */
extern "C" bool drRowViewCommand(int move, int count);

/** WHERE THE VIEW IS: the row at the top of the window and how many rows are on show. False
when the view is not being held on rows, in which case neither number means anything.

FOR SOMETHING WATCHING RATHER THAN DRIVING. A take recorded by hand has to be captioned
afterwards, and what makes a caption is not that a button went down but that something changed —
the view moved a row, the count went to three. Read once a frame by the recorder, this turns a
search through the film into a dozen lines of text with exact times on them. */
extern "C" bool drRowViewWhere(int* topRow, int* rows);

/** WHICH ROW IS AT THE TOP OF THE WINDOW, set outright. Setting up, like the one below: a demo
that has to show a particular row cannot get there by counting arrow presses from wherever the
view happened to open. Does nothing when the view is not being held on rows. */
extern "C" void drRowViewTop(int row);

/** HOW MANY ROWS ARE ON SHOW, SET OUTRIGHT. For the same caller, and for setting up rather than
demonstrating: a demonstration that begins by pressing Command and an arrow four times to reach a
known count is showing the viewer nothing. One to five; anything else is clamped. */
extern "C" void drRowViewRows(int rows);
