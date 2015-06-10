#include "stdafx.h"
#include "StormGui.h"
#include "Shared/DllMain.h"
#include "App.h"

namespace stormgui {

	os::Thread spawnAppWait(Engine &e) {
		return os::Thread::spawn(new AppWait(e));
	}

	DEFINE_STORM_THREAD_WAIT(Ui, &spawnAppWait);

}

namespace storm {

	LibData::LibData() {}

	LibData::~LibData() {
		app->terminate();
	}

}
