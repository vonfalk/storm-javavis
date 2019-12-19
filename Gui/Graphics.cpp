#include "stdafx.h"
#include "Graphics.h"
#include "Bitmap.h"
#include "Core/Exception.h"

namespace gui {

	Graphics::Graphics() {}

	void Graphics::reset() {
		while (pop())
			;
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

	void Graphics::draw(Bitmap *bitmap, Rect src, Point topLeft) {
		draw(bitmap, src, Rect(topLeft, topLeft + src.size()));
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Point topLeft, Float opacity) {
		draw(bitmap, src, Rect(topLeft, topLeft + src.size()), opacity);
	}

	void Graphics::draw(Bitmap *bitmap, Rect src, Rect dest) {
		draw(bitmap, src, dest, 1);
	}

}
