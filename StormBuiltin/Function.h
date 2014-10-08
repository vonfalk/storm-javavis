#pragma once

#include "Utils/Path.h"


namespace stormbuiltin {

	/**
	 * Describes a single function that is to be exported.
	 */
	class Function {
	public:
		String cppName;
		Path header;
		String name;
		String package;
		vector<String> params;
	};

	// Find all functions in a file.
	vector<Function> parseFile(const Path &file);

}
