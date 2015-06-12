#include "stdafx.h"
#include "Button.h"

namespace stormgui {

	Button::Button(Par<Str> title) {
		text(title);
	}

	bool Button::create(HWND parent, nat id) {
		return Window::createEx(WC_BUTTON, buttonFlags, 0, parent, id);
	}

}
