#include "stdafx.h"
#include "Shared/Main.h"

#pragma comment (lib, "Gdi32.lib")
#pragma comment (lib, "Comctl32.lib")
#pragma comment (lib, "d3d10_1.lib")
#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "dwrite.lib")

SHARED_LIB_ENTRY_POINT(x);

#undef null
#include <comdef.h>
#include <iomanip>
#define null NULL

String toS(HRESULT r) {
	return _com_error(r).ErrorMessage();
}

namespace gui {
	os::Thread spawnUiThread(Engine &e);
	os::Thread spawnRenderThread(Engine &e);
	STORM_DEFINE_THREAD_WAIT(Ui, &spawnUiThread);
	STORM_DEFINE_THREAD_WAIT(Render, &spawnRenderThread);

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

	Point convert(const POINT &a) {
		return Point(Float(a.x), Float(a.y));
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

	D2D1_MATRIX_3X2_F dx(Transform *tfm) {
		// Note that we are taking the transpose of the matrix, since DX uses a
		// row vector when multiplying while we're using a column vector.
		D2D1_MATRIX_3X2_F m = {
			tfm->at(0, 0), tfm->at(1, 0),
			tfm->at(0, 1), tfm->at(1, 1),
			tfm->at(0, 3), tfm->at(1, 3),
		};
		return m;
	}

	D2D1_MATRIX_3X2_F dxUnit() {
		D2D1_MATRIX_3X2_F m = {
			1, 0, 0, 1, 0, 0,
		};
		return m;
	}

	D2D1_MATRIX_3X2_F dxMultiply(const D2D1_MATRIX_3X2_F &a, const D2D1_MATRIX_3X2_F &b) {
		D2D1_MATRIX_3X2_F r;
		r._11 = a._11*b._11 + a._12*b._21;
		r._12 = a._11*b._12 + a._12*b._22;
		r._21 = a._21*b._11 + a._22*b._21;
		r._22 = a._21*b._12 + a._22*b._22;
		r._31 = a._31*b._11 + a._32*b._21 + b._31;
		r._32 = a._31*b._12 + a._32*b._22 + b._32;
		return r;
	}

	wostream &operator <<(wostream &to, const D2D1_MATRIX_3X2_F &m) {
		to << std::fixed << std::setprecision(3);
		to << endl << m._11 << L", " << m._12;
		to << endl << m._21 << L", " << m._22;
		to << endl << m._31 << L", " << m._32;
		return to;
	}

}
