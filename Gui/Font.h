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

		// Create font with more options.
		Font(LOGFONT &lf);

		// Destroy.
		~Font();

		/**
		 * Get/set stuff.
		 */

		// Font name.
		inline Str *STORM_FN name() { return fName; }
		void STORM_SETTER name(Str *name);

		// Font height (pt).
		inline Float STORM_FN height() { return fHeight; }
		void STORM_SETTER height(Float h);

		// Font height (dip).
		inline Float STORM_FN pxHeight() { return fHeight * 92.0f / 72.0f; }

		// Font weight. TODO: Make constants for weight.
		inline Int STORM_FN weight() { return fWeight; }
		void STORM_SETTER weight(Int w);

		// Italic.
		inline Bool STORM_FN italic() { return fItalic; }
		void STORM_SETTER italic(Bool u);

		// Underline.
		inline Bool STORM_FN underline() { return fUnderline; }
		void STORM_SETTER underline(Bool u);

		// Strike thru.
		inline Bool STORM_FN strikeOut() { return fStrikeOut; }
		void STORM_SETTER strikeOut(Bool u);

		// Get a Win32 handle. Will be alive at least as long as this object.
		HFONT handle();

		// Get TextFormat for D2D. This class retains ownership over the returned object.
		IDWriteTextFormat *textFormat();

		virtual void STORM_FN toS(StrBuf *to) const;

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

	// Create the system default font for UI.
	Font *STORM_FN defaultFont(EnginePtr e);

}