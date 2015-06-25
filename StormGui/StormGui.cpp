#include "stdafx.h"
#include "StormGui.h"
#include "Shared/DllMain.h"
#include "App.h"
#include "RenderMgr.h"

namespace stormgui {

	os::Thread spawnAppWait(Engine &e) {
		return os::Thread::spawn(new AppWait(e));
	}

	DEFINE_STORM_THREAD_WAIT(Ui, &spawnAppWait);


	os::Thread spawnRenderWait(Engine &e) {
		Auto<RenderMgr> mgr = renderMgr(e);
		return os::Thread::spawn(memberVoidFn(mgr.borrow(), &RenderMgr::main));
	}

	DEFINE_STORM_THREAD_WAIT(Render, &spawnRenderWait);
}

namespace storm {

	LibData::LibData() {}

	LibData::~LibData() {
		app->terminate();
	}

}
