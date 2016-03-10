#pragma once
#include "Value.h"
#include "Name.h"
#include "Shared/Object.h"
#include "Shared/TObject.h"
#include "Thread.h"

namespace storm {
	STORM_PKG(core.lang);

	class NameLookup;
	class Name;
	class SimpleName;
	class Named;
	class Package;

	/**
	 * The scope is divided into two parts. The first one, 'Scope' is a simple
	 * data type that holds a pointer to a leaf in the type tree as well as a
	 * pointer to the current 'ScopeLookup', which implements the lookup strategy
	 */
	class Scope;

	// Find a named from a SimpleName.
	MAYBE(Named) *STORM_FN find(Par<NameLookup> root, Par<SimpleName> name);


	/**
	 * This is the lookup for a scope. All lookups inherits from the standard one.
	 */
	class ScopeLookup : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR ScopeLookup();
		STORM_CTOR ScopeLookup(Par<Str> voidName);
		ScopeLookup(const String &voidName);

		// Find 'name' in 'in'.
		virtual MAYBE(Named) *STORM_FN find(const Scope &in, Par<SimpleName> name);

		// Resolve 'name' to a type.
		virtual Value STORM_FN value(const Scope &in, Par<SimpleName> name, SrcPos pos);

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
		// What is 'void' called in this language (if any)?
		String voidName;

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
		explicit STORM_CTOR Scope(Par<NameLookup> top);

		// Create a custom lookup.
		STORM_CTOR Scope(Par<NameLookup> top, Par<ScopeLookup> lookup);

		// Create a child scope.
		STORM_CTOR Scope(const Scope &parent, Par<NameLookup> top);

		// Create a child scope.
		inline Scope STORM_FN child(Par<NameLookup> top) { return Scope(*this, top); }

		// Topmost object (may be null).
		NameLookup *top;

		// Lookup object (may be null).
		Auto<ScopeLookup> lookup;

		// Find the given NameRef, either by using an absulute path or something relative to the
		// current object. NOTE: Not a STORM_FN since this function has to be executed on the
		// Compiler thread.
		MAYBE(Named) *find(Par<Name> name) const;
		MAYBE(Named) *find(Par<SimpleName> name) const;

		// Resolve a name to a type. Throws an exception on failure. Allows proper handling of void
		// and type aliases in the future.
		Value value(Par<Name> name, const SrcPos &pos) const;
		Value value(Par<SrcName> name) const;

		// Deep copy.
		void STORM_FN deepCopy(Par<CloneEnv> env);
	};

	// Output.
	wostream &operator <<(wostream &to, const Scope &scope);

	// Storm implementation of 'find'.
	MAYBE(Named) *STORM_FN find(Scope scope, Par<Name> name) ON(Compiler);
	MAYBE(Named) *STORM_FN find(Scope scope, Par<SimpleName> name) ON(Compiler);
	Value STORM_FN value(Scope scope, Par<Name> name, SrcPos pos) ON(Compiler);
	Value STORM_FN value(Scope scope, Par<SrcName> name) ON(Compiler);
	MAYBE(SimpleName) *STORM_FN simplify(Par<Name> name, Scope scope) ON(Compiler);
	MAYBE(SimplePart) *STORM_FN find(Par<NamePart> part, Scope scope) ON(Compiler);

	// Convert to string.
	Str *STORM_ENGINE_FN toS(EnginePtr e, Scope scope);


	/**
	 * Lookup with extra top-level finders.
	 */
	class ScopeExtra : public ScopeLookup {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ScopeExtra();
		STORM_CTOR ScopeExtra(Par<Str> voidName);
		ScopeExtra(const String &voidName);

		// Additional NameLookups to search (not recursively).
		vector<NameLookup *> extra;

		// Add an extra name lookup.
		void STORM_FN add(Par<NameLookup> l);

		// Find
		virtual MAYBE(Named) *STORM_FN find(const Scope &in, Par<SimpleName> name);

	};


	/**
	 * Get the root scope from the Engine.
	 */
	Scope STORM_ENGINE_FN rootScope(EnginePtr e) ON(Compiler);

}

#include "Named.h"
