#pragma once
#include "SrcPos.h"
#include "Type.h"

/**
 * Describes all types and functions known.
 */
class World {
public:
	// All types.
	map<CppName, Type> types;

	// ...
};

// Parse all files in SrcPos::types and return what we found.
World parseWorld();
