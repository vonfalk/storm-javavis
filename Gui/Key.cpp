#include "stdafx.h"
#include "Key.h"

namespace gui {

#ifdef GUI_WIN32

	static bool pressed(nat keycode) {
		return (GetKeyState(keycode) & 0x8000) != 0;
	}

	Modifiers modifiers() {
		Modifiers r = mod::none;
		if (pressed(VK_CONTROL))
			r |= mod::ctrl;
		if (pressed(VK_ALT))
			r |= mod::alt;
		if (pressed(VK_SHIFT))
			r |= mod::shift;
		// Super?
		return r;
	}

#endif
#ifdef GUI_GTK

	Modifiers modifiers(guint state) {
		Modifiers r = mod::none;
		if (state & GDK_CONTROL_MASK)
			r |= mod::ctrl;
		if (state & GDK_META_MASK)
			r |= mod::alt;
		if (state & GDK_SHIFT_MASK)
			r |= mod::shift;
		if (state & GDK_SUPER_MASK)
			r |= mod::super;
		return r;
	}

#endif

}
