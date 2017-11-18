#include "stdafx.h"
#include "Label.h"

namespace gui {

	Label::Label(Str *title) {
		text(title);
	}

#ifdef GUI_WIN32
	bool Label::create(HWND parent, nat id) {
		return createEx(WC_STATIC, childFlags, 0, parent, id);
	}
#endif
}
