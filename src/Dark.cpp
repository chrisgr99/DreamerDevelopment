/** Dark panels — see Dark.hpp for what this is and why it is done in memory. */
#include "Dark.hpp"

#include <app/SvgPanel.hpp>
#include <app/PortWidget.hpp>
#include <settings.hpp>
#include <window/Svg.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>


// ---- what darkening a family means -----------------------------------------------------------

/** IN CODE, NOT IN A FILE. There is no menu and no settings window: the numbers are here, and
changing one is a build rather than a click. That is the right trade for something one person
uses — a setting is a decision deferred, and these decisions have all been taken. */
struct DarkRule {
	/** Above this lightness a colour is background. */
	float lightAbove = 0.85f;
	/** Below it, lettering. */
	float darkBelow = 0.15f;
	/** No more than this much of the panel, as a percentage, is lettering. Measured on NYSTHI's
	Serge Programmer, where the lettering is 0.039 per cent of the panel and the four channel
	jacks are 0.117, so 0.08 divides them cleanly. */
	float lettering = 0.08f;
	/** At least this much of the panel has to be near-white for the drawing to be treated as a
	light panel at all. Stoermelder's are the case this exists for: a mid-toned ground that would
	otherwise have kept its colour while its lettering went light — worse than doing nothing. */
	float mustBeLight = 15.f;
	NVGcolor ground = nvgRGB(0x14, 0x14, 0x14);
	NVGcolor ink = nvgRGB(0xd8, 0xd8, 0xd8);

	/** WRITE THE MODULE'S NAME OVER THE TOP OF THE PANEL.

	Rack's SVG renderer has no text support at all — nanosvg drops every <text> element — so a
	maker who wants a title either draws it as outlines, which darkening handles like any other
	shape, or draws it in code in a colour of their own. NYSTHI does the second, and their black
	title survives on a panel that is now black.

	Nothing can be done to their drawing. What can be done is to draw the name again on top, in
	a colour that reads. Off unless a family is known to need it. */
	bool overdrawTitle = false;
	float titleHeight = 24.f;
	float titleSize = 13.f;

	/** THE WHITE PATCH BEHIND THE OUTPUT JACKS.

	VCV's own panels mark their outputs with a filled patch — black on the light artwork, white
	on the dark. On a dark panel that patch is the brightest thing on the screen, and it is
	telling you something Clarity already says with colour and with the ring it draws round an
	output.

	So for those families the panel is not darkened at all: only the patch is, along with the
	lettering sitting on it. Kept apart from ordinary darkening because it is the opposite
	operation — everything else here works on panels that are light all over. */
	bool outputPatches = false;
	/** A patch is smaller than this much of the panel. Bigger than this and it is the ground,
	which on the light artwork contains every jack there is and must not be repainted. */
	float patchMax = 40.f;
	/** WHAT THE PATCH BECOMES: a middle grey rather than the ground colour.

	Taking it all the way down would hide it, and the patch is saying something worth keeping —
	these jacks are the outputs. A middle grey leaves that visible as a shape on the panel while
	taking away the glare. The lettering on it still goes light, which is what makes it readable
	against this: dark lettering on a grey this deep is about two to one, and light lettering on
	it is about five. */
	NVGcolor patch = nvgRGB(0x5a, 0x5a, 0x5a);
};

/** The rule for a maker. Everything uses the defaults except where a family has earned a line
of its own here. */
static DarkRule darkRuleFor(const std::string& pluginSlug) {
	DarkRule rule;
	if (pluginSlug == "NYSTHI") {
		// Their titles are drawn in code, in black, and survive the darkening.
		rule.overdrawTitle = true;
	}
	// VCV'S OWN, INCLUDING CORE AND FUNDAMENTAL. Their dark panels are good; it is only the
	// output patch that dazzles.
	if (pluginSlug == "Core" || pluginSlug == "Fundamental"
			|| pluginSlug.compare(0, 3, "VCV") == 0) {
		rule.outputPatches = true;
	}
	return rule;
}

/** Whether a maker is left alone entirely. Ours are: every panel we draw is dark already. */
static bool darkSkips(const std::string& pluginSlug) {
	return pluginSlug.compare(0, 7, "Dreamer") == 0;
}


// ---- what was there before -------------------------------------------------------------------

/** ONE DRAWING, MANY MODULES. Rack parses a panel once and hands the same NSVGimage to every
instance of that model, so what is remembered is per drawing rather than per module: the paints
of every shape, in order, exactly as they were found. Putting a panel back is writing this
straight back over it. */
struct DarkOriginal {
	struct Paint {
		char fillType = 0, strokeType = 0;
		unsigned int fill = 0, stroke = 0;
	};
	std::vector<Paint> shapes;
};

static std::map<NSVGimage*, DarkOriginal> gOriginals;
/** Drawings looked at and found not to be light panels, so they are not looked at twice. */
static std::map<NSVGimage*, bool> gSkipped;
/** Set when something has changed and the rack wants walking again. */
static bool gDirty = true;
static bool gWasEnabled = false;
/** How many modules were in the rack last time, so an added module is noticed without a walk. */
static size_t gLastCount = 0;


// ---- colour ----------------------------------------------------------------------------------

/** nanosvg packs a colour as 0xAABBGGRR. */
static float lightnessOf(unsigned int c) {
	const float r = (c & 0xff) / 255.f;
	const float g = ((c >> 8) & 0xff) / 255.f;
	const float b = ((c >> 16) & 0xff) / 255.f;
	// Lightness as HSL means it: the midpoint of the brightest and darkest channels. A saturated
	// red and a mid grey come out the same, which is what we want — neither is background.
	const float hi = std::max(r, std::max(g, b));
	const float lo = std::min(r, std::min(g, b));
	return (hi + lo) * 0.5f;
}

static unsigned int packed(NVGcolor c, unsigned int alphaFrom) {
	return (alphaFrom & 0xff000000u)
		| ((unsigned int) (c.b * 255.f) << 16)
		| ((unsigned int) (c.g * 255.f) << 8)
		| ((unsigned int) (c.r * 255.f));
}


// ---- writing the name back -------------------------------------------------------------------

/** THE MODULE'S NAME, DRAWN OVER WHATEVER THE MAKER DREW.

Added as the LAST child of the module widget, so it draws after everything the module draws
itself — the only way to get over a title painted in code. It covers a band across the top with
the new ground colour and writes the name on it.

Approximate by design. The maker's own title is at a position and size chosen in their code and
we cannot ask what either is, so this is a band and a centred name: the same information, not
the same drawing. */
struct DarkTitle : widget::Widget {
	std::string name;
	/** Whether the panel under it is dark at this moment. */
	bool showing = false;
	DarkRule rule;

	void draw(const DrawArgs& args) override {
		if (!showing || name.empty() || box.size.x <= 0.f)
			return;
		const float h = rule.titleHeight;
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.f, 0.f, box.size.x, h);
		nvgFillColor(args.vg, rule.ground);
		nvgFill(args.vg);

		std::shared_ptr<window::Font> font =
			APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		if (!font || !font->handle)
			return;
		nvgFontFaceId(args.vg, font->handle);
		// SHRUNK TO FIT RATHER THAN CLIPPED. A long name on a narrow panel is the ordinary
		// case, and a name cut in half says less than a small one that fits.
		float size = rule.titleSize;
		float bounds[4];
		for (int i = 0; i < 8; i++) {
			nvgFontSize(args.vg, size);
			nvgTextBounds(args.vg, 0.f, 0.f, name.c_str(), NULL, bounds);
			if (bounds[2] - bounds[0] <= box.size.x - 6.f || size <= 5.f)
				break;
			size -= 1.f;
		}
		nvgFontSize(args.vg, size);
		nvgFillColor(args.vg, rule.ink);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, box.size.x / 2.f, h / 2.f, name.c_str(), NULL);
	}
};

static DarkTitle* titleOf(app::ModuleWidget* mw, bool make) {
	for (widget::Widget* child : mw->children) {
		if (DarkTitle* t = dynamic_cast<DarkTitle*>(child))
			return t;
	}
	if (!make)
		return NULL;
	DarkTitle* t = new DarkTitle;
	t->box.pos = math::Vec(0.f, 0.f);
	t->box.size = mw->box.size;
	t->name = mw->model ? mw->model->name : "";
	mw->addChild(t);
	return t;
}

/** Takes them all away. A widget added to somebody else's module must not outlive the reason for
it, and the module is not ours to leave things on. */
static void darkTitlesOff() {
	if (!APP->scene || !APP->scene->rack)
		return;
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		if (DarkTitle* t = titleOf(mw, false)) {
			mw->removeChild(t);
			delete t;
		}
	}
}


// ---- doing it --------------------------------------------------------------------------------

/** Every drawing a module lays down as its face.

USUALLY ONE, AND NOT ALWAYS. A panel is normally a single SvgPanel, but a maker is free to add a
second layer over it, and one of those left light on a darkened panel is worse than not
darkening at all.

ONLY THE MODULE'S OWN CHILDREN, deliberately. Knobs and jacks are SVG widgets too, nested inside
their param and port widgets, and those drawings are shared with every module in the rack:
repainting one would repaint the whole rack. */
static void facesOf(app::ModuleWidget* mw, std::vector<widget::SvgWidget*>& out,
		std::vector<app::SvgPanel*>& panels) {
	if (!mw)
		return;
	for (widget::Widget* child : mw->children) {
		if (app::SvgPanel* panel = dynamic_cast<app::SvgPanel*>(child)) {
			panels.push_back(panel);
			if (panel->sw)
				out.push_back(panel->sw);
			continue;
		}
		if (widget::SvgWidget* sw = dynamic_cast<widget::SvgWidget*>(child)) {
			out.push_back(sw);
			continue;
		}
		// A LAYER IN A FRAMEBUFFER. Anything drawn once and cached is wrapped in one of these,
		// so the SVG inside it is a child of the framebuffer rather than of the module.
		if (widget::FramebufferWidget* fb = dynamic_cast<widget::FramebufferWidget*>(child)) {
			for (widget::Widget* inner : fb->children) {
				if (widget::SvgWidget* sw = dynamic_cast<widget::SvgWidget*>(inner))
					out.push_back(sw);
			}
		}
	}
}

/** Redraws every module whose face is this drawing. A framebuffer holds what it last drew, so a
repainted SVG changes nothing on the screen until it is told. */
static void darkRedraw(NSVGimage* image) {
	if (!APP->scene || !APP->scene->rack)
		return;
	for (app::ModuleWidget* mw : APP->scene->rack->getModules()) {
		std::vector<widget::SvgWidget*> faces;
		std::vector<app::SvgPanel*> panels;
		facesOf(mw, faces, panels);
		bool ours = false;
		for (size_t i = 0; i < faces.size(); i++)
			if (faces[i]->svg && faces[i]->svg->handle == image)
				ours = true;
		if (!ours)
			continue;
		for (size_t i = 0; i < panels.size(); i++)
			if (panels[i]->fb)
				panels[i]->fb->setDirty();
		// A layer that is not in a panel draws every frame anyway, so there is nothing to mark.
	}
}

/** Whether this drawing is a light panel: enough near-white area to be a white ground rather
than a pale detail on something darker. */
static bool darkIsLightPanel(NSVGimage* image, const DarkRule& rule) {
	const float panelArea = image->width * image->height;
	if (panelArea <= 0.f)
		return false;
	float white = 0.f;
	for (NSVGshape* shape = image->shapes; shape; shape = shape->next) {
		if (shape->fill.type != NSVG_PAINT_COLOR)
			continue;
		if (lightnessOf(shape->fill.color) < rule.lightAbove)
			continue;
		const float w = shape->bounds[2] - shape->bounds[0];
		const float h = shape->bounds[3] - shape->bounds[1];
		white = std::max(white, 100.f * (w * h) / panelArea);
	}
	return white >= rule.mustBeLight;
}

/** THE OUTPUT PATCH, AND WHAT IS WRITTEN ON IT.

Found geometrically rather than by colour: a shape that contains the centre of at least one
output jack, no input jacks, and is small enough not to be the panel itself. Colour is no use
for finding it because VCV draw it both ways round — black on their light artwork, white on
their dark. The lettering on it is then whatever sits inside its bounds.

AND THE PANEL ITSELF IS DARKENED TOO, where the drawing is a light panel. Several of VCV's own
modules ship no dark artwork at all — Host XL among them — so on those this does the ordinary
darkening first and then treats the patch. On the ones that do ship dark artwork the panel fails
the light test and only the patch is touched, which is what was wanted there.

The bounds nanosvg reports are in the same pixels the module widget lays its jacks out in, so
the two can be compared directly with nothing to convert. */
static void darkPaintPatches(app::ModuleWidget* mw, NSVGimage* image, const DarkRule& rule) {
	if (!image || gOriginals.count(image) || gSkipped.count(image))
		return;
	const float panelArea = image->width * image->height;
	if (panelArea <= 0.f)
		return;

	std::vector<math::Vec> outs, ins;
	for (app::PortWidget* p : mw->getOutputs())
		outs.push_back(p->box.getCenter());
	for (app::PortWidget* p : mw->getInputs())
		ins.push_back(p->box.getCenter());

	// Where the panel is light all over, it is darkened as any other light panel would be, and
	// the patch is then dealt with on top of that.
	const bool lightPanel = darkIsLightPanel(image, rule);

	// PASS ONE: the patches, by where they are rather than by what colour they are.
	std::vector<math::Rect> patches;
	std::set<NSVGshape*> patchShapes;
	for (NSVGshape* shape = image->shapes; shape; shape = shape->next) {
		if (shape->fill.type != NSVG_PAINT_COLOR)
			continue;
		const math::Rect r = math::Rect(math::Vec(shape->bounds[0], shape->bounds[1]),
			math::Vec(shape->bounds[2] - shape->bounds[0], shape->bounds[3] - shape->bounds[1]));
		const float part = 100.f * r.size.x * r.size.y / panelArea;
		// Big enough to be a patch rather than a glyph, small enough not to be the ground.
		if (part > rule.patchMax || part < rule.lettering)
			continue;
		bool hasOut = false, hasIn = false;
		for (size_t i = 0; i < outs.size(); i++)
			if (r.contains(outs[i]))
				hasOut = true;
		for (size_t i = 0; i < ins.size(); i++)
			if (r.contains(ins[i]))
				hasIn = true;
		if (hasOut && !hasIn) {
			patches.push_back(r);
			patchShapes.insert(shape);
		}
	}
	if (patches.empty() && !lightPanel) {
		gSkipped[image] = true;
		return;
	}

	// PASS TWO: the panel, then the patches over the top of it.
	DarkOriginal original;
	bool changed = false;
	for (NSVGshape* shape = image->shapes; shape; shape = shape->next) {
		DarkOriginal::Paint was;
		was.fillType = shape->fill.type;
		was.strokeType = shape->stroke.type;
		was.fill = shape->fill.color;
		was.stroke = shape->stroke.color;
		original.shapes.push_back(was);

		if (shape->fill.type != NSVG_PAINT_COLOR)
			continue;
		const math::Rect r = math::Rect(math::Vec(shape->bounds[0], shape->bounds[1]),
			math::Vec(shape->bounds[2] - shape->bounds[0], shape->bounds[3] - shape->bounds[1]));
		const float part = 100.f * r.size.x * r.size.y / panelArea;
		const float L = lightnessOf(shape->fill.color);

		// THE PATCH COUNTS AS BEING INSIDE ITSELF, which has to be said outright: Rack's
		// Rect::contains is half-open, so a rectangle does not contain its own bottom-right
		// corner and the patch failed the test that was written for the lettering on it.
		const bool isPatch = patchShapes.count(shape) > 0;
		bool inside = false;
		for (size_t i = 0; i < patches.size() && !inside; i++)
			if (patches[i].contains(r.pos) && patches[i].contains(r.getBottomRight()))
				inside = true;

		if (isPatch) {
			shape->fill.color = packed(rule.patch, shape->fill.color);
			changed = true;
			continue;
		}
		if (inside) {
			// Whatever is written on a patch ends up light, whichever way round it started.
			shape->fill.color = packed(rule.ink, shape->fill.color);
			changed = true;
			continue;
		}
		if (!lightPanel)
			continue;
		if (L >= rule.lightAbove) {
			shape->fill.color = packed(rule.ground, shape->fill.color);
			changed = true;
		}
		else if (L <= rule.darkBelow && part <= rule.lettering) {
			shape->fill.color = packed(rule.ink, shape->fill.color);
			changed = true;
		}
		if (shape->stroke.type == NSVG_PAINT_COLOR) {
			const float S = lightnessOf(shape->stroke.color);
			if (S >= rule.lightAbove) {
				shape->stroke.color = packed(rule.ground, shape->stroke.color);
				changed = true;
			}
			else if (S <= rule.darkBelow) {
				shape->stroke.color = packed(rule.ink, shape->stroke.color);
				changed = true;
			}
		}
	}
	if (!changed) {
		gSkipped[image] = true;
		return;
	}
	gOriginals[image] = original;
	darkRedraw(image);
}


/** Repaints one drawing, remembering what it was. Does nothing to one already done. */
static void darkPaint(NSVGimage* image, const DarkRule& rule) {
	if (!image || gOriginals.count(image) || gSkipped.count(image))
		return;
	const float panelArea = image->width * image->height;
	if (panelArea <= 0.f)
		return;
	if (!darkIsLightPanel(image, rule)) {
		gSkipped[image] = true;
		return;
	}

	DarkOriginal original;
	bool changed = false;
	for (NSVGshape* shape = image->shapes; shape; shape = shape->next) {
		DarkOriginal::Paint was;
		was.fillType = shape->fill.type;
		was.strokeType = shape->stroke.type;
		was.fill = shape->fill.color;
		was.stroke = shape->stroke.color;
		original.shapes.push_back(was);

		// THE SHAPE'S OWN SIZE, as a percentage of the panel. nanosvg has already worked out
		// the bounds in final coordinates, so there is no path to walk and no transform to
		// compose.
		const float w = shape->bounds[2] - shape->bounds[0];
		const float h = shape->bounds[3] - shape->bounds[1];
		const float part = 100.f * (w * h) / panelArea;

		// A GRADIENT IS LEFT ALONE. It is somebody drawing something deliberate, and repainting
		// one end of it would be worse than leaving it light.
		//
		// SIZE IS TESTED ONLY GOING DARK TO LIGHT: every white shape is background or a mask,
		// however small. Testing it on the white side too left NYSTHI's Simpliciter with light
		// tabs behind its section headings.
		if (shape->fill.type == NSVG_PAINT_COLOR) {
			const float L = lightnessOf(shape->fill.color);
			if (L >= rule.lightAbove) {
				shape->fill.color = packed(rule.ground, shape->fill.color);
				changed = true;
			}
			else if (L <= rule.darkBelow && part <= rule.lettering) {
				shape->fill.color = packed(rule.ink, shape->fill.color);
				changed = true;
			}
		}
		// A STROKE IS A LINE, and a line is lettering-sized whatever it encloses — an outline
		// round the whole panel is still a hairline. So strokes are judged on colour alone.
		if (shape->stroke.type == NSVG_PAINT_COLOR) {
			const float L = lightnessOf(shape->stroke.color);
			if (L >= rule.lightAbove) {
				shape->stroke.color = packed(rule.ground, shape->stroke.color);
				changed = true;
			}
			else if (L <= rule.darkBelow) {
				shape->stroke.color = packed(rule.ink, shape->stroke.color);
				changed = true;
			}
		}
	}
	if (!changed) {
		gSkipped[image] = true;
		return;
	}
	gOriginals[image] = original;
	darkRedraw(image);
}

void darkRestoreAll() {
	darkTitlesOff();
	for (std::map<NSVGimage*, DarkOriginal>::iterator it = gOriginals.begin();
			it != gOriginals.end(); ++it) {
		NSVGimage* image = it->first;
		const DarkOriginal& original = it->second;
		size_t i = 0;
		for (NSVGshape* shape = image->shapes; shape && i < original.shapes.size();
				shape = shape->next, i++) {
			shape->fill.type = original.shapes[i].fillType;
			shape->stroke.type = original.shapes[i].strokeType;
			shape->fill.color = original.shapes[i].fill;
			shape->stroke.color = original.shapes[i].stroke;
		}
		darkRedraw(image);
	}
	gOriginals.clear();
	gSkipped.clear();
	gLastCount = 0;
}

void darkStep(bool enabled) {
	if (!APP->scene || !APP->scene->rack)
		return;
	if (enabled != gWasEnabled) {
		gWasEnabled = enabled;
		gDirty = true;
		if (!enabled) {
			darkRestoreAll();
			return;
		}
	}
	if (!enabled)
		return;
	// CHEAP WHEN NOTHING HAS HAPPENED, which is almost every frame. A module arriving or leaving
	// changes the count; the switch moving sets the flag. Neither happens while a patch merely
	// plays, so the ordinary cost of this is one comparison.
	// RACK'S OWN LIGHT AND DARK SWITCH swaps which drawing a themed panel is showing, so the
	// other one has never been looked at. Noticed here rather than being missed until a module
	// happens to be added.
	static bool wasPreferDark = settings::preferDarkPanels;
	if (settings::preferDarkPanels != wasPreferDark) {
		wasPreferDark = settings::preferDarkPanels;
		gDirty = true;
	}
	const std::vector<app::ModuleWidget*> modules = APP->scene->rack->getModules();
	if (!gDirty && modules.size() == gLastCount)
		return;
	gDirty = false;
	gLastCount = modules.size();

	for (app::ModuleWidget* mw : modules) {
		if (!mw->model || !mw->model->plugin)
			continue;
		const std::string slug = mw->model->plugin->slug;
		if (darkSkips(slug))
			continue;
		std::vector<widget::SvgWidget*> faces;
		std::vector<app::SvgPanel*> panels;
		facesOf(mw, faces, panels);
		const DarkRule rule = darkRuleFor(slug);
		bool dark = false;
		for (size_t i = 0; i < faces.size(); i++) {
			if (!faces[i]->svg)
				continue;
			if (rule.outputPatches)
				darkPaintPatches(mw, faces[i]->svg->handle, rule);
			else
				darkPaint(faces[i]->svg->handle, rule);
			if (gOriginals.count(faces[i]->svg->handle))
				dark = true;
		}
		// THE TITLE ONLY WHERE THE PANEL ACTUALLY WENT DARK. A module whose drawing was left
		// alone must not get a band across the top of it.
		if (rule.overdrawTitle && dark) {
			DarkTitle* title = titleOf(mw, true);
			title->rule = rule;
			title->box.size = mw->box.size;
			title->showing = true;
		}
	}
}
