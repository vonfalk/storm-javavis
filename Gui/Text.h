#pragma once
#include "Font.h"
#include "Brush.h"
#include "Core/TObject.h"
#include "Core/Array.h"

namespace gui {

	/**
	 * Information about a single line of formatted text.
	 */
	class TextLine : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TextLine(Float baseline, Str *text);

		// The distance from the top of the line to the baseline.
		Float baseline;

		// Contents of the line.
		Str *text;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

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

		// Get the text in here.
		inline Str *STORM_FN text() const { return myText; }

		// Get the font used for this object.
		inline Font *STORM_FN font() const { return myFont; }

		// Size of the text inside the layout.
		Size STORM_FN size();

		// Layout border size. If no layout border is set from the constructor, it will return the
		// largest possible float value.
		Size STORM_FN layoutBorder();
		void STORM_ASSIGN layoutBorder(Size size);

		// Get information about each line of the formatted text.
		Array<TextLine *> *STORM_FN lineInfo();

		// Set the color of a particular range of characters.
		void STORM_FN color(Str::Iter begin, Str::Iter end, Color color);
		void STORM_FN color(Str::Iter begin, Str::Iter end, SolidBrush *color);

		// Get a set of rectangles that cover a range of characters.
		Array<Rect> *STORM_FN boundsOf(Str::Iter begin, Str::Iter end);

		// TODO: We can add formatting options for parts of the string here. For example, it is
		// possible to apply a font to a specific part of the string.

#ifdef GUI_WIN32
		// Get the layout.
		IDWriteTextLayout *layout(Painter *owner);
#endif
#ifdef GUI_GTK
		// Get the layout.
		inline PangoLayout *layout(Painter *p) { return l; }
#endif

	private:
		// The layout itself.
		OsTextLayout *l;

		// The text we represent.
		Str *myText;

		// Which font are we using?
		Font *myFont;

		// Effect (currently only text color) to apply next time "prepare" is called.
		class Effect {
			STORM_VALUE;
		public:
			// Range.
			Nat from;
			Nat to;

			// Color.
			Color color;

			// Create.
			Effect(Nat from, Nat to, Color color);
		};

		// Any effects that should be applied before rendering.
		Array<Effect> *effects;

		// Create layout.
		void init(Str *text, Font *font, Size size);

		// Destroy the layout.
		void destroy();
	};

}
