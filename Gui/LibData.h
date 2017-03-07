#pragma once
#include "Shared/LibData.h"

namespace gui {
	class App;
	class RenderMgr;

	// Access values in the library data.
	App *&appData(Engine &e);
	RenderMgr *&renderData(Engine &e);

}
