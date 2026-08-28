/** Pinch to zoom, with a snapshot standing in for the rack during the gesture.

WHY A SNAPSHOT. Changing Rack's zoom marks every visible panel's framebuffer dirty, so each
panel re-rasterises on every frame of a pinch. In a modified Rack that invalidation can be
suppressed; a plugin cannot reach it. So the zoom is NOT changed during the gesture at all.
Instead the rack is photographed once, that picture is scaled while you pinch, and the real
zoom is applied once when you let go. Nothing re-rasterises until the end, so the cost
disappears rather than being hidden.

The picture goes soft as you zoom in and sharpens the moment you release, which is the same
trade a bitmap always offers: it scales for free.
*/
#include "plugin.hpp"
#include "pinch.hpp"

using namespace rack;


/** How long without a magnify event counts as the gesture being over. There is no
gesture-end event to rely on, so idleness is the only signal available. */
static const double PINCH_END_IDLE = 0.15;


struct PinchZoomOverlay : widget::TransparentWidget {
	bool* enabled = NULL;

	bool active = false;
	/** Accumulated scale for this gesture. The real zoom is untouched until it ends. */
	float factor = 1.f;
	/** Pointer position when the gesture began, in the rack scroller's coordinates. */
	math::Vec pivot;
	/** Taken in draw(), because the frame's pixels only exist during drawing. */
	bool captureRequested = false;
	int image = -1;
	math::Rect captureRect;

	PinchZoomOverlay() {
		box.pos = math::Vec();
		box.size = math::Vec(1e5, 1e5);
	}

	~PinchZoomOverlay() {
		release();
	}

	void release() {
		if (image >= 0 && APP && APP->window && APP->window->vg)
			nvgDeleteImage(APP->window->vg, image);
		image = -1;
	}

	void finish() {
		if (!active)
			return;
		active = false;
		release();

		// The one and only zoom change of the whole gesture.
		RackScrollWidget* scroll = APP->scene->rackScroll;
		if (scroll && factor != 1.f)
			scroll->setZoom(scroll->getZoom() * factor, pivot);
		factor = 1.f;
	}

	void step() override {
		if (!enabled || !*enabled) {
			finish();
			widget::TransparentWidget::step();
			return;
		}
		drui::pinchInit();

		const float mag = drui::pinchTake();
		if (mag != 0.f) {
			if (!active) {
				active = true;
				factor = 1.f;
				captureRequested = true;
				RackScrollWidget* scroll = APP->scene->rackScroll;
				pivot = scroll ? APP->scene->getMousePos().minus(scroll->box.pos) : math::Vec();
			}
			// Apple's convention: the delta is applied as a multiplier of one plus it.
			factor *= (1.f + mag);
			factor = math::clamp(factor, 0.1f, 10.f);
		}
		else if (active && drui::pinchIdleTime() > PINCH_END_IDLE) {
			finish();
		}

		widget::TransparentWidget::step();
	}

	/** Photographs the rack area out of the frame that has already been drawn beneath us. */
	void capture(const DrawArgs& args) {
		captureRequested = false;
		RackScrollWidget* scroll = APP->scene->rackScroll;
		if (!scroll)
			return;

		const float ratio = APP->window->pixelRatio;
		const math::Vec winSize = APP->window->getSize();
		captureRect = math::Rect(scroll->box.pos, scroll->box.size);

		const int px = (int) (captureRect.pos.x * ratio);
		const int pw = (int) (captureRect.size.x * ratio);
		const int ph = (int) (captureRect.size.y * ratio);
		// OpenGL's origin is bottom-left, the widget tree's is top-left.
		const int py = (int) ((winSize.y - captureRect.pos.y - captureRect.size.y) * ratio);
		if (pw <= 0 || ph <= 0)
			return;

		std::vector<uint8_t> pixels((size_t) pw * ph * 4);
		glReadPixels(px, py, pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		release();
		// FLIPY because glReadPixels returns rows bottom-up.
		image = nvgCreateImageRGBA(args.vg, pw, ph, NVG_IMAGE_FLIPY, pixels.data());
	}

	void draw(const DrawArgs& args) override {
		if (!active) {
			widget::TransparentWidget::draw(args);
			return;
		}
		if (captureRequested)
			capture(args);
		if (image < 0)
			return;

		// The picture, scaled about the pivot, confined to the rack area so the menu bar is
		// never covered.
		const math::Vec p = captureRect.pos.plus(pivot);
		const math::Rect dst = math::Rect(
			p.plus(captureRect.pos.minus(p).mult(factor)),
			captureRect.size.mult(factor));

		nvgSave(args.vg);
		nvgScissor(args.vg, captureRect.pos.x, captureRect.pos.y,
			captureRect.size.x, captureRect.size.y);

		// A ground colour, so zooming out does not reveal whatever was behind.
		nvgBeginPath(args.vg);
		nvgRect(args.vg, captureRect.pos.x, captureRect.pos.y,
			captureRect.size.x, captureRect.size.y);
		nvgFillColor(args.vg, nvgRGB(0x17, 0x17, 0x17));
		nvgFill(args.vg);

		nvgBeginPath(args.vg);
		nvgRect(args.vg, dst.pos.x, dst.pos.y, dst.size.x, dst.size.y);
		nvgFillPaint(args.vg, nvgImagePattern(args.vg, dst.pos.x, dst.pos.y,
			dst.size.x, dst.size.y, 0.f, image, 1.f));
		nvgFill(args.vg);

		nvgRestore(args.vg);
		widget::TransparentWidget::draw(args);
	}
};


widget::Widget* createPinchZoomOverlay(bool* enabled) {
	PinchZoomOverlay* o = new PinchZoomOverlay;
	o->enabled = enabled;
	return o;
}
