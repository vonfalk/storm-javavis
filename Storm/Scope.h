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


	/**
	 * Define a lookup in terms of a chain of other scopes.
	 */
	class ScopeChain : public Scope {
	public:
		// Search in these scopes, in order.
		vector<Scope*> scopes;
	protected:
		virtual Named *findHere(const Name &name);
	};
}
