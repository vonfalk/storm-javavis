#pragma once
#include "Core/EnginePtr.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Describes a value inside Storm.
	 */
	class Value {
		STORM_VALUE;
	public:
		// Create 'void'.
		STORM_CTOR Value();

		// Create from a specific type. Never a reference.
		STORM_CTOR Value(Type *type);

		// Create from a type and if it is to be a reference.
		STORM_CTOR Value(Type *type, Bool ref);

		// Deep copy.
		void deepCopy(CloneEnv *env);

		// Type we're referring to.
		MAYBE(Type*) type;

		// Reference type?
		Bool ref;

		// Make into a reference.
		Value STORM_FN asRef() const;
		Value STORM_FN asRef(Bool ref) const;

		// Same type?
		Bool STORM_FN operator ==(Value o) const;
		Bool STORM_FN operator !=(Value o) const;
	};

	// Create a this pointer for a type.
	Value STORM_FN thisPtr(Type *t);

	// Output.
	wostream &operator <<(wostream &to, const Value &v);
	StrBuf &STORM_FN operator <<(StrBuf &to, Value v);
	Str *STORM_FN toS(EnginePtr e, Value v);
}
