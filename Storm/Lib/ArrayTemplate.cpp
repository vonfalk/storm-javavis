#include "stdafx.h"
#include "ArrayTemplate.h"
#include "Shared/Array.h"
#include "Function.h"
#include "Exception.h"
#include "TypeCtor.h"
#include "Shared/CloneEnv.h"
#include "Shared/StrBuf.h"
#include "Lib/Random.h"
#include "TypeDtor.h"
#include "Engine.h"

namespace storm {

	static void CODECALL destroyClass(ArrayP<Object> *v) {
		v->~ArrayP<Object>();
	}

	static void CODECALL destroyValue(ArrayBase *v) {
		v->~ArrayBase();
	}


	static void CODECALL createClass(void *mem) {
		new (mem) ArrayP<Object>();
	}

	static void CODECALL copyClass(void *mem, ArrayP<Object> *from) {
		new (mem) ArrayP<Object>(from);
	}

	static void CODECALL copyValue(void *mem, ArrayBase *from) {
		new (mem) ArrayBase(from);
		setVTable((Object *)mem);
	}

	static void CODECALL createValue(void *mem) {
		// Extract the type...
		nat offset = OFFSET_OF(Object, myType);
		Type *type = OFFSET_IN(mem, offset, Type *);

		// Find out the handle.
		ArrayType *arr = as<ArrayType>(type);
		const Handle &handle = arr->param.handle();

		// Create it!
		new (mem) ArrayBase(handle);
		setVTable((Object *)mem);
	}

	static ArrayP<Object> *CODECALL pushClass(ArrayP<Object> *to, Par<Object> o) {
		to->push(o);
		to->addRef();
		return to;
	}

	static ArrayBase *CODECALL pushValue(ArrayBase *to, void *ptr) {
		to->pushRaw(ptr);
		to->addRef();
		return to;
	}

	static void *CODECALL getClass(ArrayP<Object> *from, Nat id) {
		return from->atRaw(id);
	}

	static void *CODECALL getValue(ArrayBase *from, Nat id) {
		return from->atRaw(id);
	}

	static void *CODECALL randomClass(ArrayP<Object> *from) {
		Nat id = rand(Nat(0), from->count());
		return from->atRaw(id);
	}

	static void *CODECALL randomValue(ArrayP<Object> *from) {
		Nat id = rand(Nat(0), from->count());
		return from->atRaw(id);
	}

	static Named *generateArray(Par<NamePart> part) {
		if (part->params.size() != 1)
			return null;

		return CREATE(ArrayType, part->engine(), part->params[0]);
	}

	void addArrayTemplate(Par<Package> to) {
		Auto<Template> t = CREATE(Template, to, L"Array", simpleFn(&generateArray));
		to->add(t);

		// Create and add Array<Str> as well, so we do not have two instances of it!
		Type *special = arrayType(to->engine(), Value(Str::stormType(to)));
		to->add(special);
	}

	ArrayType::ArrayType(const Value &param) : Type(L"Array", typeClass, valList(1, param)), param(param) {
		setSuper(ArrayBase::stormType(engine));
	}

	bool ArrayType::loadAll() {
		if (param.ref)
			throw InternalError(L"References are not supported by the array yet.");

		if (param.isClass())
			loadClassFns();
		else
			loadValueFns();

		return Type::loadAll();
	}

	void ArrayType::loadClassFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value refParam = param.asRef(true);
		Value cloneEnv(CloneEnv::stormType(e));

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createClass))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&copyClass))));
		add(steal(nativeFunction(e, t, L"<<", valList(2, t, param), address(&pushClass))));
		add(steal(nativeFunction(e, t, L"push", valList(2, t, param), address(&pushClass))));
		add(steal(nativeFunction(e, refParam, L"[]", valList(2, t, Value(natType(e))), address(&getClass))));
		add(steal(nativeFunction(e, refParam, L"random", valList(1, t), address(&randomClass))));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), address(&ArrayBase::deepCopy))));
		add(steal(nativeDtor(e, this, &destroyClass)));
	}

	void ArrayType::loadValueFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value refParam = param.asRef(true);
		Value cloneEnv(CloneEnv::stormType(e));

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createValue))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&copyValue))));
		add(steal(nativeFunction(e, t, L"<<", valList(2, t, refParam), address(&pushValue))));
		add(steal(nativeFunction(e, t, L"push", valList(2, t, refParam), address(&pushValue))));
		add(steal(nativeFunction(e, refParam, L"[]", valList(2, t, Value(natType(e))), address(&getValue))));
		add(steal(nativeFunction(e, refParam, L"random", valList(1, t), address(&randomValue))));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), address(&ArrayBase::deepCopy))));
		add(steal(nativeDtor(e, this, &destroyValue)));
	}

	Type *arrayType(Engine &e, const ValueData &type) {
		Value strParam(Str::stormType(e));
		if (type == strParam) {
			// We need this during early compiler startup, so we can not look this one up regularly...
			Type *t = e.specialBuiltIn(specialArrayStr);
			if (!t) {
				t = CREATE(ArrayType, e, strParam);
				e.setSpecialBuiltIn(specialArrayStr, steal(t));
			}
			return t;
		}

		Auto<Name> tName = CREATE(Name, e);
		tName->add(L"core");
		tName->add(L"Array", vector<Value>(1, type));
		Auto<Named> n = e.scope()->find(tName);

		// Note: this is a bit dangerous, but should be fine since the Array type is kept alive by
		// being located in the type graph.
		Type *r = as<Type>(n.borrow());
		assert(r, "The array type was not found!");
		return r;
	}

	Bool isArray(Value v) {
		return as<ArrayType>(v.type) != null;
	}

	Value unwrapArray(Value v) {
		if (ArrayType *t = as<ArrayType>(v.type))
			return t->param;
		else
			return v;
	}

	Value wrapArray(Value v) {
		if (isArray(v))
			return v;
		return Value(arrayType(v.type->engine, v));
	}

}
