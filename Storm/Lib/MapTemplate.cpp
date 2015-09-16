#include "stdafx.h"
#include "MapTemplate.h"
#include "Engine.h"
#include "Function.h"

namespace storm {

	static void destroyClass(MapBase *b) {
		b->~MapBase();
	}

	static Named *generateMap(Par<NamePart> part) {
		if (part->params.size() != 2)
			return null;

		const Value &key = part->params[0];
		const Value &value = part->params[1];

		return CREATE(MapType, part->engine(), key.asRef(false), value.asRef(false));
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
		Type(L"Map", typeClass, valList(2, key, value)), key(key), value(value) {

		setSuper(MapBase::stormType(engine));
	}

	bool MapType::loadAll() {
		// TODO: Load any special functions here!

		Engine &e = engine;
		add(steal(nativeDtor(e, this, &destroyClass)));

		return Type::loadAll();
	}

}
