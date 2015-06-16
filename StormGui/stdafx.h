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

	// Key state (as of the last processed message, only valid in the msg thread).
	bool pressed(nat keycode);
}

// D2D for rendering.
#include <d2d1.h>

// It defines a macro 'interface' that messes with the rest of the system...
#undef interface

// Release COM objects.
template <class T>
void release(T *&v) {
	if (v)
		v->Release();
	v = null;
}


