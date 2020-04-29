#include "stdafx.h"
#include "World.h"

namespace storm {

	World::World(Gc &gc) : types(gc), templates(gc), threads(gc), namedThreads(gc), sources(gc) {}

	void World::clear() {
		types.clear();
		templates.clear();
		threads.clear();
		namedThreads.clear();
		sources.clear();
	}

	void World::forNamed(NamedFn fn) {
		for (nat i = 0; i < types.count(); i++)
			(*fn)(types[i]);
		for (nat i = 0; i < templates.count(); i++)
			templates[i]->forNamed(fn);
		for (nat i = 0; i < namedThreads.count(); i++)
			(*fn)(namedThreads[i]);
	}

}
