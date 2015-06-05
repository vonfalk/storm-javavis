#pragma once

#include "Shared/Str.h"
#include "Shared/Types.h"
#include "Lib/Object.h"
#include "Value.h"
#include "BuiltInLoader.h"

namespace storm {
	class Engine;

	/**
	 * Add the standard library to packages, set up super classes and add
	 * member functions and methods. This is to be done after 'createStdTypes'.
	 */
	void addStdLib(Engine &to, BuiltInLoader &loader);

}
