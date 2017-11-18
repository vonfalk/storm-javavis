#include "stdafx.h"
#include "Shared/Main.h"

SHARED_LIB_ENTRY_POINT();

namespace gui {
	os::Thread spawnUiThread(Engine &e);
	os::Thread spawnRenderThread(Engine &e);
	STORM_DEFINE_THREAD_WAIT(Ui, &spawnUiThread);
	STORM_DEFINE_THREAD_WAIT(Render, &spawnRenderThread);
}
