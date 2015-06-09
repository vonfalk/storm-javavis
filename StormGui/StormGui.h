#pragma once

namespace stormgui {
	class App;
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
	};

}

namespace stormgui {
	typedef storm::LibData LibData;
}
