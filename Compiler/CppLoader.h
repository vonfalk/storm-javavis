#pragma once
#include "Core/Gen/CppTypes.h"
#include "RootArray.h"
#include "NamedThread.h"
#include "World.h"

namespace storm {
	STORM_PKG(core.lang);

	class Package;

	/**
	 * Load objects that are defined in C++ somewhere.
	 */
	class CppLoader {
		STORM_VALUE;
	public:
		// Create, note which set of functions to be loaded.
		CppLoader(Engine &e, const CppWorld *world, World &into, Package *root);

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

		// Create all exported variables in their appropriate places. Assumes types and threads are
		// placed in their packages.
		void loadVariables();

	private:
		// Engine to load into.
		UNKNOWN(PTR_NOGC) Engine *e;

		// Source.
		UNKNOWN(PTR_NOGC) const CppWorld *world;

		// Destination.
		UNKNOWN(PTR_NOGC) World *into;

		// Assume all non-external package paths are relative to this package. Null means the system
		// root should be used.
		Package *rootPackage;

		// Get the number of types.
		nat typeCount() const;

		// Get the number of templates.
		nat templateCount() const;

		// Get the number of named threads.
		nat threadCount() const;

		// Get the number of functions.
		nat functionCount() const;

		// Get the number of variables.
		nat variableCount() const;

		// Get the number of enum values.
		nat enumValueCount() const;

		// Find a NameSet corresponding to a given name.
		NameSet *findPkg(const wchar *name);

		// Find a type as referred by a CppTypeRef.
		Value findValue(const CppTypeRef &ref);

		// Find the vtable for the type indicated.
		const void *findVTable(const CppTypeRef &ref);

		// De-virtualize a function wrt the functions first parameter.
		const void *deVirtualize(const CppTypeRef &ref, const void *fn);

		// Create a gc type for the CppType with id 'id'.
		GcType *createGcType(Nat id);

		// Create a new type based on the type description.
		Type *createType(Nat id, const CppType &type);

		// Find an external type based on the name given in the description.
		Type *findType(const CppType &type);

		// Load a single function.
		void loadFunction(const CppFunction &fn);

		// Load a free function.
		void loadFreeFunction(const CppFunction &fn);

		// Load member function.
		void loadMemberFunction(const CppFunction &fn, bool cast);

		// Load parameters for a function.
		Array<Value> *loadFnParams(const CppTypeRef *params);

		// Load a variable.
		void loadVariable(const CppVariable &var);

		// Load an enum value.
		void loadEnumValue(const CppEnumValue &val);

		// See if various types are external.
		inline bool external(const CppType &t) const { return t.kind == CppType::superExternal; }
		inline bool external(const CppTemplate &t) const { return t.generate == null; }
		inline bool external(const CppThread &t) const { return t.external; }
	};

}
