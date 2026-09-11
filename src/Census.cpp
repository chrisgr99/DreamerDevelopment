/** The port census — see Census.hpp. */
#include "Census.hpp"

#include <cstdio>
#include <string>


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
