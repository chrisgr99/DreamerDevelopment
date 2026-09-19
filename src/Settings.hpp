#pragma once
/** CLARITY'S SETTINGS FOR THE PERSON RATHER THAN THE PATCH: how big the tooltips are drawn, and in
what colours.

NOT IN THE PATCH. Somebody who needs larger text needs it in every patch they open, including
ones other people wrote, so these are kept beside Rack's own settings, in
DreamerDevelopment/clarity.json, and read once. The switches themselves stay params on the panel,
saved with the patch and mappable, like every other feature. */

/** The size of tooltips drawn by Tooltip readability, as a multiple of Rack's own. */
float settingsTooltipScale();
void settingsSetTooltipScale(float scale);

/** Tooltips drawn in the classic light yellow with dark text, rather than light text on
black. */
bool settingsTooltipClassic();
void settingsSetTooltipClassic(bool classic);
