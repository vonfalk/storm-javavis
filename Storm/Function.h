#pragma once
#include "Overload.h"
#include "Code/RefSource.h"

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
	 *
	 * TODO: Add concept of a lookup function, that is supposed to look
	 * up which function is to be executed. This is where vtable dispatch
	 * is implemented! Maybe this will be an explicit subclass of Function?
	 *
	 * TODO: Add concept of invalidators to at least LazyFn.
	 */
	class Function : public NameOverload {
		STORM_CLASS;
	public:
		// Create a function.
		Function(Value result, const String &name, const vector<Value> &params);

		// Dtor.
		~Function();

		// Function result.
		const Value result;

		// Get the code for this function. Do not assume it is static! Use
		// 'ref' if you are doing anything more than one function call!
		void *pointer();

		// Get the reference we are providing.
		code::RefSource &ref();

	protected:
		virtual void output(wostream &to) const;

		// Overload called when the reference is first needed.
		virtual void initRef(code::RefSource &ref) = 0;

	private:
		// The reference we are providing. Initialized when needed since we do not have enough
		// information in the constructor.
		code::RefSource *source;
	};


	/**
	 * Describes a pre-compiled (native) function. These objects are just a function pointer
	 * along with some metadata.
	 */
	class NativeFunction : public Function {
		STORM_CLASS;
	public:
		// Create a native function.
		NativeFunction(Value result, const String &name, const vector<Value> &params, void *ptr);

	protected:
		// Initref
		virtual void initRef(code::RefSource &ref);

	private:
		// Pointer to the pre-compiled function.
		void *fnPtr;
	};


	/**
	 * A function that will be generated when needed. Overload the 'update' function
	 * to make it work. Note that 'update' may be called more than once in case of
	 * something else causing the generated code to be invalidated (this is not yet
	 * implemented).
	 */
	class LazyFunction : public Function {
		STORM_CLASS;
	public:
		LazyFunction(Value result, const String &name, const vector<Value> &params);

	protected:
		// Initialize reference.
		void initRef(code::RefSource &ref);

	};

}
