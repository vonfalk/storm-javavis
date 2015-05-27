#pragma once
#include "Lib/Object.h"
#include "Lib/TObject.h"
#include "Lib/Array.h"
#include "Thread.h"
#include "Name.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;

	/**
	 * An interface for objects that can lookup names.
	 */
	class NameLookup : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR NameLookup();

		// Find the specified NamePart in here, returns null if not found. BORROWED PTR.
		virtual Named *find(Par<NamePart> name);

		// Helper function for C++. (differently named due to the overloading rules in C++)
		Named *findCpp(const String &name, const vector<Value> &params);

		// Get the parent object to this lookup, or null if none.
		virtual NameLookup *parent() const;

		// Parent name lookup. This should be set by the parent. If it is null, the
		// default 'parent' implementation asserts. Therefore, root objects need to
		// override 'parent' in order to return null.
		NameLookup *parentLookup;

		// Set the parent lookup from Storm (for now...)
		virtual void STORM_FN setParent(Par<NameLookup> lookup);
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

		// Create with parameters.
		STORM_CTOR Named(Par<Str> name, Par<Array<Value>> params);

		// Create without parameters (C++)
		Named(const String &name);

		// Create with parameters.
		Named(const String &name, const vector<Value> &params);

		// Note: take care if replacing 'name' and 'params' with Storm classes. The
		// start-up process will have to be modified in that case!

		// Our name.
		const String name;

		// Our parameters.
		const vector<Value> params;

		// Flags.
		NamedFlags flags;

		// Full path.
		virtual Name *path() const;

		// Generate a unique, human-readable identifier (for use in Code api:s).
		virtual String identifier() const;

	private:
		// Find our closest named parent.
		Named *closestNamed() const;
	};

	// Get name from a Named.
	Str *STORM_FN name(Par<Named> named);
	Array<Value> *STORM_FN params(Par<Named> named);

}
