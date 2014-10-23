#pragma once
#include "Name.h"
#include "Value.h"

namespace storm {

	class Named;
	class NameOverload;

	/**
	 * Denotes a scope to use when looking up names.
	 */
	class Scope {
	public:
		Scope();

		// Find the given NameRef, either by using an absulute path or something
		// relative to the current object.
		Named *find(const Name &name);

		// Find a overloaded name. Usually a function. Equivalent to call 'find' above
		// and then try to find something with parameters.
		NameOverload *find(const Name &name, const vector<Value> &params);

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
