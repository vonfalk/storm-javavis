#pragma once
#include "Font.h"

namespace stormgui {

	/**
	 * Pre-formatted text prepared for rendering.
	 */
	class Text : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create a single line of text.
		STORM_CTOR Text(Par<Str> text, Par<Font> font);

		// Create text that fits inside a square 'size' units big.
		STORM_CTOR Text(Par<Str> text, Par<Font> font, Size size);

		// Size of the text inside the layout.
		Size STORM_FN size();

		// Layout border size.
		Size STORM_FN layoutBorder();
		void STORM_SETTER layoutBorder(Size size);

		// We can add formatting options for parts of the string here. For example, it is possible
		// to apply a font to a specific part of the string.

		// Get the layout.
		inline IDWriteTextLayout *layout() const { return l; }
	private:
		// The layout itself.
		IDWriteTextLayout *l;

		// Create layout.
		void init(Par<Str> text, Par<Font> font, Size size);
	};

}
