#pragma once
#include "Name.h"
#include "Value.h"

namespace storm {

	class Named;
	class NameOverload;

	/**
	 * An interface for objects that can lookup names.
	 */
	class NameLookup : NoCopy {
	public:
		// Find the specified name in here, returns null if not found.
		virtual Named *find(const Name &name) = 0;

		// Get the parent object to this lookup, or null if none.
		virtual NameLookup *parent() const = 0;
	};

	/**
	 * Denotes a scope to use when looking up names.
	 * The scope itself is not much more than a policy along with the currently
	 * topmost element. For example, when looking for names relative a specific
	 * type, the type itself will be the topmost element. This is used to traverse
	 * the type hierarchy in any way the current implementation wishes to find
	 * a match for the name. This is designed so that the current implementation
	 * can be overridden by specific language implementations later on.
	 */
	class Scope {
	public:
		// Create the default lookup with a given topmost object.
		Scope(NameLookup *top);

		// Topmost object.
		NameLookup *top;

		// Find the given NameRef, either by using an absulute path or something
		// relative to the current object.
		virtual Named *find(const Name &name) const;

		// Find a overloaded name. Usually a function. Equivalent to call 'find' above
		// and then try to find something with parameters.
		NameOverload *find(const Name &name, const vector<Value> &params) const;
	};


}
