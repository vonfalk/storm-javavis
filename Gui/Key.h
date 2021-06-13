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


	namespace key {
		// Keycodes used for some well-known control keys. Will probably be expanded in the future.
		enum Key {
			// Unknown key.
			unknown = 0,

			// Begin of control-character block. Using the range of characters reserved for UTF-16
			// surrogate pairs.
			firstControl = 0xD800,

			// Modifiers etc.
			shift, control, alt, super,
			menu,
			esc,
			backspace,
			tab, STORM_NAME(ret, return),
			left, right, up, down,
			pageUp, pageDown, home, end, insert, STORM_NAME(del, delete),
			F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,

			// Numpad.
			numpad0, numpad1, numpad2, numpad3, numpad4, numpad5, numpad6, numpad7, numpad8, numpad9,

			// From here on, we use ASCII codes.
			lastControl,

			// Printable characters.
			space = ' ',
			num0 = '0', num1, num2, num3, num4, num5, num6, num7, num8, num9,
			a = 'a', b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
		};
	}
	typedef key::Key Key;

	// Get the keycode for a specific key. This can be passed to Char() to get an actual codepoint.
	Nat STORM_FN keycode(key::Key key);

	// Get a character for the specific key.
	Char STORM_FN keychar(key::Key key);

	// Get the name of a key.
	const wchar *c_name(key::Key key);
	Str *STORM_FN name(EnginePtr e, key::Key key);
	Str *STORM_FN name(EnginePtr e, mod::Modifiers mod);

#ifdef GUI_WIN32
	// Get the current modifiers.
	Modifiers modifiers();

	// Convert from virtual keycodes into our representation.
	Key keycode(WPARAM vk);

	// Was 'keycode' pressed when the last window message was sent?
	bool pressed(nat keycode);

#endif
#ifdef GUI_GTK
	// Extract modifiers and keycode from a GdkEventKey.
	Modifiers modifiers(const GdkEventKey &event);
	Key keycode(const GdkEventKey &event);

#endif
}
