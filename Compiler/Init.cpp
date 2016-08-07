#include "stdafx.h"
#include "Init.h"
#include "CppLoader.h"
#include "Type.h"

namespace storm {

	void initTypes(Engine &e, RootArray<Type> &types, RootArray<TemplateList> &templates) {
		const CppWorld *world = cppWorld();
		CppLoader loader(e, world);

		types.resize(1);

		// We need to start by creating the Type-type.
		types[0] = Type::createType(e, &world->types[0]);

		// Then we can go on loading the rest of the types.
		loader.loadTypes(types);

		// Load templates.
		loader.loadTemplates(templates);
	}

}
