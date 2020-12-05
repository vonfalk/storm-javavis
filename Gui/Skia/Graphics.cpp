#include "stdafx.h"
#include "Graphics.h"
#include "Device.h"

namespace gui {

	SkiaGraphics::SkiaGraphics(SkiaSurface &surface, Nat id) : surface(surface) {
		identifier = id;

		// TODO: Set a manager at a later point.
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

	void SkiaGraphics::line(Point from, Point to, Brush *style) {
		SkPaint paint(SkColors::kBlue);
		paint.setAntiAlias(true);

		surface.canvas->drawLine(skia(from), skia(to), paint);
	}

	void SkiaGraphics::draw(Rect rect, Brush *style) {
		SkPaint paint(SkColors::kRed);
		paint.setAntiAlias(true);

		paint.setStroke(true);

		surface.canvas->drawRect(skia(rect), paint);
	}

	void SkiaGraphics::draw(Rect rect, Size edges, Brush *style) {
		SkPaint paint(SkColors::kRed);
		paint.setAntiAlias(true);

		paint.setStroke(true);

		surface.canvas->drawRRect(SkRRect::MakeRectXY(skia(rect), edges.w, edges.h), paint);
	}

	void SkiaGraphics::oval(Rect rect, Brush *style) {
		SkPaint paint(SkColors::kGreen);
		paint.setAntiAlias(true);

		paint.setStroke(true);

		surface.canvas->drawOval(skia(rect), paint);
	}

	void SkiaGraphics::draw(Path *path, Brush *style) {
		TODO(L"FIXME");
	}

	void SkiaGraphics::fill(Rect rect, Brush *style) {
		SkPaint paint(SkColors::kRed);
		paint.setAntiAlias(true);

		paint.setStroke(false);

		surface.canvas->drawRect(skia(rect), paint);
	}

	void SkiaGraphics::fill(Rect rect, Size edges, Brush *style) {
		SkPaint paint(SkColors::kRed);
		paint.setAntiAlias(true);

		paint.setStroke(false);

		surface.canvas->drawRRect(SkRRect::MakeRectXY(skia(rect), edges.w, edges.h), paint);
	}

	void SkiaGraphics::fill(Brush *style) {
		SkPaint paint(SkColors::kBlack);
		paint.setAntiAlias(true);

		paint.setStroke(false);

		surface.canvas->drawPaint(paint);
	}

	void SkiaGraphics::fillOval(Rect rect, Brush *style) {
		SkPaint paint(SkColors::kGreen);
		paint.setAntiAlias(true);

		paint.setStroke(false);

		surface.canvas->drawOval(skia(rect), paint);
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
