#pragma once
#include "Name.h"
#include "Value.h"
#include "Lib/Object.h"

namespace storm {
	STORM_PKG(core.lang);

	class NameLookup;
	class Named;
	class Package;

	/**
	 * The scope is divided into two parts. The first one, 'Scope' is a simple
	 * data type that holds a pointer to a leaf in the type tree as well as a
	 * pointer to the current 'ScopeLookup', which implements the lookup strategy
	 */
	class Scope;

	// Find a named from a Name.
	Named *find(Par<NameLookup> root, Par<Name> name);


	/**
	 * This is the lookup for a scope. All lookups inherits from the standard one.
	 */
	class ScopeLookup : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR ScopeLookup();

		// Find 'name' in 'in'. Note that the returned pointer is borrowed.
		virtual Named *find(const Scope &in, Par<Name> name);

	protected:
		/**
		 * Utility functions. All of these returns borrowed pointers.
		 */

		// Find the first package when traversing parent() ptrs.
		static Package *firstPkg(NameLookup *l);

		// Find the root package.
		static Package *rootPkg(Package *p);

		// Find the package 'core' from any source.
		static Package *corePkg(NameLookup *l);

	private:
		// Find the next candidate for the standard algorithm. (not counting 'core').
		static NameLookup *nextCandidate(NameLookup *prev);
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
		STORM_VALUE;
	public:
		// Create a scope that will never return anything.
		explicit STORM_CTOR Scope();

		// Create the default lookup with a given topmost object.
		STORM_CTOR Scope(Par<NameLookup> top);

		// Create a custom lookup.
		STORM_CTOR Scope(Par<NameLookup> top, Par<ScopeLookup> lookup);

		// Create a child scope.
		STORM_CTOR Scope(const Scope &parent, Par<NameLookup> top);

		// Create a child scope.
		inline Scope STORM_FN child(Par<NameLookup> top) { return Scope(*this, top); }

		// Topmost object.
		NameLookup *top;

		// Lookup object.
		Auto<ScopeLookup> lookup;

		// Find the given NameRef, either by using an absulute path or something
		// relative to the current object. NOTE: returns a borrowed ptr.
		Named *find(Par<Name> name) const;

		// Deep copy.
		void STORM_FN deepCopy(Par<CloneEnv> env);
	};


	/**
	 * Lookup with extra top-level finders.
	 */
	class ScopeExtra : public ScopeLookup {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ScopeExtra();

		// Additional NameLookups to search (not recursively).
		vector<NameLookup *> extra;

		// Find
		virtual Named *find(const Scope &in, Par<Name> name);

	};


	/**
	 * Get the root scope from the Engine.
	 */
	Scope STORM_ENGINE_FN rootScope(Engine &e) ON(Compiler);

}

#include "Named.h"
