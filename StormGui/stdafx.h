#pragma once

#include "Utils/Utils.h"
#include "Shared/Storm.h"

// Windows headers.
#include "Utils/Windows.h"

using namespace storm;

namespace stormgui {
	STORM_THREAD(Ui);
	STORM_THREAD(Render);
}

#define WM_THREAD_SIGNAL (WM_APP + 1)

#include "Shared/DllEngine.h"
#include "Shared/Geometry.h"

using namespace storm::geometry;

#include <Commctrl.h>

// D3D for rendering using D2D..
#include <d3d10_1.h>

// D2D for rendering.
#include <d2d1.h>

// It defines a macro 'interface' that messes with the rest of the system... (defined to 'struct')
#undef interface

namespace stormgui {
	// Various flags for child windows.
	const DWORD childFlags = WS_CHILD | WS_VISIBLE;
	const DWORD controlFlags = childFlags | WS_TABSTOP;
	const DWORD buttonFlags = controlFlags | BS_PUSHBUTTON;
	const DWORD editFlags = controlFlags | ES_AUTOHSCROLL;

	// Helpers.
	Rect convert(const RECT &r);
	Rect convert(const POINT &a, const POINT &b);
	RECT convert(const Rect &r);
	Size convert(const D2D1_SIZE_F &s);

	// Color conversion.
	COLORREF colorref(const Color &color);
	Color color(COLORREF c);

	// DX conversions.
	D2D1_COLOR_F dx(const Color &color);
	D2D1_POINT_2F dx(const Point &pt);
	D2D1_RECT_F dx(const Rect &rect);


	// Key state (as of the last processed message, only valid in the msg thread).
	bool pressed(nat keycode);
}

// Release COM objects.
template <class T>
void release(T *&v) {
	if (v)
		v->Release();
	v = null;
}

