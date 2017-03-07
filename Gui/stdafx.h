#pragma once
#include "Shared/Storm.h"

using namespace storm;

namespace gui {
	STORM_THREAD(Ui);
	STORM_THREAD(Render);
}


// Custom messages.
#define WM_THREAD_SIGNAL (WM_APP + 1)
