#include "stdafx.h"
#include "Init.h"
#include "CppLoader.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Map.h"
#include "Core/Array.h"
#include "Core/Str.h"
#include "NameSet.h"

namespace storm {

	static void lateInit(Named *n) {
		n->lateInit();
	}

	void initTypes(Engine &e, RootArray<Type> &types, RootArray<TemplateList> &templates) {
		const CppWorld *world = cppWorld();
		CppLoader loader(e, world, types, templates);

		types.resize(1);

		// We need to start by creating the Type-type.
		types[0] = Type::createType(e, &world->types[0]);

		// Then we can go on loading the rest of the types.
		loader.loadTypes();
		loader.loadSuper();

		e.advance(bootTypes);

		// Load templates.
		loader.loadTemplates();

		// Poke at some template types needed to instantiate a Type properly. This causes them to be
		// properly instantiated before we report that templates are fully functional.
		Array<Value>::stormType(e);
		Map<Str *, NameOverloads *>::stormType(e);

		// Now we can declare templates fully functional.
		e.advance(bootTemplates);

		// Do the late initialization on all previously created types.
		e.forNamed(&lateInit);

		// Insert everything into their packages.
		loader.loadPackages();
	}

}
