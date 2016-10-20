#pragma once
#include "Core/Gen/CppTypes.h"
#include "RootArray.h"
#include "NamedThread.h"
#include "World.h"

namespace storm {

	/**
	 * Load objects that are defined in C++ somewhere.
	 */
	class CppLoader : NoCopy {
	public:
		// Create, note which set of functions to be loaded.
		CppLoader(Engine &e, const CppWorld *world, World &into);

		// Load all types into a RootArray. This makes it possible to create instances of these types from C++.
		void loadTypes();

		// Load all threads.
		void loadThreads();

		// Set super types for all types here. Assumes threads are loaded.
		void loadSuper();

		// Load all templates into a RootArray. This makes template instantiations possible.
		void loadTemplates();

		// Insert everything into the packages where they belong.
		void loadPackages();

		// Create all functions in their appropriate places. Assumes types and threads are placed in
		// their packages.
		void loadFunctions();

	private:
		// Engine to load into.
		Engine &e;

		// Source.
		const CppWorld *world;

		// Destination.
		World &into;

		// Load named threads here. We only keep these while we're working.
		RootArray<NamedThread> threads;

		// Get the number of types.
		nat typeCount() const;

		// Get the number of templates.
		nat templateCount() const;

		// Get the number of named threads.
		nat threadCount() const;

		// Get the number of functions.
		nat functionCount() const;

		// Find a NameSet corresponding to a given name.
		NameSet *findPkg(const wchar *name);

		// Find a type as referred by a CppTypeRef.
		Value findValue(const CppTypeRef &ref);

		// Create a gc type for the CppType with id 'id'.
		GcType *createGcType(Nat id);

		// Load a single function.
		void loadFunction(const CppFunction &fn);

		// Load a free function.
		void loadFreeFunction(const CppFunction &fn);

		// Load member function.
		void loadMemberFunction(const CppFunction &fn);

		// Load parameters for a function.
		Array<Value> *loadFnParams(const CppTypeRef *params);
	};

}
