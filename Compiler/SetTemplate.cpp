#include "stdafx.h"
#include "SetTemplate.h"
#include "Core/Set.h"

namespace storm {

	Type *createSet(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value k = params->at(0);
		if (k.ref)
			return null;

		return new (params) SetType(name, k.type);
	}

	SetType::SetType(Str *name, Type *k) : Type(name, typeClass), k(k) {
		params = new (engine) Array<Value>(Value(k));

		setSuper(SetBase::stormType(engine));
	}

}
