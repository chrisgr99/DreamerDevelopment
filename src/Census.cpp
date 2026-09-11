/** The port census — see Census.hpp. */
#include "Census.hpp"

#include <cstdio>
#include <string>


bool censusOffered() {
	return system::isFile(asset::user("DreamerDevelopment/census.enable"));
}


static json_t* portJson(engine::PortInfo* info, int index) {
	json_t* j = json_object();
	json_object_set_new(j, "index", json_integer(index));
	json_object_set_new(j, "name", json_string(info ? info->name.c_str() : ""));
	// THE DESCRIPTION AS WELL. Hardly anybody sets one, but where it is set it says what the
	// name had no room for — "1V/octave", "gate or trigger" — which is exactly what a rule
	// wants to match on.
	json_object_set_new(j, "description", json_string(info ? info->description.c_str() : ""));
	return j;
}


/** A widget's box, in the module's own coordinates — which is the space nanosvg's panel bounds
are in, so a jack and a shape on the panel can be compared directly. */
static json_t* boxJson(int index, const char* kind, widget::Widget* w) {
	json_t* j = json_object();
	json_object_set_new(j, "index", json_integer(index));
	json_object_set_new(j, "kind", json_string(kind));
	json_object_set_new(j, "x", json_real(w->box.pos.x));
	json_object_set_new(j, "y", json_real(w->box.pos.y));
	json_object_set_new(j, "w", json_real(w->box.size.x));
	json_object_set_new(j, "h", json_real(w->box.size.y));
	json_object_set_new(j, "cx", json_real(w->box.getCenter().x));
	json_object_set_new(j, "cy", json_real(w->box.getCenter().y));
	return j;
}


int censusPositions(const std::string& only) {
	json_t* rootJ = json_object();
	json_t* modulesJ = json_array();
	int count = 0;

	for (plugin::Plugin* p : plugin::plugins) {
		if (!p)
			continue;
		for (plugin::Model* model : p->models) {
			if (!model)
				continue;
			const std::string key = p->slug + "/" + model->slug;
			if (!only.empty() && key.compare(0, only.size(), only) != 0)
				continue;
			// Said BEFORE the attempt, so if this brings Rack down the log names the culprit.
			INFO("Census: positions for %s", key.c_str());
			// NO MODULE BEHIND IT, and that is not a shortcut — it is the only safe way.
			//
			// A ModuleWidget's destructor asks the ENGINE to remove its module, and a module
			// this code made was never added to the engine, so Rack asserted and took the
			// application with it. Passing NULL makes a preview widget, exactly as the module
			// browser does for every model it shows, and a preview owns no module to remove.
			//
			// The jacks and knobs are added by the widget's constructor either way, so their
			// positions are all still here. The names come from the other census, which needs
			// a module and no widget — between them the two halves need neither at once.
			app::ModuleWidget* mw = NULL;
			try {
				mw = model->createModuleWidget(NULL);
			}
			catch (std::exception& e) {
				WARN("Census: %s threw: %s", key.c_str(), e.what());
				continue;
			}
			if (!mw)
				continue;

			json_t* j = json_object();
			json_object_set_new(j, "plugin", json_string(p->slug.c_str()));
			json_object_set_new(j, "model", json_string(model->slug.c_str()));
			json_object_set_new(j, "name", json_string(model->name.c_str()));
			json_object_set_new(j, "width", json_real(mw->box.size.x));
			json_object_set_new(j, "height", json_real(mw->box.size.y));

			json_t* portsJ = json_array();
			int i = 0;
			for (app::PortWidget* pw : mw->getInputs())
				json_array_append_new(portsJ, boxJson(pw->portId, "input", pw)), i++;
			for (app::PortWidget* pw : mw->getOutputs())
				json_array_append_new(portsJ, boxJson(pw->portId, "output", pw)), i++;
			json_object_set_new(j, "ports", portsJ);

			json_t* paramsJ = json_array();
			for (app::ParamWidget* pw : mw->getParams())
				json_array_append_new(paramsJ, boxJson(pw->paramId, "param", pw));
			json_object_set_new(j, "params", paramsJ);

			json_array_append_new(modulesJ, j);
			count++;
			delete mw;
		}
	}

	json_object_set_new(rootJ, "modules", modulesJ);
	system::createDirectories(asset::user("DreamerDevelopment"));
	const std::string path = asset::user("DreamerDevelopment/census-positions.json");
	FILE* file = std::fopen(path.c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(1));
		std::fclose(file);
		INFO("Census: wrote positions for %d models to %s", count, path.c_str());
	}
	json_decref(rootJ);
	return count;
}


// ---- the same thing, a slice at a time --------------------------------------------------------

static std::vector<plugin::Model*> gQueue;
static size_t gAt = 0;
static json_t* gModulesJ = NULL;
static double gStarted = 0.0;
static std::string gStatus;

void censusStart(const std::string& only) {
	gQueue.clear();
	gAt = 0;
	if (gModulesJ)
		json_decref(gModulesJ);
	gModulesJ = json_array();
	gStarted = system::getTime();
	for (plugin::Plugin* p : plugin::plugins) {
		if (!p)
			continue;
		for (plugin::Model* model : p->models) {
			if (!model)
				continue;
			const std::string key = p->slug + "/" + model->slug;
			if (!only.empty() && key.compare(0, only.size(), only) != 0)
				continue;
			gQueue.push_back(model);
		}
	}
	gStatus = string::f("0 of %d modules", (int) gQueue.size());
	INFO("Census: %d models to walk", (int) gQueue.size());
}

bool censusBusy() {
	return gAt < gQueue.size();
}

std::string censusStatus() {
	return gStatus;
}

void censusTick(double seconds) {
	if (!censusBusy())
		return;
	const double until = system::getTime() + seconds;
	while (censusBusy() && system::getTime() < until) {
		plugin::Model* model = gQueue[gAt++];
		const std::string key = model->plugin->slug + "/" + model->slug;
		INFO("Census: positions for %s", key.c_str());
		app::ModuleWidget* mw = NULL;
		try {
			mw = model->createModuleWidget(NULL);
		}
		catch (std::exception& e) {
			WARN("Census: %s threw: %s", key.c_str(), e.what());
			continue;
		}
		if (!mw)
			continue;
		json_t* j = json_object();
		json_object_set_new(j, "plugin", json_string(model->plugin->slug.c_str()));
		json_object_set_new(j, "model", json_string(model->slug.c_str()));
		json_object_set_new(j, "name", json_string(model->name.c_str()));
		json_object_set_new(j, "width", json_real(mw->box.size.x));
		json_object_set_new(j, "height", json_real(mw->box.size.y));
		json_t* portsJ = json_array();
		for (app::PortWidget* pw : mw->getInputs())
			json_array_append_new(portsJ, boxJson(pw->portId, "input", pw));
		for (app::PortWidget* pw : mw->getOutputs())
			json_array_append_new(portsJ, boxJson(pw->portId, "output", pw));
		json_object_set_new(j, "ports", portsJ);
		json_t* paramsJ = json_array();
		for (app::ParamWidget* pw : mw->getParams())
			json_array_append_new(paramsJ, boxJson(pw->paramId, "param", pw));
		json_object_set_new(j, "params", paramsJ);
		json_array_append_new(gModulesJ, j);
		delete mw;
	}

	const double taken = system::getTime() - gStarted;
	if (censusBusy()) {
		// HOW LONG IS LEFT, from how long the ones already done took. The only number worth
		// showing while you wait.
		const double each = taken / (double) gAt;
		gStatus = string::f("%d of %d modules\n%ds left", (int) gAt, (int) gQueue.size(),
			(int) ((gQueue.size() - gAt) * each + 0.5));
		return;
	}

	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "modules", gModulesJ);
	gModulesJ = NULL;
	system::createDirectories(asset::user("DreamerDevelopment"));
	const std::string path = asset::user("DreamerDevelopment/census-positions.json");
	FILE* file = std::fopen(path.c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(1));
		std::fclose(file);
	}
	json_decref(rootJ);
	gStatus = string::f("%d of %d modules\ndone in %.0fs", (int) gQueue.size(),
		(int) gQueue.size(), taken);
	INFO("Census: wrote positions for %d models in %.1f s to %s", (int) gQueue.size(), taken,
		path.c_str());
}


int censusWrite(const std::string& only) {
	json_t* rootJ = json_object();
	json_t* modulesJ = json_array();
	int count = 0;

	for (plugin::Plugin* p : plugin::plugins) {
		if (!p)
			continue;
		// A COMMA-SEPARATED LIST OF PREFIXES, because VCV's own modules arrive under three
		// different slugs — Core, Fundamental, and everything beginning VCV.
		if (!only.empty()) {
			bool wanted = false;
			size_t at = 0;
			while (at <= only.size() && !wanted) {
				const size_t comma = only.find(',', at);
				const std::string one = only.substr(at,
					comma == std::string::npos ? std::string::npos : comma - at);
				if (!one.empty() && p->slug.compare(0, one.size(), one) == 0)
					wanted = true;
				if (comma == std::string::npos)
					break;
				at = comma + 1;
			}
			if (!wanted)
				continue;
		}
		for (plugin::Model* model : p->models) {
			if (!model)
				continue;
			// ONE AT A TIME, AND THROWN AWAY. A module that misbehaves on construction takes
			// Rack with it whatever we do here, so the log line goes out BEFORE the attempt:
			// if this ever does bring Rack down, the last line written names the culprit.
			INFO("Census: %s %s", p->slug.c_str(), model->slug.c_str());
			engine::Module* m = NULL;
			try {
				m = model->createModule();
			}
			catch (std::exception& e) {
				WARN("Census: %s %s threw: %s", p->slug.c_str(), model->slug.c_str(), e.what());
				continue;
			}
			if (!m)
				continue;

			json_t* j = json_object();
			json_object_set_new(j, "plugin", json_string(p->slug.c_str()));
			json_object_set_new(j, "model", json_string(model->slug.c_str()));
			json_object_set_new(j, "name", json_string(model->name.c_str()));

			json_t* insJ = json_array();
			for (size_t i = 0; i < m->inputInfos.size(); i++)
				json_array_append_new(insJ, portJson(m->inputInfos[i], (int) i));
			json_object_set_new(j, "inputs", insJ);

			json_t* outsJ = json_array();
			for (size_t i = 0; i < m->outputInfos.size(); i++)
				json_array_append_new(outsJ, portJson(m->outputInfos[i], (int) i));
			json_object_set_new(j, "outputs", outsJ);

			// THE PARAMETERS TOO, because a plugin that names none of its ports usually names
			// all of its knobs, and a jack sits beside the control it belongs to.
			json_t* paramsJ = json_array();
			for (size_t i = 0; i < m->paramQuantities.size(); i++) {
				engine::ParamQuantity* q = m->paramQuantities[i];
				json_t* pj = json_object();
				json_object_set_new(pj, "index", json_integer((int) i));
				json_object_set_new(pj, "name", json_string(q ? q->name.c_str() : ""));
				json_object_set_new(pj, "unit", json_string(q ? q->unit.c_str() : ""));
				json_object_set_new(pj, "description",
					json_string(q ? q->description.c_str() : ""));
				json_array_append_new(paramsJ, pj);
			}
			json_object_set_new(j, "params", paramsJ);

			json_array_append_new(modulesJ, j);
			count++;
			delete m;
		}
	}

	json_object_set_new(rootJ, "modules", modulesJ);
	system::createDirectories(asset::user("DreamerDevelopment"));
	const std::string path = asset::user("DreamerDevelopment/census.json");
	FILE* file = std::fopen(path.c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(1));
		std::fclose(file);
		INFO("Census: wrote %d models to %s", count, path.c_str());
	}
	else {
		WARN("Census: could not write %s", path.c_str());
	}
	json_decref(rootJ);
	return count;
}
