/** Asking every module in the library whether its jacks take polyphony — see Probe.hpp. */
#include "Probe.hpp"

#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

/** HOW LONG TO RUN BEFORE BELIEVING THE ANSWER.

A module is not obliged to react on the first sample. Filters settle, sequencers wait for a
clock, and plenty of modules only recompute their channel count on a block boundary. Two hundred
and fifty-six samples is a few milliseconds of audio — long enough for anything that reacts at
all, short enough that the whole library is minutes rather than hours. */
static const int RUN = 256;
/** The rate everything is told it is running at. Any real rate would do; this one is the
commonest, so a module with a rate-dependent table is on its ordinary path. */
static const float RATE = 44100.f;
static const int POLY = 16;
/** A shorter run for the sweep, which is forty-one of them per jack rather than one. Enough for
anything that responds to a level at all; a module that needs longer than three milliseconds to
show a change of input is one that is waiting for a clock, and no length of run will help. */
static const int SETTLE = 32;

/** THE SWEEP, WHICH IS HOW A JACK IS ASKED WHAT IT ACTUALLY TAKES.

From -10V to +10V in half-volt steps: the widest span Rack modules are built for, at a resolution
fine enough to tell a selector with eight positions from a knob. Forty-one points. */
static const float SWEEP_LO = -10.f;
static const float SWEEP_HI = 10.f;
static const float SWEEP_STEP = 0.5f;
static const int SWEEP_POINTS = 41;

/** WHAT TO FEED A JACK THAT WE KNOW NOTHING ABOUT.

Small, positive, and not a round number. Zero would be indistinguishable from silence on a module
that clamps, and 10V drives anything with a threshold into saturation where every channel looks
alike. A volt and a bit sits inside every convention in Rack — above a trigger threshold, inside
an audio swing, within a unipolar CV range — which is what a probe wants when it does not know
which of those it is holding. */
static float testVolts(int channel) {
	return 1.3f + 0.37f * (float) channel;
}

struct Reading {
	std::vector<float> samples;   // the last sample of every output channel, in order
	std::vector<int> channels;    // how wide each output ran
};

/** Runs the module for a while and writes down what came out.

The module is driven, not merely ticked: process() is called sample by sample exactly as the
engine would, with a frame counter that advances, because a module that watches the frame number
to divide its clock gets a run of identical frames otherwise and concludes nothing is happening. */
static Reading runFor(engine::Module* m, int64_t& frame, int samples = RUN) {
	Reading r;
	engine::Module::ProcessArgs args;
	args.sampleRate = RATE;
	args.sampleTime = 1.f / RATE;
	for (int i = 0; i < samples; i++) {
		args.frame = frame++;
		// process(), not doProcess(): the latter is marked PRIVATE in the SDK and refuses to
		// compile in a plugin. It only adds bypass routing and light smoothing, neither of which
		// a measurement wants in the way.
		m->process(args);
	}
	for (size_t o = 0; o < m->outputs.size(); o++) {
		const int ch = m->outputs[o].getChannels();
		r.channels.push_back(ch);
		for (int c = 0; c < ch; c++)
			r.samples.push_back(m->outputs[o].getVoltage(c));
	}
	return r;
}

/** WHERE THE KNOBS ARE, WHICH DECIDES WHETHER A JACK DOES ANYTHING AT ALL.

Twelve of RPJ's modules multiply every CV by an attenuverter that comes up at zero. At the default
settings their CV jacks are disconnected from everything, and a probe that tried only the defaults
would report each of them as monophonic and inert — not because they are, but because the module
was asked while it was switched off. The same is true of any mix knob at zero, any filter with the
cutoff parked, any channel with its level down.

There is no single setting that opens every module, so each jack is tried at four: the defaults,
everything at maximum, everything at minimum, and everything centred. Maximum is the one that
opens an attenuverter; centre is the one that opens a pan or a bipolar offset; the defaults are
what a person actually sees. A jack counts as polyphonic if it looked at channel two under ANY of
them, because one demonstration is a demonstration. */
enum Setting { AS_BUILT, ALL_MAX, ALL_MIN, ALL_MID, NUM_SETTINGS };

static void setKnobs(engine::Module* m, int which) {
	for (size_t p = 0; p < m->params.size(); p++) {
		engine::ParamQuantity* q = m->paramQuantities.size() > p ? m->paramQuantities[p] : NULL;
		if (!q)
			continue;
		switch (which) {
			case ALL_MAX: m->params[p].setValue(q->maxValue); break;
			case ALL_MIN: m->params[p].setValue(q->minValue); break;
			case ALL_MID: m->params[p].setValue((q->minValue + q->maxValue) / 2.f); break;
			default: break;
		}
	}
}

/** Every input silent and one channel wide, which is what an unpatched module sees. */
static void quiet(engine::Module* m) {
	for (size_t i = 0; i < m->inputs.size(); i++) {
		m->inputs[i].setChannels(0);
		for (int c = 0; c < POLY; c++)
			m->inputs[i].setVoltage(0.f, c);
	}
}

/** Whether two readings differ anywhere that matters. */
static bool differs(const Reading& a, const Reading& b) {
	if (a.channels != b.channels || a.samples.size() != b.samples.size())
		return true;
	for (size_t i = 0; i < a.samples.size(); i++) {
		const float d = std::fabs(a.samples[i] - b.samples[i]);
		// A HAIR'S BREADTH, NOT AN EPSILON. Anything a module did deliberately in response to a
		// signal is far larger than this; anything smaller is the last bit of a float.
		if (d > 1e-6f && d > 1e-6f * std::fabs(a.samples[i]))
			return true;
	}
	return false;
}

/** WHAT A JACK DOES WITH A VOLTAGE, FOUND BY TRYING EVERY VOLTAGE.

Forty-one levels from -10V to +10V, and after each one a look at what came out. Two facts fall out
of the resulting row, and neither is readable from any manual:

  THE RANGE IT ACTUALLY USES. Walk in from each end for as long as the outputs are unchanged. A
  jack built for 0-10V ignores everything below zero, so the row is flat from -10V to 0V and the
  walk stops there; the span that is left is the span the module responds to. Where the outputs
  move at the very first step the answer is the sweep's own edge, which means only that the range
  is at least that wide — recorded as such rather than as a discovery.

  WHETHER IT IS STEPPED. A continuous input moves the outputs at nearly every one of the forty
  steps. A selector with eight positions moves them eight times and sits still in between. So the
  count of changes across the sweep separates the two, and does it without knowing anything about
  what the module is for.

A jack that never moves anything is not evidence of a narrow range: it is a module waiting for a
clock, or an expander with nobody beside it, or a mode that is switched off. That is recorded as
"nothing responded" and claims nothing at all.

WHAT THIS IS NOT. A sweep watches the OUTPUTS, and an output is the far end of the whole module.
Feed a continuous CV to a comparator and the gate at the other end changes once, so the sweep says
"stepped" and puts the range at the crossing point — both wrong, and stated with the same
confidence as a right answer. Nothing here can tell the shape of the INPUT from the shape of what
came out the other side.

So the range and the shape this records are EVIDENCE, not findings. They go into the file to be
read by somebody checking a module against its source, and they are worth having because they are
free and they are true of the installed build; they are not worth shipping as a fact about a jack.
Polyphony is different in kind — "did the module look at channel two" is a question about the jack
itself, and the answer does not depend on what any output happens to be doing. */
struct Sweep {
	bool responded = false;
	float lo = 0.f, hi = 0.f;
	int changes = 0;
	bool edgeLo = false, edgeHi = false;   // the response ran to the edge of what was tried
};

static Sweep sweepInput(engine::Module* m, int64_t& frame, size_t port) {
	std::vector<Reading> row;
	row.reserve(SWEEP_POINTS);
	for (int k = 0; k < SWEEP_POINTS; k++) {
		const float v = SWEEP_LO + SWEEP_STEP * (float) k;
		m->inputs[port].setChannels(1);
		m->inputs[port].setVoltage(v, 0);
		row.push_back(runFor(m, frame, SETTLE));
	}
	Sweep s;
	for (int k = 1; k < SWEEP_POINTS; k++) {
		if (differs(row[k - 1], row[k]))
			s.changes++;
	}
	if (!s.changes)
		return s;
	s.responded = true;
	int lo = 0, hi = SWEEP_POINTS - 1;
	while (lo < hi && !differs(row[lo], row[lo + 1]))
		lo++;
	while (hi > lo && !differs(row[hi], row[hi - 1]))
		hi--;
	s.lo = SWEEP_LO + SWEEP_STEP * (float) lo;
	s.hi = SWEEP_LO + SWEEP_STEP * (float) hi;
	s.edgeLo = (lo == 0);
	s.edgeHi = (hi == SWEEP_POINTS - 1);
	return s;
}

static bool wider(const Reading& base, const Reading& test) {
	for (size_t i = 0; i < test.channels.size() && i < base.channels.size(); i++) {
		if (test.channels[i] > base.channels[i] && test.channels[i] > 1)
			return true;
	}
	return false;
}


// ---- the run ------------------------------------------------------------------------------

static std::vector<plugin::Model*> gQueue;
static size_t gAt = 0;
static json_t* gModulesJ = NULL;
static double gStarted = 0.0;
static std::string gStatus;

static std::string probePath() {
	return asset::user("DreamerDevelopment/probe.json");
}

static void probeFlush() {
	if (!gModulesJ)
		return;
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "modules", json_deep_copy(gModulesJ));
	system::createDirectories(asset::user("DreamerDevelopment"));
	FILE* file = std::fopen(probePath().c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(1));
		std::fclose(file);
	}
	json_decref(rootJ);
}

void probeStart(const std::string& only, bool skipDone) {
	gQueue.clear();
	gAt = 0;
	if (gModulesJ)
		json_decref(gModulesJ);
	gModulesJ = json_array();
	gStarted = system::getTime();

	std::set<std::string> done;
	if (skipDone) {
		json_error_t err;
		json_t* rootJ = json_load_file(probePath().c_str(), 0, &err);
		if (rootJ) {
			json_t* modulesJ = json_object_get(rootJ, "modules");
			size_t i;
			json_t* j;
			json_array_foreach(modulesJ, i, j) {
				json_t* pj = json_object_get(j, "plugin");
				json_t* mj = json_object_get(j, "model");
				if (pj && mj)
					done.insert(std::string(json_string_value(pj)) + "/" + json_string_value(mj));
				json_array_append(gModulesJ, j);
			}
			json_decref(rootJ);
			INFO("Probe: %d models already done", (int) done.size());
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
	INFO("Probe: %d models to measure", (int) gQueue.size());
}

bool probeBusy() {
	return gAt < gQueue.size();
}

std::string probeStatus() {
	return gStatus;
}

void probeTick(double seconds) {
	if (!probeBusy())
		return;
	const double until = system::getTime() + seconds;
	while (probeBusy() && system::getTime() < until) {
		plugin::Model* model = gQueue[gAt++];
		const std::string key = model->plugin->slug + "/" + model->slug;
		// NAMED BEFORE IT IS TOUCHED, so that when one of them takes Rack down the log says which
		// one. This line is the whole crash-recovery story: everything else is already on disk.
		INFO("Probe: %s", key.c_str());

		engine::Module* m = NULL;
		try {
			m = model->createModule();
		}
		catch (std::exception& e) {
			WARN("Probe: %s threw on construction: %s", key.c_str(), e.what());
			continue;
		}
		if (!m)
			continue;
		// NO WIDGET, deliberately. The census needs one for positions and pays for it in crashes;
		// nothing here does, and every panel not loaded is a whole class of failure avoided.
		int64_t frame = 0;
		json_t* j = json_object();
		json_object_set_new(j, "plugin", json_string(model->plugin->slug.c_str()));
		json_object_set_new(j, "model", json_string(model->slug.c_str()));
		try {
			m->onReset(engine::Module::ResetEvent());
			quiet(m);
			// TWICE, to find out whether this module is the same module twice. A noise source or
			// a random sequencer is not, and its answer to the second question below is
			// meaningless — the difference it shows is its own, not a response to anything.
			const Reading base = runFor(m, frame);
			const Reading again = runFor(m, frame);
			const bool varies = differs(base, again);
			json_object_set_new(j, "varies", json_boolean(varies));

			json_t* insJ = json_array();
			for (size_t i = 0; i < m->inputs.size(); i++) {
				bool read = false, counts = false;
				Sweep best;
				int openedAt = -1;
				for (int set = 0; set < NUM_SETTINGS; set++) {
					// One input at a time, from the same quiet starting point every time, so that
					// whatever is observed belongs to this jack and not to the one before it.
					m->onReset(engine::Module::ResetEvent());
					setKnobs(m, set);
					quiet(m);
					const Reading before = runFor(m, frame);

					// CHANNEL ONE LEFT AT SILENCE, and the rest given a signal. That is what makes
					// this a test of polyphony rather than a test of whether the jack does
					// anything: a monophonic module sees exactly what it saw before.
					m->inputs[i].setChannels(POLY);
					m->inputs[i].setVoltage(0.f, 0);
					for (int c = 1; c < POLY; c++)
						m->inputs[i].setVoltage(testVolts(c), c);
					const Reading after = runFor(m, frame);

					if (!varies && differs(before, after)) {
						read = true;
						if (openedAt < 0)
							openedAt = set;
					}
					if (wider(before, after)) {
						counts = true;
						if (openedAt < 0)
							openedAt = set;
					}

					// AND WHAT IT TAKES, which the polyphony test cannot see because it holds
					// every channel at one level. A module that varies from itself cannot be
					// swept at all: every step would look like a response.
					if (!varies) {
						m->onReset(engine::Module::ResetEvent());
						setKnobs(m, set);
						quiet(m);
						const Sweep s = sweepInput(m, frame, i);
						// THE WIDEST RESPONSE ANY SETTING GAVE. A knob that narrows what a jack
						// reaches cannot widen it, so the union across settings is the better
						// estimate — and the largest change count is the one least likely to be
						// an artefact of watching a gate output.
						if (s.responded) {
							if (!best.responded) {
								best = s;
							}
							else {
								best.lo = std::fmin(best.lo, s.lo);
								best.hi = std::fmax(best.hi, s.hi);
								best.edgeLo = best.edgeLo || s.edgeLo;
								best.edgeHi = best.edgeHi || s.edgeHi;
								if (s.changes > best.changes)
									best.changes = s.changes;
							}
						}
					}
				}
				json_t* one = json_object();
				json_object_set_new(one, "port", json_integer((int) i));
				json_object_set_new(one, "readsChannels", json_boolean(read));
				json_object_set_new(one, "setsWidth", json_boolean(counts));
				json_object_set_new(one, "poly", json_boolean(read || counts));
				// WHICH SETTING WOKE IT UP, which is worth as much as the answer: a jack that
				// does nothing until every knob is at maximum is a jack behind an attenuverter,
				// and that is a fact about the module somebody will want.
				json_object_set_new(one, "openedAt", json_integer(openedAt));
				// EVIDENCE, NOT A FINDING — see the note above sweepInput. Named so that nothing
				// downstream mistakes it for something we are prepared to say out loud.
				json_t* ev = json_object();
				json_object_set_new(ev, "responded", json_boolean(best.responded));
				if (best.responded) {
					json_object_set_new(ev, "lo", json_real(best.lo));
					json_object_set_new(ev, "hi", json_real(best.hi));
					json_object_set_new(ev, "changes", json_integer(best.changes));
					json_object_set_new(ev, "points", json_integer(SWEEP_POINTS));
					json_object_set_new(ev, "atLoEdge", json_boolean(best.edgeLo));
					json_object_set_new(ev, "atHiEdge", json_boolean(best.edgeHi));
				}
				json_object_set_new(one, "sweepEvidence", ev);
				json_array_append_new(insJ, one);
			}
			json_object_set_new(j, "inputs", insJ);
		}
		catch (std::exception& e) {
			WARN("Probe: %s threw while running: %s", key.c_str(), e.what());
			json_object_set_new(j, "threw", json_string(e.what()));
		}
		json_array_append_new(gModulesJ, j);
		delete m;

		if (gAt % 25 == 0) {
			probeFlush();
			// A LINE IN THE LOG EVERY TWENTY-FIVE, so a run that takes an hour can be followed
			// from outside — `tail -f log.txt` — rather than by watching a panel. The count, the
			// share done and the time left, which is all anybody wants while waiting.
			const double so_far = system::getTime() - gStarted;
			INFO("Probe: %d of %d models (%.0f%%), %.0fs gone, about %.0fs left",
				(int) gAt, (int) gQueue.size(), 100.0 * (double) gAt / (double) gQueue.size(),
				so_far, so_far / (double) gAt * (double) (gQueue.size() - gAt));
		}
	}

	const double taken = system::getTime() - gStarted;
	if (probeBusy()) {
		const double each = taken / (double) gAt;
		gStatus = string::f("%d of %d modules\n%ds left", (int) gAt, (int) gQueue.size(),
			(int) ((gQueue.size() - gAt) * each + 0.5));
		return;
	}
	probeFlush();
	gStatus = string::f("%d of %d modules\ndone in %.0fs", (int) gQueue.size(),
		(int) gQueue.size(), taken);
	INFO("Probe: measured %d models in %.1f s to %s", (int) gQueue.size(), taken,
		probePath().c_str());
}
