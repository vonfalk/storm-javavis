#include "stdafx.h"
#include "Button.h"

namespace gui {

	Button::Button(Str *title) {
		text(title);
		onClick = null;
	}

	Button::Button(Str *title, Fn<void, Button *> *click) {
		text(title);
		onClick = click;
	}

#ifdef GUI_WIN32
	bool Button::create(HWND parent, nat id) {
		return Window::createEx(WC_BUTTON, buttonFlags, 0, parent, id);
	}

	bool Button::onCommand(nat id) {
		if (id == BN_CLICKED) {
			if (onClick)
				onClick->call(this);
			return true;
		}

		return false;
	}
#endif

}
