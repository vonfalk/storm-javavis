#pragma once
#include "Utils/Bitmask.h"

namespace gui {

	/**
	 * Keycodes and modifiers.
	 */

	namespace mod {
		enum Modifiers {
			none = 0x0,
			ctrl = 0x1,
			alt = 0x2,
			shift = 0x4,
			super = 0x8,
		};
		BITMASK_OPERATORS(Modifiers);
	}
	typedef mod::Modifiers Modifiers;


#ifdef GUI_WIN32
	// Get the current modifiers.
	Modifiers modifiers();
#endif
#ifdef GUI_GTK
	// Convert GtkModifierType into modifier flags.
	Modifiers modifiers(guint state);

	// Define some virtual key codes...
#define VK_RETURN GDK_KEY_Return

#endif

	// namespace key {
	// 	enum Key {
	// 	};
	// }
	// typedef key::Key Key;
}
