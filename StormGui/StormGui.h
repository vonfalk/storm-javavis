#pragma once

namespace storm {

	/**
	 * Global data for our instance.
	 */
	class LibData : public NoCopy {
	public:
		LibData();
		~LibData();
	};

}

namespace stormgui {
	typedef storm::LibData LibData;
}
