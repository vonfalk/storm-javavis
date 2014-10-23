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

		// Type member.
		String classMember;

		// Name of the storm function.
		String name;

		// Function shall be located in this package.
		String package;

		// Result type.
		String result;

		// Types of parameters to the function.
		vector<String> params;
	};

	/**
	 * Describes an exported class.
	 */
	struct Type {
		// Name of the class.
		String name;

		// Superclass (if any).
		String super;

		// Package.
		String package;

		// C++ name (fully qualified).
		String cppName;
	};


	/**
	 * Contents of a file.
	 */
	struct File {
		// Functions.
		vector<Function> fns;

		// Types
		vector<Type> types;

		// Add another 'file' to this one.
		void add(const File &o);
	};

	wostream &operator <<(wostream &to, const Function &fn);

	// Find all functions in a file.
	File parseFile(const Path &file);

}
