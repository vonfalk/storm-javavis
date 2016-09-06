#include "stdafx.h"
#include "Primitives.h"
#include "Core/Str.h"

namespace storm {

	Type *createBool(Str *name, Size size, GcType *type) {
		return new (name) Type(name, typeValue, size, type);
	}

	Type *createByte(Str *name, Size size, GcType *type) {
		return new (name) Type(name, typeValue, size, type);
	}

	Type *createInt(Str *name, Size size, GcType *type) {
		return new (name) Type(name, typeValue, size, type);
	}

	Type *createNat(Str *name, Size size, GcType *type) {
		return new (name) Type(name, typeValue, size, type);
	}

	Type *createLong(Str *name, Size size, GcType *type) {
		return new (name) Type(name, typeValue, size, type);
	}

	Type *createWord(Str *name, Size size, GcType *type) {
		return new (name) Type(name, typeValue, size, type);
	}

}
