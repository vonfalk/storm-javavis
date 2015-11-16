#include "stdafx.h"
#include "MapTemplate.h"
#include "Engine.h"
#include "Function.h"

namespace storm {

	static void CODECALL createClass(void *mem) {
		// Extract the type...
		nat offset = OFFSET_OF(Object, myType);
		Type *type = OFFSET_IN(mem, offset, Type *);

		// Find out the handles.
		MapType *mapType = as<MapType>(type);
		const Handle &k = mapType->key.handle();
		const Handle &v = mapType->value.handle();

		// Create!
		new (mem) MapBase(k, v);
	}

	static void CODECALL createClassCopy(void *mem, MapBase *original) {
		new (mem) MapBase(original);
	}

	static void CODECALL destroyClass(MapBase *b) {
		b->~MapBase();
	}

	static void CODECALL mapPush(MapBase *me, void *k, void *v) {
		me->putRaw(k, v);
	}

	static bool CODECALL mapHas(MapBase *me, void *k) {
		return me->hasRaw(k);
	}

	static void *CODECALL mapGet(MapBase *me, void *k) {
		return me->getRaw(k);
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

		Auto<Named> n = e.scope()->findW(tName);
		Type *r = as<Type>(n.borrow());
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
		Value t = Value::thisPtr(this);

		Value kRef = key.asRef();
		Value vRef = value.asRef();
		Value b = boolType(e);

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createClass))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&createClassCopy))));
		add(steal(nativeFunction(e, Value(), L"put", valList(3, t, kRef, vRef), address(&mapPush))));
		add(steal(nativeFunction(e, b, L"has", valList(2, t, kRef), address(&mapHas))));
		add(steal(nativeFunction(e, vRef, L"get", valList(2, t, kRef), address(&mapGet))));
		add(steal(nativeDtor(e, this, &destroyClass)));

		if (code::RefSource *defCtor = value.type->handleDefaultCtor()) {
			addAccess(defCtor);
		}

		return Type::loadAll();
	}

	void MapType::addAccess(code::RefSource *fn) {
		using namespace code;
		Listing l;

		Variable me = l.frame.createPtrParam();
		Variable k = l.frame.createPtrParam();

		l << prolog();

		l << fnParam(me);
		l << fnParam(k);
		l << fnParam(*fn);
		l << fnCall(engine.fnRefs.mapAccess, retVoid());

		l << epilog();
		l << ret(retVoid());

		Value t = Value::thisPtr(this);
		Value kRef = key.asRef();
		Value vRef = value.asRef();
		add(steal(dynamicFunction(engine, vRef, L"[]", valList(2, t, kRef), l)));
	}

}
