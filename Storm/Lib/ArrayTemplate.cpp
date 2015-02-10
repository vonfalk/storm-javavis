#include "stdafx.h"
#include "ArrayTemplate.h"
#include "Array.h"
#include "Function.h"
#include "Exception.h"
#include "TypeCtor.h"

namespace storm {

	static void CODECALL createClass(void *mem) {
		new (mem) ArrayP<Object>();
	}

	static void CODECALL createValue(void *mem) {
		// Extract the type...
		nat offset = OFFSET_OF(Object, myType);
		Type *type = OFFSET_IN(mem, offset, Type *);

		// Find out the handle.
		ArrayType *arr = as<ArrayType>(type);
		const Handle &handle = arr->param.type->handle();

		// Create it!
		new (mem) ArrayBase(handle);
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

	static Named *generateArray(Par<NamePart> part) {
		if (part->params.size() != 1)
			return null;

		Engine &e = part->engine();

		const Value &type = part->params[0];
		Type *r = CREATE(ArrayType, e, type);
		r->setSuper(ArrayBase::type(e));
		return r;
	}

	Template *arrayTemplate(Engine &e) {
		return CREATE(Template, e, L"Array", simpleFn(&generateArray));
	}

	ArrayType::ArrayType(const Value &param) : Type(L"Array", typeClass, valList(1, param)), param(param) {}

	void ArrayType::lazyLoad() {
		if (param.ref)
			throw InternalError(L"References are not supported by the array yet.");

		if (param.isClass())
			loadClassFns();
		else
			loadValueFns();

	}

	void ArrayType::loadClassFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value refParam = param.asRef(true);

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createClass))));
		add(steal(nativeFunction(e, t, L"<<", valList(2, t, param), address(&pushClass))));
		add(steal(nativeFunction(e, t, L"push", valList(2, t, param), address(&pushClass))));
		add(steal(nativeFunction(e, refParam, L"[]", valList(2, t, Value(natType(e))), address(&getClass))));
	}

	void ArrayType::loadValueFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value refParam = param.asRef(true);

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createValue))));
		add(steal(nativeFunction(e, t, L"<<", valList(2, t, refParam), address(&pushValue))));
		add(steal(nativeFunction(e, t, L"push", valList(2, t, refParam), address(&pushValue))));
		add(steal(nativeFunction(e, refParam, L"[]", valList(2, t, Value(natType(e))), address(&getValue))));
	}


	Type *arrayType(Engine &e, const Value &type) {
		Auto<Name> tName = CREATE(Name, e);
		tName->add(L"core");
		tName->add(L"Array", vector<Value>(1, type));
		Type *r = as<Type>(e.scope()->find(tName));
		assert(("The array type was not found!", r));
		return r;
	}

}
