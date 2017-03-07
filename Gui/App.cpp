#include "stdafx.h"
#include "App.h"
#include "LibData.h"
#include "Window.h"

namespace gui {

	App::App() : appWait(null), creating(null) {
		windows = new (this) Map<Handle, Window *>();
		liveWindows = new (this) Set<Window *>();
	}

	void App::terminate() {
		PLN(L"Destroying app!");
	}


	App *app(EnginePtr e) {
		App *&v = appData(e.v);
		if (!v)
			v = new (e.v) App();
		return v;
	}

}
