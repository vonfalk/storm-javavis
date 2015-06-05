#pragma once
#include "Code/Listing.h"
#include "Code/Reference.h"
#include "Shared/EnginePtr.h"

namespace storm {

	class Value;

	// Generate code to call another function with an EnginePtr inserted as the first parameter.
	// Note that member functions are _not_ yet supported!
	code::Listing enginePtrThunk(Engine &e, const Value &returnType, code::Ref fn);

}
