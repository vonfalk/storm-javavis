#pragma once
#include "Shared/Storm.h"
#include "Core/Graphics/Color.h"
#include "Core/Geometry/Angle.h"
#include "Core/Geometry/Point.h"
#include "Core/Geometry/Rect.h"
#include "Core/Geometry/Size.h"
#include "Core/Geometry/Transform.h"
#include "Core/Geometry/Vector.h"

using namespace storm;

#include <Commctrl.h>

// D3D for rendering using D2D..
#include <d3d10_1.h>

// D2D for rendering.
#include <d2d1.h>

// Direct Write for text rendering.
#include <dwrite.h>

// HRESULT to string.
String toS(HRESULT r);

// It defines a macro 'interface' that messes with the rest of the system... (defined to 'struct')
#undef interface

namespace gui {
	STORM_THREAD(Ui);
	STORM_THREAD(Render);

	using storm::geometry::Point;
	using storm::geometry::Rect;
	using storm::geometry::Size;
	using storm::geometry::Transform;

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
	Point convert(const POINT &a);

	// Color conversion.
	COLORREF colorref(const Color &color);
	Color color(COLORREF c);

	// DX conversions.
	D2D1_COLOR_F dx(const Color &color);
	D2D1_POINT_2F dx(const Point &pt);
	D2D1_RECT_F dx(const Rect &rect);
	D2D1_MATRIX_3X2_F dx(Transform *tfm);
	D2D1_MATRIX_3X2_F dxUnit();
	D2D1_MATRIX_3X2_F dxMultiply(const D2D1_MATRIX_3X2_F &a, const D2D1_MATRIX_3X2_F &b);

	wostream &operator <<(wostream &to, const D2D1_MATRIX_3X2_F &m);

	// Key state (as of the last processed message, only valid in the msg thread).
	bool pressed(nat keycode);
}


// Custom messages.
#define WM_THREAD_SIGNAL (WM_APP + 1)
