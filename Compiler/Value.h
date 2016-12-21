#pragma once
#include "Core/EnginePtr.h"
#include "Code/ValType.h"
#include "Code/Operand.h"
#include "NamedFlags.h"

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

		// Can this value store a type of 'x'?
		Bool STORM_FN canStore(Type *x) const;
		Bool STORM_FN canStore(Value v) const;

		// Does this value match another value according to NamedFlags? Note that this relation is not reflexive.
		// If no special flags are set, then it is equivalent to 'canStore'.
		Bool STORM_FN matches(Value v, NamedFlags flags) const;

		/**
		 * Code generation information.
		 */

		// Return this type in a register?
		Bool STORM_FN returnInReg() const;

		// Get a ValType representing this type.
		code::ValType STORM_FN valType() const;

		// Is this type a floating-point type?
		Bool STORM_FN isFloat() const;

		// Is this a value type, a class, an actor or built in? These are mutually exclusive, ie. at most one is true.
		Bool STORM_FN isValue() const;
		Bool STORM_FN isClass() const;
		Bool STORM_FN isActor() const;
		// Is this a built-in type? (eg. Int, Float, etc.)
		Bool STORM_FN isBuiltIn() const;

		// Combination of 'isClass' and 'isActor'.
		Bool STORM_FN isHeapObj() const;

		// Is this some kind of pointer?
		Bool STORM_FN isPtr() const;

		// The size of this type.
		Size STORM_FN size() const;

		// Get type info for this type.
		BasicTypeInfo typeInfo() const;

		/**
		 * Access to common member functions.
		 */

		// Get an Operand pointing to the copy constructor for this type. Only returns something
		// useful for value types, as other types can and shall be copied using an inline
		// mov-instruction.
		code::Operand STORM_FN copyCtor() const;

		// Get an Operand pointing to the destructor for this type. May return Operand(), meaning no
		// destructor is needed.
		code::Operand STORM_FN destructor() const;
	};

	/**
	 * Compute the common denominator of two values so that
	 * it is possible to cast both 'a' and 'b' to the resulting
	 * type. In case 'a' and 'b' are unrelated, Value() - void
	 * is returned.
	 */
	Value STORM_FN common(Value a, Value b);

	// Create a this pointer for a type.
	Value STORM_FN thisPtr(Type *t);

	// Output.
	wostream &operator <<(wostream &to, const Value &v);
	StrBuf &STORM_FN operator <<(StrBuf &to, Value v);
	Str *STORM_FN toS(EnginePtr e, Value v);

	// Generate a list of values.
#ifdef VISUAL_STUDIO
	// Uses a visual studio specific extension...
	Array<Value> *valList(Engine &e, Nat count, ...);
#else
#error "Define valList for C++11 here!"
#endif
}
