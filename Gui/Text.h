#pragma once
#include "Font.h"
#include "Brush.h"
#include "Core/TObject.h"
#include "Core/Array.h"

namespace gui {

	class TextMgr;

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
		inline Size STORM_FN layoutBorder() const { return myBorder; }
		void STORM_ASSIGN layoutBorder(Size size);

		// Get information about each line of the formatted text.
		Array<TextLine *> *STORM_FN lineInfo();

		// Get a set of rectangles that cover a range of characters.
		Array<Rect> *STORM_FN boundsOf(Str::Iter begin, Str::Iter end);

		// Set the color of a particular range of characters.
		void STORM_FN color(Str::Iter begin, Str::Iter end, Color color);
		void STORM_FN color(Str::Iter begin, Str::Iter end, SolidBrush *color);

		// TODO: We can add formatting options for parts of the string here. For example, it is
		// possible to apply a font to a specific part of the string.

		/**
		 * Text effect. Used internally.
		 */
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

		// Get text effects.
		Array<Effect> *STORM_FN effects() const;

		// Get the backend-specific representation for painting to "graphics".
		void *backendLayout(Graphics *graphics);

	private:
		// The text layout.
		UNKNOWN(PTR_NOGC) void *layout;

		// Destructor for the data.
		typedef void (*Cleanup)(void *);
		UNKNOWN(PTR_NOGC) Cleanup cleanup;

		// Text manager.
		UNKNOWN(PTR_NOGC) TextMgr *mgr;

		// The text we represent.
		Str *myText;

		// Which font are we using?
		Font *myFont;

		// Border of the layout.
		Size myBorder;

		// All text effects applied here.
		Array<Effect> *myEffects;

		// How many of the effects have been applied so far?
		Nat appliedEffects;

		// Common initialization.
		void init();

		// Re-create the layout.
		void recreate();
	};

}
