#pragma once
#include "Core/EngineFwd.h"

namespace storm {

	/**
	 * Implements the engine used in shared libraries. An engine object in the compiler is different
	 * (even by pointer) from engine objects in any shared libraries. This is since the compiler
	 * needs to keep track of the different type id:s found in different libraries.
	 */
	class Engine {
	public:
		// TODO!
	};

}
