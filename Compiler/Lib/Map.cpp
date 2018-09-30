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

		add(iter.type);
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&MapType::createClass))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&MapType::copyClass))->makePure());
		add(nativeFunction(e, Value(), S("put"), valList(e, 3, t, keyRef, valRef), address(&MapBase::putRaw)));
		add(nativeFunction(e, boolT, S("has"), thisKey, address(&MapBase::hasRaw))->makePure());
		add(nativeFunction(e, valRef, S("get"), thisKey, address(&MapBase::getRaw))->makePure());
		add(nativeFunction(e, valRef, S("get"), valList(e, 3, t, keyRef, valRef), address(&MapBase::getRawDef)));
		add(nativeFunction(e, boolT, S("remove"), thisKey, address(&MapBase::removeRaw)));
		add(nativeFunction(e, iter, S("begin"), valList(e, 1, t), address(&MapBase::beginRaw))->makePure());
		add(nativeFunction(e, iter, S("end"), valList(e, 1, t), address(&MapBase::endRaw))->makePure());
		add(nativeFunction(e, Value(), S("find"), thisKey, address(&MapBase::findRaw))->makePure());

		addAccess();

		return Type::loadAll();
	}

	void MapType::addAccess() {
		Function *ctor = k->defaultCtor();
		// The 'at' member can only be implemented if 'val' has a default constructor.
		if (!ctor)
			return;

		using namespace code;
		TypeDesc *ptr = engine.ptrDesc();
		Listing *l = new (this) Listing(true, ptr);
		Var me = l->createParam(ptr);
		Var key = l->createParam(ptr);

		*l << prolog();

		*l << fnParam(ptr, me);
		*l << fnParam(ptr, key);
		*l << fnParam(ptr, ctor->ref());
		*l << fnCall(engine.ref(Engine::rMapAt), true, ptr, ptrA);

		*l << fnRet(ptrA);

		add(dynamicFunction(engine, Value(v, true), S("[]"), valList(engine, 2, thisPtr(this), Value(k, true)), l));
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
		: Type(new (k) Str(S("Iter")), new (k) Array<Value>(), typeValue),
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

		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator))->makePure());
		add(nativeFunction(e, vBool, S("=="), refref, address(&iteratorEq))->makePure());
		add(nativeFunction(e, vBool, S("!="), refref, address(&iteratorNeq))->makePure());
		add(nativeFunction(e, r, S("++*"), ref, address(&MapBase::Iter::preIncRaw)));
		add(nativeFunction(e, v, S("*++"), ref, address(&MapBase::Iter::postIncRaw)));
		add(nativeFunction(e, keyRef, S("k"), ref, address(&MapBase::Iter::rawKey))->makePure());
		add(nativeFunction(e, valRef, S("v"), ref, address(&MapBase::Iter::rawVal))->makePure());

		return Type::loadAll();
	}

}
