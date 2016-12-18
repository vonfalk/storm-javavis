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

	FutureType::FutureType(Str *name, Type *contents) : Type(name, typeClass), contents(contents) {
		setSuper(FutureBase::stormType(engine));
	}

	Value FutureType::param() const {
		return Value(contents);
	}

}
