#pragma once
#include "Storm/Std.h"

namespace storm {

	/**
	 * Test class for vtables.
	 */
	class VTest : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR VTest(Type *t);
		VTest(Engine &e);

		~VTest();

		virtual Int STORM_FN returnOne();
		virtual Int STORM_FN returnTwo();
	};

}
