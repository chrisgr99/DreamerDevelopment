/** See Settings.hpp. */
#include "plugin.hpp"
#include "Settings.hpp"


static float gTooltipScale = 1.5f;
/** White on black, unless changed in the menu. */
static bool gTooltipClassic = false;
/** Below the pointer, unless changed in the menu. */
static bool gTooltipAbove = false;
/** Two rows, unless changed in the menu. */
static int gRowViewRows = 2;

static std::string settingsPath() {
	return asset::user("DreamerDevelopment/clarity.json");
}

/** Read once, the first time anything asks. A missing or unreadable file leaves the defaults. */
static void settingsLoad() {
	static bool loaded = false;
	if (loaded)
		return;
	loaded = true;
	json_error_t err;
	json_t* rootJ = json_load_file(settingsPath().c_str(), 0, &err);
	if (!rootJ)
		return;
	if (json_t* j = json_object_get(rootJ, "tooltipScale"))
		gTooltipScale = math::clamp((float) json_number_value(j), 1.f, 4.f);
	if (json_t* j = json_object_get(rootJ, "tooltipClassic"))
		gTooltipClassic = json_is_true(j);
	if (json_t* j = json_object_get(rootJ, "tooltipAbove"))
		gTooltipAbove = json_is_true(j);
	if (json_t* j = json_object_get(rootJ, "rowViewRows"))
		gRowViewRows = math::clamp((int) json_integer_value(j), 1, 5);
	json_decref(rootJ);
}

/** Written whole whenever one changes. Failure is silent: an unwritable settings folder is not
a reason to interrupt anybody, and the setting still holds for this session. */
static void settingsSave() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "tooltipScale", json_real(gTooltipScale));
	json_object_set_new(rootJ, "tooltipClassic", json_boolean(gTooltipClassic));
	json_object_set_new(rootJ, "tooltipAbove", json_boolean(gTooltipAbove));
	json_object_set_new(rootJ, "rowViewRows", json_integer(gRowViewRows));
	system::createDirectories(asset::user("DreamerDevelopment"));
	if (FILE* f = std::fopen(settingsPath().c_str(), "w")) {
		json_dumpf(rootJ, f, JSON_INDENT(2));
		std::fclose(f);
	}
	json_decref(rootJ);
}

float settingsTooltipScale() {
	settingsLoad();
	return gTooltipScale;
}

void settingsSetTooltipScale(float scale) {
	settingsLoad();
	gTooltipScale = math::clamp(scale, 1.f, 4.f);
	settingsSave();
}

bool settingsTooltipClassic() {
	settingsLoad();
	return gTooltipClassic;
}

void settingsSetTooltipClassic(bool classic) {
	settingsLoad();
	gTooltipClassic = classic;
	settingsSave();
}

bool settingsTooltipAbove() {
	settingsLoad();
	return gTooltipAbove;
}

void settingsSetTooltipAbove(bool above) {
	settingsLoad();
	gTooltipAbove = above;
	settingsSave();
}

int settingsRowViewRows() {
	settingsLoad();
	return gRowViewRows;
}

void settingsSetRowViewRows(int rows) {
	settingsLoad();
	gRowViewRows = math::clamp(rows, 1, 5);
	settingsSave();
}
