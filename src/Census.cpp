/** The port census — see Census.hpp. */
#include "Census.hpp"

#include <cstdio>
#include <set>
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
			// WITH A MODULE BEHIND IT, because some controls only exist when there is one.
			//
			// This used to pass NULL, making a preview widget exactly as the module browser does.
			// That is safe, and it is wrong: a maker is free to add a widget only when a module
			// is there — SurgeXTRack builds its mixer's mute and solo buttons inside
			// `if (module)`, and Surge's older plugin adds a tempo-sync switch only where that
			// parameter can be synced. A preview has none of them, so this census recorded none
			// of them, and the help could say nothing about twelve working controls a reader can
			// click. What is not in this file gets no line, so a gap here is silence there.
			//
			// AND THE MODULE MUST BE TAKEN BACK BEFORE THE WIDGET DIES. ~ModuleWidget calls
			// setModule(NULL), which calls Engine::removeModule, which asserts the module is one
			// the engine knows. A module made here was never added to the engine, so letting the
			// widget keep it asserts and takes Rack with it — which is what the NULL was avoiding.
			// Clearing mw->module by hand is what makes a real module safe here.
			engine::Module* m = NULL;
			try {
				m = model->createModule();
			}
			catch (std::exception& e) {
				WARN("Census: %s module threw: %s", key.c_str(), e.what());
				m = NULL;
			}
			app::ModuleWidget* mw = NULL;
			try {
				mw = model->createModuleWidget(m);
			}
			catch (std::exception& e) {
				WARN("Census: %s threw: %s", key.c_str(), e.what());
				delete m;
				continue;
			}
			if (!mw) {
				delete m;
				continue;
			}

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
			// Take the module back before the destructor can hand it to the engine. See above.
			mw->module = NULL;
			delete mw;
			delete m;
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

/** Where the file lives, and what is already in it. */
static std::string censusPositionsPath() {
	return asset::user("DreamerDevelopment/census-positions.json");
}

static void censusFlush() {
	if (!gModulesJ)
		return;
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "modules", json_deep_copy(gModulesJ));
	system::createDirectories(asset::user("DreamerDevelopment"));
	FILE* file = std::fopen(censusPositionsPath().c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(1));
		std::fclose(file);
	}
	json_decref(rootJ);
}

void censusStart(const std::string& only, bool skipDone) {
	gQueue.clear();
	gAt = 0;
	if (gModulesJ)
		json_decref(gModulesJ);
	gModulesJ = json_array();
	gStarted = system::getTime();

	// WHAT IS ALREADY THERE stays there, and is not walked again.
	std::set<std::string> done;
	if (skipDone) {
		json_error_t err;
		json_t* rootJ = json_load_file(censusPositionsPath().c_str(), 0, &err);
		if (rootJ) {
			json_t* modulesJ = json_object_get(rootJ, "modules");
			size_t i;
			json_t* j;
			json_array_foreach(modulesJ, i, j) {
				json_t* pj = json_object_get(j, "plugin");
				json_t* mj = json_object_get(j, "model");
				if (pj && mj) {
					done.insert(std::string(json_string_value(pj)) + "/"
						+ json_string_value(mj));
				}
				json_array_append(gModulesJ, j);
			}
			json_decref(rootJ);
			INFO("Census: %d models already scanned", (int) done.size());
		}
	}
	for (plugin::Plugin* p : plugin::plugins) {
		if (!p)
			continue;
		for (plugin::Model* model : p->models) {
			if (!model)
				continue;
			const std::string key = p->slug + "/" + model->slug;
			if (!only.empty() && key.compare(0, only.size(), only) != 0)
				continue;
			if (done.count(key))
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
		// WITH A MODULE BEHIND IT — see censusPositions above for why, and for why the module has
		// to be taken back off the widget before the widget is deleted.
		engine::Module* m = NULL;
		try {
			m = model->createModule();
		}
		catch (std::exception& e) {
			WARN("Census: %s module threw: %s", key.c_str(), e.what());
			m = NULL;
		}
		app::ModuleWidget* mw = NULL;
		try {
			mw = model->createModuleWidget(m);
		}
		catch (std::exception& e) {
			WARN("Census: %s threw: %s", key.c_str(), e.what());
			delete m;
			continue;
		}
		if (!mw) {
			delete m;
			continue;
		}
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
		mw->module = NULL;
		delete mw;
		delete m;
		// SAVED EVERY TWENTY-FIVE, so a crash costs a handful of models rather than the run.
		if (gAt % 25 == 0)
			censusFlush();
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

	censusFlush();
	const std::string path = censusPositionsPath();
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

			// AND THE WIDGET, BECAUSE SOME MAKERS NAME THEIR PORTS THERE. configInput and
			// configOutput are usually called in the module's constructor, which the line above
			// has already run — but a maker is free to call them from the WIDGET's constructor
			// instead, guarded by `if (module)`. StudioSixPlusOne does, and this census recorded
			// 205 empty names for that one plugin: ports Rack itself labels perfectly well in a
			// rack, because there the widget is built against a real module.
			//
			// So build the widget against the module before reading the names off. The widget is
			// wanted for its constructor's side effects on the module and nothing else.
			//
			// AND IT MUST BE DETACHED AGAIN BEFORE THE WIDGET DIES. ~ModuleWidget calls
			// setModule(NULL), which calls Engine::removeModule, which asserts that the module
			// is one the engine knows. A module made here was never added to the engine, so
			// letting the widget take it to the grave asserts and takes Rack with it — which is
			// exactly why the OTHER pass above builds its widgets with no module at all.
			//
			// Clearing mw->module by hand is what avoids that: the widget then owns nothing, and
			// the module is deleted here as it always was.
			app::ModuleWidget* mw = NULL;
			try {
				mw = model->createModuleWidget(m);
			}
			catch (std::exception& e) {
				// The names from the constructor are still good, so keep what we have rather than
				// dropping the model. A widget that threw may or may not have taken the module;
				// assume it did not, which leaks at worst and cannot double free.
				WARN("Census: %s %s widget threw: %s",
					p->slug.c_str(), model->slug.c_str(), e.what());
				mw = NULL;
			}

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
			// Detach BEFORE the destructor runs, then free the module ourselves. See above for
			// what happens if the widget is allowed to take it.
			if (mw) {
				mw->module = NULL;
				delete mw;
			}
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
