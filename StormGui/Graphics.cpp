#include "stdafx.h"
#include "Graphics.h"
#include "Painter.h"

namespace stormgui {

	Graphics::Graphics(ID2D1RenderTarget *target, Painter *p) : target(target), owner(p) {}

	Graphics::~Graphics() {}

	Size Graphics::size() {
		return convert(target->GetSize());
	}

	void Graphics::updateTarget(ID2D1RenderTarget *target) {
		this->target = target;
	}

	void Graphics::destroyed() {
		target = null;
		owner = null;
	}

	/**
	 * Draw stuff.
	 */

	void Graphics::line(Point from, Point to, Par<Brush> style) {
		target->DrawLine(dx(from), dx(to), style->brush(owner, abs(to - from)));
	}

	void Graphics::rect(Rect rect, Par<Brush> style) {
		target->DrawRectangle(dx(rect), style->brush(owner, rect.size()));
	}

	void Graphics::rect(Rect rect, Size edges, Par<Brush> style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->DrawRoundedRectangle(r, style->brush(owner, rect.size()));
	}

	void Graphics::oval(Rect rect, Par<Brush> style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		target->DrawEllipse(e, style->brush(owner, rect.size()));
	}

	void Graphics::fillRect(Rect rect, Par<Brush> style) {
		target->FillRectangle(dx(rect), style->brush(owner, rect.size()));
	}

	void Graphics::fillRect(Rect rect, Size edges, Par<Brush> style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->FillRoundedRectangle(r, style->brush(owner, rect.size()));
	}

	void Graphics::fillOval(Rect rect, Par<Brush> style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		target->FillEllipse(e, style->brush(owner, rect.size()));
	}

	void Graphics::draw(Par<Bitmap> bitmap, Point topLeft) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()));
	}

	void Graphics::draw(Par<Bitmap> bitmap, Rect rect) {
		target->DrawBitmap(bitmap->bitmap(owner), &dx(rect), 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	}

}
