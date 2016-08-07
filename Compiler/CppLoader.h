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
		CppLoader(Engine &e, const CppWorld *world);

		// Load all types into a RootArray. This makes it possible to create instances of these types from C++.
		void loadTypes(RootArray<Type> &into);

		// Load all templates into a RootArray. This makes template instantiations possible.
		void loadTemplates(RootArray<TemplateList> &into);

	private:
		// Engine to load into.
		Engine &e;

		// Source.
		const CppWorld *world;

		// Get the number of types.
		nat typeCount() const;

		// Get the number of templates.
		nat templateCount() const;

		// Create a gc type for the given type.
		GcType *createGcType(const CppType *type);
	};

}
