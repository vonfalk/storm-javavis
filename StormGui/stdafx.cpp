// stdafx.cpp : source file that includes just the standard includes
// StormGui.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

#pragma comment (lib, "Comctl32.lib")


namespace stormgui {

	Rect convert(const RECT &r) {
		return Rect(r.left, r.top, r.right, r.bottom);
	}

	Rect convert(const POINT &a, const POINT &b) {
		return Rect(a.x, a.y, b.x, b.y);
	}

	RECT convert(const Rect &r) {
		RECT z = { r.p0.x, r.p0.y, r.p1.x, r.p1.y };
		return z;
	}

	bool pressed(nat keycode) {
		return (GetKeyState(keycode) & 0x8000) != 0;
	}

}
