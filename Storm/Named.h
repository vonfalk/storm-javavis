#pragma once
#include "Lib/Object.h"
#include "Name.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;

	/**
	 * An interface for objects that can lookup names.
	 */
	class NameLookup : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR NameLookup();

		// Find the specified NamePart in here, returns null if not found. BORROWED PTR.
		inline Named *find(Par<NamePart> name) { return findHere(name->name, name->params); }
		inline Named *find(const String &name, const vector<Value> &params) { return findHere(name, params); }

		// Get the parent object to this lookup, or null if none.
		virtual NameLookup *parent() const;

		// Parent name lookup. This should be set by the parent. If it is null, the
		// default 'parent' implementation asserts. Therefore, root objects need to
		// override 'parent' in order to return null.
		NameLookup *parentLookup;

	protected:
		// Find something here. This one is called by both the public overloads.
		virtual Named *findHere(const String &name, const vector<Value> &params);
	};


	/**
	 * Denotes a named object in the compiler. Named objects
	 * are anything in the compiler with a name, eg functions,
	 * types among others.
	 */
	class Named : public NameLookup {
		STORM_CLASS;
	public:
		// Create without parameters.
		STORM_CTOR Named(Par<Str> name);

		// Create without parameters (C++)
		Named(const String &name);

		// Create with parameters.
		Named(const String &name, const vector<Value> &params);

		// Our name.
		const String name;

		// Our parameters.
		const vector<Value> params;

		// Match flags.
		MatchFlags matchFlags;

		// Full path.
		virtual Name *path() const;

		// Generate a unique, human-readable identifier (for use in Code api:s).
		virtual String identifier() const;

	private:
		// Find our closest named parent.
		Named *closestNamed() const;
	};

}
