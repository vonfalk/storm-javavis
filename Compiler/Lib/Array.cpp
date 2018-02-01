#include "stdafx.h"
#include "Array.h"
#include "Engine.h"
#include "Exception.h"
#include "Fn.h"
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
		Type *found = as<Type>(to->find(S("Array"), v, Scope()));
		if (!found)
			throw InternalError(L"Can not find the array type!");
		return Value(found);
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

	static ArrayBase *CODECALL pushValue(ArrayBase *to, const void *src) {
		to->pushRaw(src);
		return to;
	}

	static ArrayBase *CODECALL sortedRaw(ArrayBase *src) {
		Type *t = runtime::typeOf(src);
		ArrayBase *copy = (ArrayBase *)runtime::allocObject(sizeof(ArrayBase), t);
		copyArray(copy, src);
		copy->sortRaw();
		return copy;
	}

	static ArrayBase *CODECALL sortedRawPred(ArrayBase *src, FnBase *compare) {
		Type *t = runtime::typeOf(src);
		ArrayBase *copy = (ArrayBase *)runtime::allocObject(sizeof(ArrayBase), t);
		copyArray(copy, src);
		copy->sortRawPred(compare);
		return copy;
	}

	ArrayType::ArrayType(Str *name, Type *contents) : Type(name, typeClass), contents(contents) {
		if (engine.has(bootTemplates))
			lateInit();

		setSuper(ArrayBase::stormType(engine));
		// Avoid problems while loading Array<Value>, as Array<Value> is required to compute the
		// GcType of Array<Value>...
		useSuperGcType();
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
		Type *iter = new (this) ArrayIterType(contents);
		add(iter);

		if (param().isHeapObj())
			loadClassFns();
		else
			loadValueFns();

		// Shared functions.
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef();
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&copyArray))->makePure());
		add(nativeFunction(e, ref, S("[]"), valList(e, 2, t, natType), address(&ArrayBase::getRaw))->makePure());
		add(nativeFunction(e, ref, S("last"), valList(e, 1, t), address(&ArrayBase::lastRaw))->makePure());
		add(nativeFunction(e, ref, S("first"), valList(e, 1, t), address(&ArrayBase::firstRaw))->makePure());
		add(nativeFunction(e, ref, S("random"), valList(e, 1, t), address(&ArrayBase::randomRaw)));
		add(nativeFunction(e, Value(iter), S("begin"), valList(e, 1, t), address(&ArrayBase::beginRaw))->makePure());
		add(nativeFunction(e, Value(iter), S("end"), valList(e, 1, t), address(&ArrayBase::endRaw))->makePure());

		// Sort using <. TODO: This crashes during compiler boot for some reason. Investigate.
		// if (param().type->handle().lessFn) {
		// 	add(nativeFunction(e, Value(), S("sort"), valList(e, 1, t), address(&ArrayBase::sortRaw)));
		// 	add(nativeFunction(e, t, S("sorted"), valList(e, 1, t), address(&sortedRaw))->makePure());
		// }

		// Sort with provided predicate function.
		{
			Array<Value> *pParams = new (e) Array<Value>();
			*pParams << Value(StormInfo<Bool>::type(e));
			*pParams << param() << param();
			Value predicate = thisPtr(fnType(pParams));
			add(nativeFunction(e, Value(), S("sort"), valList(e, 2, t, predicate), address(&ArrayBase::sortRawPred)));
			add(nativeFunction(e, t, S("sorted"), valList(e, 2, t, predicate), address(&sortedRawPred))->makePure());
		}

		return Type::loadAll();
	}

	void ArrayType::loadClassFns() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&createArrayRaw))->makePure());
		add(nativeFunction(e, t, S("<<"), valList(e, 2, t, param()), address(&pushClass)));
		add(nativeFunction(e, t, S("push"), valList(e, 2, t, param()), address(&pushClass)));
		add(nativeFunction(e, Value(), S("insert"), valList(e, 3, t, natType, param()), address(&insertClass)));
	}

	void ArrayType::loadValueFns() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef(true);
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&createArrayRaw))->makePure());
		add(nativeFunction(e, t, S("<<"), valList(e, 2, t, ref), address(&pushValue)));
		add(nativeFunction(e, t, S("push"), valList(e, 2, t, ref), address(&pushValue)));
		add(nativeFunction(e, Value(), S("insert"), valList(e, 3, t, natType, ref), address(&ArrayBase::insertRaw)));
	}

	/**
	 * Iterator.
	 */

	static void copyIterator(void *to, const ArrayBase::Iter *from) {
		new (Place(to)) ArrayBase::Iter(*from);
	}

	static bool iteratorEq(ArrayBase::Iter &a, ArrayBase::Iter &b) {
		return a == b;
	}

	static bool iteratorNeq(ArrayBase::Iter &a, ArrayBase::Iter &b) {
		return a != b;
	}

	static void *iteratorGet(const ArrayBase::Iter &v) {
		return v.getRaw();
	}

	static Nat iteratorGetKey(const ArrayBase::Iter &v) {
		return v.getIndex();
	}

	ArrayIterType::ArrayIterType(Type *param)
		: Type(new (param) Str(S("Iter")), new (param) Array<Value>(), typeValue),
		  contents(param) {

		setSuper(ArrayBase::Iter::stormType(engine));
	}

	Bool ArrayIterType::loadAll() {
		Engine &e = engine;
		Value v(this, false);
		Value r(this, true);
		Value param(contents, true);

		Array<Value> *ref = valList(e, 1, r);
		Array<Value> *refref = valList(e, 2, r, r);

		Value vBool = Value(StormInfo<Bool>::type(e));
		Value vNat = Value(StormInfo<Nat>::type(e));
		add(nativeFunction(e, Value(), Type::CTOR, refref, address(&copyIterator))->makePure());
		add(nativeFunction(e, vBool, S("=="), refref, address(&iteratorEq))->makePure());
		add(nativeFunction(e, vBool, S("!="), refref, address(&iteratorNeq))->makePure());
		add(nativeFunction(e, r, S("++*"), ref, address(&ArrayBase::Iter::preIncRaw)));
		add(nativeFunction(e, v, S("*++"), ref, address(&ArrayBase::Iter::postIncRaw)));
		add(nativeFunction(e, vNat, S("k"), ref, address(&iteratorGetKey))->makePure());
		add(nativeFunction(e, param, S("v"), ref, address(&iteratorGet))->makePure());

		return Type::loadAll();
	}

}
