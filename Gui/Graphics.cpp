#include "stdafx.h"
#include "Graphics.h"
#include "Bitmap.h"
#include "Core/Exception.h"

namespace gui {

	Graphics::Graphics() {}

	void Graphics::reset() {
		throw NotSupported(L"Graphics::reset()");
	}

	void Graphics::push() {
		throw NotSupported(L"Graphics::push()");
	}

	void Graphics::push(Float opacity) {
		throw NotSupported(L"Graphics::push(Float)");
	}

	void Graphics::push(Rect clip) {
		throw NotSupported(L"Graphics::push(Rect)");
	}

	void Graphics::push(Rect clip, Float opacity) {
		throw NotSupported(L"Graphics::push(Rect, Float)");
	}

	Bool Graphics::pop() {
		throw NotSupported(L"Graphics::pop()");
	}

	void Graphics::transform(Transform *tfm) {
		throw NotSupported(L"Graphics::transform(Transform)");
	}

	void Graphics::lineWidth(Float w) {
		throw NotSupported(L"Graphics::lineWidth(Float)");
	}

	void Graphics::line(Point from, Point to, Brush *brush) {
		throw NotSupported(L"Graphics::line(Point, Point, Brush)");
	}

	void Graphics::draw(Rect rect, Brush *brush) {
		throw NotSupported(L"Graphics::draw(Rect, Brush)");
	}

	void Graphics::draw(Rect rect, Size edges, Brush *brush) {
		throw NotSupported(L"Graphics::draw(Rect, Size, Brush)");
	}

	void Graphics::oval(Rect rect, Brush *brush) {
		throw NotSupported(L"Graphics::oval(Rect, Brush)");
	}

	void Graphics::draw(Path *path, Brush *brush) {
		throw NotSupported(L"Graphics::draw(Path, Brush)");
	}

	void Graphics::fill(Rect rect, Brush *brush) {
		throw NotSupported(L"Graphics::fill(Rect, Brush)");
	}

	void Graphics::fill(Rect rect, Size edges, Brush *brush) {
		throw NotSupported(L"Graphics::fill(Rect, Size, Brush)");
	}

	void Graphics::fill(Brush *brush) {
		throw NotSupported(L"Graphics::fill(Brush)");
	}

	void Graphics::fill(Path *path, Brush *brush) {
		throw NotSupported(L"Graphics::fill(Path, Brush)");
	}

	void Graphics::fillOval(Rect rect, Brush *brush) {
		throw NotSupported(L"Graphics::fillOval(Rect, Brush)");
	}

	void Graphics::draw(Bitmap *bitmap) {
		draw(bitmap, Point());
	}

	void Graphics::draw(Bitmap *bitmap, Point topLeft) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()));
	}

	void Graphics::draw(Bitmap *bitmap, Point topLeft, Float opacity) {
		draw(bitmap, Rect(topLeft, topLeft + bitmap->size()), opacity);
	}

	void Graphics::draw(Bitmap *bitmap, Rect rect) {
		draw(bitmap, rect, 1);
	}

	void Graphics::draw(Bitmap *bitmap, Rect rect, Float opacity) {
		throw NotSupported(L"Graphics::draw(Bitmap, ...)");
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Point topLeft) {
		draw(bitmap, src, Rect(topLeft, topLeft + src.size()));
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Point topLeft, Float opacity) {
		draw(bitmap, src, Rect(topLeft, topLeft + src.size()), opacity);
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Rect dest) {
		draw(bitmap, src, dest, 1);
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Rect dest, Float opacity) {
		throw NotSupported(L"Graphics::draw(Bitmap, ...)");
	}

	void Graphics::text(Str *text, Font *font, Brush *brush, Rect rect) {
		throw NotSupported(L"Graphics::text(Text, ...)");
	}

	void Graphics::draw(Text *text, Brush *brush, Point origin) {
		throw NotSupported(L"Graphics::draw(Text, ...)");
	}

}
