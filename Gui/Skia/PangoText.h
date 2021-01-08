#pragma once

#include "Skia.h"
#include "Text.h"
#include "PangoFont.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Pango layout wrapped in the Skia rendering class.
	 *
	 * Created by SkiaPangoMgr.
	 */
	class PangoText : public SkiaText {
	public:
		// Create.
		PangoText(PangoLayout *layout, SkPangoFontCache &cache);

		// Destroy.
		~PangoText();

		// The Pango layout.
		PangoLayout *pango;

		// Invalidate the layout.
		void invalidate();

		// Draw the layout.
		void draw(SkCanvas &to, const SkPaint &paint, Point origin);

		// Text operation stored in here.
		class Operation {
		public:
			// Destroy.
			virtual ~Operation() {}

			// Draw this operation.
			virtual void draw(SkCanvas &to, const SkPaint &paint, Point origin) = 0;
		};

	private:
		// Skia font cache.
		SkPangoFontCache &cache;

		// Text-drawing operations.
		std::vector<std::unique_ptr<Operation>> operations;

		// Are the operations valid?
		bool valid;
	};

}

#endif
