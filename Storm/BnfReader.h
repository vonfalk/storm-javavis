#pragma once

#include "Utils/Path.h"
#include "SyntaxType.h"


namespace storm {

	// Check if the path is a bnf file.
	bool isBnfFile(const Path &file);

	// Read syntax definitions from the given file, and merge them
	// into the given hash map. Throws an appropriate exception on
	// failure. Note that no ownership of the SyntaxType is taken.
	void parseBnf(hash_map<String, SyntaxType*> &types, const Path &file);

}
