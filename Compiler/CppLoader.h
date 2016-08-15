#pragma once
#include "Core/Gen/CppTypes.h"
#include "RootArray.h"
#include "TemplateList.h"

namespace storm {

	/**
	 * Load objects that are defined in C++ somewhere.
	 */
	class CppLoader : NoCopy {
	public:
		// Create, note which set of functions to be loaded.
		CppLoader(Engine &e, const CppWorld *world, RootArray<Type> &types, RootArray<TemplateList> &templ);

		// Load all types into a RootArray. This makes it possible to create instances of these types from C++.
		void loadTypes();

		// Load all templates into a RootArray. This makes template instantiations possible.
		void loadTemplates();

		// Insert types and templates into their correct packages.
		void loadPackages();

	private:
		// Engine to load into.
		Engine &e;

		// Source.
		const CppWorld *world;

		// Load types into here.
		RootArray<Type> &types;

		// Load templates into here.
		RootArray<TemplateList> &templates;

		// Get the number of types.
		nat typeCount() const;

		// Get the number of templates.
		nat templateCount() const;

		// Create a gc type for the given type.
		GcType *createGcType(const CppType *type);
	};

}
