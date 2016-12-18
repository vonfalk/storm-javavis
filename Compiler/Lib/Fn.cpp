#include "stdafx.h"
#include "Fn.h"
#include "TemplateList.h"
#include "Engine.h"
#include "Exception.h"
#include "Core/Fn.h"
#include "Core/Str.h"

namespace storm {

	Type *createFn(Str *name, ValueArray *params) {
		return new (name) FnType(name, params);
	}

	FnType::FnType(Str *name, ValueArray *params) : Type(name, params->toArray(), typeClass) {
		setSuper(FnBase::stormType(engine));
	}


	Type *fnType(Array<Value> *params) {
		Engine &e = params->engine();
		TemplateList *l = e.cppTemplate(ArrayId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'fnType'.");
		Type *found = as<Type>(to->find(L"Fn", params));
		if (!found)
			throw InternalError(L"Can not find the function type!");
		return found;
	}

}
