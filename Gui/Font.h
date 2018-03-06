#pragma once
#include "Core/EnginePtr.h"

namespace gui {

	// Shared data between font objects. This includes lazily created OS objects. Only for internal use.
	struct FontData;

	/**
	 * Describes a font.
	 */
	class Font : public Object {
		STORM_CLASS;
	public:
		// Create a font.
		STORM_CTOR Font(Str *typeface, Float height);

		// Copy.
		Font(const Font &o);

#ifdef GUI_WIN32
		// Create font with more options.
		Font(LOGFONT &lf);
#endif
#ifdef GUI_GTK
		// Create font with more options.
		Font(const PangoFontDescription &desc);
#endif

		// Destroy.
		~Font();

		/**
		 * Get/set stuff.
		 */

		// Font name.
		inline Str *STORM_FN name() { return fName; }
		void STORM_ASSIGN name(Str *name);

		// Font height (pt).
		inline Float STORM_FN height() { return fHeight; }
		void STORM_ASSIGN height(Float h);

		// Font height (dip).
		inline Float STORM_FN pxHeight() { return fHeight * 92.0f / 72.0f; }

		// Font weight. TODO: Make constants for weight.
		inline Int STORM_FN weight() { return fWeight; }
		void STORM_ASSIGN weight(Int w);

		// Italic.
		inline Bool STORM_FN italic() { return fItalic; }
		void STORM_ASSIGN italic(Bool u);

		// Underline.
		inline Bool STORM_FN underline() { return fUnderline; }
		void STORM_ASSIGN underline(Bool u);

		// Strike thru.
		inline Bool STORM_FN strikeOut() { return fStrikeOut; }
		void STORM_ASSIGN strikeOut(Bool u);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

#ifdef GUI_WIN32
		// Get a Win32 handle. Will be alive at least as long as this object.
		HFONT handle();

		// Get TextFormat for D2D. This class retains ownership over the returned object.
		IDWriteTextFormat *textFormat();
#endif
#ifdef GUI_GTK
		// Get a Pango font description.
		PangoFontDescription *desc();
#endif

	private:
		// Shared data. Be careful with this!
		FontData *shared;

		// Font name.
		Str *fName;

		// Height (pt). 1 pt = 1/72 inch. 1 inch = 92 DIP
		Float fHeight;

		// Weight?
		Int fWeight;

		// Italic.
		Bool fItalic;

		// Underline.
		Bool fUnderline;

		// Strike out.
		Bool fStrikeOut;

		// Invalidate any created resources.
		void changed();
	};

}
