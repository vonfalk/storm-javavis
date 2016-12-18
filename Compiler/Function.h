#pragma once
#include "Named.h"
#include "RunOn.h"
#include "Code.h"
#include "Code/Reference.h"
#include "Core/Future.h"
#include "OS/Future.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Describes a function in the compiler, either a member function or a free function. In the
	 * case of a member function, the 'this' pointer is explicitly stated as the first parameter.
	 */
	class Function : public Named {
		STORM_CLASS;
	public:
		// Create a function.
		STORM_CTOR Function(Value result, Str *name, Array<Value> *params);

		// Function result.
		const Value result;

		// Get the code for this function. Do not assume this pointer is static! For anything else
		// than making a call in the same function, use a proper reference instead.
		const void *pointer();

		// Get a reference to the code for this function.
		code::Ref STORM_FN ref();

		// Get a reference directly to the underlying function, bypassing any vtable lookups or
		// similar. Mainly used for implementing super-calls.
		code::Ref STORM_FN directRef();

		// Is this a member function?
		Bool STORM_FN isMember();

		// Which thread shall we run on?
		virtual RunOn STORM_FN runOn() const;

		// Set where to run. Ignored if we're a member of a class.
		void STORM_FN runOn(NamedThread *thread);

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		/**
		 * Code.
		 */

		// Set code.
		void STORM_FN setCode(Code *code);

		// Get the code in use.
		MAYBE(Code *) STORM_FN getCode() const;

		// Set lookup code. A default one is provided, use null to restore it.
		void STORM_FN setLookup(MAYBE(Code *) lookup);

		// Forcefully compile this function.
		void STORM_FN compile();

		/**
		 * Generate calls to this function.
		 */

		// Output code to find the Thread we want to run on. Always returns a borrowed reference.
		// Does not generate any meaningful result unless 'runOn' returns a state other than 'any'.
		virtual code::Var STORM_FN findThread(CodeGen *to, Array<code::Operand> *params);

		// Figure out which kind of call we shall use and do the right thing based on what the
		// 'runOn' member in 'to' is indicating. Always uses lookup. Parameters passed are as
		// follows: If the type of the parameter is a class, the parameter is a pointer to the
		// class, if the type is a value, the parameter is a pointer to the value or a variable
		// containing the value; the value will be copied to the function. If the type is a built in
		// type, the parameter is the actual value.
		void STORM_FN autoCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result);

		// Generate code for this function call, assuming we are performing the call the same thread as the
		// currently running thread. if 'useLookup' is false, we will not use the lookup function (ie VTables).
		void STORM_FN localCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, Bool useLookup);

		// Generate code for this function call, assuming we want to run on a different thread.
		void STORM_FN threadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Operand thread);

		// Generate code for this function call, assuming we want to run on a different thread,
		// returning a Future object. If 'thread' is left out, the current thread is used.
		void STORM_FN asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Operand thread);
		void STORM_FN asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result);

	private:
		// Two references. One is for the lookup function, and one is for the actual code to be
		// executed. Initialized when needed due to lack of information in the ctor.
		code::RefSource *lookupRef;
		code::RefSource *codeRef;

		// Stored code.
		Code *lookup;
		Code *code;

		// Thread we shall be running on:
		NamedThread *runOnThread;

		// Initialize references.
		void initRefs();

		// Helper for calling this function from another thread. This takes care
		// of refcounting parameters and copying return values.
		code::RefSource *threadThunkRef;
		code::Binary *threadThunkCode;

		// Get the thread thunk, generates it if needed. Returns null if no thunk is needed.
		code::RefSource *threadThunk();

		// Forward parameter #id for the thread thunk.
		void threadThunkParam(nat id, code::Listing *to);

		// Result from the prepare call.
		struct PrepareResult {
			code::Var params;
			code::Var data;
		};

		// Generate the code that is shared between 'threadCall' and 'asyncThreadCall'. This is
		// basically setting up the parameters for the call.
		PrepareResult prepareThreadCall(CodeGen *to, Array<code::Operand> *params);

		// Generate code for a direct function call.
		void localCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Ref ref);

		// Add parameters for the function call.
		void addParams(CodeGen *to, Array<code::Operand> *params, code::Var resultIn);
		void addParam(CodeGen *to, Array<code::Operand> *params, nat id);
	};


	// Conveniently create various kind of functions.
	Function *inlinedFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, Fn<void, InlineParams> *fn);
	Function *nativeFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, const void *fn);
	Function *nativeEngineFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, const void *fn);
	Function *lazyFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, Fn<CodeGen *> *generate);


	// Helpers used by the generated code.
	void spawnThreadResult(const void *fn, bool member, const os::FnParams *params, void *result,
						BasicTypeInfo *resultType, Thread *on, os::UThreadData *data);
	// void spawnThreadFuture(const void *fn, bool member, const os::FnParams *params, FutureBase *result,
	// 					BasicTypeInfo *resultType, Thread *on, os::UThreadData *data);

}
