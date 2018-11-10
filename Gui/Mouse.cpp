#include "stdafx.h"
#include "Mouse.h"

#ifdef GUI_WIN32
#include <Windowsx.h>
#endif

namespace gui {

#ifdef GUI_WIN32
	Point mousePos(const Message &msg) {
		return Point(Float(GET_X_LPARAM(msg.lParam)), Float(GET_Y_LPARAM(msg.lParam)));
	}

	Point mouseAbsPos(Handle window, const Message &msg) {
		POINT p = {
			GET_X_LPARAM(msg.lParam),
			GET_Y_LPARAM(msg.lParam)
		};

		MapWindowPoints(HWND_DESKTOP, window.hwnd(), &p, 1);

		return Point(Float(p.x), Float(p.y));
	}
#endif

}
