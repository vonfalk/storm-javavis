#include "stdafx.h"
#include "WeakSet.h"
#include "Core/Str.h"
#include "Core/WeakSet.h"
#include "Maybe.h"

namespace storm {

	Type *createWeakSet(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value k = params->at(0);
		if (k.ref)
			return null;

		// Disallow non-actors.
		if (!k.type->isA(TObject::stormType(name->engine())))
			return null;

		return new (params) WeakSetType(name, k.type);
	}

	WeakSetType::WeakSetType(Str *name, Type *k) : Type(name, typeClass), k(k) {
		params = new (engine) Array<Value>(Value(k));

		setSuper(WeakSetBase::stormType(engine));
	}


	void CODECALL WeakSetType::createClass(void *mem) {
		WeakSetBase *o = new (Place(mem)) WeakSetBase();
		runtime::setVTable(o);
	}

	void CODECALL WeakSetType::copyClass(void *mem, WeakSetBase *src) {
		WeakSetBase *o = new (Place(mem)) WeakSetBase(*src);
		runtime::setVTable(o);
	}

	Bool WeakSetType::loadAll() {
		Engine &e = engine;

		Value t = thisPtr(this);
		Value key = Value(k);
		Value iter = Value(new (this) WeakSetIterType(k));
		Value boolT = Value(StormInfo<Bool>::type(e));

		Array<Value> *thisKey = valList(e, 2, t, key);

		add(iter.type);
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&WeakSetType::createClass)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&WeakSetType::copyClass)));
		add(nativeFunction(e, Value(), S("put"), thisKey, address(&WeakSetBase::putRaw)));
		add(nativeFunction(e, boolT, S("has"), thisKey, address(&WeakSetBase::hasRaw)));
		add(nativeFunction(e, boolT, S("remove"), thisKey, address(&WeakSetBase::removeRaw)));
		add(nativeFunction(e, iter, S("iter"), valList(e, 1, t), address(&WeakSetBase::iterRaw)));

		return Type::loadAll();
	}

	/**
	 * Iterator.
	 */

	static void copyIterator(void *to, const WeakSetBase::Iter *from) {
		new (Place(to)) WeakSetBase::Iter(*from);
	}

	WeakSetIterType::WeakSetIterType(Type *k)
		: Type(new (k) Str(S("Iter")), new (k) Array<Value>(), typeValue),
		  k(k) {

		setSuper(WeakSetBase::Iter::stormType(engine));
	}

	Bool WeakSetIterType::loadAll() {
		Engine &e = engine;

		Value key = wrapMaybe(Value(k));

		Value v = Value(this);
		Value r = v.asRef();

		Array<Value> *ref = valList(e, 1, r);
		Array<Value> *refref = valList(e, 2, r, r);

		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator))->makePure());
		add(nativeFunction(e, key, S("next"), ref, address(&WeakSetBase::Iter::nextRaw)));

		return Type::loadAll();
	}

}
