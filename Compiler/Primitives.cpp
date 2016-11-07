#include "stdafx.h"
#include "Primitives.h"
#include "Core/Str.h"

namespace storm {

	Type *createBool(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::boolVal);
	}

	Type *createFloat(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::floatNr);
	}


	PrimitiveType::PrimitiveType(Str *name, Size size, GcType *type, BasicTypeInfo::Kind kind) :
		Type(name, typeValue, size, type, null), kind(kind) {}

	BasicTypeInfo::Kind PrimitiveType::builtInType() const {
		return BasicTypeInfo::Kind(kind);
	}

}
