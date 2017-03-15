#pragma once
#include "SrcPos.h"
#include "Type.h"
#include "Thread.h"
#include "License.h"
#include "Auto.h"
#include "NameMap.h"

/**
 * Describes all types and functions known.
 */
class World : NoCopy {
public:
	// Create.
	World();

	// Using namespace declarations.
	vector<CppName> usingDecl;

	// Type aliases.
	map<CppName, CppName> aliases;

	// All types.
	NameMap<Type> types;

	// All templates.
	NameMap<Template> templates;

	// All threads.
	NameMap<Thread> threads;

	// All functions.
	vector<Function> functions;

	// Built-in types (into C++).
	map<String, Size> builtIn;

	// Licenses.
	vector<License> licenses;

	// ...

	// Add a type (and do the proper checking).
	void add(Auto<Type> type);

	// Prepare the world for serialization (ie. resolving types, ...).
	void prepare();

private:
	// Sort types so we get a deterministic order.
	void orderTypes();

	// Sort functions so we get a deterministic order.
	void orderFunctions();

	// Sort templates.
	void orderTemplates();

	// Sort threads so we get a deterministic order.
	void orderThreads();

	// Order licenses.
	void orderLicenses();

	// Resolve all types in this world.
	void resolveTypes();
};
