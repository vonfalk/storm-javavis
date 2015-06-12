#include "stdafx.h"
#include "Button.h"

namespace stormgui {

	Button::Button(Par<Str> title) {
		text(title);
	}

	Button::Button(Par<Str> title, Par<FnPtr<void, Par<Button>>> click) : onClick(click) {
		text(title);
	}

	bool Button::create(HWND parent, nat id) {
		return Window::createEx(WC_BUTTON, buttonFlags, 0, parent, id);
	}

	bool Button::onCommand(nat id) {
		if (id == BN_CLICKED) {
			onClick->call(this);
			return true;
		}

		return false;
	}

}
