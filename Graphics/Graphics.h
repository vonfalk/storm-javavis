#pragma once
#include "Shared/DllInterface.h"

namespace graphics {

	class LibData : public storm::LibData {
	public:
		LibData() {}
		~LibData() {}
		void shutdown() {}
	};

}
