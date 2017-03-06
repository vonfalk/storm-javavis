#include "stdafx.h"
#include "Shared/Main.h"

SHARED_LIB_ENTRY_POINT(x);

namespace gui {
	STORM_DEFINE_THREAD(Ui);
	STORM_DEFINE_THREAD(Render);
}
