#include "stdafx.h"
#include "FnTemplate.h"
#include "Core/Fn.h"
#include "Core/Str.h"

namespace storm {

	Type *createFn(Str *name, ValueArray *params) {
		return new (name) FnType(name, params);
	}

	FnType::FnType(Str *name, ValueArray *params) : Type(name, params->toArray(), typeClass) {
		setSuper(FnBase::stormType(engine));
	}

}
