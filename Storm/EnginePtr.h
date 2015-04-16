#pragma once
#include "Code/Listing.h"
#include "Code/Reference.h"

namespace storm {

	class Engine;
	class Value;

	/**
	 * Object used as the first parameter to STORM_ENGINE_FN. The wrapping utility needs
	 * to store some extra data on the stack, which this class accommodates.
	 */
	class EnginePtr {
	public:
		// The engine itself.
		Engine &v;

	private:
		// Additional data required:

#ifdef X86
		// The return address and the return value, in some order.
		void *returnData[2];
#endif
	};

	// Generate code to call another function with an EnginePtr inserted as the first parameter.
	// Note that member functions are _not_ yet supported!
	code::Listing enginePtrThunk(Engine &e, const Value &returnType, code::Ref fn);

}
