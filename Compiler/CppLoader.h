#pragma once
#include "Core/Gen/CppTypes.h"
#include "Core/Io/Url.h"
#include "CppDoc.h"
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
		CppLoader(Engine &e, const CppWorld *world, World &into, MAYBE(Package *) root, MAYBE(Url *) docPath);

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

		// Create all exported variables in their appropriate places. Assumes types and threads are
		// placed in their packages.
		void loadVariables();

		// Create all functions in their appropriate places. Assumes types and threads are placed in
		// their packages and that variables are properly created (we might need type layout etc. in this step).
		void loadFunctions();

		// Create other metadata provided by this package (such as license and version information).
		void loadMeta();

	private:
		// Engine to load into.
		UNKNOWN(PTR_NOGC) Engine *e;

		// Source.
		UNKNOWN(PTR_NOGC) const CppWorld *world;

		// Destination.
		UNKNOWN(PTR_NOGC) World *into;

		// Assume all non-external package paths are relative to this package. Null means the system
		// root should be used.
		MAYBE(Package *) rootPackage;

		// Documentation file.
		MAYBE(Url *) docUrl;

		// Get the number of types.
		Nat typeCount() const;

		// Get the number of templates.
		Nat templateCount() const;

		// Get the number of named threads.
		Nat threadCount() const;

		// Get the number of functions.
		Nat functionCount() const;

		// Get the number of variables.
		Nat variableCount() const;

		// Get the number of enum values.
		Nat enumValueCount() const;

		// Get the number of licenses.
		Nat licenseCount() const;

		// Get the number of versions.
		Nat versionCount() const;

		// Get the number of source files.
		Nat sourceCount() const;

		// Create all licenses in their appropriate places.
		void loadLicenses();

		// Create all versions in their appropriate places.
		void loadVersions();

		// Find a NameSet corresponding to a given name.
		NameSet *findPkg(const wchar *name);

		// Find a NameSet corresponding to a given name, always relative to the root. Does not try to create packages.
		NameSet *findAbsPkg(const wchar *name);

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
		Array<Value> *loadFnParams(const CppParam *params);

		// Load a variable.
		void loadVariable(const CppVariable &var);

		// Load an enum value.
		void loadEnumValue(const CppEnumValue &val);

		// Load a template.
		TemplateList *loadTemplate(const CppTemplate &t);

		// Get a visibility object from the C++ access.
		Visibility *visibility(CppAccess a);

		// Create documentation. Returns null if not present for the current function.
		CppDoc *createDoc(Named *entity, Nat id, MAYBE(const CppParam *) params);
		void setDoc(Named *entity, Nat id, MAYBE(const CppParam *) params);

		// Set position information on a named entity.
		void setPos(Named *entity, CppSrcPos pos);

		// Create all sources.
		void createSources();

		// See if various types are external.
		inline bool external(const CppType &t) const { return typeKind(t) == CppType::tExternal; }
		inline bool external(const CppTemplate &t) const { return t.generate == null; }
		inline bool external(const CppThread &t) const { return t.external; }

		// See if a type shall be delayed.
		inline bool delayed(const CppType &t) const {
			CppType::Kind k = typeKind(t);
			return k == CppType::tCustom
				|| k == CppType::tEnum
				|| k == CppType::tBitmaskEnum;
		}
	};

	/**
	 * Exception thrown during boot, before the "real" exception support is active.
	 */
	class CppLoadError : public ::Exception {
	public:
		CppLoadError(const String &msg) : msg(msg) {}
		virtual String what() const {
			return msg;
		}
	private:
		String msg;
	};

}
