/** The port census — see Census.hpp. */
#include "Census.hpp"

#include <cstdio>
#include <set>
#include <string>


bool censusOffered() {
	return system::isFile(asset::user("DreamerDevelopment/census.enable"));
}


/** Modules that must not be built or taken apart here — defined below, beside the reasons. */
static bool censusSkips(const std::string& plugin, const std::string& model);

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
			// THE SAME TWO THAT STOPPED THE NAMES WALK STOP THIS ONE HARDER. That pass could get away
		// with building a widget only where a port had no name; this pass exists to find out where
		// the widgets ARE, so it must build every one of them — which is exactly the code that
		// deadlocked on projectM's render thread and faulted in Boxes.
		if (censusSkips(model->plugin->slug, model->slug)) {
			WARN("Census: skipping positions for %s, which is known to bring Rack down here",
				key.c_str());
			continue;
		}
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
			// A WIDGET IS BUILT ONLY WHERE A PORT HAS NO NAME.
			//
			// The reason for building one at all is narrow: a few makers name their ports in the
			// widget rather than in the module, and without it 205 of one plugin's ports come out
			// blank. Everybody else has already said everything by the end of the constructor.
			//
			// And a maker's widget constructor is the most fragile code this walk touches. It
			// loads panels, builds displays, and reaches for things that only exist once the
			// module is in a rack — QuestionableDinner's Boxes walks a point list through a screen
			// pointer that is null until then. Every one we do not run is a crash we do not have.
			//
			// So: ask the module first. If every port it has is named, the widget has nothing to
			// add and is never built.
			bool anyUnnamed = false;
			for (size_t i = 0; i < m->inputInfos.size() && !anyUnnamed; i++) {
				if (!m->inputInfos[i] || m->inputInfos[i]->name.empty())
					anyUnnamed = true;
			}
			for (size_t i = 0; i < m->outputInfos.size() && !anyUnnamed; i++) {
				if (!m->outputInfos[i] || m->outputInfos[i]->name.empty())
					anyUnnamed = true;
			}
			for (size_t i = 0; i < m->paramQuantities.size() && !anyUnnamed; i++) {
				if (!m->paramQuantities[i] || m->paramQuantities[i]->name.empty())
					anyUnnamed = true;
			}
			app::ModuleWidget* mw = NULL;
			try {
				if (anyUnnamed)
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
		if (censusSkips(model->plugin->slug, model->slug)) {
			WARN("Census: skipping positions for %s, which is known to bring Rack down here",
				key.c_str());
			continue;
		}
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
		// NOTHING IS DESTROYED HERE EITHER — the same ruling as the names walk, and for the same
		// evidence. MVerb's destructor called std::terminate on this pass exactly as it did on
		// that one, and a destructor that throws cannot be caught or guarded against.
		//
		// This walk cannot avoid BUILDING widgets, because where the widgets are is the whole
		// point of it. It can avoid taking them apart, and that is where every failure has been.
		// The module is detached first so nothing can hand it to an engine that never had it.
		//
		// The cost is a Rack that may fault while quitting, after the file is written — which is
		// what happened at 11:44 today, and cost nothing.
		mw->module = NULL;
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


/** The names census, written where it stands.

TWELVE MINUTES OF WORK WAS LOST because this file was written once, at the end, and a module in
the middle never returned. Saving as it goes costs a file write every twenty-five models and turns
any future hang, crash or force-quit into the loss of a handful rather than the lot. */
static void censusNamesFlush(json_t* modulesJ, int count) {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "modules", json_deep_copy(modulesJ));
	system::createDirectories(asset::user("DreamerDevelopment"));
	const std::string path = asset::user("DreamerDevelopment/census.json");
	FILE* file = std::fopen(path.c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(1));
		std::fclose(file);
		INFO("Census: %d models so far, saved to %s", count, path.c_str());
	}
	else {
		WARN("Census: could not write %s", path.c_str());
	}
	json_decref(rootJ);
}

/** MODULES THAT MUST NOT BE BUILT OUTSIDE A RACK, because taking them apart never returns.

RPJVisualizer wraps projectM, the Milkdrop visualiser, and its renderer owns a thread. Deleting
the widget joins that thread, and the thread only ever exits when it is running in a real window
with a live graphics context — so in a census, where the widget is built and thrown away, the join
waits for ever. On 14 September it took Rack's interface with it after twelve minutes of work, and
because the census wrote its file only at the end, all twelve minutes were lost.

A crash we can survive: the walk is resumable and the log names the model it was on. A DEADLOCK we
cannot, because nothing is ever written and nothing carries on. So this one is named and stepped
over, and anything else found to hang joins it here rather than costing somebody another run. */
static bool censusSkips(const std::string& plugin, const std::string& model) {
	// Its destructor joins a projectM render thread that never exits outside a real window.
	if (plugin == "RPJVisualizer")
		return true;
	// Its widget constructor walks a point list through a screen pointer that is null until the
	// module is in a rack, and faults on 0x48. Nothing else in the plugin does this.
	if (plugin == "QuestionableDinner" && model == "Boxes")
		return true;
	return false;
}

int censusWrite(const std::string& only) {
	json_t* rootJ = json_object();
	json_t* modulesJ = json_array();
	int count = 0;

	// WHAT IS ALREADY THERE STAYS, AND IS NOT WALKED AGAIN.
	//
	// At 2,115 modules a failed run cost a few minutes and starting again was no hardship. At
	// 4,189 it is ten, and on 14 September two runs in a row ended early — one deadlocked, one
	// took a fault on the AUDIO thread while the walk was building Bogaudio. Starting from the top
	// each time means the far end of the alphabet may never be reached at all.
	//
	// So the file is read back first and every model in it is kept and skipped. Run the item
	// again after a crash and it carries on from where it stopped.
	std::set<std::string> done;
	{
		json_error_t err;
		json_t* oldJ = json_load_file(asset::user("DreamerDevelopment/census.json").c_str(),
			0, &err);
		if (oldJ) {
			json_t* oldModulesJ = json_object_get(oldJ, "modules");
			size_t i;
			json_t* j;
			json_array_foreach(oldModulesJ, i, j) {
				json_t* pj = json_object_get(j, "plugin");
				json_t* mj = json_object_get(j, "model");
				if (pj && mj) {
					done.insert(std::string(json_string_value(pj)) + "/"
						+ json_string_value(mj));
				}
				json_array_append(modulesJ, j);
				count++;
			}
			json_decref(oldJ);
			INFO("Census: %d models already recorded, carrying on from there", count);
		}
	}

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
			if (done.count(p->slug + "/" + model->slug))
				continue;
			if (censusSkips(p->slug, model->slug)) {
				WARN("Census: skipping %s %s, which is known to bring Rack down here",
					p->slug.c_str(), model->slug.c_str());
				continue;
			}
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
			// EVERY FIVE, not every twenty-five. The log does not survive the crash — Rack
			// starts a new one each launch — so this file is the only record of how far it got,
			// and the gap between the last save and the fault is the list of suspects.
			if (count % 5 == 0)
				censusNamesFlush(modulesJ, count);
			// NOTHING IS DESTROYED HERE, AND THAT IS DELIBERATE.
			//
			// Three runs on 14 September ended in three different ways and every one of them died
			// while taking a module apart, never while building one. RPJVisualizer's destructor
			// joins a render thread that never exits. A later run took a fault on the audio thread
			// with a pointer that was actually text, which is a corrupted heap. A third called
			// std::terminate from MVerb's destructor, which is an exception escaping a destructor
			// — again a heap that is already wrong.
			//
			// A maker's constructor has to work: Rack calls it every time somebody adds the module.
			// A destructor OUTSIDE a rack is a path most have never run, on a module that was never
			// added to an engine, never given a window, never stepped. It is the fragile half.
			//
			// So the census builds and reads and walks away. The module is detached from its widget
			// so nothing can hand it to the engine, and both are left where they lie. Four thousand
			// modules is a few hundred megabytes for the minutes the walk takes, and Rack is quit
			// afterwards anyway — which is a cheap price for a walk that reaches the end.
			if (mw)
				mw->module = NULL;
		}
	}

	censusNamesFlush(modulesJ, count);
	json_object_set_new(rootJ, "modules", modulesJ);
	json_decref(rootJ);
	return count;
}
