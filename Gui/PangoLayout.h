#pragma once

#include "Gui/TextMgr.h"

#ifdef GUI_GTK

namespace gui {
	namespace pango {

		/**
		 * This file implements functions that are used in both the Cairo text manager and the Skia text
		 * manager, as they both rely on a Pango layout for text rendering.
		 */

		// Create the layout from a Text object.
		PangoLayout *create(PangoContext *context, const Text *text);

		// Free a layout.
		void free(PangoLayout *layout);

		// Update the border.
		void updateBorder(PangoLayout *layout, Size border);

		// Add an effect.
		TextMgr::EffectResult addEffect(PangoLayout *layout, const TextEffect &effect, Str *text);

		// Get the size of the layout.
		Size size(PangoLayout *layout);

		// Get information on each line of the formatted text.
		Array<TextLine *> *lineInfo(PangoLayout *layout, Text *text);

		// Get rectangles that cover some characters.
		Array<Rect> *boundsOf(PangoLayout *layout, Text *text, Str::Iter begin, Str::Iter end);

	}
}

#endif
