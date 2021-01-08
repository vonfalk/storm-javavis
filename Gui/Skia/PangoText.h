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

		/**
		 * Parameters for an operation. Modifies how an operation is drawn.
		 */
		class State {
		public:
			// Create an "identity" state.
			State();

			// Grab the state from Pango.
			State(PangoRenderer *from);

			// Color to apply.
			SkColor4f color;

			// Shall we apply rgb parts of 'color'?
			bool hasColor;

			// Shall we apply alpha part of 'color'?
			bool hasAlpha;

			// Transform to apply.
			SkMatrix matrix;

			// Check if the state is the same.
			bool operator ==(const State &o) const;
		};

		// Text operation stored in here.
		class Operation {
		public:
			// Create.
			Operation(const State &state) : state(state) {}

			// Destroy.
			virtual ~Operation() {}

			// State to apply for this operation. Done externally.
			State state;

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
