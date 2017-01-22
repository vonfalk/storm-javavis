#include "stdafx.h"
#include "FutureTemplate.h"
#include "Exception.h"
#include "Function.h"
#include "TypeCtor.h"
#include "Shared/CloneEnv.h"
#include "Engine.h"

namespace storm {

	static void CODECALL destroyClass(FutureP<Object> *v) {
		v->~FutureP<Object>();
	}

	static void CODECALL destroyValue(FutureBase *v) {
		v->~FutureBase();
	}

	static void CODECALL createClass(void *mem) {
		new (mem) FutureP<Object>();
	}

	static void CODECALL copyClass(void *mem, FutureP<Object> *from) {
		new (mem) FutureP<Object>(from);
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
		const Handle &handle = t->param.handle();

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

		if (param == Value()) {
			Variable future = l.frame.createPtrParam();
			l << prolog();
			l << fnParam(future);
			l << fnParam(natPtrConst(0));
			l << fnCall(engine.fnRefs.futureResult, retVoid());
			l << epilog();
			l << ret(retVoid());
		} else if (param.isBuiltIn()) {
			Variable future = l.frame.createPtrParam();
			Variable tmpVar = l.frame.createVariable(l.frame.root(), param.size());
			l << prolog();
			l << lea(ptrA, tmpVar);
			l << fnParam(future);
			l << fnParam(ptrA);
			l << fnCall(engine.fnRefs.futureResult, retVoid());
			l << mov(asSize(ptrA, param.size()), tmpVar);
			l << epilog();
			l << ret(param.retVal());
		} else {
			Variable future = l.frame.createPtrParam();
			Variable valRef = l.frame.createPtrParam();
			l << prolog();
			l << fnParam(future);
			l << fnParam(valRef);
			l << fnCall(engine.fnRefs.futureResult, retVoid());
			l << mov(ptrA, valRef);
			l << epilog();
			l << ret(retPtr());
		}

		return l;
	}

	static void createVoid(void *mem) {
		new (mem) Future<void>();
	}

	static void copyVoid(void *mem, Future<void> *from) {
		new (mem) Future<void>(from);
	}

	static void destroyVoid(Future<void> *o) {
		o->~Future<void>();
	}

	static void postVoid(Future<void> *to) {
		to->post();
	}

	static void resultVoid(Future<void> *from) {
		from->result();
	}

	static Named *generateFuture(Par<SimplePart> part) {
		if (part->count() != 1)
			return null;

		Engine &e = part->engine();
		const Value &type = part->param(0);

		return CREATE(FutureType, e, type);
	}

	Template *futureTemplate(Engine &e) {
		return CREATE(Template, e, L"Future", simpleFn(&generateFuture));
	}

	FutureType::FutureType(const Value &param) : Type(L"Future", typeClass, valList(1, param)), param(param) {
		setSuper(FutureBase::stormType(engine));
	}

	bool FutureType::loadAll() {
		if (param.ref)
			throw InternalError(L"References are not supported by the Future yet.");

		if (param == Value())
			loadVoidFns();
		else if (param.isClass())
			loadClassFns();
		else
			loadValueFns();

		return Type::loadAll();
	}

	void FutureType::loadClassFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value cloneEnv = Value(CloneEnv::stormType(e));

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createClass))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&copyClass))));
		add(steal(nativeFunction(e, Value(), L"post", valList(2, t, param), address(&postClass))));
		add(steal(nativeFunction(e, param, L"result", valList(1, t), address(&resultClass))));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), address(&FutureBase::deepCopy))));
		add(steal(nativeDtor(e, this, &destroyClass)));
	}

	void FutureType::loadValueFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value cloneEnv = Value(CloneEnv::stormType(e));
		Value ref = param.asRef(true);

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createValue))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&copyValue))));
		add(steal(nativeFunction(e, Value(), L"post", valList(2, t, ref), address(&postValue))));
		add(steal(dynamicFunction(e, param, L"result", valList(1, t), resultValue())));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), address(&FutureBase::deepCopy))));
		add(steal(nativeDtor(e, this, &destroyValue)));
	}

	void FutureType::loadVoidFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		Value cloneEnv = Value(CloneEnv::stormType(e));

		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createVoid))));
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, t), address(&copyVoid))));
		add(steal(nativeFunction(e, Value(), L"post", valList(1, t), address(&postVoid))));
		add(steal(nativeFunction(e, Value(), L"result", valList(1, t), address(&resultVoid))));
		add(steal(nativeFunction(e, Value(), L"deepCopy", valList(2, t, cloneEnv), address(&FutureBase::deepCopy))));
		add(steal(nativeDtor(e, this, &destroyVoid)));
	}

	Type *futureType(Engine &e, const ValueData &type) {
		Auto<SimpleName> tName = CREATE(SimpleName, e);
		tName->add(L"core");
		tName->add(L"Future", vector<Value>(1, type));

		// Should be ok to return a borrowed ptr.
		Auto<Named> n = e.scope()->find(tName);
		Type *r = as<Type>(n.borrow());
		assert(r, "The future type was not found!");
		return r;
	}

}
