#pragma once
#include "Code.h"
#include "CodeGenFwd.h"
#include "Thread.h"
#include "NamedThread.h"
#include "Utils/Bitmask.h"
#include "OS/FnCall.h"

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
	 * TODO: Add concept of invalidators to at least LazyFn.
	 */
	class Function : public Named {
		STORM_CLASS;
	public:
		// Create a function.
		Function(Value result, const String &name, const vector<Value> &params);

		// Create from Storm.
		STORM_CTOR Function(Value result, Par<Str> name, Par<Array<Value>> params);

		// Dtor.
		~Function();

		// Function result.
		const Value result;

		// Easy call of functions. No support for references yet.
		template <class Result>
		inline Result call(const os::FnParams &params = os::FnParams()) {
			return os::call<Result>(pointer(), isMember(), params);
		}

		// Get the code for this function. Do not assume it is static! Use
		// 'ref' if you are doing anything more than one function call!
		void *pointer();

		// Is this a member function?
		bool isMember();

		// Get the reference we are providing. This reference will always
		// refer some lookup function if that is needed for this function
		// call. It will be updated to match as well.
		code::RefSource &ref();

		// Get the reference, bypassing any lookup functionality. In general, it
		// is better to use 'ref', but in some cases (such as implementing the lookup)
		// this reference is needed.
		code::RefSource &directRef();

		// Actual parameters typedef.
		typedef vector<code::Value> Actuals;

		// Run this function on? Defaults to the specifier of the type if it is a member,
		// otherwise nothing.
		virtual RunOn runOn() const;

		// Set this function to run on a specific thread. If the function is a member of a
		// type, this directive is overridden.
		void runOn(Par<NamedThread> thread);

		// Output code to find the Thread we want to run on. Always returns a borrowed reference.
		// Does not generate any meaningful result unless 'runOn' returns a state other than 'any'.
		virtual code::Variable findThread(Par<CodeGen> to, const Actuals &params);

		// Figure out which kind of call we shall use and do the right thing based on what the 'runOn' member
		// in 'to' is indicating. Always uses lookup.
		void autoCall(Par<CodeGen> to, const Actuals &params, Par<CodeResult> result);

		// Generate code for this function call, assuming we are performing the call the same thread as the
		// currently running thread. if 'useLookup' is false, we will not use the lookup function (ie VTables).
		void localCall(Par<CodeGen> to, const Actuals &params, Par<CodeResult> result, bool useLookup);

		// Generate code for this function call, assuming we want to run on a different thread.
		void threadCall(Par<CodeGen> to,
						const Actuals &params,
						Par<CodeResult> result,
						const code::Value &thread = code::natPtrConst(0));

		// Generate code for this function call, assuming we want to run on a different thread, returning a Future
		// object.
		void asyncThreadCall(Par<CodeGen> to,
							const Actuals &params,
							Par<CodeResult> result,
							const code::Value &thread = code::natPtrConst(0));

		// Code to be executed.
		void STORM_FN setCode(Par<Code> code);

		// Set lookup code. (default one provided!, use 'null' to restore default).
		void setLookup(Par<Code> lookup);

		// Forcefully compile this function.
		void STORM_FN compile();

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

		// Helper for calling this function from another thread. This takes care
		// of refcounting parameters and copying return values.
		code::RefSource *threadThunkRef;
		code::Binary *threadThunkCode;

		// Get the thread thunk, generates it if needed. Returns null if no thunk is needed.
		code::RefSource *threadThunk();

		// Forward parameter #id for the thread thunk.
		void threadThunkParam(nat id, code::Listing &to);

		// Run on a specific thread.
		Auto<NamedThread> runOnThread;

		// Result from the prepare call.
		struct PrepareResult {
			code::Variable params, data;
		};

		// Generate the code that is shared between 'threadCall' and 'asyncThreadCall'. This is
		// basically setting up the parameters for the call.
		PrepareResult prepareThreadCall(Par<CodeGen> to, const Actuals &params);

		// Initialize references if needed.
		void initRefs();

		// Generate code for a direct function call.
		void localCall(Par<CodeGen> to, const Actuals &params, Par<CodeResult> result, code::Ref ref);

		// Generate code for an indirect function call, ie post it to another thread.
		void genCodePost(Par<CodeGen> to, const Actuals &params, Par<CodeResult> result,
						code::Ref ref, const code::Value &thread);

		// Add parameters for the function call.
		void addParams(Par<CodeGen> to, const Actuals &params, const code::Variable &resultIn);
		void addParam(Par<CodeGen> to, const Actuals &params, nat id);
	};

	/**
	 * Native function, contains the original VTable slot as well!
	 */
	class NativeFunction : public Function {
		STORM_CLASS;
	public:
		NativeFunction(Value result, const String &name, const vector<Value> &params, nat slot);

		// The vtable slot of this function.
		const nat vtableSlot;
	};


	// Determine if 'a' is an overload of 'b'.
	bool isOverload(Function *base, Function *overload);

	// Create a function referring a pre-compiled function.
	Function *nativeFunction(Engine &e, Value result, const String &name, const vector<Value> &params, void *ptr);

	// Create a function referring a pre-compiled function, adding 'EnginePtr' as the first parameter.
	Function *nativeEngineFunction(Engine &e, Value result, const String &name, const vector<Value> &params, void *ptr);

	// Create a function referring a pre-compiled function that is possibly using vtable calls.
	Function *nativeMemberFunction(Engine &e, Type *member, Value result,
								const String &name, const vector<Value> &params,
								void *ptr);

	// Create a function referring a pre-compiled destructor (using vtable calls).
	Function *nativeDtor(Engine &e, Type *member, void *ptr);

	// Create an inlined function.
	Function *inlinedFunction(Engine &e, Value result, const String &name,
							const vector<Value> &params, Fn<void, InlinedParams> fn);

	// Create a dynamic function.
	Function *dynamicFunction(Engine &e, Value result, const String &name,
							const vector<Value> &params, const code::Listing &listing);

	// Create a lazy function.
	Function *lazyFunction(Engine &e, Value result, const String &name,
						const vector<Value> &params, Par<FnPtr<CodeGen *>> generate);

	// Steal and add the autocast flag.
	inline Auto<Function> stealAutoCast(Function *ptr) { ptr->flags |= namedAutoCast; return Auto<Function>(ptr); }
}
