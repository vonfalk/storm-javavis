#pragma once

#include "Utils/Path.h"
#include "SyntaxRule.h"


namespace storm {

	// Check if the path is a bnf file.
	bool isBnfFile(const Path &file);

	// Read syntax definitions from the given file, and merge them
	// into the given hash map. Throws an appropriate exception on
	// failure. Note that no ownership of the SyntaxRule is taken.
	void parseBnf(hash_map<String, SyntaxRule*> &types, const Path &file);

}
