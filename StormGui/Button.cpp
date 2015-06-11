#include "stdafx.h"
#include "Button.h"

namespace stormgui {

	Button::Button(Par<Str> title, Par<Container> parent) {
		if (!create(WC_BUTTON, title->v.c_str(), buttonFlags, 10, 10, 100, 100, parent->handle(), 0))
			PLN("FAILED" << GetLastError());
	}

}
