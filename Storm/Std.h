#pragma once

#include "Shared/Str.h"
#include "Shared/Types.h"
#include "Lib/Object.h"
#include "Value.h"

namespace storm {
	class Engine;

	/**
	 * Add types from the built-in types. This will only add the types to
	 * the cache, not to any packages.
	 * The process is split into two since we need to have the Type-object
	 * for Package to be able to create packages.
	 */
	void createStdTypes(Engine &to, vector<Auto<Type> > &cached);

	/**
	 * Add the standard library to packages, set up super classes and add
	 * member functions and methods. This is to be done after 'createStdTypes'.
	 */
	void addStdLib(Engine &to);

	/**
	 * Compute the maxium needed VTable size for C++ types.
	 */
	nat maxVTableCount();

}
