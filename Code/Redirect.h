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
	 * Note: the cleanup function is only run if an exception is throw, not during
	 * normal execution!
	 */
	class Redirect {
	public:
		Redirect();

		// Add a parameter to the function.
		void param(Size size, Value dtor, bool byPtr);

		// Set return value. Built in types are returned in registers on most platforms.
		void result(Size size, bool builtIn);

		// Create the code (platform dependent).
		Listing code(const Value &fn, bool memberFn, const Value &param);

	private:
		// All data we need about a parameter.
		struct Param {
			Size size;
			Value dtor;
			bool byPtr;
		};

		// Parameters.
		vector<Param> params;

		// Result size.
		Size resultSize;

		// Result built into the C++ compiler?
		bool resultBuiltIn;

		// Add a single parameter?
		void addParam(code::Listing &to, const Param &p);
	};

}
