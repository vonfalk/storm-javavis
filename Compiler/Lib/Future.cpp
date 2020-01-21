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
		Type *found = as<Type>(to->find(S("Future"), v, Scope()));
		if (!found)
			throw new (e.v) InternalError(S("Can not find the future type!"));
		return Value(found);
	}

	static void CODECALL futureCreate(void *mem) {
		FutureType *t = (FutureType *)runtime::typeOf((RootObject *)mem);
		const Handle &h = runtime::typeHandle(t->param().type);
		FutureBase *b = new (Place(mem)) FutureBase(h);
		runtime::setVTable(b);
	}

	static void CODECALL futureCreateVoid(void *mem) {
		new (Place(mem)) Future<void>();
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
		Value param = this->param();

		// Members differing between implementations.
		if (param == Value()) {
			loadVoid();
		} else if (param.isObject()) {
			loadClass();
		} else {
			loadValue();
		}

		return Type::loadAll();
	}

	void FutureType::loadVoid() {
		Engine &e = engine;
		Value t = thisPtr(this);
		//Value cloneEnv = Value(CloneEnv::stormType(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&futureCreateVoid)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&futureCopy)));
		add(nativeFunction(e, Value(), S("post"), valList(e, 1, t), address(&postVoid)));
		add(nativeFunction(e, Value(), S("result"), valList(e, 1, t), address(&resultVoid)));
	}

	void FutureType::loadClass() {
		Engine &e = engine;
		Value t = thisPtr(this);
		//Value cloneEnv = Value(CloneEnv::stormType(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&futureCreate)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&futureCopy)));
		add(nativeFunction(e, Value(), S("post"), valList(e, 2, t, param()), address(&postClass)));
		add(nativeFunction(e, param(), S("result"), valList(e, 1, t), address(&resultClass)));
	}

	void FutureType::loadValue() {
		Engine &e = engine;
		Value t = thisPtr(this);
		//Value cloneEnv = Value(CloneEnv::stormType(e));
		Value ref = param().asRef();

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&futureCreate)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&futureCopy)));
		add(nativeFunction(e, Value(), S("post"), valList(e, 2, t, ref), address(&FutureBase::postRaw)));
		// TODO: When 'param' is not a built in type, we can use FutureBase::resultRaw() directly,
		// as their signatures match (at least on X86).
		add(dynamicFunction(e, param(), S("result"), valList(e, 1, t), resultValue()));
	}

	code::Listing *FutureType::resultValue() {
		using namespace code;
		Value param = this->param();

		Listing *l = new (this) Listing(true, param.desc(engine));

		TypeDesc *ptr = engine.ptrDesc();
		Var me = l->createParam(ptr);
		Var data = l->createVar(l->root(), param.size());

		*l << prolog();

		*l << lea(ptrA, data);
		*l << fnParam(ptr, me);
		*l << fnParam(ptr, ptrA);
		*l << fnCall(engine.ref(Engine::rFutureResult), false);
		*l << fnRet(data);

		return l;
	}

}
