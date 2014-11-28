#pragma once
#include "SrcPos.h"
#include "Lib/Auto.h"
#include "Code/Value.h"

namespace storm {

	class Type;

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
		Value(Type *type);

		// The type referred.
		Type *type;

		// Get the size of this type (always returns 0 for pointers, like Code does).
		nat size() const;

		// Get the destructor for this type. A destructor has the signature void dtor(T).
		code::Value destructor() const;

		// Is this type built into the C++ compiler?
		bool isBuiltIn() const;

		// Can this value store a type of 'x'?
		bool canStore(Type *x) const;
		bool canStore(const Value &v) const;

		// Ensure that we can store.
		void mustStore(const Value &v, const SrcPos &pos) const;

		// Any extra modifiers goes here:
		// TODO: implement

		// Type equality.
		bool operator ==(const Value &o) const;
		inline bool operator !=(const Value &o) const { return !(*this == o); }

	protected:
		virtual void output(wostream &to) const;
	};


	/**
	 * Various helper functions.
	 */
	class Scope;
	class Name;

	// Find the result type of a function call. Constructor calls are also handled.
	Value fnResultType(Auto<Scope> scope, const Name &fn, const vector<Value> &params);

}
