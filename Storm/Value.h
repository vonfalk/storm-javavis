#pragma once
#include "Shared/Value.h"
#include "NamedFlags.h"
#include "Shared/Types.h"
#include "Shared/Auto.h"
#include "Code/Value.h"
#include "Code/Size.h"
#include "Utils/TypeInfo.h"
#include "EnginePtr.h"

namespace storm {
	STORM_PKG(core.lang);

	class Type;
	class Engine;
	class SrcPos;
	class Str;
	class Handle;
	class ExprResult;

	Type *boolType(Engine &e);
	Type *intType(Engine &e);
	Type *natType(Engine &e);
	Type *byteType(Engine &e);

	/**
	 * A value is a 'handle' to a type. The value itself is to be considered
	 * a handle to any type in the system, possible along with modifiers. For
	 * example, if the pointer (if relevant) may be null.
	 *
	 * Note: This object is assumed to be binary compatible with SValue, no fields
	 * may therefore be added here. Add any fields in Shared/Value.h instead!
	 * TODO: ValueT instead?
	 */
	class Value : public STORM_IGNORE(ValueData) {
		STORM_VALUE;
	public:
		// Create the 'null' value.
		STORM_CTOR Value();

		// Convert from ValueData.
		Value(const ValueData &data);

		// Create a value based on a type.
		Value(Type *type, bool ref = false);
		STORM_CTOR Value(Par<Type> type);
		STORM_CTOR Value(Par<Type> type, Bool ref);

		// Get the type. TODO: Rename/replace with read-only version of 'type' member variable.
		// NOTE: This should probably be MAYBE(Type *)
		Type *STORM_FN getType() const;
		Bool STORM_FN isRef() const;

		// Get a handle for this type.
		const Handle &handle() const;

		// Get the size of this type.
		Size size() const;

		// Return value type.
		code::RetVal retVal() const;

		// As a reference.
		Value STORM_FN asRef() const;
		Value STORM_FN asRef(Bool v) const;

		// Get the destructor for this type. A destructor has the signature void dtor(T).
		code::Value destructor() const;

		// Get a direct reference for this type (if any). This means that no vtable will be used for
		// the destructor. This kind of reference is only usable when you know you do not have
		// inheritance, or when you know the exact runtime type (eg. in constructors/destructors).
		code::Value directDestructor() const;

		// Get the copy ctor for this type if there is any for this type.
		code::Value copyCtor() const;

		// Get the default ctor for this type if there is any.
		code::Value defaultCtor() const;

		// Get the assignment function for this type if there is any.
		code::Value assignFn() const;

		// Get the deep copy function.
		code::Value deepCopyFn() const;

		// Return in register? (builtIn + pointers and references)
		bool returnInReg() const;

		// Is this type built into the C++ compiler? (not pointers or references)
		bool isBuiltIn() const;

		// Is this a built-in float type?
		bool isFloat() const;

		// Refcounted value?
		bool refcounted() const;

		// Is it a value type (null is not a value type). Any built-in type does not count either.
		bool isValue() const;

		// Is it a class type?
		bool isClass() const;

		// Can this value store a type of 'x'?
		bool canStore(Type *x) const;
		bool canStore(const Value &v) const;

		// Note: if !v.any(), then we deem the operation to be successful, as that means that this
		// code will (probably) never be executed.
		bool canStore(const ExprResult &v) const;

		// Matches another value, according to MatchFlags? Note that this relation is not reflexive.
		// If no special flags are set, then it is equivalent to 'canStore'.
		bool matches(const Value &v, NamedFlags match) const;

		// Ensure that we can store another type.
		void mustStore(const Value &v, const SrcPos &pos) const;

		// Generate a TypeInfo describing this type. May be extended to the full TypeInfo in the future.
		BasicTypeInfo typeInfo() const;

		// Any extra modifiers goes here:
		// TODO: implement

		Bool STORM_FN operator ==(const Value &o) const;
		inline Bool STORM_FN operator !=(const Value &o) const { return !(*this == o); }

		// Some standard types.
		static inline Value stdBool(Engine &e) { return Value(boolType(e)); }
		static inline Value stdInt(Engine &e) { return Value(intType(e)); }
		static inline Value stdNat(Engine &e) { return Value(natType(e)); }

		// Create a this-pointer for a type.
		static Value thisPtr(Type *t);
		static Value thisPtr(Par<Type> t);

		// Deep copy.
		void STORM_FN deepCopy(Par<CloneEnv> env);
	};

	// Output.
	wostream &operator <<(wostream &to, const Value &from);

	// toS
	Str *STORM_ENGINE_FN toS(EnginePtr e, Value from);

	/**
	 * Compute the common denominator of two values so that
	 * it is possible to cast both 'a' and 'b' to the resulting
	 * type. In case 'a' and 'b' are unrelated, Value() - void
	 * is returned.
	 */
	Value STORM_FN common(Value a, Value b);


	/**
	 * Various helper functions.
	 */
	class Scope;
	class Name;

	// Find the result type of a function call. Constructor calls are also handled.
	Value fnResultType(const Scope &scope, Par<Name> fn);

	// Array of values.
	typedef vector<Value> ValList;

#ifdef VISUAL_STUDIO
	// This function uses a VS specific extension for variable arguments.
	ValList valList(nat count, ...);
#else
#error "Define valList for C++11 here"
#endif
}
