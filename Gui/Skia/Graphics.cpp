#include "stdafx.h"
#include "Graphics.h"
#include "Device.h"
#include "Manager.h"

namespace gui {

	SkiaGraphics::SkiaGraphics(SkiaSurface &surface, Nat id) : surface(surface) {
		identifier = id;

#ifdef GUI_GTK
		manager(new (this) SkiaManager());
#endif
	}

	SkiaGraphics::~SkiaGraphics() {}

#ifdef GUI_GTK

	void SkiaGraphics::surfaceResized() {
		// Not needed.
	}

	void SkiaGraphics::surfaceDestroyed() {
		// Not needed.
	}

	/**
	 * State management.
	 */

	void SkiaGraphics::beforeRender(Color bgColor) {
		SkPaint paint(skia(bgColor));
		paint.setStroke(false);
		surface.canvas->drawPaint(paint);
	}

	bool SkiaGraphics::afterRender() {
		// Make sure all layers are returned to the stack.
		reset();

		// Flush operations so that it is safe to present the contents of the texture.
		surface.surface->flushAndSubmit();

		return true;
	}


	/**
	 * State management.
	 */

	void SkiaGraphics::reset() {
		// Clear any remaining states from the stack.
		while (pop())
			;
	}

	void SkiaGraphics::push() {
		TODO(L"FIXME");
	}

	void SkiaGraphics::push(Float opacity) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::push(Rect clip) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::push(Rect clip, Float opacity) {
		TODO(L"FIXME");
	}

	Bool SkiaGraphics::pop() {
		return false;
	}

	void SkiaGraphics::transform(Transform *tfm) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::lineWidth(Float w) {
		TODO(L"FIXME");
	}


	/**
	 * Draw stuff.
	 */

	SkPaint *SkiaGraphics::paint(Brush *style, Bool stroke) {
		SkPaint *paint = (SkPaint *)style->forGraphicsRaw(this);
		paint->setStroke(stroke);
		return paint;
	}

	void SkiaGraphics::line(Point from, Point to, Brush *style) {
		surface.canvas->drawLine(skia(from), skia(to), *paint(style, true));
	}

	void SkiaGraphics::draw(Rect rect, Brush *style) {
		surface.canvas->drawRect(skia(rect), *paint(style, true));
	}

	void SkiaGraphics::draw(Rect rect, Size edges, Brush *style) {
		surface.canvas->drawRRect(SkRRect::MakeRectXY(skia(rect), edges.w, edges.h), *paint(style, true));
	}

	void SkiaGraphics::oval(Rect rect, Brush *style) {
		surface.canvas->drawOval(skia(rect), *paint(style, true));
	}

	void SkiaGraphics::draw(Path *path, Brush *style) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::fill(Rect rect, Brush *style) {
		surface.canvas->drawRect(skia(rect), *paint(style, false));
	}

	void SkiaGraphics::fill(Rect rect, Size edges, Brush *style) {
		surface.canvas->drawRRect(SkRRect::MakeRectXY(skia(rect), edges.w, edges.h), *paint(style, false));
	}

	void SkiaGraphics::fill(Brush *style) {
		surface.canvas->drawPaint(*paint(style, false));
	}

	void SkiaGraphics::fillOval(Rect rect, Brush *style) {
		surface.canvas->drawOval(skia(rect), *paint(style, false));
	}

	void SkiaGraphics::fill(Path *path, Brush *style) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::text(Str *text, Font *font, Brush *style, Rect rect) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::draw(Text *text, Brush *style, Point origin) {
		TODO(L"FIXME");
	}

#else

	DEFINE_WINDOW_GRAPHICS_FNS(SkiaGraphics)

#endif

}
