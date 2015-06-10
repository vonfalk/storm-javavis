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
