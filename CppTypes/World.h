#pragma once
#include "SrcPos.h"
#include "Type.h"
#include "Thread.h"
#include "Auto.h"
#include "NameMap.h"

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
	NameMap<Type> types;

	// All templates.
	NameMap<Template> templates;

	// All threads.
	NameMap<Thread> threads;

	// Built-in types (into C++).
	map<String, Size> builtIn;

	// ...

	// Add a type (and do the proper checking).
	void add(Auto<Type> type);

	// Prepare the world for serialization (ie. resolving types, ...).
	void prepare();

private:
	// Sort types so we get a deterministic order.
	void orderTypes();

	// Sort templates.
	void orderTemplates();

	// Sort threads so we get a deterministic order.
	void orderThreads();

	// Resolve all types in this world.
	void resolveTypes();
};
