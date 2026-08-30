/** Making an unconnected output produce a signal.

THE PROBLEM. A scope clipped onto an output that has no cable in it usually shows nothing —
not because the tap failed, but because the module never computed anything. Rack modules
almost all begin with `if (!outputs[X].isConnected()) return;`, and skipping unconnected
outputs is the right thing for them to do: it is free performance, and nobody was listening.

An output counts as connected when it has polyphonic channels, and the engine gives it
channels when a cable is attached. There is no way to ask for the channels directly from
another plugin, and setting them behind the engine's back would be undone, or worse, race with
the audio thread.

THE ANSWER is a real cable that goes nowhere useful: from the watched output into a spare
input on the Test Gear module, hidden the way the injectors' cables are. The source module
then sees a genuine connection, computes as it would for any patch lead, and the tap gets its
signal. Nothing reads the far end.

WHAT IT COSTS. The source module now does the work it was skipping, which is the point. A
module whose behaviour changes when an output is patched — one that normals a signal
elsewhere while the jack is empty — will behave as though it has been patched, because it has
been. That is the honest trade, and it is the same trade a probe with a physical lead makes on
a bench.

The cables are reconciled every frame rather than tracked: one is laid when a viewer sits on
an output nobody else has patched, and taken away when the viewer leaves or a real cable
arrives. Anything else — an undo, a module removed, a patch loaded with stale ones in it —
converges on the next frame without a special case.
*/
#include "plugin.hpp"
#include "Clip.hpp"
#include "Sink.hpp"

#include <map>
#include <set>
#include <vector>


static app::ModuleWidget* findTestGearWidget() {
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		if (!mw->module || !mw->model || !mw->model->plugin)
			continue;
		if (mw->model->plugin->slug == "DreamerDevelopment" && mw->model->slug == "TestGear")
			return mw;
	}
	return NULL;
}


static app::PortWidget* findSinkPort(app::ModuleWidget* mw, int slot) {
	if (!mw)
		return NULL;
	for (app::PortWidget* p : mw->getPorts()) {
		if (p->type == engine::Port::INPUT && p->portId == slot)
			return p;
	}
	return NULL;
}


/** Is this cable one of ours, laid to wake an output up? */
static bool isSinkCable(app::CableWidget* cw, Module* testGear) {
	return cw && cw->cable && cw->cable->inputModule == testGear
		&& cw->cable->inputId >= 0 && cw->cable->inputId < SINK_MAX;
}


static void hideCable(app::CableWidget* cw) {
	cw->visible = false;
	if (cw->inputPlug)
		cw->inputPlug->visible = false;
	if (cw->outputPlug)
		cw->outputPlug->visible = false;
}


void sinkStep() {
	app::ModuleWidget* gearWidget = findTestGearWidget();
	Module* gear = gearWidget ? gearWidget->module : NULL;

	// Every output a viewer is watching.
	std::set<app::PortWidget*> watched;
	for (widget::Widget* child : APP->scene->rack->children) {
		ClipWidget* clip = dynamic_cast<ClipWidget*>(child);
		if (clip && clip->needsSignal() && clip->port
			&& clip->port->type == engine::Port::OUTPUT)
			watched.insert(clip->port);
	}

	// What is patched already, and which of our sinks exist.
	std::set<app::PortWidget*> hasRealCable;
	std::map<app::PortWidget*, app::CableWidget*> sinks;
	std::set<int> usedSlots;
	std::vector<app::CableWidget*> strays;

	for (app::CableWidget* cw : APP->scene->rack->getCompleteCables()) {
		if (!cw->cable || !cw->outputPort)
			continue;
		if (isSinkCable(cw, gear)) {
			// One sink per output. A second is a leftover — from an undo, or from a patch
			// saved while one was in place.
			if (watched.count(cw->outputPort) && !sinks.count(cw->outputPort)) {
				sinks[cw->outputPort] = cw;
				usedSlots.insert(cw->cable->inputId);
			}
			else {
				strays.push_back(cw);
			}
			continue;
		}
		hasRealCable.insert(cw->outputPort);
	}

	// A sink is only wanted where nothing else is patched: with a real cable in the jack the
	// module is already computing, and ours would be one more cable to explain.
	for (auto& entry : sinks) {
		if (hasRealCable.count(entry.first))
			strays.push_back(entry.second);
	}

	for (app::CableWidget* cw : strays) {
		APP->scene->rack->removeCable(cw);
		delete cw;
	}

	if (!gear || !gearWidget)
		return;

	for (app::PortWidget* out : watched) {
		if (hasRealCable.count(out) || sinks.count(out) || !out->module)
			continue;

		// A free sink input.
		int slot = -1;
		for (int i = 0; i < SINK_MAX; i++) {
			if (!usedSlots.count(i)) {
				slot = i;
				break;
			}
		}
		if (slot < 0) {
			WARN("Sink: all %d sink inputs are in use", SINK_MAX);
			return;
		}
		app::PortWidget* sinkPort = findSinkPort(gearWidget, slot);
		if (!sinkPort)
			return;

		engine::Cable* c = new engine::Cable;
		c->outputModule = out->module;
		c->outputId = out->portId;
		c->inputModule = gear;
		c->inputId = slot;
		APP->engine->addCable(c);

		app::CableWidget* cw = new app::CableWidget;
		cw->setCable(c);
		cw->outputPort = out;
		cw->inputPort = sinkPort;
		APP->scene->rack->addCable(cw);
		hideCable(cw);
		usedSlots.insert(slot);

		INFO("Sink: woke output %d of module %lld", out->portId,
			(long long) out->module->id);
	}
}
