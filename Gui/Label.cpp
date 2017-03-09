#include "stdafx.h"
#include "Label.h"

namespace gui {

	Label::Label(Str *title) {
		text(title);
	}

	bool Label::create(HWND parent, nat id) {
		return createEx(WC_STATIC, childFlags, 0, parent, id);
	}

}
