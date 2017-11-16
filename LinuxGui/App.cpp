#include "stdafx.h"
#include "App.h"
#include "LibData.h"

namespace gui {

	App::App() {}

	void App::terminate() {}

	App *app(EnginePtr e) {
		App *&v = appData(e.v);
		if (!v)
			v = new (e.v) App();
		return v;
	}

	os::Thread spawnUiThread(Engine &e) {
		return os::Thread::spawn(runtime::threadGroup(e));
	}

}
