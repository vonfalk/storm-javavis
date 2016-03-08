#include "stdafx.h"
#include "MapTemplate.h"
#include "Engine.h"
#include "Function.h"
#include "Exception.h"

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

	static Named *generateMap(Par<SimplePart> part) {
		if (part->count() != 2)
			return null;

		const Value &key = part->param(0);
		const Value &value = part->param(1);

		return CREATE(MapType, part->engine(), key.asRef(false), value.asRef(false));
	}

	void addMapTemplate(Par<Package> to) {
		Auto<Template> t = CREATE(Template, to, L"Map", simpleFn(&generateMap));
		to->add(t);
	}

	Type *mapType(Engine &e, const ValueData &key, const ValueData &value) {
		Auto<SimpleName> tName = CREATE(SimpleName, e);
		tName->add(L"core");
		tName->add(L"Map", valList(2, Value(key), Value(value)));

		Auto<Named> n = e.scope()->find(tName);
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
		if (key.ref || value.ref)
			throw InternalError(L"References are not supported by the map yet.");

		Auto<Type> iterator = CREATE(MapIterType, this, key, value);
		add(iterator);

		Engine &e = engine;
		Value t = Value::thisPtr(this);

		Value kRef = key.asRef();
		Value vRef = value.asRef();
		Value b = boolType(e);
		Value iter(iterator.borrow());

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createClass))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&createClassCopy))));
		add(steal(nativeFunction(e, Value(), L"put", valList(3, t, kRef, vRef), address(&mapPush))));
		add(steal(nativeFunction(e, b, L"has", valList(2, t, kRef), address(&mapHas))));
		add(steal(nativeFunction(e, vRef, L"get", valList(2, t, kRef), address(&mapGet))));
		add(steal(nativeFunction(e, iter, L"begin", valList(1, t), address(&MapBase::beginRaw))));
		add(steal(nativeFunction(e, iter, L"end", valList(1, t), address(&MapBase::endRaw))));
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

	/**
	 * Map iterator.
	 */

	typedef MapBase::Iter Iter;

	static void copyIterator(void *to, const Iter *from) {
		new (to) Iter(*from);
	}

	static void destroyIterator(Iter *del) {
		del->~Iter();
	}

	static bool iteratorEq(Iter &a, Iter &b) {
		return a == b;
	}

	static bool iteratorNeq(Iter &a, Iter &b) {
		return a != b;
	}

	static void *iteratorGetKey(const Iter &v) {
		return v.getRawKey();
	}

	static void *iteratorGetVal(const Iter &v) {
		return v.getRawVal();
	}

	static Size iterSize() {
		Size s = Size::sPtr;
		s += Size::sNat;
		assert(s.current() == sizeof(Iter));
		return s;
	}


	MapIterType::MapIterType(const Value &k, const Value &v) :
		Type(L"Iter", typeValue, valList(0), iterSize()), key(k), value(v) {}

	bool MapIterType::loadAll() {
		Engine &e = engine;
		Value val(this);
		Value ref = val.asRef();
		Value refKey = key.asRef();
		Value refValue = value.asRef();

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, ref, ref), address(&copyIterator))));
		add(steal(nativeFunction(e, boolType(e), L"==", valList(2, ref, ref), address(&iteratorEq))));
		add(steal(nativeFunction(e, boolType(e), L"!=", valList(2, ref, ref), address(&iteratorNeq))));
		add(steal(nativeFunction(e, ref, L"++*", valList(1, ref), address(&Iter::preIncRaw))));
		add(steal(nativeFunction(e, val, L"*++", valList(1, ref), address(&Iter::postIncRaw))));
		add(steal(nativeFunction(e, refKey, L"k", valList(1, ref), address(&iteratorGetKey))));
		add(steal(nativeFunction(e, refValue, L"v", valList(1, ref), address(&iteratorGetVal))));
		add(steal(nativeDtor(e, this, &destroyIterator)));

		return Type::loadAll();
	}

}
