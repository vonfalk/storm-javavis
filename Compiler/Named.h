#pragma once
#include "Object.h"
#include "NamedFlags.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;

	/**
	 * Interface for objects that can look up names.
	 */
	class NameLookup : public Object { // TODO: ObjectOn<Compiler>
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

		// Create with parameters (TODO).
		// STORM_CTOR Named(Str *name, Array<Value> *params);

		// Our name. Note: this can be null for a while when starting up the compiler.
		Str *name;

		// Our parameters (TODO).
		// Array<Value> *params;

		// Flags for this named object.
		NamedFlags flags;

		// Get an unique human-readable identifier for this named object.
		Str *STORM_FN identifier() const;

		// Better asserts for 'parent'.
		virtual NameLookup *STORM_FN parent() const;

		// String representation.
		virtual void STORM_FN toS(StrBuf *buf) const;
	};

}
