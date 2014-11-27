#pragma once
#include "RefSource.h"
#include "Listing.h"

namespace code {

	/**
	 * Redirect a function call to another function temporarily. This
	 * is useful when lazily compiling code. What this does is to replace
	 * the call foo(....) with:
	 * call = fn(param);
	 * return (*call)(...);
	 * where 'param' is pre-defined.
	 *
	 * TODO: Properly destroy parameters on exception!
	 */
	class Redirect {
	public:
		Redirect();

		// Add a parameter to the function.
		void param(nat size, Ref dtor);

		// Set return value.
		void result(nat size, bool builtIn);

		// Create the code (platform dependent).
		Listing code(const Value &fn, const Value &param);

	private:
		// All data we need about a parameter.
		struct Param {
			nat size;
			Ref dtor;
		};

		// Parameters.
		vector<Param> params;

		// Result size.
		nat resultSize;

		// Result built into the C++ compiler?
		bool resultBuiltIn;
	};

}
