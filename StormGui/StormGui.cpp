#include "stdafx.h"
#include "StormGui.h"
#include "Shared/DllMain.h"
#include "App.h"
#include "RenderMgr.h"

STORM_LIB_ENTRY_POINT(stormgui::LibData);

namespace stormgui {

	os::Thread spawnAppWait(Engine &e) {
		return os::Thread::spawn(new AppWait(e), threadGroup(e));
	}

	DEFINE_STORM_THREAD_WAIT(Ui, &spawnAppWait);


	os::Thread spawnRenderWait(Engine &e) {
		Auto<RenderMgr> mgr = renderMgr(e);
		return os::Thread::spawn(memberVoidFn(mgr.borrow(), &RenderMgr::main), threadGroup(e));
	}

	DEFINE_STORM_THREAD_WAIT(Render, &spawnRenderWait);

	LibData::LibData() {}

	LibData::~LibData() {}

	void LibData::shutdown() {
		if (app)
			app->terminate();
		if (renderMgr)
			renderMgr->terminate();
	}

}
