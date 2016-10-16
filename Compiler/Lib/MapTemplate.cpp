#include "stdafx.h"
#include "MapTemplate.h"
#include "Core/Map.h"
#include "Core/Str.h"
#include "Engine.h"

namespace storm {

	Type *createMap(Str *name, ValueArray *params) {
		if (params->count() != 2)
			return null;

		Value k = params->at(0);
		Value v = params->at(1);
		if (k.ref || v.ref)
			return null;

		return new (params) MapType(name, k.type, v.type);
	}

	MapType::MapType(Str *name, Type *k, Type *v) : Type(name, typeClass), k(k), v(v) {
		if (engine.has(bootTemplates))
			lateInit();

		setSuper(MapBase::stormType(engine));
	}

	void MapType::lateInit() {
		if (!params)
			params = new (engine) Array<Value>();
		if (params->count() != 2) {
			params->clear();
			params->push(Value(k));
			params->push(Value(v));
		}
	}

}
