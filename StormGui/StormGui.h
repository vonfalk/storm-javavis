#pragma once

namespace stormgui {
	class App;
	class RenderMgr;
}

namespace storm {

	/**
	 * Global data for our instance.
	 */
	class LibData : public NoCopy {
	public:
		LibData();
		~LibData();

		// The one and only App object. Created when needed.
		Auto<stormgui::App> app;

		// The render manager. Created when needed.
		Auto<stormgui::RenderMgr> renderMgr;
	};

}

namespace stormgui {
	typedef storm::LibData LibData;
}
