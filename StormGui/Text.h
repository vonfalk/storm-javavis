#pragma once
#include "Font.h"

namespace stormgui {

	/**
	 * Pre-formatted text prepared for rendering.
	 */
	class Text : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		STORM_CTOR Text(Par<Str> text, Par<Font> font, Size size);

		// Layout size.
		inline Size STORM_FN size() const { return s; }

		// Get the layout.
		inline IDWriteTextLayout *layout() const { return l; }
	private:
		// The layout itself.
		IDWriteTextLayout *l;

		// Layout size.
		Size s;
	};

}
