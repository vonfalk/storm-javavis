#include "stdafx.h"
#include "Init.h"
#include "CppLoader.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	static void lateInit(Named *n) {
		n->lateInit();
	}

	void initTypes(Engine &e, RootArray<Type> &types, RootArray<TemplateList> &templates) {
		const CppWorld *world = cppWorld();
		CppLoader loader(e, world);

		types.resize(1);

		// We need to start by creating the Type-type.
		types[0] = Type::createType(e, &world->types[0]);

		// Then we can go on loading the rest of the types.
		loader.loadTypes(types);

		e.advance(bootTypes);

		// Load templates.
		loader.loadTemplates(templates);

		// Poke the Array<Value> type so that it does not go into infinite recursion when creating
		// Types (which contain Array<Value> instances).
		Array<Value>::stormType(e);

		// Now we can declare templates fully functional.
		e.advance(bootTemplates);

		// Do the late initialization on all previously created types.
		e.forNamed(&lateInit);
	}

}
