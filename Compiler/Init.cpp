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

	void initTypes(Engine &e, World &into) {
		const CppWorld *world = cppWorld();
		CppLoader loader(e, world, into);

		into.types.resize(1);

		// We need to start by creating the Type-type.
		into.types[0] = Type::createType(e, &world->types[0]);

		// Then we can go on loading the rest of the types.
		loader.loadTypes();
		loader.loadThreads();
		loader.loadSuper();

		e.advance(bootTypes);

		// Load templates.
		loader.loadTemplates();

		// Poke at some template types needed to instantiate a Type properly. This causes them to be
		// properly instantiated before we report that templates are fully functional.
		Array<Value>::stormType(e);
		Map<Str *, NameOverloads *>::stormType(e);
		WeakSet<TypeChain>::stormType(e);
		Array<Type *>::stormType(e);

		// Now we can declare templates fully functional.
		e.advance(bootTemplates);

		// Do the late initialization on all previously created types.
		e.forNamed(&lateInit);

		// Insert everything into their packages.
		loader.loadPackages();

		// Load functions.
		loader.loadFunctions();
	}

}
