#pragma once
#include "Code.h"
#include "CodeGen.h"
#include "Thread.h"
#include "Utils/Bitmask.h"

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
	class Function : public Named {
		STORM_CLASS;
	public:
		// Create a function.
		Function(Value result, const String &name, const vector<Value> &params);

		// Dtor.
		~Function();

		// Function result.
		const Value result;

		// Run this function on? Defaults to the specifier of the type if it is a member,
		// otherwise nothing.
		virtual RunOn runOn() const;

		// Get the code for this function. Do not assume it is static! Use
		// 'ref' if you are doing anything more than one function call!
		void *pointer();

		// Get the reference we are providing. This reference will always
		// refer some lookup function if that is needed for this function
		// call. It will be updated to match as well.
		code::RefSource &ref();

		// Get the reference, bypassing any lookup functionality. In general, it
		// is better to use 'ref', but in some cases (such as implementing the lookup)
		// this reference is needed.
		code::RefSource &directRef();

		// Generate code for this function call. If 'thread' is something other than code::Value(),
		// the function will be used on the thread indicated there instead.
		typedef vector<code::Value> Actuals;
		void genCode(const GenState &to, const Actuals &params, GenResult &result,
					const code::Value &thread = code::Value(), bool useLookup = true);

		// Code to be executed.
		void setCode(Par<Code> code);

		// Set lookup code. (default one provided!, use 'null' to restore default).
		void setLookup(Par<Code> lookup);

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

		// Generate code for a direct function call.
		void genCodeDirect(const GenState &to, const Actuals &params, GenResult &result, code::Ref ref);

		// Generate code for an inlined function call.
		void genCodeInline(const GenState &to, const Actuals &params, GenResult &result);

		// Generate code for an indirect function call, ie post it to another thread.
		void genCodePost(const GenState &to, const Actuals &params, GenResult &result,
						code::Ref ref, const code::Value &thread);
	};

	// Determine if 'a' is an overload of 'b'.
	bool isOverload(Function *base, Function *overload);

	// Create a function referring a pre-compiled function.
	Function *nativeFunction(Engine &e, Value result, const String &name, const vector<Value> &params, void *ptr);

	// Create a function referring a pre-compiled function that is possibly using vtable calls.
	Function *nativeMemberFunction(Engine &e, Type *member, Value result,
								const String &name, const vector<Value> &params,
								void *ptr);

	// Create a function referring a pre-compiled destructor (using vtable calls).
	Function *nativeDtor(Engine &e, Type *member, void *ptr);

	// Create an inlined function.
	Function *inlinedFunction(Engine &e, Value result, const String &name,
							const vector<Value> &params, Fn<void, InlinedParams> fn);
}
