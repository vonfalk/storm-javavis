#pragma once
#include "SrcPos.h"
#include "Type.h"
#include "Auto.h"

/**
 * Describes all types and functions known.
 */
class World {
public:
	// Create.
	World();

	// Using namespace declarations.
	vector<CppName> usingDecl;

	// All types.
	vector<Auto<Type>> types;

	// Quick lookup of all types.
	map<CppName, nat> typeLookup;

	// Built-in types (into C++).
	map<String, Size> builtIn;

	// ...

	// Add a type (and do the proper checking).
	void add(Auto<Type> type);

	// Prepare the world for serialization (ie. resolving types, ...).
	void prepare();

	// Find a type.
	Type *findType(const CppName &name, const CppName &context, const SrcPos &pos);
	Type *findTypeUnsafe(const CppName &name, CppName context);

private:
	// Sort types so we get a deterministic order.
	void orderTypes();

	// Resolve all types in this world.
	void resolveTypes();
};

// Parse all files in SrcPos::types and return what we found.
World parseWorld();
