#pragma once

#include "Utils/Path.h"


namespace stormbuiltin {

	/**
	 * Describes a single function that is to be exported.
	 */
	struct Function {
		// Name of the cpp function.
		String cppName;

		// Header file this function is located in.
		Path header;

		// Name of the storm function.
		String name;

		// Function shall be located in this package.
		String package;

		// Result type.
		String result;

		// Types of parameters to the function.
		vector<String> params;
	};

	wostream &operator <<(wostream &to, const Function &fn);

	// Find all functions in a file.
	vector<Function> parseFile(const Path &file);

}
