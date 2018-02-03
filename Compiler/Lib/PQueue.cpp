#include "stdafx.h"
#include "PQueue.h"
#include "Fn.h"
#include "Array.h"
#include "Core/PQueue.h"

namespace storm {

	Type *createPQueue(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		return new (params) PQueueType(name, param.type);
	}

	static void copyPQ(void *mem, const PQueueBase *src) {
		PQueueBase *o = new (Place(mem)) PQueueBase(*src);
		runtime::setVTable(o);
	}

	static void createPQ(void *mem) {
		PQueueType *t = (PQueueType *)runtime::typeOf((RootObject *)mem);
		PQueueBase *o = new (Place(mem)) PQueueBase(t->param().type->handle());
		runtime::setVTable(o);
	}

	static void createPQFn(void *mem, FnBase *compare) {
		PQueueType *t = (PQueueType *)runtime::typeOf((RootObject *)mem);
		PQueueBase *o = new (Place(mem)) PQueueBase(t->param().type->handle(), compare);
		runtime::setVTable(o);
	}

	static void createPQArray(void *mem, ArrayBase *src) {
		PQueueBase *o = new (Place(mem)) PQueueBase(src);
		runtime::setVTable(o);
	}

	static void createPQArrayFn(void *mem, ArrayBase *src, FnBase *compare) {
		PQueueBase *o = new (Place(mem)) PQueueBase(src, compare);
		runtime::setVTable(o);
	}

	// Note: This allows altering the contents of the priority queue, which is fairly bad.
	static const void *topPQ(PQueueBase *me) {
		return me->topRaw();
	}

	static RootObject *topPQObj(PQueueBase *me) {
		return *(RootObject **)me->topRaw();
	}

	static PQueueBase *pushPQ(PQueueBase *me, const void *elem) {
		me->pushRaw(elem);
		return me;
	}

	static PQueueBase *pushPQObj(PQueueBase *me, RootObject *obj) {
		me->pushRaw(&obj);
		return me;
	}

	PQueueType::PQueueType(Str *name, Type *contents) : Type(name, typeClass), contents(contents) {
		params = new (engine) Array<Value>(param());

		setSuper(PQueueBase::stormType(engine));
		useSuperGcType();
	}

	Value PQueueType::param() const {
		return Value(contents);
	}

	Bool PQueueType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef();

		Array<Value> *pParams = new (e) Array<Value>();
		*pParams << Value(StormInfo<Bool>::type(e));
		*pParams << param() << param();
		Value f = Value(fnType(pParams));

		Value a = wrapArray(param());


		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&copyPQ))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, f), address(&createPQFn))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 3, t, a, f), address(&createPQArrayFn))->makePure());

		if (param().isHeapObj()) {
			add(nativeFunction(e, Value(), S("push"), valList(e, 2, t, param()), address(&pushPQObj)));
			add(nativeFunction(e, t, S("<<"), valList(e, 2, t, param()), address(&pushPQObj)));
			add(nativeFunction(e, param(), S("top"), valList(e, 1, t), address(&topPQObj))->makePure());
		} else {
			// Note: Returning a reference from 'top' allows altering values in the PQueue, which could be bad.
			add(nativeFunction(e, Value(), S("push"), valList(e, 2, t, ref), address(&pushPQ)));
			add(nativeFunction(e, t, S("<<"), valList(e, 2, t, ref), address(&pushPQ)));
			add(nativeFunction(e, ref, S("top"), valList(e, 1, t), address(&topPQ))->makePure());
		}

		if (param().type->handle().lessFn) {
			addLess();
		} else {
			param().type->watchAdd(this);
		}

		return Type::loadAll();
	}

	void PQueueType::notifyAdded(NameSet *to, Named *added) {
		if (added == param().type) {
			if (*added->name == S("<") &&
				added->params->count() == 2 &&
				added->params->at(0).type == to &&
				added->params->at(1).type == to) {

				addLess();
				to->watchRemove(this);
			}
		}
	}

	void PQueueType::addLess() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value a = wrapArray(param());

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&createPQ))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, a), address(&createPQArray))->makePure());
	}

}
