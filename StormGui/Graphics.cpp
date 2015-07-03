#include "stdafx.h"
#include "Graphics.h"
#include "Painter.h"

namespace stormgui {

	Graphics::Graphics(ID2D1RenderTarget *target, Painter *p) : target(target), owner(p) {
		state = defaultState();
		oldStates.push_back(state);
	}

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
	 * State management.
	 */

	Graphics::State Graphics::defaultState() {
		State s = {
			dxUnit(),
			1.0f,
		};
		return s;
	}

	void Graphics::reset() {
		oldStates.resize(1);
		state = oldStates[0];
	}

	Bool Graphics::pop() {
		state = oldStates.back();
		target->SetTransform(state.transform);
		if (oldStates.size() > 1) {
			oldStates.pop_back();
			return true;
		} else {
			return false;
		}
	}

	void Graphics::push() {
		oldStates.push_back(state);
	}

	void Graphics::transform(Par<Transform> tfm) {
		state.transform = dxMultiply(dx(tfm), oldStates.back().transform);
		target->SetTransform(state.transform);
	}

	void Graphics::lineWidth(Float w) {
		state.lineWidth = oldStates.back().lineWidth * w;
	}

	/**
	 * Draw stuff.
	 */

	void Graphics::line(Point from, Point to, Par<Brush> style) {
		target->DrawLine(dx(from), dx(to), style->brush(owner, abs(to - from)), state.lineWidth);
	}

	void Graphics::rect(Rect rect, Par<Brush> style) {
		target->DrawRectangle(dx(rect), style->brush(owner, rect.size()), state.lineWidth);
	}

	void Graphics::rect(Rect rect, Size edges, Par<Brush> style) {
		D2D1_ROUNDED_RECT r = { dx(rect), edges.w, edges.h };
		target->DrawRoundedRectangle(r, style->brush(owner, rect.size()), state.lineWidth);
	}

	void Graphics::oval(Rect rect, Par<Brush> style) {
		Size s = rect.size() / 2;
		D2D1_ELLIPSE e = { dx(rect.center()), s.w, s.h };
		target->DrawEllipse(e, style->brush(owner, rect.size()), state.lineWidth);
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

	void Graphics::draw(Par<Bitmap> bitmap) {
		draw(bitmap, Point());
	}

	void Graphics::draw(Par<Bitmap> bitmap, Point topLeft) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()));
	}

	void Graphics::draw(Par<Bitmap> bitmap, Rect rect) {
		target->DrawBitmap(bitmap->bitmap(owner), &dx(rect), 1, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);
	}

	void Graphics::text(Par<Str> text, Par<Font> font, Par<Brush> brush, Rect rect) {
		ID2D1Brush *b = brush->brush(owner, rect.size());
		target->DrawText(text->v.c_str(), text->v.size(), font->textFormat(), dx(rect), b);
	}

	void Graphics::draw(Par<Text> text, Par<Brush> brush, Point origin) {
		target->DrawTextLayout(dx(origin), text->layout(), brush->brush(owner, text->size()));
	}

}
