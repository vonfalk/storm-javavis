#pragma once
#include "Gen/CppTypes.h"

namespace storm {

	/**
	 * Load objects that are defined in C++ somewhere.
	 */
	class CppLoader : NoCopy {
	public:
		// Create, note which set of functions to be loaded.
		CppLoader(Engine &e, const CppWorld *world);

	private:
		// Engine to load into.
		Engine &e;

		// Source.
		const CppWorld *world;
	};

}
