#include "stdafx.h"
#include "App.h"
#include "LibData.h"

namespace gui {

	App::App() {
		PLN(L"Creating app!");
	}

	void App::destroy() {
		PLN(L"Destroying app!");
	}

	App *app(EnginePtr e) {
		App *&v = appData(e.v);
		if (!v)
			v = new (e.v) App();
		return v;
	}

}
