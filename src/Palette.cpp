#include "Palette.hpp"

#include <osdialog.h>
#include <tag.hpp>

#include <cctype>
#include <cmath>
#include <vector>
#include <cstdio>
#include <cstring>
#include <map>

// The dialogue's own metrics, deliberately the same shape as the hint's: same width, same
// padding, same buttons. Two dialogues from one plugin that look like two dialogues from two
// plugins is worse than either of them alone.
static const float PAL_W = 320.f;
static const float PAL_PAD = 14.f;
static const float PAL_TITLE = 15.f;
static const float PAL_TEXT = 11.f;
static const float PAL_SWATCH_H = 32.f;
static const float PAL_SWATCH_GAP = 6.f;
/** The row of names under the swatches. */
static const float PAL_SWATCH_LABEL = 15.f;
/** The wheel: hue around it, saturation out from the middle. */
static const float PAL_WHEEL = 175.f;
/** How large the wheel is drawn into its texture. Twice the size it appears at, so it stays
sharp on a screen that renders at two device pixels to the point. */
static const int PAL_WHEEL_TEX = 512;
/** The brightness bar under it. */
static const float PAL_BAR_H = 22.f;
static const float PAL_BTN_H = 24.f;

static const NVGcolor PAL_BG = nvgRGB(0x21, 0x26, 0x2e);
static const NVGcolor PAL_INK = nvgRGB(0xe9, 0xec, 0xf1);
static const NVGcolor PAL_DIM = nvgRGB(0x9a, 0xa3, 0xaf);
static const NVGcolor PAL_EDGE = nvgRGB(0x3d, 0xd6, 0x8c);
static const NVGcolor PAL_LINE = nvgRGB(0x4a, 0x52, 0x5e);
static const NVGcolor PAL_FIELD = nvgRGB(0x16, 0x1a, 0x20);


// ---- the palette itself --------------------------------------------------------------------

/** THE DEFAULT: Omri Cohen's colours. Yellow for pitch and clock rates, blue for gates and
triggers, green for modulation, red for audio. The four values are the ones the convention
actually circulates with, taken from the Omri Cohen preset that ships with Inklen's Cable
Colour Key, rather than being eyeballed from a description of it.

They were not the first defaults. The first ones were chosen here, and they disagreed with this
scheme about the colour that matters most: yellow meant audio rather than pitch. Anyone who had
learned the common convention — and it is common because Omri Cohen's tutorials are how a great
many people learned Rack at all — read every patch backwards, which is worse than having no
colour code. Three people said so independently on the forum before the module was a week old.
A colour code is a shared language or it is nothing, so this one now speaks the language that
was already being spoken.

The original set is still there, under "Colour scheme", and every colour can be changed. Kept
as the reset target rather than as the values, so "put it back" means something exact. */
static const NVGcolor PAL_DEFAULT[NUM_FAMILIES] = {
	nvgRGB(0xc9, 0x18, 0x47),   // audio, red
	nvgRGB(0x0c, 0x8e, 0x15),   // cv, green
	nvgRGB(0x09, 0x86, 0xad),   // trigger, blue
	nvgRGB(0xc9, 0xb7, 0x0e),   // pitch, yellow
	nvgRGB(0xff, 0x3c, 0xc8),   // MPX, magenta — no convention covers it
};

/** The colours this plugin shipped with, kept because some people have built patches around
them and because they are still the clearer set on a dark rack. */
static const NVGcolor PAL_DREAMER[NUM_FAMILIES] = {
	nvgRGB(0xf3, 0xc4, 0x0b),   // audio, yellow
	nvgRGB(0xff, 0x73, 0x00),   // cv, orange
	nvgRGB(0x5a, 0xa0, 0xe6),   // trigger, light blue
	nvgRGB(0x39, 0xa8, 0x5a),   // pitch, green
	nvgRGB(0xff, 0x3c, 0xc8),   // MPX, magenta
};

static const char* PAL_NAME[NUM_FAMILIES] = {
	"Audio",
	"CV",
	"Gate",
	"Pitch",
	"MPX",
};

/** The key each colour is saved under. Separate from the display name so the file survives the
display name being reworded. */
static const char* PAL_KEY[NUM_FAMILIES] = {"audio", "cv", "trigger", "pitch", "mpx"};

static NVGcolor palette[NUM_FAMILIES];
static bool paletteLoaded = false;
/** Bumped whenever a colour or a categorisation changes, so that anything holding a decision
it made from the old ones can tell that it has to make it again. */
static uint64_t paletteGen = 1;

/** THE OTHER TWO THINGS THE FILE HOLDS, both about categorisation rather than about colour.

`rules` is a list the user writes: a piece of text to look for in a port's name, and the family
a port whose name contains it belongs to. Theirs are asked before ours, in the order written,
so a scheme can be replaced wholesale rather than only added to.

`overrides` is what the port's own menu writes: one exact port of one exact module, named by
plugin, model, direction and number, so it survives the module being moved, copied, or loaded
into another patch. */
/** WHICH WAY A RULE FACES. Most words mean different things on the two sides of a module: a
filter's outputs are audio whatever its inputs are, and saying so is the difference between a
rule that works and a rule that has to be hedged. */
static const int PAL_EITHER = 0, PAL_IN = 1, PAL_OUT = 2;

/** ONE RULE. Every field beyond the family is a CONDITION, and a rule fires only when all of the
ones it has are met — so a rule with nothing but a word behaves exactly as rules always did, and
anything more is narrowing.

WHY THERE IS MORE THAN A WORD NOW. A name on its own cannot answer the question. Fundamental's
LFO and Fundamental's VCO both call their output "Triangle", and no amount of reading that word
will tell you that one of them is modulation and the other is a sound. What does tell you is the
module: its author already declared what it is, in the tags Rack shows in the browser. So a rule
can ask about the module as well as about the port, and the LFO stops being a wrong answer we
had to accept. */
struct PaletteRule {
	/** A piece of the port's name, upper case. Empty means the rule is not about the name at all
	— which is only meaningful alongside a module or a tag, and is refused otherwise. */
	std::string match;
	/** A WHOLE WORD rather than any run of letters. This is what makes short words usable: TRI
	no longer lives inside TRIM, and IN, OUT, L and R become rules you can actually write. */
	bool word = false;
	/** Words that disqualify the port however well the rest of the rule fits. */
	std::vector<std::string> except;
	/** A piece of the module's plugin slug, model slug or model name, upper case. For pinning a
	rule to one maker's modules, or to one module. */
	std::string module;
	/** A tag the module carries, as Rack names it in the browser — "LFO", "Filter", "Envelope
	generator". Case does not matter and Rack's own aliases are accepted, so VCA and
	"Voltage-controlled amplifier" are the same tag. */
	std::string tag;
	/** Looked up once and kept: -2 not yet asked, -1 no such tag. */
	mutable int tagId = -2;
	/** PAL_EITHER, PAL_IN or PAL_OUT. */
	int dir = PAL_EITHER;
	int family = 0;

	/** WHAT MAKES THIS RULE THIS RULE, for deciding whether the file already has it. The family
	is deliberately not part of it: a rule whose family the user has changed is still their
	version of that rule and must not be handed back a second copy. */
	std::string key() const {
		return match + "\x1f" + (word ? "w" : "s") + "\x1f" + module + "\x1f"
			+ string::uppercase(tag) + "\x1f" + std::to_string(dir);
	}
};
static std::vector<PaletteRule> paletteRules;
/** The rules version the file in force was written from, and whether reading it changed
anything — which is when it is worth writing back. */
static int paletteFileVersion = 0;
static bool paletteMerged = false;

/** THE RULES THE PLUGIN COMES WITH, and the only place they are written down.

They are the file's starting contents rather than something hidden in the code that the file
merely adds to. A user who opens colours.json sees exactly what decides a port's family, in the
order it is decided, and can change a rule, reorder them, or take one out — which is what a
configuration file is for. It also means nothing in here can drift from what is written out.

ORDER MATTERS AND IS PRESERVED. An MPX port is called something like "MPX note in", and the
pitch rule below would claim it on the word NOTE — which is how a cable carrying a whole
instrument came out green. First match wins, so MPX is first. */
/** THE TABLE IS THE TRUTH, AND THE FILE RECORDS IT.

There was a version on this table once, and a merge that brought each newly added rule into an
existing file at the position the table intended, so that a rule added in a later release reached
somebody who had already run the plugin. It worked, and the cost was that the number had to move
every single time the table changed — twice in one morning, at which point the version was
counting edits rather than releases, which is not what a version is.

So while the table is being worked out, it is simply read every launch. Rules in the file that
the table does not have are the user's own: those are kept, and kept FIRST, since theirs are
asked before ours. Everything else comes from the table as it stands.

WHAT THIS GIVES UP, and it is worth naming. A default rule deleted by hand comes back, and a
default rule whose family or position was changed by hand goes back to what the table says. That
is the right trade only while nobody has hand-edited the file into something they care about; the
version and its merge are what to bring back when the table settles. */
static const int PAL_RULES_VERSION = 1;

/** A default rule. The `since` is what the merge used to read and is kept only so that bringing
the merge back is a matter of restoring the code around it rather than dating every rule again. */
struct DefaultRule {
	PaletteRule rule;
	int since;
};

/** The three shapes a default rule comes in, so the table below reads as what it means rather
than as rows of empty fields. */
static PaletteRule ruleWord(const char* match, int family) {
	PaletteRule r;
	r.match = match;
	r.word = true;
	r.family = family;
	return r;
}

static PaletteRule ruleAny(const char* match, int family) {
	PaletteRule r;
	r.match = match;
	r.family = family;
	return r;
}

/** A rule pinned to one module: its plugin slug, model slug or model name, with no word to
match. For the ports whose names are a position rather than a description — "Cell 3", "Row 5",
"Channel 2" — where only the module can say what is on them. */
static PaletteRule ruleModule(const char* module, int dir, int family) {
	PaletteRule r;
	r.module = module;
	r.dir = dir;
	r.family = family;
	return r;
}

static PaletteRule ruleTag(const char* tag, int dir, int family) {
	PaletteRule r;
	r.tag = tag;
	r.dir = dir;
	r.family = family;
	return r;
}

static const std::vector<DefaultRule>& defaultRules() {
	static std::vector<DefaultRule> list;
	if (!list.empty())
		return list;
	auto add = [](PaletteRule r, int since) { list.push_back(DefaultRule{r, since}); };

	add(ruleAny("MPX", FAM_MPX), 1);
	add(ruleAny("V/OCT", FAM_PITCH), 1);
	add(ruleAny("PITCH", FAM_PITCH), 1);
	add(ruleAny("NOTE", FAM_PITCH), 1);
	add(ruleAny("GATE", FAM_TRIGGER), 1);
	add(ruleAny("TRIG", FAM_TRIGGER), 1);
	add(ruleAny("CLOCK", FAM_TRIGGER), 1);
	add(ruleAny("CLK", FAM_TRIGGER), 1);
	add(ruleAny("RESET", FAM_TRIGGER), 1);
	add(ruleAny("SYNC", FAM_TRIGGER), 1);
	// BPM IS A PITCH, whatever it is driving. A BPM control voltage is exponential and doubles
	// per volt — nought volts is 120, one volt is 240, minus one is 60 — which is volt per
	// octave in every respect except that the thing it sets is a tempo rather than a note. It
	// belongs with V/Oct because it IS V/Oct, and the convention we follow already colours it
	// that way. Suggested on the forum.
	//
	// AFTER the trigger rules, so that a port called "BPM clock", which sends pulses, is still
	// read as a clock. Only a BPM port that is not also named as a clock lands here.
	add(ruleAny("BPM", FAM_PITCH), 2);
	add(ruleAny("CV", FAM_CV), 1);
	add(ruleAny("MOD", FAM_CV), 1);
	add(ruleAny("FM", FAM_CV), 1);
	// A LEVEL IS A CONTROL VOLTAGE, not the audio it controls. An envelope, a velocity and a
	// breath all arrive at a port called level, and with no rule for the word they fall through
	// to audio — which is the fallback rather than a decision, and makes a control input the same
	// colour as the signal it is scaling.
	//
	// LAST AMONG THE CV RULES, so a port named "CV level" is still read by the earlier one; it
	// makes no difference here, since both are the same family, and it keeps the group's order
	// meaning what it says.
	add(ruleAny("LEVEL", FAM_CV), 3);

	// THE MODULE ANSWERS WHAT THE NAME CANNOT. An LFO's outputs are called "Sine" and "Triangle"
	// exactly as an oscillator's are, and they are modulation rather than sound. The word cannot
	// tell them apart and no word ever will; the tag does, because the module's author set it.
	//
	// BEFORE THE WAVEFORM WORDS BELOW, which is the whole point — those words are right about a
	// VCO and wrong about an LFO, and this group takes the LFO out of their way first.
	//
	// OUTPUTS ONLY, every one of them. A tag describes the module, not the port, and these
	// modules' INPUTS are a different matter: an envelope generator's inputs are gates, an LFO's
	// are its rate and its reset. Those are already named by the rules above, and a tag rule
	// facing both ways would reach past them for anything they missed.
	add(ruleTag("LFO", PAL_OUT, FAM_CV), 5);
	add(ruleTag("Envelope generator", PAL_OUT, FAM_CV), 5);
	add(ruleTag("Function generator", PAL_OUT, FAM_CV), 5);
	add(ruleTag("Slew limiter", PAL_OUT, FAM_CV), 5);
	add(ruleTag("Envelope follower", PAL_OUT, FAM_CV), 5);
	add(ruleTag("Sample and hold", PAL_OUT, FAM_CV), 5);
	add(ruleTag("Random", PAL_OUT, FAM_CV), 5);
	// A quantizer emits notes, whatever it was fed.
	add(ruleTag("Quantizer", PAL_OUT, FAM_PITCH), 5);

	// AUDIO HAS TO BE SAID NOW THAT IT IS NOT THE FALLBACK. While an unmatched port was painted
	// as audio, no rule was needed to make an oscillator's outputs red: they were red by default.
	// With an unmatched port left as Rack drew it, a VCO went colourless — its outputs are called
	// "Sine", "Triangle", "Sawtooth" and "Square", and not one of those words was in this table.
	//
	// So the waveforms are named. They are what an oscillator calls its outputs and what a noise
	// source, a folder and a mixer call theirs, which is most of the audio anybody patches.
	//
	// AFTER the trigger rules, so a port called "Trigger" is a trigger; TRI carries an exception
	// for TRIM as well, since a trim is a knob's worth of voltage and not a waveform.
	add(ruleAny("AUDIO", FAM_AUDIO), 4);
	add(ruleAny("SINE", FAM_AUDIO), 4);
	PaletteRule tri = ruleAny("TRI", FAM_AUDIO);
	tri.except.push_back("TRIM");
	add(tri, 4);
	add(ruleAny("SAW", FAM_AUDIO), 4);
	add(ruleAny("SQU", FAM_AUDIO), 4);
	add(ruleAny("PULSE", FAM_AUDIO), 4);
	add(ruleAny("NOISE", FAM_AUDIO), 4);
	add(ruleAny("MIX", FAM_AUDIO), 4);

	// THE WORDS A CONTROL INPUT IS USUALLY CALLED, which nothing above catches. A filter's cutoff
	// and resonance, a delay's feedback, an effect's depth and rate: all of them are voltages that
	// set something, and all of them were falling through.
	add(ruleAny("CUTOFF", FAM_CV), 5);
	add(ruleAny("RESON", FAM_CV), 5);
	add(ruleAny("FEEDBACK", FAM_CV), 5);
	add(ruleAny("DEPTH", FAM_CV), 5);
	add(ruleAny("AMOUNT", FAM_CV), 5);
	add(ruleAny("RATE", FAM_CV), 5);
	add(ruleAny("OFFSET", FAM_CV), 5);
	add(ruleAny("SHAPE", FAM_CV), 5);
	// AN ENVELOPE'S FOUR TIMES. The tag rules face outputs only — a tag says what the module is,
	// and an envelope generator's OUTPUT is what that tells you about — so the inputs on the front
	// of every ADSR in the rack were still falling through. They are times and a level, set by
	// voltage, and they are called the same four words wherever you find them.
	add(ruleAny("ATTACK", FAM_CV), 6);
	add(ruleAny("DECAY", FAM_CV), 6);
	add(ruleAny("SUSTAIN", FAM_CV), 6);
	add(ruleAny("RELEASE", FAM_CV), 6);
	// And the rest of what a module's front panel asks for by voltage.
	add(ruleAny("FREQ", FAM_CV), 6);
	add(ruleAny("WIDTH", FAM_CV), 6);
	add(ruleAny("PAN", FAM_CV), 6);
	add(ruleAny("VELOCITY", FAM_CV), 6);
	add(ruleAny("PRESSURE", FAM_CV), 6);
	add(ruleAny("TIMBRE", FAM_CV), 6);

	// AND THE MODULE ANSWERS AGAIN, this time for the sound. An output on a filter, an amplifier,
	// a reverb or a drum is audio whatever its author chose to call it — which covers every port
	// named "Out", every port named nothing in particular, and the long tail of names no table
	// will ever hold.
	//
	// LAST BUT ONE, so it is genuinely a fallback: every word above still decides first, and this
	// only picks up what they left. Anything on a module with none of these tags is still left in
	// Rack's own colours, which is the honest answer and now applies to far fewer ports.
	static const char* AUDIO_TAGS[] = {
		"Oscillator", "VCA", "Filter", "Low-pass gate", "Waveshaper", "Distortion",
		"Ring modulator", "Reverb", "Delay", "Chorus", "Phaser", "Flanger", "Equalizer",
		"Compressor", "Limiter", "Mixer", "Noise", "Drum", "Sampler", "Granular",
		"Synth voice", "Physical modeling", "Vocoder", "Speech",
	};
	for (const char* t : AUDIO_TAGS)
		add(ruleTag(t, PAL_OUT, FAM_AUDIO), 5);

	// ---- WHAT VCV'S OWN MODULES CALL THINGS ------------------------------------------------
	//
	// From a census taken inside Rack: every model in Core, Fundamental and the VCV plugins
	// instantiated, and its ports asked what they are called. 840 ports, and 839 of them are
	// named — which is why this family was worth doing first, and why so much of what follows
	// is a word rather than a module.
	//
	// These carry well beyond VCV. "Retrigger", "accent", "sweep" and the logic names mean the
	// same thing in everybody's plugin, which is the test a rule has to pass to be in this
	// table at all.

	// Gates and triggers, by what the module is being told to do.
	add(ruleAny("RETRIG", FAM_TRIGGER), 7);
	add(ruleWord("RUN", FAM_TRIGGER), 7);
	add(ruleWord("START", FAM_TRIGGER), 7);
	add(ruleWord("STOP", FAM_TRIGGER), 7);
	add(ruleWord("CONTINUE", FAM_TRIGGER), 7);
	add(ruleAny("STROBE", FAM_TRIGGER), 7);
	add(ruleWord("MUTE", FAM_TRIGGER), 7);
	add(ruleWord("HOLD", FAM_TRIGGER), 7);
	add(ruleWord("PUSH", FAM_TRIGGER), 7);
	add(ruleWord("FLIP", FAM_TRIGGER), 7);
	add(ruleWord("FLOP", FAM_TRIGGER), 7);
	add(ruleWord("EOC", FAM_TRIGGER), 7);
	add(ruleWord("EOF", FAM_TRIGGER), 7);
	// A LOGIC MODULE'S OUTPUTS ARE ITS OPERATIONS. Whole words throughout: OR lives inside a
	// great many names and AND inside more.
	add(ruleWord("AND", FAM_TRIGGER), 7);
	add(ruleWord("NAND", FAM_TRIGGER), 7);
	add(ruleWord("OR", FAM_TRIGGER), 7);
	add(ruleWord("NOR", FAM_TRIGGER), 7);
	add(ruleWord("XOR", FAM_TRIGGER), 7);
	add(ruleWord("XNOR", FAM_TRIGGER), 7);

	// Control voltages: the rest of what a front panel asks for by voltage, and what VCV's
	// drums call the things their knobs set — the port and the knob share a name.
	add(ruleAny("AFTERTOUCH", FAM_CV), 7);
	add(ruleAny("TUNE", FAM_CV), 7);
	add(ruleAny("SWEEP", FAM_CV), 7);
	add(ruleAny("SNAP", FAM_CV), 7);
	add(ruleAny("METAL", FAM_CV), 7);
	add(ruleAny("ACCENT", FAM_CV), 7);
	add(ruleAny("ENVELOPE", FAM_CV), 7);
	add(ruleAny("SLEW", FAM_CV), 7);
	add(ruleAny("GLIDE", FAM_CV), 7);
	add(ruleAny("MORPH", FAM_CV), 7);
	add(ruleAny("THRESHOLD", FAM_CV), 7);
	add(ruleAny("GAIN", FAM_CV), 7);
	add(ruleAny("TEMPO", FAM_CV), 7);
	add(ruleAny("CROSSFADE", FAM_CV), 7);
	add(ruleAny("POSITION", FAM_CV), 7);
	add(ruleAny("DIFFUSION", FAM_CV), 7);
	add(ruleAny("REFLECT", FAM_CV), 7);
	add(ruleAny("SPREAD", FAM_CV), 7);
	add(ruleAny("SMOOTH", FAM_CV), 7);
	add(ruleAny("STEPPED", FAM_CV), 7);
	add(ruleAny("EXPONENTIAL", FAM_CV), 7);
	add(ruleWord("LINEAR", FAM_CV), 7);
	add(ruleWord("VOLTAGE", FAM_CV), 7);
	add(ruleWord("EXTERNAL", FAM_CV), 7);
	add(ruleWord("ADDRESS", FAM_CV), 7);
	// A DELAY TIME AND A HIGH-PASS CORNER ARE CONTROLS, on every effect that has them. After
	// the audio words, so an effect's wet OUTPUT is still audio.
	add(ruleAny("HIGH-PASS", FAM_CV), 7);
	add(ruleAny("HIGHPASS", FAM_CV), 7);
	add(ruleAny("LOW-PASS", FAM_CV), 7);
	add(ruleAny("LOWPASS", FAM_CV), 7);

	// Audio, and the one word for it VCV use that nothing else does.
	add(ruleAny("WAVETABLE", FAM_AUDIO), 7);
	add(ruleAny("DEVICE INPUT", FAM_AUDIO), 7);
	add(ruleAny("DEVICE OUTPUT", FAM_AUDIO), 7);

	// ---- AND WHERE THE NAME IS A POSITION, THE MODULE ANSWERS --------------------------------
	//
	// "Cell 3", "Row 5", "Channel 2" say where a jack is on the panel and nothing about what it
	// carries. These are the modules from the census whose generic names all mean one thing.
	// Pinned by model slug, which is what the module rule matches on.
	add(ruleModule("CV-CC", PAL_EITHER, FAM_CV), 7);
	add(ruleModule("MIDICCToCVInterface", PAL_EITHER, FAM_CV), 7);
	add(ruleModule("Host-CC", PAL_EITHER, FAM_CV), 7);
	add(ruleModule("CV-Gate", PAL_EITHER, FAM_TRIGGER), 7);
	add(ruleModule("Host-Gate", PAL_EITHER, FAM_TRIGGER), 7);
	add(ruleModule("RandomValues", PAL_OUT, FAM_CV), 7);
	add(ruleModule("SHASR", PAL_EITHER, FAM_CV), 7);
	add(ruleModule("8vert", PAL_EITHER, FAM_CV), 7);
	add(ruleModule("VCMixer", PAL_EITHER, FAM_AUDIO), 7);
	add(ruleModule("Unity", PAL_EITHER, FAM_AUDIO), 7);
	add(ruleModule("MidSide", PAL_EITHER, FAM_AUDIO), 7);
	add(ruleModule("SoundStage", PAL_EITHER, FAM_AUDIO), 7);
	add(ruleModule("AudioInterface", PAL_EITHER, FAM_AUDIO), 7);
	// A drum's inputs are the voltages that shape it; its outputs are the drum.
	add(ruleModule("DrumMachine", PAL_IN, FAM_CV), 7);
	add(ruleModule("DrumMachine", PAL_OUT, FAM_AUDIO), 7);

	// THE PLAIN NAMES, WHOLE WORDS ONLY. A port called "In", "Out", "L" or "R" is audio in almost
	// every module that uses those names, and now that a whole word can be asked for they are
	// safe to write: IN does not live inside GAIN, and L does not live inside LEVEL.
	//
	// LAST OF ALL, because they are the vaguest thing here and every rule above deserves to beat
	// them — including the tag rules, which know what kind of module the port is on.
	add(ruleWord("IN", FAM_AUDIO), 5);
	add(ruleWord("OUT", FAM_AUDIO), 5);
	add(ruleWord("LEFT", FAM_AUDIO), 5);
	add(ruleWord("RIGHT", FAM_AUDIO), 5);
	add(ruleWord("L", FAM_AUDIO), 5);
	add(ruleWord("R", FAM_AUDIO), 5);

	// Every rule's word is compared upper case, so the table is folded once here rather than
	// being trusted to have been typed that way.
	for (DefaultRule& d : list) {
		d.rule.match = string::uppercase(d.rule.match);
		d.rule.module = string::uppercase(d.rule.module);
		for (std::string& ex : d.rule.except)
			ex = string::uppercase(ex);
	}
	return list;
}

static std::map<std::string, int> paletteOverrides;


static int familyFromKey(const char* key) {
	if (!key)
		return -1;
	for (int i = 0; i < NUM_FAMILIES; i++) {
		if (std::strcmp(key, PAL_KEY[i]) == 0)
			return i;
	}
	return -1;
}

/** Names one port of one model, for the overrides map. The plugin and model slugs rather than
anything to do with the widget in front of us: the same port of the same module is the same
port in every patch, which is the whole point of remembering it. */
static std::string portKey(app::PortWidget* port) {
	if (!port || !port->module)
		return "";
	rack::plugin::Model* model = port->module->model;
	if (!model || !model->plugin)
		return "";
	return model->plugin->slug + "/" + model->slug
		+ (port->type == engine::Port::OUTPUT ? "/out/" : "/in/")
		+ std::to_string(port->portId);
}


static std::string paletteFilePath() {
	return asset::user("DreamerDevelopment/colours.json");
}

static bool parseHex(const char* text, NVGcolor& out) {
	unsigned r, g, b;
	if (!text || std::sscanf(text, "#%2x%2x%2x", &r, &g, &b) != 3)
		return false;
	out = nvgRGB((unsigned char) r, (unsigned char) g, (unsigned char) b);
	return true;
}

static std::string toHex(NVGcolor c) {
	char buf[8];
	std::snprintf(buf, sizeof(buf), "#%02x%02x%02x",
		(int) std::lround(math::clamp(c.r, 0.f, 1.f) * 255.f),
		(int) std::lround(math::clamp(c.g, 0.f, 1.f) * 255.f),
		(int) std::lround(math::clamp(c.b, 0.f, 1.f) * 255.f));
	return buf;
}

/** Reads a palette document into the palette, the rules and the overrides. Shared by the file
that is in force and by any other set chosen from beside it, so the two cannot read the same
document differently. */
/** Whether a rule with this text is already in the list. Case is not part of a rule. */
static void paletteRulesToDefault();

/** Whether the built-in table already has this rule, which is what tells a rule the user wrote
apart from one that came from us. */
static bool tableHasRule(const PaletteRule& rule) {
	const std::string key = rule.key();
	for (const DefaultRule& d : defaultRules()) {
		if (d.rule.key() == key)
			return true;
	}
	return false;
}

/** Brings in the default rules added since the file was written, each one placed where it
belongs: immediately before the first later default rule the file already has, so its position
relative to the rules around it is the one the table intends. Appended only if nothing that
follows it is there. */
static void paletteTakeTable() {
	std::vector<PaletteRule> theirs;
	for (const PaletteRule& rule : paletteRules) {
		if (!tableHasRule(rule))
			theirs.push_back(rule);
	}
	paletteRules = theirs;
	for (const DefaultRule& d : defaultRules())
		paletteRules.push_back(d.rule);
	paletteMerged = true;
}

static void paletteReadInto(json_t* rootJ) {
	for (int i = 0; i < NUM_FAMILIES; i++) {
		json_t* colorJ = json_object_get(rootJ, PAL_KEY[i]);
		// A key that is missing or malformed leaves that family at its default rather than
		// failing the whole file: a palette half read is better than a palette not read.
		if (colorJ && json_is_string(colorJ))
			parseHex(json_string_value(colorJ), palette[i]);
	}

	// The user's own name rules. Anything malformed is passed over rather than failing the
	// file: a rule the reader cannot make sense of should cost that rule and nothing else.
	paletteRules.clear();
	json_t* rulesJ = json_object_get(rootJ, "rules");
	if (rulesJ && json_is_array(rulesJ)) {
		size_t index;
		json_t* ruleJ;
		json_array_foreach(rulesJ, index, ruleJ) {
			if (!json_is_object(ruleJ))
				continue;
			json_t* familyJ = json_object_get(ruleJ, "family");
			if (!familyJ || !json_is_string(familyJ))
				continue;
			const int family = familyFromKey(json_string_value(familyJ));
			if (family < 0)
				continue;

			PaletteRule rule;
			rule.family = family;
			if (json_t* matchJ = json_object_get(ruleJ, "match")) {
				if (json_is_string(matchJ))
					rule.match = string::uppercase(json_string_value(matchJ));
			}
			if (json_t* wordJ = json_object_get(ruleJ, "word"))
				rule.word = json_is_true(wordJ);
			if (json_t* moduleJ = json_object_get(ruleJ, "module")) {
				if (json_is_string(moduleJ))
					rule.module = string::uppercase(json_string_value(moduleJ));
			}
			if (json_t* tagJ = json_object_get(ruleJ, "tag")) {
				if (json_is_string(tagJ))
					rule.tag = json_string_value(tagJ);
			}
			if (json_t* dirJ = json_object_get(ruleJ, "dir")) {
				if (json_is_string(dirJ)) {
					const std::string d = string::lowercase(json_string_value(dirJ));
					if (d == "in" || d == "input")
						rule.dir = PAL_IN;
					else if (d == "out" || d == "output")
						rule.dir = PAL_OUT;
				}
			}
			// EITHER ONE WORD OR SEVERAL, because a rule usually needs one exception and
			// having to write a list of one is a thing to get wrong by hand.
			if (json_t* exceptJ = json_object_get(ruleJ, "except")) {
				if (json_is_string(exceptJ))
					rule.except.push_back(string::uppercase(json_string_value(exceptJ)));
				else if (json_is_array(exceptJ)) {
					size_t k;
					json_t* oneJ;
					json_array_foreach(exceptJ, k, oneJ) {
						if (json_is_string(oneJ))
							rule.except.push_back(string::uppercase(json_string_value(oneJ)));
					}
				}
			}
			// A RULE THAT ASKS NOTHING WOULD CLAIM EVERY PORT IN THE RACK, which is not something
			// anybody means to write. Dropped rather than obeyed.
			if (rule.match.empty() && rule.module.empty() && rule.tag.empty())
				continue;
			paletteRules.push_back(rule);
		}
	}

	// The table decides, and anything of the user's own is kept in front of it. Read for the
	// version anyway, so a file written by a build that still merged is understood rather than
	// argued with.
	paletteFileVersion = PAL_RULES_VERSION;
	if (json_t* versionJ = json_object_get(rootJ, "version"))
		paletteFileVersion = (int) json_integer_value(versionJ);
	paletteMerged = false;
	paletteTakeTable();

	paletteOverrides.clear();
	json_t* portsJ = json_object_get(rootJ, "ports");
	if (portsJ && json_is_object(portsJ)) {
		const char* key;
		json_t* valueJ;
		json_object_foreach(portsJ, key, valueJ) {
			if (!json_is_string(valueJ))
				continue;
			const int family = familyFromKey(json_string_value(valueJ));
			if (family >= 0)
				paletteOverrides[key] = family;
		}
	}
}

static void paletteSave();

static void paletteLoad() {
	for (int i = 0; i < NUM_FAMILIES; i++)
		palette[i] = PAL_DEFAULT[i];
	paletteLoaded = true;

	FILE* file = std::fopen(paletteFilePath().c_str(), "r");
	if (!file) {
		paletteRulesToDefault();
		// NOTHING THERE YET, so write it. The file was only ever created when somebody changed
		// a colour, which meant that anyone wanting to edit it by hand — the whole reason it is
		// a file rather than a setting buried in a patch — had to first find the chooser and
		// change something at random to make one appear. A file that documents its own format
		// by existing is worth the one write.
		paletteSave();
		return;
	}
	json_error_t error;
	json_t* rootJ = json_loadf(file, 0, &error);
	std::fclose(file);
	if (!rootJ)
		return;
	paletteReadInto(rootJ);
	json_decref(rootJ);
	paletteGen++;
	// Written back when the merge added something, or when the file simply predates the
	// current table — so the same merge is not worked out again on every launch.
	if (paletteMerged || paletteFileVersion != PAL_RULES_VERSION)
		paletteSave();
}

/** The palette, the rules and the overrides as a document. */
static json_t* paletteToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "version", json_integer(PAL_RULES_VERSION));
	for (int i = 0; i < NUM_FAMILIES; i++)
		json_object_set_new(rootJ, PAL_KEY[i], json_string(toHex(palette[i]).c_str()));

	// Written back whether or not this run changed them, so that saving a colour cannot lose
	// rules the user typed in by hand.
	json_t* rulesJ = json_array();
	for (const PaletteRule& rule : paletteRules) {
		json_t* ruleJ = json_object();
		// ONLY WHAT THE RULE ACTUALLY SAYS. A file where every rule carried every field, most of
		// them empty, would be four times the length and no clearer — and the point of the file
		// is that somebody can read it.
		if (!rule.match.empty())
			json_object_set_new(ruleJ, "match", json_string(rule.match.c_str()));
		if (rule.word)
			json_object_set_new(ruleJ, "word", json_true());
		if (!rule.except.empty()) {
			json_t* exceptJ = json_array();
			for (const std::string& ex : rule.except)
				json_array_append_new(exceptJ, json_string(ex.c_str()));
			json_object_set_new(ruleJ, "except", exceptJ);
		}
		if (!rule.module.empty())
			json_object_set_new(ruleJ, "module", json_string(rule.module.c_str()));
		if (!rule.tag.empty())
			json_object_set_new(ruleJ, "tag", json_string(rule.tag.c_str()));
		if (rule.dir == PAL_IN)
			json_object_set_new(ruleJ, "dir", json_string("in"));
		else if (rule.dir == PAL_OUT)
			json_object_set_new(ruleJ, "dir", json_string("out"));
		json_object_set_new(ruleJ, "family", json_string(PAL_KEY[rule.family]));
		json_array_append_new(rulesJ, ruleJ);
	}
	json_object_set_new(rootJ, "rules", rulesJ);

	json_t* portsJ = json_object();
	for (const auto& pair : paletteOverrides)
		json_object_set_new(portsJ, pair.first.c_str(), json_string(PAL_KEY[pair.second]));
	json_object_set_new(rootJ, "ports", portsJ);
	return rootJ;
}

static void paletteSave() {
	json_t* rootJ = paletteToJson();
	system::createDirectories(asset::user("DreamerDevelopment"));
	FILE* file = std::fopen(paletteFilePath().c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(2));
		std::fclose(file);
	}
	json_decref(rootJ);
}


NVGcolor paletteColor(int family) {
	if (!paletteLoaded)
		paletteLoad();
	// A PORT NOTHING RECOGNISES IS DRAWN OFF-WHITE, at ninety per cent, rather than left in
	// Rack's own colours or given a family it may not belong to.
	//
	// It says what is true: this jack takes whatever you patch into it. That is the honest
	// answer for a mult, a merge, an attenuverter and a scope — a hundred and ten of VCV's own
	// ports are like that — and it is also what an unrecognised name should look like, since a
	// colour that means nothing is worse than a colour that means "no opinion".
	//
	// NOT PURE WHITE. On a dark rack a white jack is the brightest thing on the screen, and
	// these are the ports we have least to say about.
	if (family == FAM_NONE)
		return nvgRGB(0xe6, 0xe6, 0xe6);
	if (family < 0 || family >= NUM_FAMILIES)
		return palette[FAM_AUDIO];
	return palette[family];
}

/** EVERYTHING KNOWN ABOUT THE PORT BEING ASKED ABOUT. A name alone for a cable in flight; a name
and the module it is on for a port in the rack. A rule that asks about something not known here
cannot fire, which is why a cable being dragged is still coloured by its words alone. */
struct PaletteWhere {
	std::string name;              /**< Upper case. */
	rack::plugin::Model* model = NULL;
	int dir = PAL_EITHER;

	/** THE PLUGIN SLUG, MODEL SLUG AND MODEL NAME run together, upper case — and put together
	only if a rule actually asks for it. This is called for every port of every module on every
	frame, and the rules that name a module are the rare ones; building the string for all of
	them to serve none of them would be the most expensive thing here. */
	mutable std::string moduleText;
	mutable bool moduleTextBuilt = false;

	const std::string& moduleWords() const {
		if (!moduleTextBuilt) {
			moduleTextBuilt = true;
			if (model && model->plugin) {
				moduleText = string::uppercase(model->plugin->slug + " " + model->slug + " "
					+ model->name);
			}
		}
		return moduleText;
	}
};

/** Whether the needle stands alone in the haystack rather than merely appearing inside a longer
word. Letters and digits are what makes a word; everything else — spaces, slashes, brackets,
hyphens — is a boundary, which is what lets "V/OCT" be one word and "Out L" be two. */
static bool containsWord(const std::string& hay, const std::string& needle) {
	if (needle.empty())
		return false;
	size_t at = 0;
	while ((at = hay.find(needle, at)) != std::string::npos) {
		const size_t end = at + needle.size();
		const bool before = (at == 0) || !std::isalnum((unsigned char) hay[at - 1]);
		const bool after = (end >= hay.size()) || !std::isalnum((unsigned char) hay[end]);
		if (before && after)
			return true;
		at++;
	}
	return false;
}

/** ALL THE CONDITIONS OR NONE OF THEM. A rule is a set of tests joined by "and", so adding a
field to a rule can only ever make it fire less often — which is what makes it safe to add one to
a rule that is already working. */
static bool ruleMatches(const PaletteRule& r, const PaletteWhere& w) {
	if (r.dir != PAL_EITHER && r.dir != w.dir)
		return false;
	if (!r.module.empty()) {
		if (!w.model || w.moduleWords().find(r.module) == std::string::npos)
			return false;
	}
	if (!r.tag.empty()) {
		if (!w.model)
			return false;
		if (r.tagId == -2)
			r.tagId = rack::tag::findId(r.tag);
		if (r.tagId < 0)
			return false;
		bool has = false;
		for (int id : w.model->tagIds) {
			if (id == r.tagId) {
				has = true;
				break;
			}
		}
		if (!has)
			return false;
	}
	// The exceptions are asked even of a rule that says nothing about the name, so a tag rule can
	// still be told to leave one word alone.
	for (const std::string& ex : r.except) {
		if (w.name.find(ex) != std::string::npos)
			return false;
	}
	if (r.match.empty()) {
		// Nothing about the name, so the module was the whole of it — and a rule with no module
		// either would claim every port in the rack.
		return !r.module.empty() || !r.tag.empty();
	}
	return r.word ? containsWord(w.name, r.match)
		: w.name.find(r.match) != std::string::npos;
}

/** The built-in guess, from the same table the file is written from — so the behaviour of a
plugin whose file has no rules is exactly the behaviour of one whose file has the defaults. */
static int paletteGuess(const PaletteWhere& w) {
	for (const DefaultRule& d : defaultRules()) {
		if (ruleMatches(d.rule, w))
			return d.rule.family;
	}
	return FAM_AUDIO;
}

static int paletteFamilyFor(const PaletteWhere& w) {
	if (!paletteLoaded)
		paletteLoad();
	// AN EMPTY LIST MEANS THE BUILT-IN ONES, not "no rules at all". A file written before the
	// defaults were put in it has "rules": [], and reading that as "everything is audio" would
	// have recoloured every rack that already had one.
	if (paletteRules.empty())
		return paletteGuess(w);
	for (const PaletteRule& rule : paletteRules) {
		if (ruleMatches(rule, w))
			return rule.family;
	}
	return FAM_AUDIO;
}

int paletteFamilyForName(const std::string& name) {
	PaletteWhere w;
	w.name = string::uppercase(name);
	return paletteFamilyFor(w);
}

int palettePortOverride(app::PortWidget* port) {
	if (!paletteLoaded)
		paletteLoad();
	// Asked for every port of every module on every frame, so the ordinary case — nobody has
	// overridden anything — costs a test rather than a string being built and looked up.
	if (paletteOverrides.empty())
		return -1;
	const std::string key = portKey(port);
	if (key.empty())
		return -1;
	auto it = paletteOverrides.find(key);
	return it == paletteOverrides.end() ? -1 : it->second;
}

void paletteSetPortOverride(app::PortWidget* port, int family) {
	if (!paletteLoaded)
		paletteLoad();
	const std::string key = portKey(port);
	if (key.empty())
		return;
	if (family < 0 || family >= NUM_FAMILIES)
		paletteOverrides.erase(key);
	else
		paletteOverrides[key] = family;
	paletteGen++;
	paletteSave();
}

int paletteFamilyForPort(app::PortWidget* port) {
	const int override_ = palettePortOverride(port);
	if (override_ >= 0)
		return override_;
	PaletteWhere w;
	if (port) {
		if (engine::PortInfo* info = port->getPortInfo())
			w.name = string::uppercase(info->getName());
		w.dir = (port->type == engine::Port::OUTPUT) ? PAL_OUT : PAL_IN;
		if (port->module)
			w.model = port->module->model;
	}
	return paletteFamilyFor(w);
}


/** THE SETS. Each is a palette AND the rules that decide which family a port belongs to.

Three are built in. Anything else is a file: any other .json beside colours.json is offered by
its filename, so making a set is saving one and sharing a set is sending somebody a file. */
static const PaletteScheme PAL_SCHEMES[] = {
	{"default", "Default"},
	{"dreamrack", "DreamRack"},
	{NULL, NULL},
};

/** DreamRack's own colours, which are where this colour code came from: Clarity was built to
put on a Rack patch what DreamRack already did with its own cords. */
static const NVGcolor PAL_DREAMRACK[NUM_FAMILIES] = {
	nvgRGB(0xf3, 0xc4, 0x0b),   // audio, yellow
	nvgRGB(0xff, 0x73, 0x00),   // control, orange
	nvgRGB(0x5a, 0xa0, 0xe6),   // trigger, light blue
	nvgRGB(0x39, 0xa8, 0x5a),   // pitch, green
	nvgRGB(0xff, 0x3c, 0xc8),   // MPX, magenta
};

const PaletteScheme* paletteSchemes() {
	return PAL_SCHEMES;
}

/** Puts the built-in rules back, whatever the file had. */
static void paletteRulesToDefault() {
	paletteRules.clear();
	for (const DefaultRule& d : defaultRules())
		paletteRules.push_back(d.rule);
}

void paletteApplyScheme(const char* key) {
	if (!paletteLoaded)
		paletteLoad();
	const NVGcolor* from = PAL_DEFAULT;
	if (key && std::strcmp(key, "dreamrack") == 0)
		from = PAL_DREAMRACK;
	for (int i = 0; i < NUM_FAMILIES; i++)
		palette[i] = from[i];
	// EVERY SET CARRIES THE SAME NAME RULES for now. They differ in what the families look
	// like, not in what belongs to them — a port called GATE is a gate whoever is looking. A
	// set that wanted its own rules would be a file, which is what files are for.
	paletteRulesToDefault();
	paletteGen++;
	paletteSave();
}


/** The folder colours.json lives in. */
static std::string paletteDir() {
	return asset::user("DreamerDevelopment");
}

/** Whether a file is a palette document rather than merely a .json in the same folder.

IT HAS TO BE READ TO KNOW. The folder is ours and other things live in it — the hints the plugin
used to keep were a hints.json, and that was duly offered as a set of colours called "hints".
A file named .json says nothing about what is in it, so the test is that it holds at least one
of the five families. */
static bool looksLikePalette(const std::string& path) {
	FILE* file = std::fopen(path.c_str(), "r");
	if (!file)
		return false;
	json_error_t error;
	json_t* rootJ = json_loadf(file, 0, &error);
	std::fclose(file);
	if (!rootJ)
		return false;
	bool ok = false;
	if (json_is_object(rootJ)) {
		for (int i = 0; i < NUM_FAMILIES && !ok; i++) {
			json_t* colorJ = json_object_get(rootJ, PAL_KEY[i]);
			ok = colorJ && json_is_string(colorJ);
		}
	}
	json_decref(rootJ);
	return ok;
}

std::vector<std::string> paletteFileSets() {
	std::vector<std::string> out;
	for (const std::string& path : system::getEntries(paletteDir())) {
		const std::string name = system::getStem(path);
		if (system::getExtension(path) != ".json" || name == "colours")
			continue;
		if (!looksLikePalette(path))
			continue;
		out.push_back(name);
	}
	std::sort(out.begin(), out.end());
	return out;
}

void paletteApplyFileSet(const std::string& name) {
	const std::string path = paletteDir() + "/" + name + ".json";
	FILE* file = std::fopen(path.c_str(), "r");
	if (!file)
		return;
	json_error_t error;
	json_t* rootJ = json_loadf(file, 0, &error);
	std::fclose(file);
	if (!rootJ)
		return;
	paletteReadInto(rootJ);
	json_decref(rootJ);
	paletteGen++;
	// Written straight back to colours.json, so the chosen set IS the one in force and there is
	// no second place for the answer to live.
	paletteSave();
}

void paletteSaveAs(const std::string& name) {
	if (!paletteLoaded)
		paletteLoad();
	if (name.empty())
		return;
	system::createDirectories(paletteDir());
	const std::string path = paletteDir() + "/" + name + ".json";
	json_t* rootJ = paletteToJson();
	FILE* file = std::fopen(path.c_str(), "w");
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(2));
		std::fclose(file);
	}
	json_decref(rootJ);
}

uint64_t paletteGeneration() {
	if (!paletteLoaded)
		paletteLoad();
	return paletteGen;
}

const char* paletteName(int family) {
	if (family < 0 || family >= NUM_FAMILIES)
		return "";
	return PAL_NAME[family];
}


// ---- colour conversion ---------------------------------------------------------------------
// HSV RATHER THAN HSL, because of the shape of the field. A saturation-by-lightness square in
// HSL wastes half its area: the whole top edge is white and the whole bottom edge is black,
// whatever the hue. The HSV square runs white to full colour along the top and black along the
// bottom, so every part of it is a colour worth landing on.
//
// nanovg converts HSL to a colour and nothing back, so both directions are here.

static NVGcolor hsvToRgb(float h, float s, float v) {
	h = h - std::floor(h);
	const float i = std::floor(h * 6.f);
	const float f = h * 6.f - i;
	const float p = v * (1.f - s);
	const float q = v * (1.f - f * s);
	const float t = v * (1.f - (1.f - f) * s);
	switch ((int) i % 6) {
		case 0: return nvgRGBf(v, t, p);
		case 1: return nvgRGBf(q, v, p);
		case 2: return nvgRGBf(p, v, t);
		case 3: return nvgRGBf(p, q, v);
		case 4: return nvgRGBf(t, p, v);
		default: return nvgRGBf(v, p, q);
	}
}

static void rgbToHsv(NVGcolor c, float& h, float& s, float& v) {
	const float max = std::fmax(c.r, std::fmax(c.g, c.b));
	const float min = std::fmin(c.r, std::fmin(c.g, c.b));
	v = max;
	const float d = max - min;
	s = (max <= 0.f) ? 0.f : d / max;
	if (d <= 0.f) {
		// Grey has no hue. Zero rather than undefined, so the strip has somewhere to sit.
		h = 0.f;
		return;
	}
	if (max == c.r)
		h = (c.g - c.b) / d + (c.g < c.b ? 6.f : 0.f);
	else if (max == c.g)
		h = (c.b - c.r) / d + 2.f;
	else
		h = (c.r - c.g) / d + 4.f;
	h /= 6.f;
}

/** Black on a light colour, white on a dark one, so the family's name is readable whatever the
user chooses. The weights are the usual perceptual ones: green carries most of the brightness a
person sees and blue almost none. */
static NVGcolor inkOn(NVGcolor bg) {
	const float lum = 0.2126f * bg.r + 0.7152f * bg.g + 0.0722f * bg.b;
	return lum > 0.55f ? nvgRGB(0x14, 0x18, 0x1d) : nvgRGB(0xf2, 0xf5, 0xf8);
}


// ---- the dialogue ---------------------------------------------------------------------------

static std::shared_ptr<window::Font> bodyFont() {
	return APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
}

static std::shared_ptr<window::Font> titleFont() {
	return APP->window->loadFont(asset::system("res/fonts/Nunito-Bold.ttf"));
}

struct PaletteWidget;
static PaletteWidget* gPalette = NULL;

/** A child of the SCENE rather than of the rack, so it holds still while the rack is scrolled
and zoomed. */
struct PaletteWidget : widget::OpaqueWidget {
	int selected = FAM_AUDIO;
	float h = 0.f, s = 0.f, v = 0.f;
	/** What the palette held when the dialogue opened, so Cancel means something. */
	NVGcolor entry[NUM_FAMILIES];

	math::Rect swatch[NUM_FAMILIES];
	math::Rect wheel;      /**< The square the wheel is inscribed in. */
	math::Rect bar;
	math::Rect btnDefaults, btnSaveAs, btnCancel, btnDone;
	/** 0 while dragging on the wheel, 1 while dragging the bar, -1 otherwise. */
	int dragging = -1;

	/** THE WHEEL IS AN IMAGE, MADE ONCE. Drawn as geometry it would be a few hundred wedges
	every frame, each with its own gradient. Drawn as a texture it is one fill.

	AND IT IS ONLY EVER MADE AT FULL BRIGHTNESS, because brightness in this colour model is a
	multiplication — a colour at brightness v is the same colour at full brightness times v —
	and multiplying by v is exactly what compositing black over it at one minus v does. So the
	brightness bar darkens the wheel with one translucent disc rather than rebuilding it. */
	int wheelImage = -1;

	PaletteWidget() {
		for (int i = 0; i < NUM_FAMILIES; i++)
			entry[i] = paletteColor(i);
		select(FAM_AUDIO);
		layout();
		box.pos = math::Vec(std::floor((APP->scene->box.size.x - PAL_W) / 2.f), 50.f);
	}

	/** NOT ONLY CALLED WHEN THE DIALOGUE IS CLOSED. Quitting Rack with it still open destroys
	the scene, which destroys this — and by then the window is gone, so asking it to free the
	texture reads through a null pointer. Freeing it then would be pointless anyway: the
	graphics context is being torn down and takes its textures with it.

	Clearing the handle here too, because a widget the scene destroys never went through
	paletteDismiss and would otherwise leave it pointing at freed memory. */
	~PaletteWidget() {
		if (gPalette == this)
			gPalette = NULL;
		if (wheelImage >= 0 && APP && APP->window && APP->window->vg)
			nvgDeleteImage(APP->window->vg, wheelImage);
	}

	void select(int family) {
		selected = family;
		rgbToHsv(paletteColor(family), h, s, v);
	}

	/** Written straight into the palette rather than into a preview, so the rack behind the
	dialogue redraws as the pointer moves. A colour is judged against a patch, not against a
	swatch, and there is no way to judge it against a patch you cannot see. */
	void apply() {
		palette[selected] = hsvToRgb(h, s, v);
		paletteGen++;
	}

	void layout() {
		float y = PAL_PAD + PAL_TITLE + 12.f;
		const float inner = PAL_W - 2.f * PAL_PAD;
		const float sw = (inner - (NUM_FAMILIES - 1) * PAL_SWATCH_GAP) / NUM_FAMILIES;
		for (int i = 0; i < NUM_FAMILIES; i++) {
			swatch[i] = math::Rect(math::Vec(PAL_PAD + i * (sw + PAL_SWATCH_GAP), y),
				math::Vec(sw, PAL_SWATCH_H));
		}
		y += PAL_SWATCH_H + PAL_SWATCH_LABEL + 12.f;
		wheel = math::Rect(math::Vec((PAL_W - PAL_WHEEL) / 2.f, y),
			math::Vec(PAL_WHEEL, PAL_WHEEL));
		y += PAL_WHEEL + 14.f;
		bar = math::Rect(math::Vec(PAL_PAD, y),
			math::Vec(PAL_W - 2.f * PAL_PAD, PAL_BAR_H));
		y += PAL_BAR_H + 16.f;

		// SAVE AS ON ITS OWN LINE. It is not one of the three ways out of the dialogue — it
		// keeps what is on screen and leaves you here — and putting it in that row would have
		// made four buttons where the eye expects to find Cancel and Done.
		btnSaveAs = math::Rect(math::Vec(PAL_PAD, y),
			math::Vec(PAL_W - 2.f * PAL_PAD, PAL_BTN_H));
		y += PAL_BTN_H + 10.f;

		btnDefaults = math::Rect(math::Vec(PAL_PAD, y), math::Vec(64.f, PAL_BTN_H));
		btnDone = math::Rect(math::Vec(PAL_W - PAL_PAD - 52.f, y), math::Vec(52.f, PAL_BTN_H));
		btnCancel = math::Rect(math::Vec(btnDone.pos.x - 6.f - 56.f, y),
			math::Vec(56.f, PAL_BTN_H));
		box.size = math::Vec(PAL_W, y + PAL_BTN_H + PAL_PAD);
	}

	// ---- the wheel's geometry, in one place so drawing and hit-testing cannot disagree ----

	math::Vec wheelCentre() {
		return wheel.getCenter();
	}

	float wheelRadius() {
		return PAL_WHEEL / 2.f;
	}

	/** Red at the top, going clockwise, which is how a colour wheel is usually drawn and which
	way round a hand expects to turn it. */
	math::Vec pointFor(float hue, float sat) {
		const float a = hue * 2.f * M_PI;
		const math::Vec c = wheelCentre();
		const float r = sat * wheelRadius();
		return math::Vec(c.x + std::sin(a) * r, c.y - std::cos(a) * r);
	}

	// ---- drawing ----

	void drawButton(const DrawArgs& args, const math::Rect& r, const char* text, bool accent) {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 4.f);
		nvgFillColor(args.vg, accent ? nvgRGB(0x24, 0x3a, 0x30) : nvgRGB(0x2a, 0x31, 0x3b));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, PAL_INK);
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
		// The border is the same on all three; Done stays the obvious one by its fill and its
		// lettering rather than by its edge.
		nvgFillColor(args.vg, accent ? PAL_EDGE : PAL_INK);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, r.getCenter().x, r.getCenter().y, text, NULL);
	}

	/** A marker that can be seen on any colour, including the one underneath it: a white ring
	with a dark ring outside it, so one of the two always contrasts. */
	void drawMarker(const DrawArgs& args, math::Vec p, float radius) {
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, p.x, p.y, radius + 1.f);
		nvgStrokeColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xc0));
		nvgStrokeWidth(args.vg, 2.f);
		nvgStroke(args.vg);
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, p.x, p.y, radius);
		nvgStrokeColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgStrokeWidth(args.vg, 2.f);
		nvgStroke(args.vg);
	}

	void buildWheel(NVGcontext* vg) {
		const int N = PAL_WHEEL_TEX;
		std::vector<unsigned char> pixels(N * N * 4, 0);
		const float centre = (N - 1) / 2.f;
		const float radius = N / 2.f;
		for (int y = 0; y < N; y++) {
			for (int x = 0; x < N; x++) {
				const float dx = x - centre;
				const float dy = y - centre;
				const float dist = std::sqrt(dx * dx + dy * dy);
				unsigned char* px = &pixels[(y * N + x) * 4];
				// Soft over the last pixel and a half rather than a hard cut, or the rim is a
				// staircase.
				const float alpha = math::clamp((radius - dist) / 1.5f, 0.f, 1.f);
				if (alpha <= 0.f)
					continue;
				float hue = std::atan2(dx, -dy) / (2.f * (float) M_PI);
				if (hue < 0.f)
					hue += 1.f;
				const NVGcolor c = hsvToRgb(hue, math::clamp(dist / radius, 0.f, 1.f), 1.f);
				px[0] = (unsigned char) std::lround(c.r * 255.f);
				px[1] = (unsigned char) std::lround(c.g * 255.f);
				px[2] = (unsigned char) std::lround(c.b * 255.f);
				px[3] = (unsigned char) std::lround(alpha * 255.f);
			}
		}
		wheelImage = nvgCreateImageRGBA(vg, N, N, 0, pixels.data());
	}

	void drawWheel(const DrawArgs& args) {
		if (wheelImage < 0)
			buildWheel(args.vg);
		if (wheelImage < 0)
			return;

		const math::Vec c = wheelCentre();
		const float r = wheelRadius();

		nvgBeginPath(args.vg);
		nvgCircle(args.vg, c.x, c.y, r);
		nvgFillPaint(args.vg, nvgImagePattern(args.vg, wheel.pos.x, wheel.pos.y,
			wheel.size.x, wheel.size.y, 0.f, wheelImage, 1.f));
		nvgFill(args.vg);

		// Brightness, as one translucent disc. See wheelImage for why this is exact rather
		// than an approximation of darkening.
		if (v < 1.f) {
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, c.x, c.y, r);
			nvgFillColor(args.vg, nvgRGBA(0, 0, 0,
				(unsigned char) std::lround((1.f - v) * 255.f)));
			nvgFill(args.vg);
		}

		nvgBeginPath(args.vg);
		nvgCircle(args.vg, c.x, c.y, r);
		nvgStrokeColor(args.vg, PAL_INK);
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		drawMarker(args, pointFor(h, s), 7.f);
	}

	/** Black to this colour at full brightness — the actual range the bar covers, rather than a
	general black-to-white ramp that would be the same for every colour. */
	void drawBar(const DrawArgs& args) {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, bar.pos.x, bar.pos.y, bar.size.x, bar.size.y);
		nvgFillPaint(args.vg, nvgLinearGradient(args.vg, bar.pos.x, bar.pos.y,
			bar.pos.x + bar.size.x, bar.pos.y, nvgRGB(0, 0, 0), hsvToRgb(h, s, 1.f)));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, PAL_INK);
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);

		const float x = bar.pos.x + math::clamp(v, 0.f, 1.f) * bar.size.x;
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, x - 4.f, bar.pos.y - 4.f, 8.f, bar.size.y + 8.f, 3.f);
		nvgFillColor(args.vg, nvgRGB(0xf2, 0xf5, 0xf8));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGB(0x14, 0x18, 0x1d));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
	}

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 5.f);
		nvgFillColor(args.vg, PAL_BG);
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, PAL_EDGE);
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);

		std::shared_ptr<window::Font> body = bodyFont();
		std::shared_ptr<window::Font> heading = titleFont();
		if (!body || body->handle < 0)
			return;

		nvgFontFaceId(args.vg, (heading && heading->handle >= 0) ? heading->handle : body->handle);
		nvgFontSize(args.vg, PAL_TITLE);
		nvgFillColor(args.vg, PAL_INK);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, PAL_PAD, PAL_PAD + PAL_TITLE / 2.f, "Port and cable colours", NULL);

		nvgFontFaceId(args.vg, body->handle);
		nvgFontSize(args.vg, PAL_TEXT);

		for (int i = 0; i < NUM_FAMILIES; i++) {
			const math::Rect& r = swatch[i];
			const NVGcolor c = paletteColor(i);
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 4.f);
			nvgFillColor(args.vg, c);
			nvgFill(args.vg);
			// The selected one is outlined rather than merely brighter: on a palette the user
			// has chosen, "brighter" may not be a difference at all.
			nvgStrokeColor(args.vg, i == selected ? PAL_INK : PAL_LINE);
			nvgStrokeWidth(args.vg, i == selected ? 2.5f : 1.f);
			nvgStroke(args.vg);

			// Under the swatch rather than on it. A name written on the colour has to be
			// legible against every colour anyone might choose; a name under it has one
			// background and stays put.
			nvgFillColor(args.vg, i == selected ? PAL_INK : PAL_DIM);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
			nvgText(args.vg, r.getCenter().x, r.pos.y + r.size.y + 3.f, PAL_NAME[i], NULL);
		}

		drawWheel(args);
		drawBar(args);

		drawButton(args, btnSaveAs, "Save these colours as a named set\u2026", false);
		drawButton(args, btnDefaults, "Defaults", false);
		drawButton(args, btnCancel, "Cancel", false);
		drawButton(args, btnDone, "Done", true);
	}

	// ---- input ----

	/** The pointer in this widget's own coordinates. A direct child of the scene, so the
	scene's mouse position minus this box's corner is exactly that. */
	math::Vec localMouse() {
		return APP->scene->getMousePos().minus(box.pos);
	}

	/** Saturation is clamped at the rim rather than the drag being dropped there, so running
	the pointer off the edge means full saturation and keeps the hue following the hand — which
	is how you set a fully saturated colour without having to land exactly on the edge. */
	void setFromWheel(math::Vec pos) {
		const math::Vec c = wheelCentre();
		const float dx = pos.x - c.x;
		const float dy = pos.y - c.y;
		const float dist = std::sqrt(dx * dx + dy * dy);
		// Dead centre has no angle, so the hue is left where it was rather than jumping to red.
		if (dist > 0.001f) {
			float hue = std::atan2(dx, -dy) / (2.f * (float) M_PI);
			if (hue < 0.f)
				hue += 1.f;
			h = hue;
		}
		s = math::clamp(dist / wheelRadius(), 0.f, 1.f);
		apply();
	}

	void setFromBar(math::Vec pos) {
		v = math::clamp((pos.x - bar.pos.x) / bar.size.x, 0.f, 1.f);
		apply();
	}

	void onButton(const ButtonEvent& e) override {
		// Consumed whatever is hit, including the background: a dialogue that lets clicks
		// through is a dialogue that adjusts the knob behind it.
		if (e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT) {
			widget::OpaqueWidget::onButton(e);
			return;
		}
		e.consume(this);

		for (int i = 0; i < NUM_FAMILIES; i++) {
			math::Rect hit = swatch[i];
			hit.size.y += PAL_SWATCH_LABEL;
			if (hit.contains(e.pos)) {
				select(i);
				return;
			}
		}
		// The square, not the disc: a press just outside the rim is a press meant for the rim.
		if (wheel.contains(e.pos)) {
			dragging = 0;
			setFromWheel(e.pos);
			return;
		}
		math::Rect grab = bar;
		grab.pos.y -= 5.f;
		grab.size.y += 10.f;
		if (grab.contains(e.pos)) {
			dragging = 1;
			setFromBar(e.pos);
			return;
		}
		// KEEPS WHAT IS ON SCREEN AS A NEW FILE, and does not touch the one in force. Mixing a
		// palette and then having to choose between keeping it and keeping the one you had is
		// a choice nobody should be made to make: the set is written under its own name, and
		// whether it also becomes the current one is still Done's business.
		if (btnSaveAs.contains(e.pos)) {
			char* typed = osdialog_prompt(OSDIALOG_INFO,
				"Name for this set of colours and rules:", "My colours");
			if (typed) {
				std::string name = typed;
				std::free(typed);
				// A name is a filename, so the characters a filename cannot hold come out.
				std::string clean;
				for (char c : name) {
					if (c == '/' || c == '\\' || c == ':' || c == '.')
						continue;
					clean += c;
				}
				if (!clean.empty())
					paletteSaveAs(clean);
			}
			return;
		}
		if (btnDefaults.contains(e.pos)) {
			for (int i = 0; i < NUM_FAMILIES; i++)
				palette[i] = PAL_DEFAULT[i];
			paletteGen++;
			select(selected);
			return;
		}
		if (btnCancel.contains(e.pos)) {
			for (int i = 0; i < NUM_FAMILIES; i++)
				palette[i] = entry[i];
			paletteGen++;
			paletteDismiss();
			return;
		}
		if (btnDone.contains(e.pos)) {
			paletteSave();
			paletteDismiss();
			return;
		}
	}

	void onDragMove(const DragMoveEvent& e) override {
		if (dragging == 0)
			setFromWheel(localMouse());
		else if (dragging == 1)
			setFromBar(localMouse());
		widget::OpaqueWidget::onDragMove(e);
	}

	void onDragEnd(const DragEndEvent& e) override {
		dragging = -1;
		widget::OpaqueWidget::onDragEnd(e);
	}

	/** The brightness bar takes the wheel as well as a drag. Dragging sets a value in one
	movement; scrolling nudges it, which is what you want once the colour is nearly right and
	the last few percent are the whole of the question. Fifty notches across the range, five
	hundred with Shift held, matching what Rack gives a knob for coarse and fine. */
	void onHoverScroll(const HoverScrollEvent& e) override {
		math::Rect grab = bar;
		grab.pos.y -= 5.f;
		grab.size.y += 10.f;
		if (!grab.contains(e.pos)) {
			widget::OpaqueWidget::onHoverScroll(e);
			return;
		}
		const float step = (APP->window->getMods() & GLFW_MOD_SHIFT) ? 0.002f : 0.02f;
		v = math::clamp(v + (e.scrollDelta.y > 0.f ? step : -step), 0.f, 1.f);
		apply();
		// Consumed, or the rack scrolls out from under the dialogue at the same time.
		e.consume(this);
	}

	/** Escape cancels, which is what Escape means everywhere else. It restores the palette
	rather than merely closing, so a colour tried and disliked leaves nothing behind. */
	void onHoverKey(const HoverKeyEvent& e) override {
		if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ESCAPE) {
			e.consume(this);
			for (int i = 0; i < NUM_FAMILIES; i++)
				palette[i] = entry[i];
			paletteGen++;
			paletteDismiss();
			return;
		}
		widget::OpaqueWidget::onHoverKey(e);
	}

	void step() override {
		// Re-clamped rather than re-placed, so a window resize cannot leave it off screen.
		box.pos.x = math::clamp(box.pos.x, 8.f,
			std::fmax(8.f, APP->scene->box.size.x - box.size.x - 8.f));
		box.pos.y = math::clamp(box.pos.y, 8.f,
			std::fmax(8.f, APP->scene->box.size.y - box.size.y - 8.f));
		widget::OpaqueWidget::step();
	}
};


void paletteShow() {
	if (!paletteLoaded)
		paletteLoad();
	if (gPalette)
		return;
	gPalette = new PaletteWidget;
	APP->scene->addChild(gPalette);
}

bool paletteCovers(math::Vec scenePos) {
	if (!gPalette)
		return false;
	return gPalette->box.contains(scenePos);
}

/** REQUESTED, NOT DONE HERE. Every caller but one is inside the dialogue's own click handler,
and deleting a widget while the event dispatch is still walking it means the dispatch returns
into freed memory. Rack's own answer is to ask the parent to remove it on the next step. */
void paletteDismiss() {
	if (!gPalette)
		return;
	gPalette->requestDelete();
	gPalette = NULL;
}
