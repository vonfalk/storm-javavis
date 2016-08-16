#include "stdafx.h"
#include "World.h"

namespace storm {

	World::World(Gc &gc) : types(gc), templates(gc), threads(gc) {}

	void World::clear() {
		types.clear();
		templates.clear();
		threads.clear();
	}

	void World::forNamed(NamedFn fn) {
		for (nat i = 0; i < types.count(); i++)
			(*fn)(types[i]);
		for (nat i = 0; i < templates.count(); i++)
			templates[i]->forNamed(fn);
	}

}
