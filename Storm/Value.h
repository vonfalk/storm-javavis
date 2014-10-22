#pragma once

namespace storm {

	class Type;

	/**
	 * A value is a 'handle' to a type. The value itself is to be considered
	 * a handle to any type in the system, possible along with modifiers. For
	 * example, if the pointer (if relevant) may be null.
	 * TODO: ValueType instead?
	 */
	class Value : public Printable {
	public:
		// Create the 'null' value.
		Value();

		// Create a value based on a type.
		Value(Type *type);

		// The type referred.
		Type *type;

		// Any extra modifiers goes here:
		// TODO: implement

		// Type equality.
		bool operator ==(const Value &o) const;
		inline bool operator !=(const Value &o) const { return !(*this == o); }

		// Define some ordering, which allows us to be inside std::map's
		bool operator <(const Value &o) const;
	protected:
		virtual void output(wostream &to) const;
	};


	/**
	 * Various helper functions.
	 */
	class Scope;
	class Name;

	// Find the result type of a function call. Constructor calls are also handled.
	Value fnResultType(Scope *scope, const Name &fn, const vector<Value> &params);

}
