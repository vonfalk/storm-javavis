#pragma once
#include "Core/EnginePtr.h"
#include "Code/Operand.h"
#include "Code/TypeDesc.h"
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

		// Is this a value type, a class, an actor or built in? These are mutually exclusive, ie. at most one is true.
		Bool STORM_FN isValue() const;
		Bool STORM_FN isClass() const;
		Bool STORM_FN isActor() const;
		// Is this a built-in type? (eg. Int, Float, etc.)
		// TODO: Rename to 'isPrimitive'.
		Bool STORM_FN isBuiltIn() const;

		// Combination of 'isClass' and 'isActor'.
		Bool STORM_FN isHeapObj() const;

		// Is this some kind of pointer?
		Bool STORM_FN isPtr() const;

		// The size of this type.
		Size STORM_FN size() const;

		// Get type info for this type.
		BasicTypeInfo typeInfo() const;

		// Get a type description of this type.
		code::TypeDesc *desc(Engine &e) const;

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

	// Get a description of this type.
	code::TypeDesc *STORM_FN desc(EnginePtr e, Value v);


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

	// Generate a list of values.
#ifdef VISUAL_STUDIO
	// Uses a visual studio specific extension...
	Array<Value> *valList(Engine &e, Nat count, ...);
#elif defined(USE_VA_TEMPLATE)
	template <class... Val>
	void valListAdd(Array<Value> *to, const Value &first, const Val&... rest) {
		to->push(first);
		valListAdd(to, rest...);
	}

	inline void valListAdd(Array<Value> *to) {
		UNUSED(to);
	}

	template <class... Val>
	Array<Value> *valList(Engine &e, Nat count, const Val&... rest) {
		assert(sizeof...(rest) == count, L"'count' does not match the number of parameters passed to 'valList'.");
		Array<Value> *v = new (e) Array<Value>();

		valListAdd(v, rest...);

		return v;
	}
#else
#error "Can not implement 'valList'"
#endif
}
