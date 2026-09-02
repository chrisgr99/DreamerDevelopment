#include "Palette.hpp"

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
struct PaletteRule {
	std::string match;   /**< Already upper case, so the test is a plain find. */
	int family = 0;
};
static std::vector<PaletteRule> paletteRules;
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

static void paletteLoad() {
	for (int i = 0; i < NUM_FAMILIES; i++)
		palette[i] = PAL_DEFAULT[i];
	paletteLoaded = true;

	FILE* file = std::fopen(paletteFilePath().c_str(), "r");
	if (!file)
		return;
	json_error_t error;
	json_t* rootJ = json_loadf(file, 0, &error);
	std::fclose(file);
	if (!rootJ)
		return;
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
			json_t* matchJ = json_object_get(ruleJ, "match");
			json_t* familyJ = json_object_get(ruleJ, "family");
			if (!matchJ || !json_is_string(matchJ) || !familyJ || !json_is_string(familyJ))
				continue;
			const int family = familyFromKey(json_string_value(familyJ));
			const std::string match = json_string_value(matchJ);
			if (family < 0 || match.empty())
				continue;
			PaletteRule rule;
			rule.match = string::uppercase(match);
			rule.family = family;
			paletteRules.push_back(rule);
		}
	}

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
	json_decref(rootJ);
	paletteGen++;
}

static void paletteSave() {
	json_t* rootJ = json_object();
	for (int i = 0; i < NUM_FAMILIES; i++)
		json_object_set_new(rootJ, PAL_KEY[i], json_string(toHex(palette[i]).c_str()));

	// Written back whether or not this run changed them, so that saving a colour cannot lose
	// rules the user typed in by hand.
	json_t* rulesJ = json_array();
	for (const PaletteRule& rule : paletteRules) {
		json_t* ruleJ = json_object();
		json_object_set_new(ruleJ, "match", json_string(rule.match.c_str()));
		json_object_set_new(ruleJ, "family", json_string(PAL_KEY[rule.family]));
		json_array_append_new(rulesJ, ruleJ);
	}
	json_object_set_new(rootJ, "rules", rulesJ);

	json_t* portsJ = json_object();
	for (const auto& pair : paletteOverrides)
		json_object_set_new(portsJ, pair.first.c_str(), json_string(PAL_KEY[pair.second]));
	json_object_set_new(rootJ, "ports", portsJ);

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
	if (family < 0 || family >= NUM_FAMILIES)
		return palette[FAM_AUDIO];
	return palette[family];
}

/** The built-in guess. Rack has no concept of signal family, but it does expose port names,
and guessing from the name is what makes the colour code work on plugins nobody has described
by hand. */
static int paletteGuess(const std::string& name) {
	const std::string n = string::uppercase(name);
	// FIRST, and it has to be. An MPX port is called something like "MPX note in", and the
	// pitch test below would claim it on the word NOTE — which is how a cable carrying a whole
	// instrument came out green.
	if (n.find("MPX") != std::string::npos)
		return FAM_MPX;
	if (n.find("V/OCT") != std::string::npos || n.find("PITCH") != std::string::npos
		|| n.find("NOTE") != std::string::npos)
		return FAM_PITCH;
	if (n.find("GATE") != std::string::npos || n.find("TRIG") != std::string::npos
		|| n.find("CLOCK") != std::string::npos || n.find("CLK") != std::string::npos
		|| n.find("RESET") != std::string::npos || n.find("SYNC") != std::string::npos)
		return FAM_TRIGGER;
	if (n.find("CV") != std::string::npos || n.find("MOD") != std::string::npos
		|| n.find("FM") != std::string::npos)
		return FAM_CV;
	return FAM_AUDIO;
}

int paletteFamilyForName(const std::string& name) {
	if (!paletteLoaded)
		paletteLoad();
	const std::string n = string::uppercase(name);
	for (const PaletteRule& rule : paletteRules) {
		if (n.find(rule.match) != std::string::npos)
			return rule.family;
	}
	return paletteGuess(name);
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
	std::string name;
	if (port) {
		if (engine::PortInfo* info = port->getPortInfo())
			name = info->getName();
	}
	return paletteFamilyForName(name);
}


/** THE SCHEMES. Asked for by Ohmer, who named the one that is now the default. */
static const PaletteScheme PAL_SCHEMES[] = {
	{"omri", "Omri Cohen (default)"},
	{"dreamer", "Dreamer Development"},
	{NULL, NULL},
};

const PaletteScheme* paletteSchemes() {
	return PAL_SCHEMES;
}

void paletteApplyScheme(const char* key) {
	if (!paletteLoaded)
		paletteLoad();
	const NVGcolor* from = (key && std::strcmp(key, "dreamer") == 0) ? PAL_DREAMER : PAL_DEFAULT;
	for (int i = 0; i < NUM_FAMILIES; i++)
		palette[i] = from[i];
	paletteGen++;
	paletteSave();
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
	math::Rect btnDefaults, btnCancel, btnDone;
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
