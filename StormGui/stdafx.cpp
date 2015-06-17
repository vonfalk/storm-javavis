// stdafx.cpp : source file that includes just the standard includes
// StormGui.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

#pragma comment (lib, "Comctl32.lib")
#pragma comment (lib, "d2d1.lib")

namespace stormgui {

	Rect convert(const RECT &r) {
		return Rect(Float(r.left), Float(r.top), Float(r.right), Float(r.bottom));
	}

	Rect convert(const POINT &a, const POINT &b) {
		return Rect(Float(a.x), Float(a.y), Float(b.x), Float(b.y));
	}

	RECT convert(const Rect &r) {
		RECT z = { (LONG)r.p0.x, (LONG)r.p0.y, (LONG)r.p1.x, (LONG)r.p1.y };
		return z;
	}

	Size convert(const D2D1_SIZE_F &s) {
		return Size(s.width, s.height);
	}

	bool pressed(nat keycode) {
		return (GetKeyState(keycode) & 0x8000) != 0;
	}

	static nat toByte(float f) {
		return byte(f * 255.0f);
	}

	COLORREF colorref(const Color &color) {
		COLORREF r = toByte(color.r);
		r |= toByte(color.g) << 8;
		r |= toByte(color.b) << 16;
		return r;
	}

	Color color(COLORREF c) {
		byte r = byte(c & 0xFF);
		byte g = byte((c & 0xFF00) >> 8);
		byte b = byte((c & 0xFF0000) >> 16);
		return Color(r, g, b);
	}

	D2D_COLOR_F dx(const Color &color) {
		D2D_COLOR_F c = { color.r, color.g, color.b, color.a };
		return c;
	}

	D2D_POINT_2F dx(const Point &pt) {
		D2D_POINT_2F p = { pt.x, pt.y };
		return p;
	}

	D2D1_RECT_F dx(const Rect &rect) {
		D2D1_RECT_F r = { rect.p0.x, rect.p0.y, rect.p1.x, rect.p1.y };
		return r;
	}


}
