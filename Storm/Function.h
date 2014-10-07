#pragma once
#include "Named.h"

namespace storm {

	class Type;

	/**
	 * Describes a function in the compiler, either a member function or
	 * a free function. In the case of a member function, the 'this' pointer
	 * is explicitly stated as the first parameter.
	 */
	class Function : public Named {
	public:
		// Create a function. No ownership of the Type * is taken.
		Function(Type *result, const String &name, const vector<Type*> &params);
		~Function();

		// Function name.
		const String name;

		// Function result.
		Type *const result;

		// Function parameters.
		const vector<Type*> params;

	protected:
		virtual void output(wostream &to) const;
	};

}
