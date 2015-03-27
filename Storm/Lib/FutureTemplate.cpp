#include "stdafx.h"
#include "FutureTemplate.h"
#include "Exception.h"
#include "Function.h"
#include "TypeCtor.h"
#include "CloneEnv.h"

namespace storm {

	static void CODECALL createClass(void *mem) {
		new (mem) FutureP<Object>();
	}

	static void CODECALL copyClass(void *mem, FutureP<Object> *from) {
		new (mem) FutureP<Object>();
	}

	static void CODECALL postClass(FutureP<Object> *to, Par<Object> obj) {
		to->post(obj);
	}

	static Object *CODECALL resultClass(FutureP<Object> *to) {
		return to->result().ret();
	}

	static void CODECALL createValue(void *mem) {
		nat offset = OFFSET_OF(Object, myType);
		Type *type = OFFSET_IN(mem, offset, Type *);

		FutureType *t = as<FutureType>(type);
		const Handle &handle = t->param.type->handle();

		new (mem) FutureBase(handle);
	}

	static void CODECALL copyValue(void *mem, FutureBase *from) {
		new (mem) FutureBase(from);
	}

	static void CODECALL postValue(FutureBase *to, const void *obj) {
		to->postRaw(obj);
	}

	code::Listing FutureType::resultValue() {
		using namespace code;

		Listing l;

		if (param.isBuiltIn()) {
			Variable future = l.frame.createPtrParam();
			Variable tmpVar = l.frame.createVariable(l.frame.root(), param.size());
			l << prolog();
			l << lea(ptrA, tmpVar);
			l << fnParam(future);
			l << fnParam(ptrA);
			l << fnCall(engine.fnRefs.futureResult, Size());
			l << mov(asSize(ptrA, param.size()), tmpVar);
			l << epilog();
			l << ret(param.size());
		} else {
			Variable valRef = l.frame.createPtrParam();
			Variable future = l.frame.createPtrParam();
			l << prolog();
			l << fnParam(future);
			l << fnParam(valRef);
			l << fnCall(engine.fnRefs.futureResult, Size());
			l << mov(ptrA, valRef);
			l << epilog();
			l << ret(Size::sPtr);
		}

		return l;
	}

	static Named *generateFuture(Par<NamePart> part) {
		if (part->params.size() != 1)
			return null;

		Engine &e = part->engine();
		const Value &type = part->params[0];
		Type *r = CREATE(FutureType, e, type);
		r->setSuper(FutureBase::type(e));
		r->matchFlags = matchNoInheritance;
		return r;
	}

	Template *futureTemplate(Engine &e) {
		return CREATE(Template, e, L"Future", simpleFn(&generateFuture));
	}

	FutureType::FutureType(const Value &param) : Type(L"Future", typeClass, valList(1, param)), param(param) {}

	void FutureType::lazyLoad() {
		if (param.ref)
			throw InternalError(L"References are not supported by the Future yet.");

		if (param.isClass())
			loadClassFns();
		else
			loadValueFns();
	}

	void FutureType::loadClassFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value cloneEnv = Value(CloneEnv::type(e));

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createClass))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&copyClass))));
		add(steal(nativeFunction(e, Value(), L"post", valList(2, t, param), address(&postClass))));
		add(steal(nativeFunction(e, param, L"result", valList(1, t), address(&resultClass))));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), address(&FutureBase::deepCopy))));
	}

	void FutureType::loadValueFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value cloneEnv = Value(CloneEnv::type(e));
		Value ref = param.asRef(true);

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createValue))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&copyValue))));
		add(steal(nativeFunction(e, Value(), L"post", valList(2, t, ref), address(&postValue))));
		add(steal(dynamicFunction(e, param, L"result", valList(1, t), resultValue())));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), address(&FutureBase::deepCopy))));
	}

	Type *futureType(Engine &e, const Value &type) {
		Auto<Name> tName = CREATE(Name, e);
		tName->add(L"core");
		tName->add(L"Future", vector<Value>(1, type));
		Type *r = as<Type>(e.scope()->find(tName));
		assert(r, "The future type was not found!");
		return r;
	}

}
