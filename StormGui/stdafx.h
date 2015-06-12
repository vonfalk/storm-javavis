#pragma once


#include "Utils/Utils.h"
#include "Shared/Storm.h"

// Windows headers.
#include "Utils/Windows.h"

using namespace storm;

namespace stormgui {
	STORM_THREAD(Ui);
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

	// Helpers.
	Rect convert(const RECT &r);
	Rect convert(const POINT &a, const POINT &b);
	RECT convert(const Rect &r);
}
