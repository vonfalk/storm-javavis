#pragma once
#include "SrcPos.h"
#include "Lib/Auto.h"
#include "Code/Value.h"
#include "Code/Size.h"

namespace storm {

	class Type;
	class Engine;

	Type *boolType(Engine &e);
	Type *intType(Engine &e);
	Type *natType(Engine &e);

	/**
	 * A value is a 'handle' to a type. The value itself is to be considered
	 * a handle to any type in the system, possible along with modifiers. For
	 * example, if the pointer (if relevant) may be null.
	 * TODO: ValueT instead?
	 */
	class Value : public Printable {
	public:
		// Create the 'null' value.
		Value();

		// Create a value based on a type.
		Value(Type *type, bool ref = false);
		Value(Par<Type> type, bool ref = false);

		// The type referred.
		Type *type;

		// Reference? Not a good idea for return values.
		bool ref;

		// Get the size of this type.
		Size size() const;

		// As a reference.
		Value asRef(bool v = true) const;

		// Get the destructor for this type. A destructor has the signature void dtor(T).
		code::Value destructor() const;

		// Get the copy ctor for this type if there is any for this type.
		code::Value copyCtor() const;

		// Get the default ctor for this type if there is any.
		code::Value defaultCtor() const;

		// Get the assignment function for this type if there is any.
		code::Value assignFn() const;

		// Return on stack?
		bool returnOnStack() const;

		// Is this type built into the C++ compiler? (not pointers or references)
		bool isBuiltIn() const;

		// Refcounted value?
		bool refcounted() const;

		// Is it a value type (null is not a value type). Any built-in type does not count either.
		bool isValue() const;

		// Is it a class type?
		bool isClass() const;

		// Can this value store a type of 'x'?
		bool canStore(Type *x) const;
		bool canStore(const Value &v) const;

		// Ensure that we can store another type.
		void mustStore(const Value &v, const SrcPos &pos) const;

		// Any extra modifiers goes here:
		// TODO: implement

		// Type equality.
		bool operator ==(const Value &o) const;
		inline bool operator !=(const Value &o) const { return !(*this == o); }

		// Some standard types.
		static inline Value stdBool(Engine &e) { return Value(boolType(e)); }
		static inline Value stdInt(Engine &e) { return Value(intType(e)); }
		static inline Value stdNat(Engine &e) { return Value(natType(e)); }

		// Create a this-pointer for a type.
		static Value thisPtr(Type *t);

	protected:
		virtual void output(wostream &to) const;
	};

	/**
	 * Compute the common denominator of two values so that
	 * it is possible to cast both 'a' and 'b' to the resulting
	 * type. In case 'a' and 'b' are unrelated, Value() - void
	 * is returned.
	 */
	Value common(const Value &a, const Value &b);

	/**
	 * Template magic for finding the Value of a C++ type.
	 */

	template <class T>
	struct LookupValue {
		static Type *type(Engine &e) {
			return T::type(e);
		}
	};

	template <class T>
	struct LookupValue<T *> {
		static Type *type(Engine &e) {
			return T::type(e);
		}
	};

	template <class T>
	struct LookupValue<T &> {
		static Type *type(Engine &e) {
			return T::type(e);
		}
	};

	template <>
	struct LookupValue<Int> {
		static Type *type(Engine &e) {
			return intType(e);
		}
	};

	template <>
	struct LookupValue<Nat> {
		static Type *type(Engine &e) {
			return natType(e);
		}
	};

	template <>
	struct LookupValue<Bool> {
		static Type *type(Engine &e) {
			return boolType(e);
		}
	};

	template <class T>
	Value value(Engine &e) {
		bool isRef = TypeInfo<T>::reference() || TypeInfo<T>::pointer() || IsAuto<T>::v;
		Type *t = LookupValue<T>::type(e);
		if (t->flags & typeClass) {
			assert(("Class type tried to be used by value!", isRef));
			isRef = false;
		}
		return Value(t, isRef);
	}

	/**
	 * Various helper functions.
	 */
	class Scope;
	class Name;

	// Find the result type of a function call. Constructor calls are also handled.
	Value fnResultType(const Scope &scope, Par<Name> fn);

	// Array of values.
#ifdef VS
	vector<Value> valList(nat count, ...);
#else
#error "Define valList for C++11 here"
#endif

}
