#pragma once

namespace stormgui {

	/**
	 * Describes a font.
	 */
	class Font : public Object {
		STORM_CLASS;
	public:
		// Wrap a HFont.
		Font(HFONT f);

		// TODO: Create from other parameters.

		// Get the Win32 font object.
		HFONT handle();

	private:
		// The Win32 font object.
		HFONT font;
	};

	// Create the system default font for UI.
	Font *STORM_ENGINE_FN defaultFont(EnginePtr e);

}
