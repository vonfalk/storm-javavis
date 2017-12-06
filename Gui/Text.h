#pragma once
#include "Font.h"
#include "Core/TObject.h"

namespace gui {

	/**
	 * Pre-formatted text prepared for rendering.
	 */
	class Text : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create a single line of text.
		STORM_CTOR Text(Str *text, Font *font);

		// Create text that fits inside a square 'size' units big.
		STORM_CTOR Text(Str *text, Font *font, Size size);

		// Destroy.
		virtual ~Text();

		// Size of the text inside the layout.
		Size STORM_FN size();

		// Layout border size.
		Size STORM_FN layoutBorder();
		void STORM_SETTER layoutBorder(Size size);

		// TODO: We can add formatting options for parts of the string here. For example, it is
		// possible to apply a font to a specific part of the string.


#ifdef GUI_WIN32
		// Get the layout.
		inline IDWriteTextLayout *layout() const { return l; }
#endif
#ifdef GUI_GTK
		// Get the layout.
		inline PangoLayout *layout() const { return l; }
#endif

	private:
		// The layout itself.
		OsTextLayout *l;

		// Create layout.
		void init(Str *text, Font *font, Size size);

		// Destroy the layout.
		void destroy();
	};

}
