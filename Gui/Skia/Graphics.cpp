#include "stdafx.h"
#include "Graphics.h"
#include "Device.h"
#include "Manager.h"

namespace gui {

	SkiaGraphics::SkiaGraphics(SkiaSurface &surface, Nat id) : surface(surface) {
		identifier = id;
		states = new (this) Array<State>();

#ifdef GUI_GTK
		states->push(State());
		manager(new (this) SkiaManager(this, surface));
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

	void SkiaGraphics::pushState() {
		states->push(State(surface.canvas->getTotalMatrix(), lineW));
	}

	void SkiaGraphics::push() {
		surface.canvas->save();
		pushState();
	}

	void SkiaGraphics::push(Float opacity) {
		if (opacity >= 1.0f) {
			surface.canvas->save();
		} else {
			// TODO: Hints for the layer size according to clip? Skia probably does that anyway...
			SkPaint paint;
			paint.setAlphaf(opacity);
			paint.setAntiAlias(true);
			surface.canvas->saveLayer(nullptr, &paint);
		}
		pushState();
	}

	void SkiaGraphics::push(Rect clip) {
		surface.canvas->save();
		pushState();

		surface.canvas->clipRect(skia(clip), SkClipOp::kIntersect, true);
	}

	void SkiaGraphics::push(Rect clip, Float opacity) {
		// TODO: Is the layer size suggestion in device units (I guess so) or in transformed units?
		SkRect rect = skia(clip);
		SkPaint paint;
		paint.setAlphaf(opacity);
		paint.setAntiAlias(true);
		surface.canvas->saveLayer(&rect, &paint);
		pushState();

		surface.canvas->clipRect(skia(clip), SkClipOp::kIntersect, true);
	}

	Bool SkiaGraphics::pop() {
		if (states->count() <= 1)
			return false;

		states->pop();
		surface.canvas->restore();

		return true;
	}

	void SkiaGraphics::transform(Transform *tfm) {
		SkMatrix m = states->last().matrix();
		m.preConcat(skia(tfm));
		surface.canvas->setMatrix(m);
	}

	void SkiaGraphics::lineWidth(Float w) {
		lineW = states->last().lineWidth * w;
	}


	/**
	 * Draw stuff.
	 */

	SkPaint *SkiaGraphics::paint(Brush *style, Bool stroke) {
		SkPaint *paint = (SkPaint *)style->forGraphicsRaw(this);
		paint->setStroke(stroke);
		paint->setStrokeWidth(lineW);
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
		SkPath *p = (SkPath *)path->forGraphicsRaw(this);
		surface.canvas->drawPath(*p, *paint(style, true));
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
		SkPath *p = (SkPath *)path->forGraphicsRaw(this);
		surface.canvas->drawPath(*p, *paint(style, false));
	}

	void SkiaGraphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		SkPaint paint;
		paint.setAntiAlias(true);
		paint.setAlphaf(opacity);

		SkImage *b = (SkImage *)bitmap->forGraphicsRaw(this);
		surface.canvas->drawImageRect(b, skia(rect), &paint);
	}

	void SkiaGraphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		SkPaint paint;
		paint.setAntiAlias(true);
		paint.setAlphaf(opacity);

		// TODO: The fast version might not be appropriate here. Consider a tightly packed sprite sheet.
		SkImage *b = (SkImage *)bitmap->forGraphicsRaw(this);
		surface.canvas->drawImageRect(b, skia(src), skia(dest), &paint, SkCanvas::kFast_SrcRectConstraint);
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
