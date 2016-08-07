#pragma once
#include "Core/Array.h"
#include "Thread.h"
#include "NamedFlags.h"
#include "Value.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;
	class SimpleName;

	/**
	 * Interface for objects that can look up names.
	 */
	class NameLookup : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR NameLookup();

		// TODO: add the find() api here!

		// Get the parent object to this lookup, or null if none.
		virtual NameLookup *STORM_FN parent() const;

		// Parent name lookup. This should be set by the parent. If it is null, the default 'parent'
		// implementation asserts. Therefore, root objects need to override 'parent' in order to
		// return null.
		NameLookup *parentLookup;
	};


	/**
	 * Denotes a named object in the compiler. Named objects are for example functions, types.
	 */
	class Named : public NameLookup {
		STORM_CLASS;
	public:
		// Create without parameters.
		STORM_CTOR Named(Str *name);

		// Create with parameters.
		STORM_CTOR Named(Str *name, Array<Value> *params);

		// Our name. Note: this can be null for a while when starting up the compiler.
		Str *name;

		// Our parameters. Note: this can be null for a while when starting up the compiler.
		Array<Value> *params;

		// Flags for this named object.
		NamedFlags flags;

		// Late initialization. Called when the type-system is up enough to initialize templates. Otherwise not needed.
		virtual void lateInit();

		// Get a path to this Named.
		SimpleName *path() const;

		// Get an unique human-readable identifier for this named object.
		Str *STORM_FN identifier() const;

		// Better asserts for 'parent'.
		virtual NameLookup *STORM_FN parent() const;

		// String representation.
		virtual void STORM_FN toS(StrBuf *buf) const;

	private:
		// Find closest named parent.
		Named *closestNamed() const;
	};

}
