#include "stdafx.h"
#include "MapTemplate.h"
#include "Engine.h"

namespace storm {

	static Named *generateMap(Par<NamePart> part) {
		if (part->params.size() != 2)
			return null;

		return CREATE(MapType, part->engine(), part->params[0], part->params[1]);
	}

	void addMapTemplate(Par<Package> to) {
		Auto<Template> t = CREATE(Template, to, L"Map", simpleFn(&generateMap));
		to->add(t);
	}

	Type *mapType(Engine &e, const ValueData &key, const ValueData &value) {
		Auto<Name> tName = CREATE(Name, e);
		tName->add(L"core");
		tName->add(L"Map", valList(2, Value(key), Value(value)));
		Type *r = as<Type>(e.scope()->find(tName));
		assert(r, "The map type was not found!");
		return r;
	}

	MapType::MapType(const Value &key, const Value &value) :
		Type(L"Map", typeClass, valList(2, key, value)), key(key), value(value) {}

	bool MapType::loadAll() {
		// TODO: Load any special functions here!

		return Type::loadAll();
	}

}
