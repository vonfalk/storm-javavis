#pragma once
#include "Storm/Std.h"

namespace storm {

	/**
	 * Test class for vtables.
	 */
	class VTest : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR VTest();

		~VTest();

		virtual Int STORM_FN returnOne();
		virtual Int STORM_FN returnTwo();
	};

}
