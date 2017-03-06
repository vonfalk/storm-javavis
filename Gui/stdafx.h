#pragma once
#include "Shared/Storm.h"

using namespace storm;

namespace gui {
	STORM_THREAD(Ui);
	STORM_THREAD(Render);
}
