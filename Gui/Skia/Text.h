#pragma once

#include "Skia.h"

#ifdef GUI_ENABLE_SKIA

namespace gui {

	/**
	 * Our representation of a renderable chunk of text.
	 *
	 * We have this indirection to support multiple text rendering backends.
	 */
	class SkiaText {
	public:
		virtual ~SkiaText() {}

		// Draw the text to an SkCanvas with the supplied paint as the default style.
		virtual void draw(SkCanvas &canvas, const SkPaint &paint, Point origin) = 0;
	};

}

#endif
