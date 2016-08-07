#include "stdafx.h"
#include "ArrayTemplate.h"
#include "Core/Str.h"

namespace storm {

	Type *createArray(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		return new (params) ArrayType(name, param.type);
	}

	ArrayType::ArrayType(Str *name, Type *contents) : Type(name, typeClass), contents(contents) {
		setSuper(ArrayBase::stormType(engine));
	}

}
