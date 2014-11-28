#pragma once
#include "Name.h"
#include "Value.h"
#include "Lib/Object.h"

namespace storm {

	class NameLookup;
	class NameOverload;
	class Named;
	class Package;

	/**
	 * Denotes a scope to use when looking up names.
	 * The scope itself is not much more than a policy along with the currently
	 * topmost element. For example, when looking for names relative a specific
	 * type, the type itself will be the topmost element. This is used to traverse
	 * the type hierarchy in any way the current implementation wishes to find
	 * a match for the name. This is designed so that the current implementation
	 * can be overridden by specific language implementations later on.
	 */
	class Scope : public Object {
		STORM_CLASS;
	public:
		// Create the default lookup with a given topmost object.
		STORM_CTOR Scope(Auto<NameLookup> top);

		// Topmost object. (circular trouble)
		NameLookup *top;

		// Find the given NameRef, either by using an absulute path or something
		// relative to the current object. NOTE: returns a borrowed ptr.
		virtual Named *find(const Name &name) const;

		// Find a overloaded name. Usually a function. Equivalent to call 'find' above
		// and then try to find something with parameters.
		NameOverload *find(const Name &name, const vector<Value> &params) const;

	protected:
		// Find the first package.
		static Package *firstPkg(NameLookup *top);

		// Find the root package.
		static Package *rootPkg(Package *pkg);

		// Find "core"
		static Package *corePkg(NameLookup *top);

		// Next candidate.
		static NameLookup *nextCandidate(NameLookup *top);
	};


	/**
	 * Lookup with extra top-level finders.
	 */
	class ScopeExtra : public Scope {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ScopeExtra(Auto<NameLookup> top);

		// Find
		virtual Named *find(const Name &name) const;

		// Additional NameLookups to search (not recursively).
		vector<NameLookup *> extra;
	};

}
