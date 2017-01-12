#include "stdafx.h"
#include "Array.h"
#include "Engine.h"
#include "Exception.h"
#include "Core/Str.h"

namespace storm {

	Type *createArray(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		return new (params) ArrayType(name, param.type);
	}

	Bool isArray(Value v) {
		return as<ArrayType>(v.type) != null;
	}

	Value unwrapArray(Value v) {
		if (ArrayType *t = as<ArrayType>(v.type))
			return t->param();
		else
			return v;
	}

	Value wrapArray(Value v) {
		if (!v.type)
			return v;
		if (v.ref)
			return v;

		Engine &e = v.type->engine;
		TemplateList *l = e.cppTemplate(ArrayId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'wrapArray'.");
		Type *found = as<Type>(to->find(new (e) SimplePart(new (e) Str(L"Array"), v)));
		if (!found)
			throw InternalError(L"Can not find the array type!");
		return Value(found);
	}

	ArrayType::ArrayType(Str *name, Type *contents) : Type(name, typeClass), contents(contents) {
		if (engine.has(bootTemplates))
			lateInit();

		setSuper(ArrayBase::stormType(engine));
	}

	void ArrayType::lateInit() {
		if (!params)
			params = new (engine) Array<Value>();
		if (params->count() < 1)
			params->push(Value(contents));

		Type::lateInit();
	}

	Value ArrayType::param() const {
		return Value(contents);
	}

	Bool ArrayType::loadAll() {
		TODO(L"Add the iterator!");

		if (param().isHeapObj())
			loadClassFns();
		else
			loadValueFns();

		// Shared functions.
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef();
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, ref, L"[]", valList(e, 2, t, natType), address(&ArrayBase::getRaw)));
		add(nativeFunction(e, ref, L"last", valList(e, 1, t), address(&ArrayBase::lastRaw)));
		add(nativeFunction(e, ref, L"first", valList(e, 1, t), address(&ArrayBase::firstRaw)));

		return Type::loadAll();
	}

	static void CODECALL createArrayRaw(void *mem) {
		ArrayType *t = (ArrayType *)runtime::typeOf((RootObject *)mem);
		ArrayBase *o = new (Place(mem)) ArrayBase(t->param().type->handle());
		runtime::setVTable(o);
	}

	static void CODECALL copyArray(void *mem, ArrayBase *from) {
		ArrayBase *o = new (Place(mem)) ArrayBase(*from);
		runtime::setVTable(o);
	}

	static ArrayBase *CODECALL pushClass(ArrayBase *to, const void *src) {
		to->pushRaw(&src);
		return to;
	}

	static void CODECALL insertClass(ArrayBase *to, Nat pos, const void *src) {
		to->insertRaw(pos, src);
	}

	void ArrayType::loadClassFns() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), &createArrayRaw));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), &copyArray));
		add(nativeFunction(e, t, L"<<", valList(e, 2, t, param()), address(&pushClass)));
		add(nativeFunction(e, t, L"push", valList(e, 2, t, param()), address(&pushClass)));
		add(nativeFunction(e, Value(), L"insert", valList(e, 3, t, natType, param()), address(&insertClass)));
	}

	static ArrayBase *CODECALL pushValue(ArrayBase *to, const void *src) {
		to->pushRaw(src);
		return to;
	}

	void ArrayType::loadValueFns() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef(true);
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), &createArrayRaw));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), &copyArray));
		add(nativeFunction(e, t, L"<<", valList(e, 2, t, ref), address(&pushValue)));
		add(nativeFunction(e, t, L"push", valList(e, 2, t, ref), address(&pushValue)));
		add(nativeFunction(e, Value(), L"insert", valList(e, 3, t, natType, ref), address(&ArrayBase::insertRaw)));
	}
}
