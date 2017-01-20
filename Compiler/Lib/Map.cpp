#include "stdafx.h"
#include "Map.h"
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

	void CODECALL MapType::createClass(void *mem) {
		MapType *t = (MapType *)runtime::typeOf((RootObject *)mem);
		MapBase *o = new (Place(mem)) MapBase(t->k->handle(), t->v->handle());
		runtime::setVTable(o);
	}

	void CODECALL MapType::copyClass(void *mem, MapBase *src) {
		MapBase *o = new (Place(mem)) MapBase(*src);
		runtime::setVTable(o);
	}

	Bool MapType::loadAll() {
		Engine &e = engine;

		Value t = thisPtr(this);
		Value key = Value(k);
		Value val = Value(v);
		Value keyRef = key.asRef();
		Value valRef = val.asRef();
		Value iter = Value(new (this) MapIterType(k, v));
		Value boolT = Value(StormInfo<Bool>::type(e));

		Array<Value> *thisKey = valList(e, 2, t, keyRef);

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&MapType::createClass)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&MapType::copyClass)));
		add(nativeFunction(e, Value(), L"put", valList(e, 3, t, keyRef, valRef), address(&MapBase::putRaw)));
		add(nativeFunction(e, boolT, L"has", thisKey, address(&MapBase::hasRaw)));
		add(nativeFunction(e, valRef, L"get", thisKey, address(&MapBase::getRaw)));
		add(nativeFunction(e, valRef, L"get", valList(e, 3, t, keyRef, valRef), address(&MapBase::getRawDef)));
		add(nativeFunction(e, boolT, L"remove", thisKey, address(&MapBase::removeRaw)));
		add(nativeFunction(e, Value(), L"begin", valList(e, 1, t), address(&MapBase::beginRaw)));
		add(nativeFunction(e, Value(), L"end", valList(e, 1, t), address(&MapBase::endRaw)));
		add(nativeFunction(e, Value(), L"find", thisKey, address(&MapBase::findRaw)));

		addAccess();

		return Type::loadAll();
	}

	void MapType::addAccess() {
		Function *ctor = k->defaultCtor();
		// The 'at' member can only be implemented if 'val' has a default constructor.
		if (!ctor)
			return;

		using namespace code;
		Listing *l = new (this) Listing();
		Var me = l->createParam(valPtr());
		Var key = l->createParam(valPtr());

		*l << prolog();

		*l << fnParam(me);
		*l << fnParam(key);
		*l << fnParam(ctor->ref());
		*l << fnCall(engine.ref(Engine::rMapAt), valPtr());

		*l << epilog();
		*l << ret(valPtr());

		add(dynamicFunction(engine, Value(), L"[]", valList(engine, 2, thisPtr(this), Value(k, true)), l));
	}


	/**
	 * Iterator.
	 */

	static void copyIterator(void *to, const MapBase::Iter *from) {
		new (Place(to)) MapBase::Iter(*from);
	}

	static bool iteratorEq(MapBase::Iter &a, MapBase::Iter &b) {
		return a == b;
	}

	static bool iteratorNeq(MapBase::Iter &a, MapBase::Iter &b) {
		return a != b;
	}

	MapIterType::MapIterType(Type *k, Type *v)
		: Type(new (k) Str(L"Iter"), new (k) Array<Value>(), typeValue),
		  k(k),
		  v(v) {

		setSuper(MapBase::Iter::stormType(engine));
	}

	Bool MapIterType::loadAll() {
		Engine &e = engine;

		// TODO: Return a value for the key!

		Value key = Value(k);
		Value val = Value(v);
		Value keyRef = key.asRef();
		Value valRef = val.asRef();
		Value vBool = Value(StormInfo<Bool>::type(e));

		Value v = Value(this);
		Value r = v.asRef();

		Array<Value> *ref = valList(e, 1, r);
		Array<Value> *refref = valList(e, 2, r, r);

		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator)));
		add(nativeFunction(e, vBool, L"==", refref, address(&iteratorEq)));
		add(nativeFunction(e, vBool, L"!=", refref, address(&iteratorNeq)));
		add(nativeFunction(e, r, L"++*", ref, address(&MapBase::Iter::preIncRaw)));
		add(nativeFunction(e, v, L"*++", ref, address(&MapBase::Iter::postIncRaw)));
		add(nativeFunction(e, keyRef, L"k", ref, address(&MapBase::Iter::rawKey)));
		add(nativeFunction(e, valRef, L"v", ref, address(&MapBase::Iter::rawVal)));

		return Type::loadAll();
	}

}
