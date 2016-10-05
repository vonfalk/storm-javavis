#pragma once
#include "Named.h"
#include "RunOn.h"
#include "Code.h"
#include "Code/Reference.h"

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
		code::RefSource *STORM_FN ref();

		// Get a reference directly to the underlying function, bypassing any vtable lookups or
		// similar. Mainly used for implementing super-calls.
		code::RefSource *STORM_FN directRef();

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

		// Set lookup code. A default one is provided, use null to restore it.
		void STORM_FN setLookup(MAYBE(Code *) lookup);

		// Forcefully compile this function.
		void STORM_FN compile();

		/**
		 * Generate calls to this function.
		 */

		// TODO: add autoCall, localCall, threadCall and asyncThreadCall.

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
	};


	// Conveniently create various kind of functions.
	// Function *inlinedFunction(EnginePtr e, Value result, const wchar *name, Array<Value> *params, );
	Function *nativeFunction(EnginePtr e, Value result, const wchar *name, Array<Value> *params, const void *fn);

}
