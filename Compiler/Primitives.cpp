#include "stdafx.h"
#include "Primitives.h"
#include "Core/Str.h"

namespace storm {

	Type *createBool(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::boolVal);
	}

	Type *createByte(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::unsignedNr);
	}

	Type *createInt(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::signedNr);
	}

	Type *createNat(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::unsignedNr);
	}

	Type *createLong(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::signedNr);
	}

	Type *createWord(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::unsignedNr);
	}

	Type *createFloat(Str *name, Size size, GcType *type) {
		return new (name) PrimitiveType(name, size, type, BasicTypeInfo::floatNr);
	}


	PrimitiveType::PrimitiveType(Str *name, Size size, GcType *type, BasicTypeInfo::Kind kind) :
		Type(name, typeValue, size, type), kind(kind) {}

	BasicTypeInfo::Kind PrimitiveType::builtInType() const {
		return BasicTypeInfo::Kind(kind);
	}

}
