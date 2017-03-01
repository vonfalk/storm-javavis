#include "stdafx.h"
#include "Set.h"
#include "Core/Str.h"
#include "Core/Set.h"

namespace storm {

	Type *createSet(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value k = params->at(0);
		if (k.ref)
			return null;

		return new (params) SetType(name, k.type);
	}

	SetType::SetType(Str *name, Type *k) : Type(name, typeClass), k(k) {
		params = new (engine) Array<Value>(Value(k));

		setSuper(SetBase::stormType(engine));
	}

	void CODECALL SetType::createClass(void *mem) {
		SetType *t = (SetType *)runtime::typeOf((RootObject *)mem);
		SetBase *o = new (Place(mem)) SetBase(t->k->handle());
		runtime::setVTable(o);
	}

	void CODECALL SetType::copyClass(void *mem, SetBase *src) {
		SetBase *o = new (Place(mem)) SetBase(*src);
		runtime::setVTable(o);
	}

	Bool SetType::loadAll() {
		Engine &e = engine;

		Value t = thisPtr(this);
		Value key = Value(k);
		Value keyRef = key.asRef();
		Value iter = Value(new (this) SetIterType(k));
		Value boolT = Value(StormInfo<Bool>::type(e));

		Array<Value> *thisKey = valList(e, 2, t, keyRef);

		add(iter.type);
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&SetType::createClass)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&SetType::copyClass)));
		add(nativeFunction(e, Value(), L"put", thisKey, address(&SetBase::putRaw)));
		add(nativeFunction(e, Value(), L"put", valList(e, 2, t, t), address(&SetBase::putSetRaw)));
		add(nativeFunction(e, boolT, L"has", thisKey, address(&SetBase::hasRaw)));
		add(nativeFunction(e, keyRef, L"get", thisKey, address(&SetBase::getRaw)));
		add(nativeFunction(e, keyRef, L"[]", thisKey, address(&SetBase::atRaw)));
		add(nativeFunction(e, boolT, L"remove", thisKey, address(&SetBase::removeRaw)));
		add(nativeFunction(e, Value(), L"begin", valList(e, 1, t), address(&SetBase::beginRaw)));
		add(nativeFunction(e, Value(), L"end", valList(e, 1, t), address(&SetBase::endRaw)));

		return Type::loadAll();
	}

	/**
	 * Iterator.
	 */

	static void copyIterator(void *to, const SetBase::Iter *from) {
		new (Place(to)) SetBase::Iter(*from);
	}

	static bool iteratorEq(SetBase::Iter &a, SetBase::Iter &b) {
		return a == b;
	}

	static bool iteratorNeq(SetBase::Iter &a, SetBase::Iter &b) {
		return a != b;
	}

	SetIterType::SetIterType(Type *k)
		: Type(new (k) Str(L"Iter"), new (k) Array<Value>(), typeValue),
		  k(k) {

		setSuper(SetBase::Iter::stormType(engine));
	}

	Bool SetIterType::loadAll() {
		Engine &e = engine;

		// TODO: Return a value for the key!

		Value key = Value(k);
		Value keyRef = key.asRef();
		Value vBool = Value(StormInfo<Bool>::type(e));

		Value v = Value(this);
		Value r = v.asRef();

		Array<Value> *ref = valList(e, 1, r);
		Array<Value> *refref = valList(e, 2, r, r);

		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator)));
		add(nativeFunction(e, vBool, L"==", refref, address(&iteratorEq)));
		add(nativeFunction(e, vBool, L"!=", refref, address(&iteratorNeq)));
		add(nativeFunction(e, r, L"++*", ref, address(&SetBase::Iter::preIncRaw)));
		add(nativeFunction(e, v, L"*++", ref, address(&SetBase::Iter::postIncRaw)));
		add(nativeFunction(e, keyRef, L"k", ref, address(&SetBase::Iter::rawVal)));
		add(nativeFunction(e, keyRef, L"v", ref, address(&SetBase::Iter::rawVal)));

		return Type::loadAll();
	}

}
