#pragma once
#include "Shared/DllEngine.h"

namespace stormgui {
	class App;
	class RenderMgr;

	/**
	 * Global data for our instance.
	 */
	class LibData : public storm::LibData {
	public:
		// Create.
		LibData();

		// Destroy.
		~LibData();

		// Shut down.
		void shutdown();

		// The one and only App object. Created when needed.
		Auto<stormgui::App> app;

		// The render manager. Created when needed.
		Auto<stormgui::RenderMgr> renderMgr;
	};

}
