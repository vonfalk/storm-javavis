#pragma once
#include "Overload.h"

namespace storm {

	class Type;

	/**
	 * Describes a function in the compiler, either a member function or
	 * a free function. In the case of a member function, the 'this' pointer
	 * is explicitly stated as the first parameter.
	 * A function always causes a call, and is never inlined (so far at least).
	 * If inline code is desired look at InlineCode (TODO).
	 * This class is eventually supposed to act as a RefSource.
	 *
	 * Note that the "implicit" this-pointer is actually explicit here.
	 */
	class Function : public NameOverload {
	public:
		// Create a function.
		Function(Value result, const String &name, const vector<Value> &params);
		~Function();

		// Function result.
		const Value result;

		// Get the code for this function. Do not assume it is static!
		// This may be replaced by the corresponding API from RefSource.
		virtual void *pointer() const = 0;

	protected:
		virtual void output(wostream &to) const;
	};


	/**
	 * Describes a pre-compiled (native) function. These objects are just a function pointer
	 * along with some metadata.
	 */
	class NativeFunction : public Function {
	public:
		// Create a native function.
		NativeFunction(Value result, const String &name, const vector<Value> &params, void *ptr);

		// Get the fn pointer.
		virtual void *pointer() const;

	private:
		// Pointer to the pre-compiled function.
		void *fnPtr;
	};

}
