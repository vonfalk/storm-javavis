#pragma once
#include "Name.h"

namespace storm {

	class Named;

	/**
	 * Denotes a scope to use when looking up names.
	 */
	class Scope {
	public:
		Scope();

		// Find the given NameRef, either by using an absulute path or something
		// relative to the current object.
		Named *find(const Name &name);

	protected:
		// Fallback for name lookup.
		Scope *nameFallback;

		// Find a name in this scope.
		virtual Named *findHere(const Name &name) = 0;
	};

}
