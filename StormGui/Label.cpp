#include "stdafx.h"
#include "Label.h"

namespace stormgui {

	Label::Label(Par<Str> title) {
		text(title);
	}

	bool Label::create(HWND parent, nat id) {
		PLN("Created!");
		return createEx(WC_STATIC, childFlags, 0, parent, id);
	}

}
