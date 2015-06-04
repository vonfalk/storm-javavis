#pragma once
#include "Shared/Object.h"

namespace stormgui {

	class Test : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR Test();

		Str *STORM_FN test();
	};

}
