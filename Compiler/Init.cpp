#include "stdafx.h"
#include "Init.h"
#include "CppLoader.h"
#include "Type.h"

namespace storm {

	void initTypes(Engine &e) {
		const CppWorld *world = cppWorld();

		// We need to start by creating the Type-type.
		Type::createType(e, &world->types[0]);
	}

}
