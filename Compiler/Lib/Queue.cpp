#include "stdafx.h"
#include "Queue.h"
#include "Engine.h"
#include "Core/Queue.h"

namespace storm {

	Type *createQueue(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		return new (params) QueueType(name, param.type);
	}

	Bool isQueue(Value v) {
		return as<QueueType>(v.type) != null;
	}

	Value unwrapQueue(Value v) {
		if (QueueType *t = as<QueueType>(v.type))
			return t->param();
		else
			return v;
	}

	Value wrapQueue(Value v) {
		if (!v.type)
			return v;
		if (v.ref)
			return v;

		Engine &e = v.type->engine;
		TemplateList *l = e.cppTemplate(QueueId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'wrapQueue'.");
		Type *found = as<Type>(to->find(S("Queue"), v, Scope()));
		if (!found)
			throw new (v.type) InternalError(S("Can not find the queue type!"));
		return Value(found);
	}

	QueueType::QueueType(Str *name, Type *contents) : Type(name, typeClass), contents(contents) {
		if (engine.has(bootTemplates))
			lateInit();

		setSuper(QueueBase::stormType(engine));
	}

	void QueueType::lateInit() {
		if (!params)
			params = new (engine) Array<Value>();
		if (params->count() < 1)
			params->push(Value(contents));

		Type::lateInit();
	}

	Value QueueType::param() const {
		return Value(contents);
	}

	static void CODECALL createQueueRaw(void *mem) {
		QueueType *t = (QueueType *)runtime::typeOf((RootObject *)mem);
		QueueBase *o = new (Place(mem)) QueueBase(t->param().type->handle());
		runtime::setVTable(o);
	}

	static void CODECALL copyQueue(void *mem, QueueBase *from) {
		QueueBase *o = new (Place(mem)) QueueBase(*from);
		runtime::setVTable(o);
	}

	static void CODECALL pushQueueClass(QueueBase *q, void *elem) {
		q->pushRaw(&elem);
	}

	static QueueBase *CODECALL addQueueClass(QueueBase *q, void *elem) {
		q->pushRaw(&elem);
		return q;
	}

	static QueueBase *CODECALL addQueueVal(QueueBase *q, void *elem) {
		q->pushRaw(elem);
		return q;
	}


	Bool QueueType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value ref = param().asRef();
		Value natType = Value(StormInfo<Nat>::type(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&createQueueRaw))->makePure());
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&copyQueue))->makePure());
		add(nativeFunction(e, ref, S("top"), valList(e, 1, t), address(&QueueBase::topRaw))->makePure());

		if (param().isObject()) {
			add(nativeFunction(e, Value(), S("push"), valList(e, 2, t, param()), address(&pushQueueClass)));
			add(nativeFunction(e, t, S("<<"), valList(e, 2, t, param()), address(&addQueueClass)));
		} else {
			add(nativeFunction(e, Value(), S("push"), valList(e, 2, t, ref), address(&QueueBase::pushRaw)));
			add(nativeFunction(e, t, S("<<"), valList(e, 2, t, ref), address(&addQueueVal)));
		}

		return Type::loadAll();
	}

}
