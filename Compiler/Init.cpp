#include "stdafx.h"
#include "Init.h"
#include "CppLoader.h"
#include "Type.h"

namespace storm {

	void initTypes(Engine &e, RootArray<Type> &types) {
		const CppWorld *world = cppWorld();
		CppLoader loader(e, world);

		types.resize(loader.typeCount());

		// We need to start by creating the Type-type.
		types[0] = Type::createType(e, &world->types[0]);

		// Then we can go on loading the rest of the types.
		loader.loadTypes(types);
	}

}
