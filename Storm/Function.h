#pragma once
#include "Overload.h"

namespace storm {

	class Type;

	/**
	 * Describes a function in the compiler, either a member function or
	 * a free function. In the case of a member function, the 'this' pointer
	 * is explicitly stated as the first parameter.
	 */
	class Function : public NameOverload {
	public:
		// Create a function. No ownership of the Type * is taken.
		Function(Value result, const String &name, const vector<Value> &params);
		~Function();

		// Function name.
		const String name;

		// Function result.
		const Value result;

		// Function parameters.
		const vector<Value> params;

	protected:
		virtual void output(wostream &to) const;
	};

}
