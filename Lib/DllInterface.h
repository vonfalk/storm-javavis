#pragma once

namespace storm {

	class Type;

	/**
	 * Function pointers that makes up the interface between a master Engine and a slave Engine.
	 */
	struct DllInterface {
		typedef Type *(*BuiltIn)(const Engine *, nat id);
		BuiltIn builtIn;
	};

}
