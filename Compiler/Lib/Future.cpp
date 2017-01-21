#include "stdafx.h"
#include "Future.h"
#include "TemplateList.h"
#include "Engine.h"
#include "Exception.h"
#include "Core/Str.h"
#include "Core/Future.h"

namespace storm {

	Type *createFuture(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		return new (params) FutureType(name, param.type);
	}

	Bool isFuture(Value v) {
		return as<FutureType>(v.type) != null;
	}

	Value unwrapFuture(Value v) {
		if (FutureType *t = as<FutureType>(v.type))
			return t->param();
		else
			return v;
	}

	Value wrapFuture(EnginePtr e, Value v) {
		if (v.ref)
			return v;

		TemplateList *l = e.v.cppTemplate(FutureId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'wrapFuture'.");
		Type *found = as<Type>(to->find(new (e.v) SimplePart(new (e.v) Str(L"Future"), v)));
		if (!found)
			throw InternalError(L"Can not find the future type!");
		return Value(found);
	}

	static void CODECALL futureCreate(void *mem) {
		FutureType *t = (FutureType *)runtime::typeOf((RootObject *)mem);
		const Handle &h = runtime::typeHandle(t->param().type);
		FutureBase *b = new (Place(mem)) FutureBase(h);
		runtime::setVTable(b);
	}

	static void CODECALL futureCopy(void *mem, FutureBase *src) {
		FutureBase *b = new (Place(mem)) FutureBase(*src);
		runtime::setVTable(b);
	}

	static void CODECALL postVoid(FutureBase *src) {
		src->postRaw(null);
	}

	static void CODECALL resultVoid(FutureBase *src) {
		src->resultRaw(null);
	}

	static void CODECALL postClass(FutureBase *src, Object *post) {
		src->postRaw(&post);
	}

	static RootObject *CODECALL resultClass(FutureBase *src) {
		RootObject *result;
		src->resultRaw(&result);
		return result;
	}

	FutureType::FutureType(Str *name, Type *contents)
		: Type(name, new (name) Array<Value>(1, Value(contents)), typeClass),
		  contents(contents) {

		setSuper(FutureBase::stormType(engine));
	}

	Value FutureType::param() const {
		return Value(contents);
	}

	Bool FutureType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value param = this->param();

		// Members shared by all classes.
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&futureCreate)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&futureCopy)));

		// Members differing between implementations.
		if (param == Value()) {
			loadVoid();
		} else if (param.isHeapObj()) {
			loadClass();
		} else {
			loadValue();
		}

		return Type::loadAll();
	}

	void FutureType::loadVoid() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value cloneEnv = Value(CloneEnv::stormType(e));

		add(nativeFunction(e, Value(), L"post", valList(e, 1, t), address(&postVoid)));
		add(nativeFunction(e, Value(), L"result", valList(e, 1, t), address(&resultVoid)));
	}

	void FutureType::loadClass() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value cloneEnv = Value(CloneEnv::stormType(e));

		add(nativeFunction(e, Value(), L"post", valList(e, 2, t, param()), address(&postClass)));
		add(nativeFunction(e, param(), L"result", valList(e, 1, t), address(&resultClass)));
	}

	void FutureType::loadValue() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value cloneEnv = Value(CloneEnv::stormType(e));
		Value ref = param().asRef();

		add(nativeFunction(e, Value(), L"post", valList(e, 2, t, ref), address(&FutureBase::postRaw)));
		// TODO: When 'param' is not a built in type, we can use FutureBase::resultRaw() directly,
		// as their signatures match (at least on X86).
		add(dynamicFunction(e, param(), L"result", valList(e, 1, t), resultValue()));
	}

	code::Listing *FutureType::resultValue() {
		using namespace code;
		Value param = this->param();

		Listing *l = new (this) Listing();

		Var me = l->createParam(valPtr());

		*l << prolog();

		if (param.isBuiltIn()) {
			// Nothing special to do, return in register.
			Var result = l->createVar(l->root(), param.size());
			*l << lea(ptrA, result);
			*l << fnParam(me);
			*l << fnParam(ptrA);
			*l << fnCall(engine.ref(Engine::rFutureResult), valVoid());
			*l << mov(asSize(ptrA, param.size()), result);
		} else {
			// We will be passed an additional pointer, just pass that on and we're good!
			Var result = l->createParam(valPtr());
			*l << fnParam(me);
			*l << fnParam(result);
			*l << fnCall(engine.ref(Engine::rFutureResult), valVoid());
			*l << mov(ptrA, result);
		}

		*l << epilog();
		*l << ret(param.valTypeRet());

		return l;
	}

}
