#pragma once
#include "ValType.h"
#include "Operand.h"
#include "Listing.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A redirect is a piece of code which 'catches' the function call in flight and asks another
	 * function how to create the actual function to be called.
	 *
	 * This is useful when implementing lazy compilation. When a function is needed, a redirect is
	 * created instead of the real function. The first time the redirect is called, it will call
	 * another function to figure out which function to actually call. This function may compile the
	 * code, or just look the proper piece of code up from a cache or similar. This function is free
	 * to replace the redirect in the RefSource with the actual function, which causes the overhead
	 * of the redirect to disappear the next time the function is called.
	 *
	 * Note: the cleanup functions provided here will only be executed if an exception is thrown
	 * during the call to determine the actual function.
	 */

	class RedirectParam {
		STORM_VALUE;
	public:
		STORM_CTOR RedirectParam(ValType val);
		STORM_CTOR RedirectParam(ValType val, Operand freeFn, Bool byPtr);

		ValType val;
		Bool byPtr; // Free by pointer?
		Operand freeFn;
	};


	/**
	 * Create the redirect itself. Calls 'fn' with 'param' as parameter to compute the actual
	 * function to call.
	 *
	 * Note: if you are trying to return a non-built-in object, a pointer to the destination memory
	 * location is usually passed as the first parameter. The redirect does not handle this by itself.
	 */
	Listing *STORM_FN redirect(Array<RedirectParam> *params, Operand fn, Operand param);

}
