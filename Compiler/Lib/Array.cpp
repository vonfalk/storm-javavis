#include "stdafx.h"
#include "Array.h"
#include "Engine.h"
#include "Exception.h"
#include "Fn.h"
#include "Core/Str.h"
#include "Serialization.h"

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

	static void CODECALL createArrayClassCount(void *mem, Nat count, const RootObject *data) {
		ArrayType *t = (ArrayType *)runtime::typeOf((RootObject *)mem);
		ArrayBase *o = new (Place(mem)) ArrayBase(t->param().type->handle(), count, &data);
		runtime::setVTable(o);
	}

	static void CODECALL createArrayValCount(void *mem, Nat count, const void *data) {
		ArrayType *t = (ArrayType *)runtime::typeOf((RootObject *)mem);
		ArrayBase *o = new (Place(mem)) ArrayBase(t->param().type->handle(), count, data);
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

	ArrayType::ArrayType(Str *name, Type *contents) : Type(name, typeClass), contents(contents), watchFor(0) {
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

		// Sort with operator <.
		if (param().type->handle().lessFn) {
			addSort();
		} else {
			watchFor |= watchLess;
		}


		// Sort with provided predicate function.
		{
			Array<Value> *pParams = new (e) Array<Value>();
			*pParams << Value(StormInfo<Bool>::type(e));
			*pParams << param() << param();
			Value predicate = Value(fnType(pParams));
			add(nativeFunction(e, Value(), S("sort"), valList(e, 2, t, predicate), address(&ArrayBase::sortRawPred)));
			add(nativeFunction(e, t, S("sorted"), valList(e, 2, t, predicate), address(&sortedRawPred))->makePure());
		}

		if (!param().type->isA(StormInfo<TObject>::type(engine))) {
			if (SerializeInfo *info = serializeInfo(param().type)) {
				addSerialization(info);
			} else {
				watchFor |= watchSerialization;
			}
		}

		if (watchFor)
			param().type->watchAdd(this);

		return Type::loadAll();
	}

	void ArrayType::loadClassFns() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&createArrayRaw))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 3, t, natType, param()), address(&createArrayClassCount))->makePure());
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
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 3, t, natType, ref), address(&createArrayValCount))->makePure());
		add(nativeFunction(e, t, S("<<"), valList(e, 2, t, ref), address(&pushValue)));
		add(nativeFunction(e, t, S("push"), valList(e, 2, t, ref), address(&pushValue)));
		add(nativeFunction(e, Value(), S("insert"), valList(e, 3, t, natType, ref), address(&ArrayBase::insertRaw)));
	}

	void ArrayType::notifyAdded(NameSet *to, Named *added) {
		if (to != param().type)
			return;

		Function *fn = as<Function>(added);
		if (!fn)
			return;

		if (*fn->name == S("<") &&
			fn->result == Value(StormInfo<Bool>::type(engine)) &&
			fn->params->count() == 2 &&
			fn->params->at(0).type == to &&
			fn->params->at(1).type == to) {

			watchFor &= ~watchLess;
			addSort();
		} else if (SerializeInfo *info = serializeInfo(param().type)) {
			if (watchFor & watchSerialization) {
				watchFor &= ~watchSerialization;
				addSerialization(info);
			}
		}


		if (watchFor == watchNone)
			to->watchRemove(this);
	}

	void ArrayType::addSort() {
		Engine &e = engine;
		Array<Value> *params = valList(e, 1, thisPtr(this));

		// Sort using <.
		add(nativeFunction(e, Value(), S("sort"), params, address(&ArrayBase::sortRaw)));
		add(nativeFunction(e, Value(this), S("sorted"), params, address(&sortedRaw))->makePure());

		// TODO: Would be nice to have < for comparison as well!
	}

	void ArrayType::addSerialization(SerializeInfo *info) {
		Value param = this->param();

		// TODO: We should provide a constructor!
		SerializedTuples *type = new (this) SerializedTuples(this, null);
		type->add(param.type);
		add(serializedTypeFn(type));

		// Add the 'write' function.
		add(writeFn(type, info));
	}

	Function *ArrayType::writeFn(SerializedType *type, SerializeInfo *info) {
		using namespace code;

		Value me = thisPtr(this);
		Value objStream(StormInfo<ObjOStream>::type(engine));

		Function *startFn, *endFn;
		Function *countFn, *atFn;
		{
			SimplePart *startName = new (this) SimplePart(S("startObject"));
			*startName->params << thisPtr(objStream.type)
							   << Value(StormInfo<SerializedType>::type(engine))
							   << Value(StormInfo<Object>::type(engine));
			startFn = as<Function>(objStream.type->find(startName, Scope()));

			SimplePart *endName = new (this) SimplePart(S("end"));
			*endName->params << thisPtr(objStream.type);
			endFn = as<Function>(objStream.type->find(endName, Scope()));

			if (!startFn || !endFn)
				throw InternalError(L"ObjIStream does not have 'startObject' and 'end' as expected.");

			SimplePart *countName = new (this) SimplePart(S("count"));
			*countName->params << me;
			countFn = as<Function>(find(countName, Scope()));

			SimplePart *atName = new (this) SimplePart(S("[]"));
			*atName->params << me << Value(StormInfo<Nat>::type(engine));
			atFn = as<Function>(find(atName, Scope()));

			if (!countFn || !atFn)
				throw InternalError(L"The array does not have 'count' and '[]' as expected.");
		}

		Listing *l = new (this) Listing(true, engine.voidDesc());
		code::Var meVar = l->createParam(me.desc(engine));
		code::Var streamVar = l->createParam(objStream.desc(engine));
		code::Var count = l->createVar(l->root(), Size::sInt);
		code::Var curr = l->createVar(l->root(), Size::sInt);

		TypeDesc *natDesc = intDesc(engine); // int === nat in this context.

		*l << prolog();

		// Find number of elements.
		*l << fnParam(me.desc(engine), meVar);
		*l << fnCall(countFn->ref(), true, natDesc, count);


		*l << fnRet();

		PVAR(l);

		Array<Value> *params = new (this) Array<Value>();
		params->reserve(2);
		*params << me << objStream;
		Function *fn = new (this) Function(Value(), new (this) Str(S("write")), params);
		fn->setCode(new (this) DynamicCode(l));
		return fn;
	}

	/**
	 * Iterator.
	 */

	static void CODECALL copyIterator(void *to, const ArrayBase::Iter *from) {
		new (Place(to)) ArrayBase::Iter(*from);
	}

	static bool CODECALL iteratorEq(ArrayBase::Iter &a, ArrayBase::Iter &b) {
		return a == b;
	}

	static bool CODECALL iteratorNeq(ArrayBase::Iter &a, ArrayBase::Iter &b) {
		return a != b;
	}

	static void *CODECALL iteratorGet(const ArrayBase::Iter &v) {
		return v.getRaw();
	}

	static Nat CODECALL iteratorGetKey(const ArrayBase::Iter &v) {
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
