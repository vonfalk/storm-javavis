#pragma once
#include "Overload.h"
#include "Code.h"
#include "CodeGen.h"

namespace storm {
	STORM_PKG(core.lang);

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

		// Get the reference we are providing. This reference will always
		// refer some lookup function if that is needed for this function
		// call. It will be updated to match as well.
		code::RefSource &ref();

		// Generate code for this function call.
		void genCode(GenState to, const vector<code::Value> &params, code::Value result);

		// Code to be executed.
		void setCode(Auto<Code> code);

		// Set lookup code. (default one provided!)
		void setLookup(Auto<Code> lookup);

	protected:
		virtual void output(wostream &to) const;

	private:
		// Two references. One is for the lookup function, and one is for the actual
		// code to be executed. Initialized when needed because of lack of information in the ctor.
		code::RefSource *lookupRef;
		code::RefSource *codeRef;

		// Current code to be executed.
		Auto<Code> code;
		Auto<Code> lookup;

		// Initialize references if needed.
		void initRefs();
	};


	// Create a function referring a pre-compiled function.
	Function *nativeFunction(Engine &e, Value result, const String &name, const vector<Value> &params, void *ptr);

	// Create an inlined function.
	Function *inlinedFunction(Engine &e, Value result, const String &name,
							const vector<Value> &params, Fn<void, InlinedParams> fn);
}
