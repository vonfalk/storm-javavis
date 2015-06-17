#include "stdafx.h"
#include "Graphics.h"
#include "Painter.h"

namespace stormgui {

	Graphics::Graphics(ID2D1RenderTarget *target, Painter *p) : target(target), owner(p) {}

	Graphics::~Graphics() {}

	Size Graphics::size() {
		return convert(target->GetSize());
	}

	void Graphics::destroyed() {
		target = null;
		owner = null;
	}

	/**
	 * Draw stuff.
	 */

	void Graphics::line(Point from, Point to, Par<Brush> style) {
		target->DrawLine(dx(from), dx(to), style->brush(owner));
	}

	void Graphics::rect(Rect rect, Par<Brush> style) {
		target->DrawRectangle(dx(rect), style->brush(owner));
	}

	void Graphics::rect(Rect rect, Size edges, Par<Brush> style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->DrawRoundedRectangle(r, style->brush(owner));
	}

	void Graphics::fillRect(Rect rect, Par<Brush> style) {
		target->FillRectangle(dx(rect), style->brush(owner));
	}

	void Graphics::fillRect(Rect rect, Size edges, Par<Brush> style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->FillRoundedRectangle(r, style->brush(owner));
	}

}
